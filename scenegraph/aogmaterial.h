// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
//
// Custom QSGMaterial classes for scene graph rendering with 3D projection

#ifndef AOGMATERIAL_H
#define AOGMATERIAL_H

#include <QSGMaterial>
#include <QSGMaterialShader>
#include <QMatrix4x4>
#include <QSize>
#include <QSGTexture>

// ============================================================================b
// AOGMaterial - incomplete base class for all our materials
// Contains a common setFullMatrix method that all our nodes will
// use to make updating the matrix easier.
// ============================================================================

class AOGMaterial : public QSGMaterial
{
public:
    AOGMaterial()=default;

    void setMvpMatrix(const QMatrix4x4 &fullMatrix);
    QMatrix4x4 mvpMatrix() const { return m_mvpMatrix; }

    void setViewportSize(const QSize &size);
    QSize viewportSize() const { return m_viewportSize; }


protected:
    QMatrix4x4 m_mvpMatrix;
    QSize m_viewportSize;
};

// ============================================================================
// AOGMaterialShader - base class for all our shaders
// Provides default blend function: glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA)
// ============================================================================

class AOGMaterialShader : public QSGMaterialShader
{
public:
    AOGMaterialShader()
    {
        setFlag(UpdatesGraphicsPipelineState, true);
    }

    bool updateGraphicsPipelineState(RenderState &state, GraphicsPipelineState *ps,
                                     QSGMaterial *newMaterial, QSGMaterial *oldMaterial) override
    {
        Q_UNUSED(state);
        Q_UNUSED(newMaterial);
        Q_UNUSED(oldMaterial);

        // Set blend function: glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA)
        ps->blendEnable = true;
        ps->srcColor = GraphicsPipelineState::SrcAlpha;
        ps->dstColor = GraphicsPipelineState::OneMinusSrcAlpha;
        ps->srcAlpha = GraphicsPipelineState::SrcAlpha;
        ps->dstAlpha = GraphicsPipelineState::OneMinusSrcAlpha;

        return true;
    }
};

#endif // AOGMATERIAL_H
