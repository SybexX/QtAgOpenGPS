// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
//
// Round point material implementation

#include "roundpointsizematerial.h"

static QSGMaterialType roundPointMaterialType;

RoundPointSizeMaterial::RoundPointSizeMaterial()
{
    setFlag(Blending);  // Enable blending for soft edges
}

QSGMaterialType *RoundPointSizeMaterial::type() const
{
    return &roundPointMaterialType;
}

QSGMaterialShader *RoundPointSizeMaterial::createShader(QSGRendererInterface::RenderMode) const
{
    return new RoundPointSizeShader();
}

int RoundPointSizeMaterial::compare(const QSGMaterial *other) const
{
    const auto *o = static_cast<const RoundPointSizeMaterial *>(other);

    if (m_mvpMatrix != o->m_mvpMatrix)
        return &m_mvpMatrix < &o->m_mvpMatrix ? -1 : 1;

    if (m_softness != o->m_softness)
        return m_softness < o->m_softness ? -1 : 1;

    return 0;
}

// ============================================================================
// RoundPointSizeShader Implementation
// ============================================================================

RoundPointSizeShader::RoundPointSizeShader()
{
    setShaderFileName(VertexStage, QLatin1String(":/AOG/shaders/roundpointsize.vert.qsb"));
    setShaderFileName(FragmentStage, QLatin1String(":/AOG/shaders/roundpointsize.frag.qsb"));
}

bool RoundPointSizeShader::updateUniformData(RenderState &state, QSGMaterial *newMaterial, QSGMaterial *oldMaterial)
{
    Q_UNUSED(oldMaterial);

    bool changed = false;
    QByteArray *buf = state.uniformData();

    auto *material = static_cast<RoundPointSizeMaterial *>(newMaterial);

    // Uniform buffer layout (std140):
    // offset 0:  mat4 mvpMatrix (64 bytes)
    // offset 64: float softness (4 bytes)

    // Update MVP matrix
    QMatrix4x4 combinedMatrix = state.combinedMatrix() * material->mvpMatrix();
    memcpy(buf->data(), combinedMatrix.constData(), 64);
    changed = true;

    // Update softness
    float softness = material->softness();
    memcpy(buf->data() + 64, &softness, 4);

    return changed;
}
