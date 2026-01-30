// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
//
// Text node implementation

#include "textnode.h"
#include "texturefactory.h"
#include "aoggeometry.h"
#include "materials.h"

TextNode::TextNode(TextureFactory *textureFactory)
    : m_textureFactory(textureFactory)
{
}

TextNode::TextNode(TextureFactory *textureFactory, const QString &text, double size)
    : m_textureFactory(textureFactory)
{
    createTextNode(text, size);
}

void TextNode::createTextNode(const QString &text, double size)
{
    // Delete existing children
    removeAllChildNodes();

    if (!m_textureFactory || text.isEmpty()) {
        return;
    }

    // Get font texture and its dimensions
    QSGTexture *fontTexture = m_textureFactory->texture(TextureId::Font);
    if (!fontTexture) {
        return;
    }

    int texWidth = m_textureFactory->textureWidth(TextureId::Font);
    int texHeight = m_textureFactory->textureHeight(TextureId::Font);

    if (texWidth == 0 || texHeight == 0) {
        return;
    }

    // Calculate UV step sizes
    double u_step = static_cast<double>(FontMetrics::GlyphWidth) / static_cast<double>(texWidth);
    double v_step = static_cast<double>(FontMetrics::GlyphHeight) / static_cast<double>(texHeight);

    double x = 0.0;
    double y = 0.0;

    for (int n = 0; n < text.length(); n++) {
        char idx = text.at(n).toLatin1();

        // Calculate texture coordinates for this character
        double u = static_cast<double>(idx % FontMetrics::GlyphsPerLine) * u_step;
        double v = static_cast<double>(idx / FontMetrics::GlyphsPerLine) * v_step;

        // Create geometry for this character (two triangles forming a quad)
        QSGGeometry *geometry = new QSGGeometry(AOGGeometry::texturedVertexAttributes(), 6);
        geometry->setDrawingMode(QSGGeometry::DrawTriangles);

        TexturedVertex *vertices = static_cast<TexturedVertex *>(geometry->vertexData());

        double glyphW = FontMetrics::GlyphWidth * size;
        double glyphH = FontMetrics::GlyphHeight * size;

        // Triangle 1: bottom-left, bottom-right, top-left
        // Vertex 0: bottom-left
        vertices[0].x = x;
        vertices[0].y = y;
        vertices[0].z = 0;
        vertices[0].u = u;
        vertices[0].v = v + v_step;

        // Vertex 1: bottom-right
        vertices[1].x = x + glyphW;
        vertices[1].y = y;
        vertices[1].z = 0;
        vertices[1].u = u + u_step;
        vertices[1].v = v + v_step;

        // Vertex 2: top-left
        vertices[2].x = x;
        vertices[2].y = y + glyphH;
        vertices[2].z = 0;
        vertices[2].u = u;
        vertices[2].v = v;

        // Triangle 2: top-left, bottom-right, top-right
        // Vertex 3: top-left (same as vertex 2)
        vertices[3].x = x;
        vertices[3].y = y + glyphH;
        vertices[3].z = 0;
        vertices[3].u = u;
        vertices[3].v = v;

        // Vertex 4: bottom-right (same as vertex 1)
        vertices[4].x = x + glyphW;
        vertices[4].y = y;
        vertices[4].z = 0;
        vertices[4].u = u + u_step;
        vertices[4].v = v + v_step;

        // Vertex 5: top-right
        vertices[5].x = x + glyphW;
        vertices[5].y = y + glyphH;
        vertices[5].z = 0;
        vertices[5].u = u + u_step;
        vertices[5].v = v;

        // Create material
        AOGTextureMaterial *material = new AOGTextureMaterial;
        material->setTexture(fontTexture);
        material->setFlag(QSGMaterial::Blending);

        // Create geometry node
        QSGGeometryNode *charNode = new QSGGeometryNode;
        charNode->setGeometry(geometry);
        charNode->setMaterial(material);
        charNode->setFlag(QSGNode::OwnsGeometry);
        charNode->setFlag(QSGNode::OwnsMaterial);

        appendChildNode(charNode);

        // Advance to next character position
        x += FontMetrics::CharXSpacing * size;
    }
}

void TextNode::updateUniforms(const QMatrix4x4 &mvp,
                              const QSize &viewportSize,
                              const QColor &color,
                              bool useColor)
{
    // Walk all child nodes and update their materials
    QSGNode *child = firstChild();
    while (child) {
        QSGGeometryNode *geomNode = dynamic_cast<QSGGeometryNode *>(child);
        if (geomNode) {
            AOGTextureMaterial *material = dynamic_cast<AOGTextureMaterial *>(geomNode->material());
            if (material) {
                material->setMvpMatrix(mvp);
                material->setViewportSize(viewportSize);
                material->setColor(color);
                material->setUseColor(useColor);
            }
            geomNode->markDirty(QSGNode::DirtyMaterial);
        }
        child = child->nextSibling();
    }
}

double TextNode::textWidth(const QString &text, double size)
{
    if (text.isEmpty()) {
        return 0.0;
    }
    // Width is CharXSpacing * size for each character except the last,
    // plus the full glyph width for the last character
    return (text.length() - 1) * FontMetrics::CharXSpacing * size
           + FontMetrics::GlyphWidth * size;
}
