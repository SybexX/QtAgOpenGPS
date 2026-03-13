// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
//
// Text node - scene graph node for text rendering using font texture

#ifndef TEXTNODE_H
#define TEXTNODE_H

#include <QSGNode>
#include <QSGGeometryNode>
#include <QMatrix4x4>
#include <QColor>
#include <QString>
#include <QSize>

class TextureFactory;

// Font texture atlas parameters (matching glutils.cpp)
namespace FontMetrics {
    constexpr int GlyphsPerLine = 16;
    constexpr int GlyphWidth = 16;
    constexpr int GlyphHeight = 32;
    constexpr int CharXSpacing = 14;
}

class TextNode : public QSGNode
{
public:
    explicit TextNode(TextureFactory *textureFactory);
    TextNode(TextureFactory *textureFactory, const QString &text, double size);

    // Rebuild the text geometry
    // Deletes existing children and creates new child nodes for each character
    // size: scale factor for the glyphs
    void createTextNode(const QString &text, double size);

    // Update the MVP matrix and color for all child nodes
    // mvp: the full model-view-projection matrix
    // viewportSize: current viewport size (needed by some materials)
    // color: tint color for the texture
    // useColor: whether to apply the color tint
    void updateUniforms(const QMatrix4x4 &mvp,
                        const QSize &viewportSize,
                        const QColor &color,
                        bool useColor = true);

    // Get the width of rendered text in world units (before size scaling)
    static double textWidth(const QString &text, double size);

private:
    TextureFactory *m_textureFactory;
};

#endif // TEXTNODE_H
