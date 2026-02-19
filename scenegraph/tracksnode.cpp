// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
//
// Tracks node - renders track reference lines, guidance lines, and design previews

#include "tracksnode.h"
#include <QVector4D>
#include <cmath>
#include "materials.h"
#include "aoggeometry.h"
#include "textnode.h"
#include "dotsnode.h"
#include "texturefactory.h"
#include "thicklinematerial.h"
#include "glm.h"

// ---------------------------------------------------------------------------
// Near-plane clip helper
//
// The thick-line shader computes line direction as:
//   lineVec = nextClip.xy * currClip.w - currClip.xy * nextClip.w
// and then scales the perpendicular offset by abs(currClip.w).
// When a vertex is behind the camera its clip.w is negative and large, which
// both inverts the direction and produces an enormous offset — appearing as an
// extremely thick line.
//
// Fix: before building geometry, clip each world-space polyline against the
// view-space z = clipZ plane (default slightly in front of camera, z < 0).
// Segment endpoints that lie behind the plane are replaced with the
// intersection point, so no geometry vertex ever has a negative clip.w.
//
// The interpolation is done in world space: because the MV transform is
// affine, the parametric t at which a segment crosses view-space z=clipZ is
// the same as in world space.
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
// Segment subdivision helper
//
// Inserts intermediate vertices so that no segment is longer than maxLen
// world-space units.  This keeps the per-segment depth variation small,
// which stabilises the thick-line shader's homogeneous direction vector:
//   lineVec = nextClip.xy * currClip.w - currClip.xy * nextClip.w
// When both endpoints are at similar depths their clip.w values match and
// the direction is computed accurately.  For a 2000 m AB reference line
// this turns one huge segment into ~40 × 50 m segments.
// ---------------------------------------------------------------------------
static QVector<QVector3D> subdividePolyline(const QVector<QVector3D> &points,
                                            float maxLen = 50.0f)
{
    if (points.count() < 2)
        return points;

    QVector<QVector3D> result;
    result.reserve(points.count() * 2);
    result.append(points[0]);

    for (int i = 1; i < points.count(); ++i) {
        const QVector3D &a = points[i - 1];
        const QVector3D &b = points[i];
        float len = (b - a).length();
        if (len > maxLen) {
            int divisions = static_cast<int>(std::ceil(len / maxLen));
            for (int j = 1; j < divisions; ++j) {
                float t = static_cast<float>(j) / static_cast<float>(divisions);
                result.append(a + t * (b - a));
            }
        }
        result.append(b);
    }

    return result;
}

static QVector<QVector3D> clipLineToFront(const QVector<QVector3D> &worldPoints,
                                          const QMatrix4x4 &mv,
                                          float clipZ = -0.5f)
{
    if (worldPoints.count() < 2)
        return worldPoints;

    // View-space Z for a world-space point (negative = in front of camera).
    auto viewZ = [&](const QVector3D &wp) -> float {
        return (mv * QVector4D(wp, 1.0f)).z();
    };

    QVector<QVector3D> result;
    result.reserve(worldPoints.count());

    float z0 = viewZ(worldPoints[0]);
    bool front0 = z0 < clipZ;
    if (front0)
        result.append(worldPoints[0]);

    for (int i = 1; i < worldPoints.count(); ++i) {
        float z1 = viewZ(worldPoints[i]);
        bool front1 = z1 < clipZ;

        if (front0 && front1) {
            // Both in front: keep this point.
            result.append(worldPoints[i]);
        } else if (front0 && !front1) {
            // Crossing from front to back: add the intersection, stop.
            float t = (z0 - clipZ) / (z0 - z1);
            result.append(worldPoints[i - 1] + t * (worldPoints[i] - worldPoints[i - 1]));
        } else if (!front0 && front1) {
            // Crossing from back to front: add intersection, then this point.
            float t = (z0 - clipZ) / (z0 - z1);
            result.append(worldPoints[i - 1] + t * (worldPoints[i] - worldPoints[i - 1]));
            result.append(worldPoints[i]);
        }
        // Both behind: skip.

        z0 = z1;
        front0 = front1;
    }

    return result;
}


TracksNode::TracksNode() {}

TracksNode::~TracksNode() {}

void TracksNode::clearChildren()
{
    while (childCount() > 0) {
        QSGNode *child = firstChild();
        removeChildNode(child);
        delete child;
    }

    m_refLineNode = nullptr;
    m_currentLineNode = nullptr;
    m_newTrack = nullptr;
    m_aRefFlag = nullptr;
    m_bRefFlag = nullptr;
    m_refDotsNode = nullptr;
}

void TracksNode::update(const QMatrix4x4 &mv,
                        const QMatrix4x4 &p,
                        const QMatrix4x4 &ndc,
                        const QSize &viewportSize,
                        float vehicleX,
                        float vehicleY,
                        float vehicleHeading,
                        bool isOutOfBounds,
                        int lineWidth,
                        float textSize,
                        TextureFactory *texture,
                        TracksProperties *properties)
{
    Q_UNUSED(vehicleX)
    Q_UNUSED(vehicleY)
    Q_UNUSED(vehicleHeading)
    Q_UNUSED(isOutOfBounds)

    // Subdivide then clip each line collection before building geometry.
    // Subdivision keeps per-segment depth variation small so the shader's
    // homogeneous direction vector is well-conditioned (AB lines can be
    // 2000 m long with only two points; 50 m sub-segments fix this).
    // Clipping removes vertices behind the near plane so that clip.w never
    // goes negative, which would otherwise invert and amplify the offset.
    const QVector<QVector3D> refLine    = clipLineToFront(subdividePolyline(properties->refLine()),     mv);
    const QVector<QVector3D> currentLine = clipLineToFront(subdividePolyline(properties->currentLine()), mv);
    const QVector<QVector3D> newTrack   = clipLineToFront(subdividePolyline(properties->newTrack()),    mv);

    const QMatrix4x4 mvp = ndc * p * mv;

    if (childCount() < 1) {
        // Build all geometry from scratch.

        // Reference line (dark red): 2-point AB segment or curve polyline.
        if (refLine.count() >= 2) {
            auto *geo = AOGGeometry::createThickLineGeometry(refLine);
            if (geo) {
                m_refLineNode = new QSGGeometryNode();
                m_refLineNode->setGeometry(geo);
                m_refLineNode->setFlag(QSGNode::OwnsGeometry);
                auto *mat = new ThickLineMaterial();
                m_refLineNode->setMaterial(mat);
                m_refLineNode->setFlag(QSGNode::OwnsMaterial);
                appendChildNode(m_refLineNode);
            }
        }

        // Current guidance line (magenta): parallel offset from reference.
        if (currentLine.count() >= 2) {
            auto *geo = AOGGeometry::createThickLineGeometry(currentLine);
            if (geo) {
                m_currentLineNode = new QSGGeometryNode();
                m_currentLineNode->setGeometry(geo);
                m_currentLineNode->setFlag(QSGNode::OwnsGeometry);
                auto *mat = new ThickLineMaterial();
                m_currentLineNode->setMaterial(mat);
                m_currentLineNode->setFlag(QSGNode::OwnsMaterial);
                appendChildNode(m_currentLineNode);
            }
        }

        // Design preview (thin magenta): shown while placing a new track.
        if (newTrack.count() >= 2) {
            auto *geo = AOGGeometry::createThickLineGeometry(newTrack);
            if (geo) {
                m_newTrack = new QSGGeometryNode();
                m_newTrack->setGeometry(geo);
                m_newTrack->setFlag(QSGNode::OwnsGeometry);
                auto *mat = new ThickLineMaterial();
                m_newTrack->setMaterial(mat);
                m_newTrack->setFlag(QSGNode::OwnsMaterial);
                appendChildNode(m_newTrack);
            }
        }

        // A/B reference flags: dots + text labels.
        // Guard on refLine being non-empty: the simulator can set showRefFlags=true
        // with a demo AB line even before a real field is opened.
        if (properties->showRefFlags() && properties->refLine().count() >= 2) {
            m_refDotsNode = new DotsNode();
            // A point: cyan
            m_refDotsNode->addDot(properties->aRefFlag(),
                                  QColor::fromRgbF(0.0f, 0.90f, 0.95f, 1.0f), glm::dp(8.0f));
            // B point: red
            m_refDotsNode->addDot(properties->bRefFlag(),
                                  QColor::fromRgbF(0.95f, 0.0f, 0.0f, 1.0f), glm::dp(8.0f));
            m_refDotsNode->build();
            appendChildNode(m_refDotsNode);

            m_aRefFlag = new TextNode(texture, "&A", textSize);
            appendChildNode(m_aRefFlag);
            m_bRefFlag = new TextNode(texture, "&B", textSize);
            appendChildNode(m_bRefFlag);
        }
    }

    // ALWAYS update MVP / material uniforms every frame.

    if (m_refLineNode)
        updateThickLineNode(m_refLineNode, mvp, viewportSize, lineWidth,
                            QColor::fromRgbF(0.93f, 0.2f, 0.2f));

    if (m_currentLineNode)
        updateThickLineNode(m_currentLineNode, mvp, viewportSize, lineWidth,
                            QColor::fromRgbF(0.95f, 0.2f, 0.95f));

    if (m_newTrack)
        updateThickLineNode(m_newTrack, mvp, viewportSize, qMax(1, lineWidth - 1),
                            QColor::fromRgbF(0.95f, 0.42f, 0.75f));

    if (m_aRefFlag) {
        QMatrix4x4 flagMv = mv;
        flagMv.translate(properties->aRefFlag());
        m_aRefFlag->updateUniforms(ndc * p * flagMv, viewportSize,
                                   QColor::fromRgbF(0.0f, 0.90f, 0.95f), true);
    }

    if (m_bRefFlag) {
        QMatrix4x4 flagMv = mv;
        flagMv.translate(properties->bRefFlag());
        m_bRefFlag->updateUniforms(ndc * p * flagMv, viewportSize,
                                   QColor::fromRgbF(0.95f, 0.0f, 0.0f), true);
    }

    if (m_refDotsNode)
        m_refDotsNode->updateUniforms(mvp, viewportSize);
}

void TracksNode::updateThickLineNode(QSGGeometryNode *node,
                                     const QMatrix4x4 &mvp,
                                     const QSize &viewportSize,
                                     int lineWidth,
                                     const QColor &color)
{
    auto *material = static_cast<ThickLineMaterial *>(node->material());
    if (material) {
        material->setMvpMatrix(mvp);
        material->setViewportSize(viewportSize);
        material->setLineWidth(lineWidth);
        material->setColor(color);
    }
}
