// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
//
// Field surface node implementation

#include "fieldsurfacenode.h"
#include "materials.h"
#include "aoggeometry.h"

#include <QSGTexture>

FieldSurfaceNode::FieldSurfaceNode()
    : m_geomNode(nullptr)
    , m_isTextured(false)
{
}

FieldSurfaceNode::~FieldSurfaceNode()
{
    // Child nodes are owned by the scene graph and will be deleted automatically
}

void FieldSurfaceNode::clearChildren()
{
    while (childCount() > 0) {
        QSGNode *child = firstChild();
        removeChildNode(child);
        delete child;
    }
    m_geomNode = nullptr;
}

void FieldSurfaceNode::update(const QMatrix4x4 &mvp,
                               const QColor &fieldColor,
                               bool isTextureOn,
                               QSGTexture *texture,
                               double eastingMin, double eastingMax,
                               double northingMin, double northingMax,
                               int textureCount)
{
    // Determine if we should use texture
    bool useTexture = isTextureOn && texture;

    // If mode changed, we need to rebuild, or at very start
    if (useTexture != m_isTextured || !m_init_children) {
        clearChildren();
        m_isTextured = useTexture;
        m_init_children = true;
    }

    if (useTexture) {
        updateTextured(mvp, fieldColor, texture,
                       eastingMin, eastingMax, northingMin, northingMax,
                       textureCount);
    } else {
        updateSolidColor(mvp, fieldColor,
                         eastingMin, eastingMax, northingMin, northingMax);
    }
}

void FieldSurfaceNode::updateTextured(const QMatrix4x4 &mvp, const QColor &fieldColor,
                                       QSGTexture *texture,
                                       double eastingMin, double eastingMax,
                                       double northingMin, double northingMax,
                                       int textureCount)
{
    const float surfaceZ = -0.10f;

    if (!m_geomNode) {
        // Create new geometry node
        auto *geometry = new QSGGeometry(AOGGeometry::texturedVertexAttributes(), 4);
        geometry->setDrawingMode(QSGGeometry::DrawTriangleStrip);

        m_geomNode = new QSGGeometryNode();
        m_geomNode->setGeometry(geometry);
        m_geomNode->setFlag(QSGNode::OwnsGeometry);

        auto *material = new AOGTextureMaterial();
        m_geomNode->setMaterial(material);
        m_geomNode->setFlag(QSGNode::OwnsMaterial);

        appendChildNode(m_geomNode);
    }

    // Update geometry
    auto *geometry = m_geomNode->geometry();
    TexturedVertex *data = static_cast<TexturedVertex *>(geometry->vertexData());

    // Vertex order for triangle strip: top-left, top-right, bottom-left, bottom-right
    data[0].x = static_cast<float>(eastingMin);
    data[0].y = static_cast<float>(northingMax);
    data[0].z = surfaceZ;
    data[0].u = 0.0f;
    data[0].v = 0.0f;

    data[1].x = static_cast<float>(eastingMax);
    data[1].y = static_cast<float>(northingMax);
    data[1].z = surfaceZ;
    data[1].u = static_cast<float>(textureCount);
    data[1].v = 0.0f;

    data[2].x = static_cast<float>(eastingMin);
    data[2].y = static_cast<float>(northingMin);
    data[2].z = surfaceZ;
    data[2].u = 0.0f;
    data[2].v = static_cast<float>(textureCount);

    data[3].x = static_cast<float>(eastingMax);
    data[3].y = static_cast<float>(northingMin);
    data[3].z = surfaceZ;
    data[3].u = static_cast<float>(textureCount);
    data[3].v = static_cast<float>(textureCount);

    m_geomNode->markDirty(QSGNode::DirtyGeometry);

    // Update material
    auto *material = static_cast<AOGTextureMaterial *>(m_geomNode->material());
    material->setTexture(texture);
    material->setColor(fieldColor);
    material->setUseColor(true);
    material->setMvpMatrix(mvp);
    m_geomNode->markDirty(QSGNode::DirtyMaterial);
}

void FieldSurfaceNode::updateSolidColor(const QMatrix4x4 &mvp, const QColor &fieldColor,
                                         double eastingMin, double eastingMax,
                                         double northingMin, double northingMax)
{
    const float surfaceZ = -0.10f;

    if (!m_geomNode) {
        // Create new geometry node
        auto *geometry = new QSGGeometry(AOGGeometry::positionAttributes(), 4);
        geometry->setDrawingMode(QSGGeometry::DrawTriangleStrip);

        m_geomNode = new QSGGeometryNode();
        m_geomNode->setGeometry(geometry);
        m_geomNode->setFlag(QSGNode::OwnsGeometry);

        auto *material = new AOGFlatColorMaterial();
        m_geomNode->setMaterial(material);
        m_geomNode->setFlag(QSGNode::OwnsMaterial);

        appendChildNode(m_geomNode);
    }

    // Update existing geometry
    auto *geometry = m_geomNode->geometry();
    PositionVertex *data = static_cast<PositionVertex *>(geometry->vertexData());

    data[0].x = static_cast<float>(eastingMin);
    data[0].y = static_cast<float>(northingMax);
    data[0].z = surfaceZ;

    data[1].x = static_cast<float>(eastingMax);
    data[1].y = static_cast<float>(northingMax);
    data[1].z = surfaceZ;

    data[2].x = static_cast<float>(eastingMin);
    data[2].y = static_cast<float>(northingMin);
    data[2].z = surfaceZ;

    data[3].x = static_cast<float>(eastingMax);
    data[3].y = static_cast<float>(northingMin);
    data[3].z = surfaceZ;

    m_geomNode->markDirty(QSGNode::DirtyGeometry);

    // Update material
    auto *material = static_cast<AOGFlatColorMaterial *>(m_geomNode->material());
    material->setColor(fieldColor);
    material->setMvpMatrix(mvp);
    m_geomNode->markDirty(QSGNode::DirtyMaterial);
}
