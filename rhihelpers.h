// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
//
#ifndef RHIHELPERS_H
#define RHIHELPERS_H

#include <QVector>
#include <QVector3D>
#include <QMatrix4x4>
#include <QColor>
#include <rhi/qrhi.h>
#include "rhiresources.h"

/**
 * @brief Helper class for drawing single-color primitives with QRhi
 *
 * This is the RHI equivalent of GLHelperOneColor from glutils.h.
 * Usage pattern:
 *
 * RhiHelperOneColor helper;
 * helper.append(QVector3D(0, 0, 0));
 * helper.append(QVector3D(10, 0, 0));
 * helper.draw(rhiResources, cb, mvp, Qt::red, QRhiGraphicsPipeline::Lines);
 *
 * For optimal performance, reuse the same helper instance across frames
 * rather than creating a new one each time.
 */
class RhiHelperOneColor : public QVector<QVector3D>
{
public:
    RhiHelperOneColor();
    ~RhiHelperOneColor();

    /**
     * @brief Draw the vertices using RhiResources
     * @param resources RhiResources instance (must be initialized)
     * @param cb Command buffer from QSGRenderNode::render() or window
     * @param mvp Model-View-Projection matrix
     * @param color Color for all vertices
     * @param topology Primitive topology (Lines, LineStrip, Triangles, etc.)
     * @param pointSize Size for points (used when topology is Points)
     *
     * Note: This assumes you're already inside a render pass (QSGRenderNode does this).
     * Don't call beginPass/endPass yourself.
     */
    void draw(RhiResources *resources,
              QRhiCommandBuffer *cb,
              const QMatrix4x4 &mvp,
              const QColor &color,
              QRhiGraphicsPipeline::Topology topology = QRhiGraphicsPipeline::Lines,
              float pointSize = 1.0f);

    /**
     * @brief Release the vertex buffer
     * Call this when you're done with the helper to free GPU memory.
     * The helper can still be reused after calling this - a new buffer
     * will be created on the next draw() call.
     */
    void releaseBuffer();

private:
    QRhiBuffer *m_vertexBuffer = nullptr;
    int m_vertexBufferSize = 0;  // Track allocated size

    // Helper to ensure buffer is large enough
    bool ensureBuffer(QRhi *rhi, int requiredSize);
};

/**
 * @brief Helper class for drawing per-vertex colored primitives with QRhi
 *
 * This is the RHI equivalent of GLHelperColors from glutils.h.
 * Each vertex has its own color which is interpolated across primitives.
 *
 * Usage:
 * RhiHelperColors helper;
 * ColorVertex v1 = { QVector3D(0, 0, 0), QVector4D(1, 0, 0, 1) }; // red
 * ColorVertex v2 = { QVector3D(10, 0, 0), QVector4D(0, 1, 0, 1) }; // green
 * helper.append(v1);
 * helper.append(v2);
 * helper.draw(rhiResources, cb, mvp, QRhiGraphicsPipeline::Lines);
 */
class RhiHelperColors : public QVector<ColorVertex>
{
public:
    RhiHelperColors();
    ~RhiHelperColors();

    /**
     * @brief Draw the colored vertices using RhiResources
     * @param resources RhiResources instance (must be initialized)
     * @param cb Command buffer from QSGRenderNode::render() or window
     * @param mvp Model-View-Projection matrix
     * @param topology Primitive topology (Triangles, TriangleStrip, etc.)
     * @param pointSize Size for points (used when topology is Points)
     */
    void draw(RhiResources *resources,
              QRhiCommandBuffer *cb,
              const QMatrix4x4 &mvp,
              QRhiGraphicsPipeline::Topology topology = QRhiGraphicsPipeline::Triangles,
              float pointSize = 1.0f);

    void releaseBuffer();

private:
    QRhiBuffer *m_vertexBuffer = nullptr;
    int m_vertexBufferSize = 0;

    bool ensureBuffer(QRhi *rhi, int requiredSize);
};

/**
 * @brief Helper class for drawing textured primitives with QRhi
 *
 * This is the RHI equivalent of GLHelperTexture from glutils.h.
 * Each vertex has position and texture coordinates.
 *
 * Usage:
 * RhiHelperTexture helper;
 * VertexTexcoord v1 = { QVector3D(0, 0, 0), QVector2D(0, 0) };
 * VertexTexcoord v2 = { QVector3D(10, 0, 0), QVector2D(1, 0) };
 * helper.append(v1);
 * helper.append(v2);
 * helper.draw(rhiResources, cb, mvp, RHI_FLOOR);
 */
class RhiHelperTexture : public QVector<VertexTexcoord>
{
public:
    RhiHelperTexture();
    ~RhiHelperTexture();

    /**
     * @brief Draw textured vertices using RhiResources
     * @param resources RhiResources instance (must be initialized)
     * @param cb Command buffer from QSGRenderNode::render() or window
     * @param mvp Model-View-Projection matrix
     * @param textureIndex Index into RhiResources texture array (RHI_FLOOR, RHI_FONT, etc.)
     * @param topology Primitive topology (Triangles, TriangleStrip, etc.)
     * @param colorize If true, multiply texture by color
     * @param color Color to multiply with texture (if colorize is true)
     */
    void draw(RhiResources *resources,
              QRhiCommandBuffer *cb,
              const QMatrix4x4 &mvp,
              int textureIndex,
              QRhiGraphicsPipeline::Topology topology = QRhiGraphicsPipeline::Triangles,
              bool colorize = false,
              const QColor &color = QColor::fromRgbF(1, 1, 1));

    void releaseBuffer();

private:
    QRhiBuffer *m_vertexBuffer = nullptr;
    int m_vertexBufferSize = 0;

    bool ensureBuffer(QRhi *rhi, int requiredSize);

    // Keep track of current texture bindings to create compatible shader resources
    QRhiShaderResourceBindings *m_currentBindings = nullptr;
    int m_lastTextureIndex = -1;
};

#endif // RHIHELPERS_H
