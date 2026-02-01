// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
//
// Custom QSGMaterial classes for scene graph rendering with 3D projection

#ifndef MATERIALS_H
#define MATERIALS_H

#include <QSGMaterial>
#include <QSGMaterialShader>
#include <QMatrix4x4>
#include <QColor>
#include <QSGTexture>
#include "aogmaterial.h"

// ============================================================================
// AOGFlatColorMaterial - Single color with MVP matrix for 3D projection
// Used for: boundaries, guidance lines, simple shapes
// ============================================================================

class AOGFlatColorMaterial : public AOGMaterial
{
public:
    AOGFlatColorMaterial();

    QSGMaterialType *type() const override;
    QSGMaterialShader *createShader(QSGRendererInterface::RenderMode renderMode) const override;
    int compare(const QSGMaterial *other) const override;

    void setColor(const QColor &color);
    QColor color() const { return m_color; }

    void setPointSize(float size);
    float pointSize() const { return m_pointSize; }

private:
    QColor m_color = Qt::white;
    float m_pointSize = 1.0f;
};

class AOGFlatColorShader : public AOGMaterialShader
{
public:
    AOGFlatColorShader();

    bool updateUniformData(RenderState &state, QSGMaterial *newMaterial, QSGMaterial *oldMaterial) override;
};

// ============================================================================
// AOGVertexColorMaterial - Per-vertex color with MVP matrix
// Used for: coverage patches, colored polygons
// ============================================================================

class AOGVertexColorMaterial : public AOGMaterial
{
public:
    AOGVertexColorMaterial();

    QSGMaterialType *type() const override;
    QSGMaterialShader *createShader(QSGRendererInterface::RenderMode renderMode) const override;
    int compare(const QSGMaterial *other) const override;

    void setPointSize(float size);
    float pointSize() const { return m_pointSize; }

private:
    float m_pointSize = 1.0f;
};

class AOGVertexColorShader : public AOGMaterialShader
{
public:
    AOGVertexColorShader();

    bool updateUniformData(RenderState &state, QSGMaterial *newMaterial, QSGMaterial *oldMaterial) override;
};

// ============================================================================
// AOGTextureMaterial - Textured geometry with MVP matrix
// Used for: vehicle icons, markers, images
// ============================================================================

class AOGTextureMaterial : public AOGMaterial
{
public:
    AOGTextureMaterial();

    QSGMaterialType *type() const override;
    QSGMaterialShader *createShader(QSGRendererInterface::RenderMode renderMode) const override;
    int compare(const QSGMaterial *other) const override;

    void setTexture(QSGTexture *texture);
    QSGTexture *texture() const { return m_texture; }

    void setColor(const QColor &color);
    QColor color() const { return m_color; }

    void setUseColor(bool use);
    bool useColor() const { return m_useColor; }

private:
    QSGTexture *m_texture = nullptr;
    QColor m_color = Qt::white;
    bool m_useColor = false;
};

class AOGTextureShader : public AOGMaterialShader
{
public:
    AOGTextureShader();

    bool updateUniformData(RenderState &state, QSGMaterial *newMaterial, QSGMaterial *oldMaterial) override;
    void updateSampledImage(RenderState &state, int binding, QSGTexture **texture,
                            QSGMaterial *newMaterial, QSGMaterial *oldMaterial) override;
};

#endif // MATERIALS_H
