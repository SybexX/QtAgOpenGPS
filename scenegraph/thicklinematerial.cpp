// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
//
// Thick line material implementation

#include "thicklinematerial.h"
#include <QFile>
#include <QDebug>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(thicklinematerial_log, "thicklinematerial.qtagopengps")

// ============================================================================
// ThickLineMaterial
// ============================================================================

ThickLineMaterial::ThickLineMaterial()
{
    setFlag(Blending);  // Support transparent lines
}

QSGMaterialType *ThickLineMaterial::type() const
{
    static QSGMaterialType type;
    return &type;
}

QSGMaterialShader *ThickLineMaterial::createShader(QSGRendererInterface::RenderMode) const
{
    return new ThickLineMaterialShader();
}

int ThickLineMaterial::compare(const QSGMaterial *other) const
{
    const auto *o = static_cast<const ThickLineMaterial *>(other);

    if (m_color != o->m_color)
        return m_color.rgba() < o->m_color.rgba() ? -1 : 1;

    if (m_lineWidth != o->m_lineWidth)
        return m_lineWidth < o->m_lineWidth ? -1 : 1;

    if (m_viewportSize != o->m_viewportSize)
        return -1;

    // Matrices differ per frame, always return non-equal to force update
    if (m_mvpMatrix != o->m_mvpMatrix)
        return -1;

    return 0;
}

void ThickLineMaterial::setColor(const QColor &color)
{
    m_color = color;
}

void ThickLineMaterial::setLineWidth(float width)
{
    m_lineWidth = width;
}

const QSGGeometry::AttributeSet &ThickLineMaterial::attributes()
{
    // Use the attribute set defined in AOGGeometry
    return AOGGeometry::thickLineAttributes();
}

// ============================================================================
// ThickLineMaterialShader
// ============================================================================

ThickLineMaterialShader::ThickLineMaterialShader()
{
    setShaderFileName(VertexStage, QLatin1String(":/AOG/shaders/thickline.vert.qsb"));
    setShaderFileName(FragmentStage, QLatin1String(":/AOG/shaders/thickline.frag.qsb"));
}

bool ThickLineMaterialShader::updateUniformData(RenderState &state,
                                                 QSGMaterial *newMaterial,
                                                 QSGMaterial *oldMaterial)
{
    Q_UNUSED(oldMaterial);
    Q_UNUSED(state);

    bool changed = false;
    QByteArray *buf = state.uniformData();

    //qDebug(thicklinematerial_log) << buf->size();

    auto *material = static_cast<ThickLineMaterial *>(newMaterial);

    // Uniform buffer layout (std140):
    // offset 0:  mat4 full mvpMatrix (64 bytes)
    // offset 64: vec4 color (16 bytes)
    // offset 80: vec2 viewPort (8 bytes)
    // offset 88: float lineWidth (4 bytes, padded to 16)

    // Update MVP matrix
    // Our MVP transforms: world coords -> item-local coords
    // state.combinedMatrix() transforms: item-local coords -> window clip space
    // Combined: world coords -> window clip space, properly positioned in the item
    QMatrix4x4 combinedMatrix = state.combinedMatrix() * material->mvpMatrix();
    memcpy(buf->data(), combinedMatrix.constData(), 64);
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
    changed = true;

    // Update viewport size (offset 208)
    QSize vp = material->viewportSize();
    float viewportSize[2] = {
        static_cast<float>(vp.width()),
        static_cast<float>(vp.height())
    };
    memcpy(buf->data() + 64+16, viewportSize, 8);

    // Update line width (offset 216)
    float lineWidth = material->lineWidth();
    memcpy(buf->data() + 64+16+8 , &lineWidth, 4);

    return changed;

#if 0
    // Uniform buffer layout (must match shader, std140):
    // mat4 mvpMatrix    @ offset 0   (64 bytes)
    // mat4 ndcMatrix    @ offset 64  (64 bytes)
    // vec4 color        @ offset 128 (16 bytes)
    // vec2 viewportSize @ offset 144 (8 bytes)
    // float lineWidth   @ offset 152 (4 bytes)
    // (padding to 160 bytes for std140 alignment)

    // MVP matrix
    const QMatrix4x4 &mvp = material->mvpMatrix();
    memcpy(buf->data() + 0, mvp.constData(), 64);

    // NDC matrix
    const QMatrix4x4 &ndc = material->ndcMatrix();
    memcpy(buf->data() + 64, ndc.constData(), 64);

    // Color (as vec4)
    const QColor &c = material->color();
    float color[4] = {
        static_cast<float>(c.redF()),
        static_cast<float>(c.greenF()),
        static_cast<float>(c.blueF()),
        static_cast<float>(c.alphaF())
    };
    memcpy(buf->data() + 128, color, 16);

    // Viewport size (as vec2)
    QSize vp = material->viewportSize();
    float viewportSize[2] = {
        static_cast<float>(vp.width()),
        static_cast<float>(vp.height())
    };
    memcpy(buf->data() + 144, viewportSize, 8);

    // Line width
    float lineWidth = material->lineWidth();
    memcpy(buf->data() + 152, &lineWidth, 4);

    changed = true;
    return changed;
#endif
}
