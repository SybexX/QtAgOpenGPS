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

    m_shadowOutlineNode = nullptr;
    m_shadowFillNode = nullptr;
    m_sideGuideNodes.clear();
    m_curveGuideNodes.clear();
    m_lookaheadNode = nullptr;
    m_pursuitCircleNode = nullptr;
    m_smoothedCurveNode = nullptr;
    m_currentLineDotsNode = nullptr;
    m_youTurnDotsNode = nullptr;

    // Contour nodes
    m_contourLineNode = nullptr;
    m_contourPointsNode = nullptr;
    m_stripPointsNearbyNode = nullptr;
    m_stripPointsSparseNodes.clear();
    m_stripPointsSparseLineNodes.clear();
    m_lastStripChunks = 0;
    m_contourCurrentPointNode = nullptr;
    m_contourGoalPointNode = nullptr;
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

    // Shadow quad: close the loop (4 corners + first point) then clip/subdivide
    QVector<QVector3D> shadowQuadClipped;
    if (properties->shadowQuad().count() == 4) {
        QVector<QVector3D> shadowLoop = properties->shadowQuad();
        shadowLoop.append(shadowLoop[0]);
        shadowQuadClipped = clipLineToFront(subdividePolyline(shadowLoop, 50.0f), mv);
    }

    // Side guide lines: process each 2-point segment individually
    QVector<QVector<QVector3D>> sideGuideSegments;
    const auto &sgLines = properties->sideGuideLines();
    for (int i = 0; i + 1 < sgLines.count(); i += 2) {
        QVector<QVector3D> seg = { sgLines[i], sgLines[i + 1] };
        QVector<QVector3D> segClipped = clipLineToFront(subdividePolyline(seg, 50.0f), mv);
        if (segClipped.count() >= 2)
            sideGuideSegments.append(segClipped);
    }

    // Lookahead points: no line processing needed (dots only)

    // Pursuit circle: already subdivided with 100 segments, just clip
    const QVector<QVector3D> pursuitCircleClipped = clipLineToFront(properties->pursuitCircle(), mv);

    // Smoothed curve: clip/subdivide
    const QVector<QVector3D> smoothedCurveClipped = clipLineToFront(subdividePolyline(properties->smoothedCurve(), 50.0f), mv);

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

        /*
        // Shadow outline quad
        if (shadowQuadClipped.count() >= 2) {
            auto *geo = AOGGeometry::createThickLineGeometry(shadowQuadClipped);
            if (geo) {
                m_shadowOutlineNode = new QSGGeometryNode();
                m_shadowOutlineNode->setGeometry(geo);
                m_shadowOutlineNode->setFlag(QSGNode::OwnsGeometry);
                auto *mat = new ThickLineMaterial();
                m_shadowOutlineNode->setMaterial(mat);
                m_shadowOutlineNode->setFlag(QSGNode::OwnsMaterial);
                appendChildNode(m_shadowOutlineNode);
            }
        }*/

        // Shadow fill (semi-transparent quad)
        if (properties->shadowQuad().count() == 4) {
            auto *geo = AOGGeometry::createTriangleFanGeometry(properties->shadowQuad());
            if (geo) {
                m_shadowFillNode = new QSGGeometryNode();
                m_shadowFillNode->setGeometry(geo);
                m_shadowFillNode->setFlag(QSGNode::OwnsGeometry);
                auto *mat = new AOGFlatColorMaterial();
                m_shadowFillNode->setMaterial(mat);
                m_shadowFillNode->setFlag(QSGNode::OwnsMaterial);
                appendChildNode(m_shadowFillNode);
            }
        }

        // Side guide lines
        for (const auto &seg : sideGuideSegments) {
            auto *geo = AOGGeometry::createThickLineGeometry(seg);
            if (geo) {
                auto *node = new QSGGeometryNode();
                node->setGeometry(geo);
                node->setFlag(QSGNode::OwnsGeometry);
                auto *mat = new ThickLineMaterial();
                node->setMaterial(mat);
                node->setFlag(QSGNode::OwnsMaterial);
                appendChildNode(node);
                m_sideGuideNodes.append(node);
            }
        }

        // Curve side guide lines (1px lines)
        const auto &curveGuides = properties->curveGuideLines();
        for (const auto &guideLine : curveGuides) {
            if (guideLine.count() >= 2) {
                auto *geo = AOGGeometry::createLineStripGeometry(guideLine);
                if (geo) {
                    auto *node = new QSGGeometryNode();
                    node->setGeometry(geo);
                    node->setFlag(QSGNode::OwnsGeometry);
                    auto *mat = new AOGFlatColorMaterial();
                    mat->setColor(QColor::fromRgbF(0.567f, 0.650f, 0.567f));
                    node->setMaterial(mat);
                    node->setFlag(QSGNode::OwnsMaterial);
                    appendChildNode(node);
                    m_curveGuideNodes.append(node);
                }
            }
        }

        // Lookahead dots
        if (!properties->lookaheadPoints().isEmpty()) {
            m_lookaheadNode = new DotsNode();
            for (const QVector3D &pt : properties->lookaheadPoints())
                m_lookaheadNode->addDot(pt, QColor::fromRgbF(1.0f, 1.0f, 0.0f, 1.0f), glm::dp(8.0f));
            m_lookaheadNode->build();
            appendChildNode(m_lookaheadNode);
        }

        // Pure pursuit circle
        if (pursuitCircleClipped.count() >= 2) {
            auto *geo = AOGGeometry::createThickLineGeometry(pursuitCircleClipped);
            if (geo) {
                m_pursuitCircleNode = new QSGGeometryNode();
                m_pursuitCircleNode->setGeometry(geo);
                m_pursuitCircleNode->setFlag(QSGNode::OwnsGeometry);
                auto *mat = new ThickLineMaterial();
                m_pursuitCircleNode->setMaterial(mat);
                m_pursuitCircleNode->setFlag(QSGNode::OwnsMaterial);
                appendChildNode(m_pursuitCircleNode);
            }
        }

        // Smoothed curve
        if (smoothedCurveClipped.count() >= 2) {
            auto *geo = AOGGeometry::createThickLineGeometry(smoothedCurveClipped);
            if (geo) {
                m_smoothedCurveNode = new QSGGeometryNode();
                m_smoothedCurveNode->setGeometry(geo);
                m_smoothedCurveNode->setFlag(QSGNode::OwnsGeometry);
                auto *mat = new ThickLineMaterial();
                m_smoothedCurveNode->setMaterial(mat);
                m_smoothedCurveNode->setFlag(QSGNode::OwnsMaterial);
                appendChildNode(m_smoothedCurveNode);
            }
        }

        // Curve vertex dots
        if (properties->showCurrentLineDots() && properties->currentLine().count() >= 1) {
            m_currentLineDotsNode = new DotsNode();
            for (const QVector3D &pt : properties->currentLine())
                m_currentLineDotsNode->addDot(pt, QColor::fromRgbF(0.92f, 0.6f, 0.95f, 1.0f),
                                              glm::dp(3.0f));
            m_currentLineDotsNode->build();
            appendChildNode(m_currentLineDotsNode);
        }

        // YouTurn dots
        if (properties->youTurnPoints().count() >= 3) {
            m_youTurnDotsNode = new DotsNode();
            for (const QVector3D &pt : properties->youTurnPoints())
                m_youTurnDotsNode->addDot(pt, QColor::fromRgbF(0.95f, 0.5f, 0.95f, 1.0f),
                                           glm::dp(3.0f));
            m_youTurnDotsNode->build();
            appendChildNode(m_youTurnDotsNode);
        }

        // === Contour rendering ===

        // Contour line (LINE_STRIP) - purple - use flat color material for stability
        const auto &contourLine = properties->contourLine();
        if (contourLine.count() >= 2) {
            auto *geo = AOGGeometry::createLinesGeometry(contourLine);
            if (geo) {
                m_contourLineNode = new QSGGeometryNode();
                m_contourLineNode->setGeometry(geo);
                m_contourLineNode->setFlag(QSGNode::OwnsGeometry);
                auto *mat = new AOGFlatColorMaterial();
                mat->setColor(QColor::fromRgbF(0.75f, 0.25f, 0.95f));
                m_contourLineNode->setMaterial(mat);
                m_contourLineNode->setFlag(QSGNode::OwnsMaterial);
                appendChildNode(m_contourLineNode);
            }
        }

        // Contour line points (POINTS) - greenish - use DotsNode
        DotsNode *contourPointsNode = new DotsNode();
        for (const QVector3D &pt : contourLine) {
            contourPointsNode->addDot(pt, QColor::fromRgbF(0.87f, 0.87f, 0.25f, 1.0f), glm::dp(lineWidth));
        }
        contourPointsNode->build();
        appendChildNode(contourPointsNode);
        m_contourPointsNode = contourPointsNode;

        // Strip points nearby (dense) - rebuilt every frame
        const auto &stripPointsNearby = properties->stripPointsNearby();
        QColor stripColor = properties->isContourOn()
            ? QColor::fromRgbF(0.983f, 0.92f, 0.420f, 1.0f)  // yellow when locked
            : QColor::fromRgbF(0.3f, 0.982f, 0.0f, 1.0f);   // green when not locked
        if (!stripPointsNearby.isEmpty()) {
            m_stripPointsNearbyNode = new DotsNode();
            for (const QVector3D &pt : stripPointsNearby) {
                m_stripPointsNearbyNode->addDot(pt, stripColor, glm::dp(lineWidth));
            }
            m_stripPointsNearbyNode->build();
            appendChildNode(m_stripPointsNearbyNode);
        }

        // Strip points sparse (chunked) - grows as strip grows
        const int STRIP_CHUNK_SIZE = 10000;
        const auto &stripPointsSparse = properties->stripPointsSparse();
        int numChunks = (stripPointsSparse.count() + STRIP_CHUNK_SIZE - 1) / STRIP_CHUNK_SIZE;
        for (int c = 0; c < numChunks; c++) {
            DotsNode *stripChunkNode = new DotsNode();
            int start = c * STRIP_CHUNK_SIZE;
            int end = qMin(start + STRIP_CHUNK_SIZE, stripPointsSparse.count());
            for (int i = start; i < end; i++) {
                stripChunkNode->addDot(stripPointsSparse[i], stripColor, glm::dp(lineWidth));
            }
            stripChunkNode->build();
            appendChildNode(stripChunkNode);
            m_stripPointsSparseNodes.append(stripChunkNode);
        }
        m_lastStripChunks = numChunks;

        // Sparse line strips (thin 1px) - chunked for efficiency
        // Copy last vertex to next chunk to make line appear continuous
        for (int c = 0; c < numChunks; c++) {
            int start = c * STRIP_CHUNK_SIZE;
            int end = qMin(start + STRIP_CHUNK_SIZE, stripPointsSparse.count());
            // Include one extra point from next chunk for continuity (except last chunk)
            int count = end - start;
            if (c < numChunks - 1) {
                count = end - start + 1;
            }

            QVector<QVector3D> chunkPoints;
            chunkPoints.reserve(count);
            for (int i = start; i < start + count && i < stripPointsSparse.count(); i++) {
                chunkPoints.append(stripPointsSparse[i]);
            }

            if (chunkPoints.count() >= 2) {
                auto *geo = AOGGeometry::createLineStripGeometry(chunkPoints);
                if (geo) {
                    QSGGeometryNode *lineNode = new QSGGeometryNode();
                    lineNode->setGeometry(geo);
                    lineNode->setFlag(QSGNode::OwnsGeometry);
                    auto *mat = new AOGFlatColorMaterial();
                    mat->setColor(stripColor);
                    lineNode->setMaterial(mat);
                    lineNode->setFlag(QSGNode::OwnsMaterial);
                    appendChildNode(lineNode);
                    m_stripPointsSparseLineNodes.append(lineNode);
                }
            }
        }

        // Current point on strip (POINTS) - blue
        m_contourCurrentPointNode = new DotsNode();
        m_contourCurrentPointNode->addDot(properties->contourCurrentPoint(),
                                          QColor::fromRgbF(0.35f, 0.30f, 0.90f, 1.0f), glm::dp(6.0f));
        m_contourCurrentPointNode->build();
        appendChildNode(m_contourCurrentPointNode);

        // Goal point (POINTS) - yellow
        m_contourGoalPointNode = new DotsNode();
        m_contourGoalPointNode->addDot(properties->contourGoalPoint(),
                                       QColor::fromRgbF(1.0f, 0.95f, 0.095f, 1.0f), glm::dp(6.0f));
        m_contourGoalPointNode->build();
        appendChildNode(m_contourGoalPointNode);

        m_lastContourLineCount = contourLine.count();
        m_lastStripPointsNearbyCount = stripPointsNearby.count();
    } else {
        // Check if strip grew - add more sparse chunks
        const int STRIP_CHUNK_SIZE = 10000;
        const auto &stripPointsSparse = properties->stripPointsSparse();
        int numChunks = (stripPointsSparse.count() + STRIP_CHUNK_SIZE - 1) / STRIP_CHUNK_SIZE;

        if (numChunks > m_lastStripChunks) {
            QColor stripColor = properties->isContourOn()
                ? QColor::fromRgbF(0.983f, 0.92f, 0.420f, 1.0f)
                : QColor::fromRgbF(0.3f, 0.982f, 0.0f, 1.0f);

            for (int c = m_lastStripChunks; c < numChunks; c++) {
                DotsNode *stripChunkNode = new DotsNode();
                int start = c * STRIP_CHUNK_SIZE;
                int end = qMin(start + STRIP_CHUNK_SIZE, stripPointsSparse.count());
                for (int i = start; i < end; i++) {
                    stripChunkNode->addDot(stripPointsSparse[i], stripColor, glm::dp(lineWidth));
                }
                stripChunkNode->build();
                appendChildNode(stripChunkNode);
                m_stripPointsSparseNodes.append(stripChunkNode);
            }
            m_lastStripChunks = numChunks;
        }

        // Update nearby points every frame (rebuild since vehicle moved)
        const auto &stripPointsNearby = properties->stripPointsNearby();
        if (m_stripPointsNearbyNode) {
            // Remove old nearby node
            removeChildNode(m_stripPointsNearbyNode);
            delete m_stripPointsNearbyNode;
        }
        if (!stripPointsNearby.isEmpty()) {
            QColor stripColor = properties->isContourOn()
                ? QColor::fromRgbF(0.983f, 0.92f, 0.420f, 1.0f)
                : QColor::fromRgbF(0.3f, 0.982f, 0.0f, 1.0f);
            m_stripPointsNearbyNode = new DotsNode();
            for (const QVector3D &pt : stripPointsNearby) {
                m_stripPointsNearbyNode->addDot(pt, stripColor, glm::dp(lineWidth));
            }
            m_stripPointsNearbyNode->build();
            appendChildNode(m_stripPointsNearbyNode);
        }

        m_lastContourLineCount = properties->contourLine().count();
        m_lastStripPointsNearbyCount = stripPointsNearby.count();
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

    if (m_shadowOutlineNode)
        updateThickLineNode(m_shadowOutlineNode, mvp, viewportSize, 1,
                            QColor::fromRgbF(0.55f, 0.55f, 0.55f, 0.3f));

    if (m_shadowFillNode) {
        auto *mat = static_cast<AOGFlatColorMaterial *>(m_shadowFillNode->material());
        mat->setMvpMatrix(mvp);
        mat->setViewportSize(viewportSize);
        // Premultiply alpha for Qt scene graph blending
        float a = 0.3f;
        mat->setColor(QColor::fromRgbF(0.5f * a, 0.5f * a, 0.5f * a, a));
    }

    for (QSGGeometryNode *node : m_sideGuideNodes)
        updateThickLineNode(node, mvp, viewportSize, 1,
                            QColor::fromRgbF(0.756f, 0.765f, 0.765f));

    for (QSGGeometryNode *node : m_curveGuideNodes) {
        auto *mat = static_cast<AOGFlatColorMaterial *>(node->material());
        if (mat) {
            mat->setMvpMatrix(mvp);
            mat->setViewportSize(viewportSize);
        }
    }

    if (m_lookaheadNode)
        m_lookaheadNode->updateUniforms(mvp, viewportSize);

    if (m_pursuitCircleNode)
        updateThickLineNode(m_pursuitCircleNode, mvp, viewportSize, lineWidth,
                            QColor::fromRgbF(0.53f, 0.53f, 0.95f));

    if (m_smoothedCurveNode)
        updateThickLineNode(m_smoothedCurveNode, mvp, viewportSize, lineWidth,
                            QColor::fromRgbF(0.93f, 0.92f, 0.26f));

    if (m_currentLineDotsNode)
        m_currentLineDotsNode->updateUniforms(mvp, viewportSize);

    if (m_youTurnDotsNode)
        m_youTurnDotsNode->updateUniforms(mvp, viewportSize);

    // Contour updates
    if (m_contourLineNode) {
        auto *mat = static_cast<AOGFlatColorMaterial *>(m_contourLineNode->material());
        if (mat) {
            mat->setMvpMatrix(mvp);
            mat->setViewportSize(viewportSize);
        }
    }

    for (QSGGeometryNode *lineNode : m_stripPointsSparseLineNodes) {
        if (lineNode) {
            auto *mat = static_cast<AOGFlatColorMaterial *>(lineNode->material());
            if (mat) {
                mat->setMvpMatrix(mvp);
                mat->setViewportSize(viewportSize);
            }
        }
    }

    if (m_contourPointsNode)
        m_contourPointsNode->updateUniforms(mvp, viewportSize);

    if (m_stripPointsNearbyNode)
        m_stripPointsNearbyNode->updateUniforms(mvp, viewportSize);

    for (DotsNode *node : m_stripPointsSparseNodes) {
        if (node)
            node->updateUniforms(mvp, viewportSize);
    }

    if (m_contourCurrentPointNode)
        m_contourCurrentPointNode->updateUniforms(mvp, viewportSize);

    if (m_contourGoalPointNode)
        m_contourGoalPointNode->updateUniforms(mvp, viewportSize);
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
