// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
//
// Boundary node implementation

#include "boundariesnode.h"
#include "boundariesproperties.h"
#include "materials.h"
#include "thicklinematerial.h"
#include "dashedthicklinematerial.h"
#include "thicklinecolorsmaterial.h"
#include "roundpointsizematerial.h"
#include "aoggeometry.h"
#include "glm.h"

BoundariesNode::BoundariesNode()
{
}

BoundariesNode::~BoundariesNode()
{
}

void BoundariesNode::clearChildren()
{
    while (childCount() > 0) {
        QSGNode *child = firstChild();
        removeChildNode(child);
        delete child;
    }

    m_outerNodes.clear();
    m_innerNodes.clear();
    m_bndBeingMadeNode = nullptr;
    m_lastPointToPivotNode = nullptr;
    m_beingMadeDotsNode = nullptr;
}

void BoundariesNode::update(const QMatrix4x4 &mv,
                            const QMatrix4x4 &p,
                            const QMatrix4x4 &ndc,
                            const QSize &viewportSize,
                            float vehicleX, float vehicleY, float vehicleHeading,
                            bool isOutOfBounds,
                            int lineWidth,
                            const BoundariesProperties *properties)
{
    // Clear existing boundary geometry
    if (childCount() < 1) {
        // Draw outer boundaries

        // Create geometry for each boundary
        for (const BoundaryProperties *boundary : std::as_const(properties->outer())) {

            if (!boundary->visible() || boundary->points().count() < 3)
                continue;

            // Create line loop geometry
            auto *geometry = AOGGeometry::createThickLineLoopGeometry(boundary->points());
            if (!geometry)
                continue;

            auto *geomNode = new QSGGeometryNode();
            geomNode->setGeometry(geometry);
            geomNode->setFlag(QSGNode::OwnsGeometry);

            // Create material with MVP matrix
            auto *material = new ThickLineMaterial();
            geomNode->setMaterial(material);
            geomNode->setFlag(QSGNode::OwnsMaterial);

            appendChildNode(geomNode);
            m_outerNodes.append(geomNode);
        }

        // Create geometry for each boundary
        for (const BoundaryProperties *boundary : std::as_const(properties->inner())) {

            if (!boundary->visible() || boundary->points().count() < 3)
                continue;

            // Create line loop geometry
            auto *geometry = AOGGeometry::createThickLineLoopGeometry(boundary->points());
            if (!geometry)
                continue;

            auto *geomNode = new QSGGeometryNode();
            geomNode->setGeometry(geometry);
            geomNode->setFlag(QSGNode::OwnsGeometry);

            // Create material with MVP matrix
            auto *material = new ThickLineMaterial();

            geomNode->setMaterial(material);
            geomNode->setFlag(QSGNode::OwnsMaterial);

            appendChildNode(geomNode);
            m_innerNodes.append(geomNode);
        }

    }

    //every update update the boundary being made since it changes
    if (properties->beingMade().count() > 0) {
        // draw boundary being made so far.
        QVector<ColorVertexVectors> beingMadePoints;

        for (const QVector3D &v: properties->beingMade()) {
            beingMadePoints.append ({ v, QColor::fromRgbF(0.825f, 0.22f, 0.90f, 1.0f) });
        }

        //add a green line that closes the boundary shape

        beingMadePoints[beingMadePoints.count()-1].color = QColor::fromRgbF(0.295f, 0.972f, 0.290f, 1.0f);
        beingMadePoints.append( { beingMadePoints[0].vertex,
                                  QColor::fromRgbF(0.295f, 0.972f, 0.290f, 1.0f)});

        QSGGeometry *geometry;
        if (!m_bndBeingMadeNode) {
            geometry = AOGGeometry::createThickLineColorsGeometry(beingMadePoints);
            m_bndBeingMadeNode = new QSGGeometryNode();
            m_bndBeingMadeNode->setGeometry(geometry);
            m_bndBeingMadeNode->setFlag(QSGNode::OwnsGeometry);

            auto *material = new ThickLineColorsMaterial();

            m_bndBeingMadeNode->setMaterial(material);
            m_bndBeingMadeNode->setFlag(QSGNode::OwnsMaterial);
            appendChildNode(m_bndBeingMadeNode);

        } else {
            //update the geometry, have to allocate enough for all the extra vertices
            //double wide lines need -- see createThickLineColorsGeometry()
            int numVertices = (beingMadePoints.count()-1) * 4 + (beingMadePoints.count() - 2) * 2;

            geometry = m_bndBeingMadeNode->geometry();
            geometry->allocate(numVertices);
            m_bndBeingMadeNode->markDirty(QSGNode::DirtyGeometry);
            ThickLineColorsVertex *data = static_cast<ThickLineColorsVertex *>(geometry->vertexData());

            //use update wrapper because of how each line needs four vertices
            AOGGeometry::updateThickLineColorsGeometry(data, beingMadePoints);
        }

        //draw dashed line from last point to the pivot marker
        if (properties->markBoundary()) {
            QVector<QVector3D> toPivot;
            toPivot.append( properties->beingMade()[0] );
            toPivot.append({static_cast<float>(vehicleX + sin(glm::toRadians(vehicleHeading) - glm::PIBy2) * -properties->markBoundary()),
                            static_cast<float>(vehicleY + cos(glm::toRadians(vehicleHeading) - glm::PIBy2) * -properties->markBoundary()),
                            0});
            toPivot.append( properties->beingMade().last());

            if (!m_lastPointToPivotNode) {
                auto *geometry = AOGGeometry::createDashedThickLineGeometry(toPivot);
                if (geometry) {
                    m_lastPointToPivotNode = new QSGGeometryNode();
                    m_lastPointToPivotNode->setGeometry(geometry);
                    m_lastPointToPivotNode->setFlag(QSGNode::OwnsGeometry);

                    // Create material with MVP matrix
                    auto *material = new DashedThickLineMaterial();

                    m_lastPointToPivotNode->setMaterial(material);
                    m_lastPointToPivotNode->setFlag(QSGNode::OwnsMaterial);

                    appendChildNode(m_lastPointToPivotNode);
                }
            } else {
                //update geometry
                int numVertices = (toPivot.count()-1) * 4 + (toPivot.count() - 2) * 2;
                geometry = m_lastPointToPivotNode->geometry();
                geometry->allocate(numVertices);
                m_lastPointToPivotNode->markDirty(QSGNode::DirtyGeometry);
                DashedThickLineVertex *data = static_cast<DashedThickLineVertex *>(geometry->vertexData());
                AOGGeometry::updateDashedThickLineGeometry(data, toPivot);
            }
        }

        //draw dots on all the boundary points being made now
        if (!m_beingMadeDotsNode) {
            geometry = new QSGGeometry(AOGGeometry::colorSizeVertexAttributes(), properties->beingMade().count());
            geometry->setDrawingMode(QSGGeometry::DrawPoints);
            // Fill vertex data
            ColorSizeVertex *data = static_cast<ColorSizeVertex*>(geometry->vertexData());
            float dotSize = glm::dp(6.0f); //take into account scaling.
            for (const QVector3D &v : properties->beingMade()) {
                *data = { v.x(), v.y(), v.z(),  0.0f, 0.95f, 0.95f, 1.0f, dotSize };
                data++;
            }

            m_beingMadeDotsNode = new QSGGeometryNode();
            m_beingMadeDotsNode->setGeometry(geometry);
            m_beingMadeDotsNode->setFlag(QSGNode::OwnsGeometry);

            auto *material = new RoundPointSizeMaterial();
            m_beingMadeDotsNode->setMaterial(material);
            m_beingMadeDotsNode->setFlag(QSGNode::OwnsMaterial);
            appendChildNode(m_beingMadeDotsNode);

        } else {
            //update the geometry
            geometry = m_beingMadeDotsNode->geometry();
            geometry->allocate(properties->beingMade().count());
            m_beingMadeDotsNode->markDirty(QSGNode::DirtyGeometry);
            ColorSizeVertex *data = static_cast<ColorSizeVertex *>(geometry->vertexData());
            float dotSize = glm::dp(6.0f); //take into account scaling.
            for (const QVector3D &v : properties->beingMade()) {
                *data = { v.x(), v.y(), v.z(),  0.0f, 0.95f, 0.95f, 1.0f, dotSize };
                data++;
            }
        }

    }

    //update uniforms

    int lineWidth2 = isOutOfBounds ? lineWidth * 3 : lineWidth;
    for (QSGGeometryNode *node:m_outerNodes) {
        updateThickLineNode(node, ndc * p * mv, viewportSize, lineWidth2, properties->colorOuter());
    }
    for (QSGGeometryNode *node:m_innerNodes) {
        updateThickLineNode(node, ndc * p * mv, viewportSize, lineWidth2, properties->colorInner());
    }

    if (m_bndBeingMadeNode) {
        auto *material = static_cast<ThickLineColorsMaterial *>(m_bndBeingMadeNode->material());
        if (material) {
            material->setMvpMatrix(ndc * p * mv);
            material->setViewportSize(viewportSize);
            material->setLineWidth(lineWidth2);
        }
    }

    if (m_lastPointToPivotNode) {
        updateDashedLineNode(m_lastPointToPivotNode, ndc*p*mv,
                             viewportSize,
                             lineWidth2,
                             QColor::fromRgbF(0.825f, 0.842f, 0.0f));
    }

    if (m_beingMadeDotsNode) {
        auto *material = static_cast<AOGMaterial *>(m_beingMadeDotsNode->material());
        if (material) {
            material->setMvpMatrix(ndc * p * mv);
            material->setViewportSize(viewportSize);
        }
    }
}

void BoundariesNode::updateThickLineNode(QSGGeometryNode *node,
                                         const QMatrix4x4 mvp,
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

void BoundariesNode::updateDashedLineNode(QSGGeometryNode *node,
                                          const QMatrix4x4 mvp,
                                          const QSize &viewportSize,
                                          int lineWidth, const QColor &color,
                                          int dashLength,
                                          int dashGap)
{
    auto *material = static_cast<DashedThickLineMaterial *>(node->material());
    if (material) {
        material->setMvpMatrix(mvp);
        material->setViewportSize(viewportSize);
        material->setLineWidth(lineWidth);
        material->setDashLength(dashLength);
        material->setGapLength(dashGap);
        material->setColor(color);
    }
}
