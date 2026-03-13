// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
//
// Round point material implementation

#include "roundpointmaterial.h"

static QSGMaterialType roundPointMaterialType;

RoundPointMaterial::RoundPointMaterial()
{
    setFlag(Blending);  // Enable blending for soft edges
}

QSGMaterialType *RoundPointMaterial::type() const
{
    return &roundPointMaterialType;
}

QSGMaterialShader *RoundPointMaterial::createShader(QSGRendererInterface::RenderMode) const
{
    return new RoundPointShader();
}

int RoundPointMaterial::compare(const QSGMaterial *other) const
{
    const auto *o = static_cast<const RoundPointMaterial *>(other);

    if (m_mvpMatrix != o->m_mvpMatrix)
        return &m_mvpMatrix < &o->m_mvpMatrix ? -1 : 1;

    if (m_pointSize != o->m_pointSize)
        return m_pointSize < o->m_pointSize ? -1 : 1;

    if (m_softness != o->m_softness)
        return m_softness < o->m_softness ? -1 : 1;

    return 0;
}

// ============================================================================
// RoundPointShader Implementation
// ============================================================================

RoundPointShader::RoundPointShader()
{
    setShaderFileName(VertexStage, QLatin1String(":/AOG/shaders/roundpoint.vert.qsb"));
    setShaderFileName(FragmentStage, QLatin1String(":/AOG/shaders/roundpoint.frag.qsb"));
}

bool RoundPointShader::updateUniformData(RenderState &state, QSGMaterial *newMaterial, QSGMaterial *oldMaterial)
{
    Q_UNUSED(oldMaterial);

    bool changed = false;
    QByteArray *buf = state.uniformData();

    auto *material = static_cast<RoundPointMaterial *>(newMaterial);

    // Uniform buffer layout (std140):
    // offset 0:  mat4 mvpMatrix (64 bytes)
    // offset 64: float pointSize (4 bytes)
    // offset 68: float softness (4 bytes)

    // Update MVP matrix
    QMatrix4x4 combinedMatrix = state.combinedMatrix() * material->mvpMatrix();
    memcpy(buf->data(), combinedMatrix.constData(), 64);
    changed = true;

    // Update point size
    float pointSize = material->pointSize();
    memcpy(buf->data() + 64, &pointSize, 4);

    // Update softness
    float softness = material->softness();
    memcpy(buf->data() + 68, &softness, 4);

    return changed;
}
