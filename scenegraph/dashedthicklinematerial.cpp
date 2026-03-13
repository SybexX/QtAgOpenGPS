// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
//
// Dashed thick line material implementation

#include "dashedthicklinematerial.h"
#include <QFile>
#include <QDebug>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(dashedthicklinematerial_log, "dashedthicklinematerial.qtagopengps")

// ============================================================================
// DashedThickLineMaterial
// ============================================================================

DashedThickLineMaterial::DashedThickLineMaterial()
{
    setFlag(Blending);  // Support transparent lines
}

QSGMaterialType *DashedThickLineMaterial::type() const
{
    static QSGMaterialType type;
    return &type;
}

QSGMaterialShader *DashedThickLineMaterial::createShader(QSGRendererInterface::RenderMode) const
{
    return new DashedThickLineMaterialShader();
}

int DashedThickLineMaterial::compare(const QSGMaterial *other) const
{
    const auto *o = static_cast<const DashedThickLineMaterial *>(other);

    if (m_color != o->m_color)
        return m_color.rgba() < o->m_color.rgba() ? -1 : 1;

    if (m_lineWidth != o->m_lineWidth)
        return m_lineWidth < o->m_lineWidth ? -1 : 1;

    if (m_dashLength != o->m_dashLength)
        return m_dashLength < o->m_dashLength ? -1 : 1;

    if (m_gapLength != o->m_gapLength)
        return m_gapLength < o->m_gapLength ? -1 : 1;

    if (m_viewportSize != o->m_viewportSize)
        return -1;

    // Matrices differ per frame, always return non-equal to force update
    if (m_mvpMatrix != o->m_mvpMatrix)
        return -1;

    return 0;
}

void DashedThickLineMaterial::setColor(const QColor &color)
{
    m_color = color;
}

void DashedThickLineMaterial::setLineWidth(float width)
{
    m_lineWidth = width;
}

void DashedThickLineMaterial::setDashLength(float length)
{
    m_dashLength = length;
}

void DashedThickLineMaterial::setGapLength(float length)
{
    m_gapLength = length;
}

const QSGGeometry::AttributeSet &DashedThickLineMaterial::attributes()
{
    // Use the attribute set defined in AOGGeometry
    return AOGGeometry::dashedThickLineAttributes();
}

// ============================================================================
// DashedThickLineMaterialShader
// ============================================================================

DashedThickLineMaterialShader::DashedThickLineMaterialShader()
{
    setShaderFileName(VertexStage, QLatin1String(":/AOG/shaders/dashedthickline.vert.qsb"));
    setShaderFileName(FragmentStage, QLatin1String(":/AOG/shaders/dashedthickline.frag.qsb"));
}

bool DashedThickLineMaterialShader::updateUniformData(RenderState &state,
                                                       QSGMaterial *newMaterial,
                                                       QSGMaterial *oldMaterial)
{
    Q_UNUSED(oldMaterial);
    Q_UNUSED(state);

    bool changed = false;
    QByteArray *buf = state.uniformData();

    auto *material = static_cast<DashedThickLineMaterial *>(newMaterial);

    // Uniform buffer layout (std140):
    // offset 0:   mat4 mvpMatrix    (64 bytes)
    // offset 64: vec4 color        (16 bytes)
    // offset 80: vec2 viewportSize (8 bytes)
    // offset 88: float lineWidth   (4 bytes)
    // offset 92: float dashLength  (4 bytes)
    // offset 96: float gapLength   (4 bytes)
    // Total: 228 bytes (padded to 240 for std140 alignment)

    // Update full, combined MVP matrix
    QMatrix4x4 combined = state.combinedMatrix() * material->mvpMatrix();
    memcpy(buf->data(), combined.constData(), 64);
    changed = true;

    // Update color (offset 192)
    QColor c = material->color();
    float colorData[4] = {
        static_cast<float>(c.redF()),
        static_cast<float>(c.greenF()),
        static_cast<float>(c.blueF()),
        static_cast<float>(c.alphaF())
    };
    memcpy(buf->data() + 64, colorData, 16);

    // Update viewport size (offset 208)
    QSize vp = material->viewportSize();
    float viewportSize[2] = {
        static_cast<float>(vp.width()),
        static_cast<float>(vp.height())
    };
    memcpy(buf->data() + 80, viewportSize, 8);

    // Update line width (offset 216)
    float lineWidth = material->lineWidth();
    memcpy(buf->data() + 88, &lineWidth, 4);

    // Update dash length (offset 220)
    float dashLength = material->dashLength();
    memcpy(buf->data() + 92, &dashLength, 4);

    // Update gap length (offset 224)
    float gapLength = material->gapLength();
    memcpy(buf->data() + 96, &gapLength, 4);

    return changed;
}
