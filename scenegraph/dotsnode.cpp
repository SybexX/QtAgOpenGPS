// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
//
// Dot node implementation

#include "dotsnode.h"
#include "roundpointsizematerial.h"

DotsNode::DotsNode()
{
}

void DotsNode::addDot(const QVector3D &position, const QColor &color, float size)
{
    m_dots.append({position, color, size});
}

void DotsNode::addDot(const ColorSizeVertexVectors &dot)
{
    m_dots.append(dot);
}

void DotsNode::addDots(const QVector<ColorSizeVertexVectors> &dots)
{
    m_dots.append(dots);
}

void DotsNode::build()
{
    if (m_dots.isEmpty()) {
        // No dots to render, remove existing node if any
        if (m_geometryNode) {
            removeChildNode(m_geometryNode);
            delete m_geometryNode;
            m_geometryNode = nullptr;
        }
        return;
    }

    if (!m_geometryNode) {
        // Create new geometry node
        QSGGeometry *geometry = new QSGGeometry(AOGGeometry::colorSizeVertexAttributes(), m_dots.count());
        geometry->setDrawingMode(QSGGeometry::DrawPoints);

        m_geometryNode = new QSGGeometryNode();
        m_geometryNode->setGeometry(geometry);
        m_geometryNode->setFlag(QSGNode::OwnsGeometry);

        auto *material = new RoundPointSizeMaterial();
        m_geometryNode->setMaterial(material);
        m_geometryNode->setFlag(QSGNode::OwnsMaterial);

        appendChildNode(m_geometryNode);
    } else {
        // Update existing geometry
        QSGGeometry *geometry = m_geometryNode->geometry();
        geometry->allocate(m_dots.count());
        m_geometryNode->markDirty(QSGNode::DirtyGeometry);
    }

    // Fill vertex data
    ColorSizeVertex *data = static_cast<ColorSizeVertex *>(m_geometryNode->geometry()->vertexData());
    for (const ColorSizeVertexVectors &dot : m_dots) {
        data->x = dot.position.x();
        data->y = dot.position.y();
        data->z = dot.position.z();
        data->r = dot.color.redF();
        data->g = dot.color.greenF();
        data->b = dot.color.blueF();
        data->a = dot.color.alphaF();
        data->s = dot.size;
        data++;
    }
}

void DotsNode::clearChildren()
{
    if (m_geometryNode) {
        removeChildNode(m_geometryNode);
        delete m_geometryNode;
        m_geometryNode = nullptr;
    }
    m_dots.clear();
}

void DotsNode::updateUniforms(const QMatrix4x4 &mvp, const QSize &viewportSize)
{
    if (m_geometryNode) {
        RoundPointSizeMaterial *material = static_cast<RoundPointSizeMaterial *>(m_geometryNode->material());
        if (material) {
            material->setMvpMatrix(mvp);
            material->setViewportSize(viewportSize);
        }
        m_geometryNode->markDirty(QSGNode::DirtyMaterial);
    }
}
