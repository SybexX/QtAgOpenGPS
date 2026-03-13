// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
//
#include "rhihelpers.h"
#include <QDebug>

// ============================================================================
// RhiHelperOneColor Implementation
// ============================================================================

RhiHelperOneColor::RhiHelperOneColor()
{
}

RhiHelperOneColor::~RhiHelperOneColor()
{
    releaseBuffer();
}

bool RhiHelperOneColor::ensureBuffer(QRhi *rhi, int requiredSize)
{
    // If buffer doesn't exist or is too small, create/recreate it
    if (!m_vertexBuffer || m_vertexBufferSize < requiredSize) {
        // Destroy old buffer if it exists
        if (m_vertexBuffer) {
            delete m_vertexBuffer;
        }

        // Create new buffer with some extra space to avoid frequent reallocations
        int allocSize = requiredSize * 2; // Allocate 2x to reduce reallocations
        m_vertexBuffer = rhi->newBuffer(
            QRhiBuffer::Dynamic,
            QRhiBuffer::VertexBuffer,
            allocSize
        );

        if (!m_vertexBuffer->create()) {
            qWarning() << "RhiHelperOneColor: Failed to create vertex buffer";
            delete m_vertexBuffer;
            m_vertexBuffer = nullptr;
            m_vertexBufferSize = 0;
            return false;
        }

        m_vertexBufferSize = allocSize;
    }

    return true;
}

void RhiHelperOneColor::draw(RhiResources *resources,
                              QRhiCommandBuffer *cb,
                              const QMatrix4x4 &mvp,
                              const QColor &color,
                              QRhiGraphicsPipeline::Topology topology,
                              float pointSize)
{
    if (!resources || !cb) {
        qWarning() << "RhiHelperOneColor::draw: Invalid resources or command buffer";
        return;
    }

    if (isEmpty()) {
        return; // Nothing to draw
    }

    // Get the appropriate pipeline for this topology
    QRhiGraphicsPipeline *pipeline = resources->getColorPipeline(topology);
    if (!pipeline) {
        qWarning() << "RhiHelperOneColor::draw: No pipeline for topology:" << topology;
        return;
    }

    QRhi *rhi = resources->rhi();
    int dataSize = size() * sizeof(QVector3D);

    // Ensure we have a vertex buffer large enough
    if (!ensureBuffer(rhi, dataSize)) {
        return;
    }

    // Upload vertex data to GPU
    QRhiResourceUpdateBatch *batch = rhi->nextResourceUpdateBatch();
    batch->updateDynamicBuffer(m_vertexBuffer, 0, dataSize, constData());

    // Update uniform buffer with color, MVP matrix, and point size
    ColorUniforms uniforms;
    uniforms.mvpMatrix = mvp;
    uniforms.color = QVector4D(color.redF(), color.greenF(), color.blueF(), color.alphaF());
    uniforms.pointSize = pointSize;

    batch->updateDynamicBuffer(
        resources->colorPipelineSet().uniformBuffer,
        0,
        sizeof(ColorUniforms),
        &uniforms
    );

    // Submit resource updates
    cb->resourceUpdate(batch);

    // Set the graphics pipeline (topology-specific)
    cb->setGraphicsPipeline(pipeline);

    // Bind shader resources (uniform buffer)
    cb->setShaderResources();

    // Bind vertex buffer
    const QRhiCommandBuffer::VertexInput vbufBindings[] = {
        { m_vertexBuffer, 0 }
    };
    cb->setVertexInput(0, 1, vbufBindings);

    // Draw
    cb->draw(size());
}

void RhiHelperOneColor::releaseBuffer()
{
    if (m_vertexBuffer) {
        delete m_vertexBuffer;
        m_vertexBuffer = nullptr;
        m_vertexBufferSize = 0;
    }
}

// ============================================================================
// RhiHelperColors Implementation
// ============================================================================

RhiHelperColors::RhiHelperColors()
{
}

RhiHelperColors::~RhiHelperColors()
{
    releaseBuffer();
}

bool RhiHelperColors::ensureBuffer(QRhi *rhi, int requiredSize)
{
    if (!m_vertexBuffer || m_vertexBufferSize < requiredSize) {
        if (m_vertexBuffer) {
            delete m_vertexBuffer;
        }

        int allocSize = requiredSize * 2;
        m_vertexBuffer = rhi->newBuffer(
            QRhiBuffer::Dynamic,
            QRhiBuffer::VertexBuffer,
            allocSize
        );

        if (!m_vertexBuffer->create()) {
            qWarning() << "RhiHelperColors: Failed to create vertex buffer";
            delete m_vertexBuffer;
            m_vertexBuffer = nullptr;
            m_vertexBufferSize = 0;
            return false;
        }

        m_vertexBufferSize = allocSize;
    }

    return true;
}

void RhiHelperColors::draw(RhiResources *resources,
                            QRhiCommandBuffer *cb,
                            const QMatrix4x4 &mvp,
                            QRhiGraphicsPipeline::Topology topology,
                            float pointSize)
{
    if (!resources || !cb) {
        qWarning() << "RhiHelperColors::draw: Invalid resources or command buffer";
        return;
    }

    if (isEmpty()) {
        return;
    }

    // Get the appropriate pipeline for this topology
    QRhiGraphicsPipeline *pipeline = resources->getColorsPipeline(topology);
    if (!pipeline) {
        qWarning() << "RhiHelperColors::draw: No pipeline for topology:" << topology;
        return;
    }

    QRhi *rhi = resources->rhi();
    int dataSize = size() * sizeof(ColorVertex);

    if (!ensureBuffer(rhi, dataSize)) {
        return;
    }

    // Upload vertex data
    QRhiResourceUpdateBatch *batch = rhi->nextResourceUpdateBatch();
    batch->updateDynamicBuffer(m_vertexBuffer, 0, dataSize, constData());

    // Update uniform buffer (only MVP and point size for this pipeline)
    ColorsUniforms uniforms;
    uniforms.mvpMatrix = mvp;
    uniforms.pointSize = pointSize;

    batch->updateDynamicBuffer(
        resources->colorsPipelineSet().uniformBuffer,
        0,
        sizeof(ColorsUniforms),
        &uniforms
    );

    cb->resourceUpdate(batch);

    // Set pipeline and draw
    cb->setGraphicsPipeline(pipeline);
    cb->setShaderResources();

    const QRhiCommandBuffer::VertexInput vbufBindings[] = {
        { m_vertexBuffer, 0 }
    };
    cb->setVertexInput(0, 1, vbufBindings);

    cb->draw(size());
}

void RhiHelperColors::releaseBuffer()
{
    if (m_vertexBuffer) {
        delete m_vertexBuffer;
        m_vertexBuffer = nullptr;
        m_vertexBufferSize = 0;
    }
}

// ============================================================================
// RhiHelperTexture Implementation
// ============================================================================

RhiHelperTexture::RhiHelperTexture()
{
}

RhiHelperTexture::~RhiHelperTexture()
{
    releaseBuffer();
    if (m_currentBindings) {
        delete m_currentBindings;
    }
}

bool RhiHelperTexture::ensureBuffer(QRhi *rhi, int requiredSize)
{
    if (!m_vertexBuffer || m_vertexBufferSize < requiredSize) {
        if (m_vertexBuffer) {
            delete m_vertexBuffer;
        }

        int allocSize = requiredSize * 2;
        m_vertexBuffer = rhi->newBuffer(
            QRhiBuffer::Dynamic,
            QRhiBuffer::VertexBuffer,
            allocSize
        );

        if (!m_vertexBuffer->create()) {
            qWarning() << "RhiHelperTexture: Failed to create vertex buffer";
            delete m_vertexBuffer;
            m_vertexBuffer = nullptr;
            m_vertexBufferSize = 0;
            return false;
        }

        m_vertexBufferSize = allocSize;
    }

    return true;
}

void RhiHelperTexture::draw(RhiResources *resources,
                             QRhiCommandBuffer *cb,
                             const QMatrix4x4 &mvp,
                             int textureIndex,
                             QRhiGraphicsPipeline::Topology topology,
                             bool colorize,
                             const QColor &color)
{
    if (!resources || !cb) {
        qWarning() << "RhiHelperTexture::draw: Invalid resources or command buffer";
        return;
    }

    if (isEmpty()) {
        return;
    }

    // Get the appropriate pipeline for this topology
    QRhiGraphicsPipeline *pipeline = resources->getTexturePipeline(topology);
    if (!pipeline) {
        qWarning() << "RhiHelperTexture::draw: No pipeline for topology:" << topology;
        return;
    }

    if (textureIndex < 0 || textureIndex >= RHI_TEXTURE_COUNT) {
        qWarning() << "RhiHelperTexture::draw: Invalid texture index:" << textureIndex;
        return;
    }

    const RhiTextureData &texData = resources->texture(textureIndex);
    if (!texData.isValid()) {
        qWarning() << "RhiHelperTexture::draw: Texture not loaded:" << textureIndex;
        return;
    }

    QRhi *rhi = resources->rhi();
    int dataSize = size() * sizeof(VertexTexcoord);

    if (!ensureBuffer(rhi, dataSize)) {
        return;
    }

    // Upload vertex data
    QRhiResourceUpdateBatch *batch = rhi->nextResourceUpdateBatch();
    batch->updateDynamicBuffer(m_vertexBuffer, 0, dataSize, constData());

    // Update uniform buffer
    TextureUniforms uniforms;
    uniforms.mvpMatrix = mvp;
    uniforms.color = QVector4D(color.redF(), color.greenF(), color.blueF(), color.alphaF());
    uniforms.useColor = colorize ? 1 : 0;

    batch->updateDynamicBuffer(
        resources->texturePipelineSet().uniformBuffer,
        0,
        sizeof(TextureUniforms),
        &uniforms
    );

    cb->resourceUpdate(batch);

    // If texture changed, we need to create new shader resource bindings
    // with the correct texture
    if (m_lastTextureIndex != textureIndex) {
        // Clean up old bindings if they exist
        if (m_currentBindings) {
            delete m_currentBindings;
            m_currentBindings = nullptr;
        }

        // Create shader resource bindings with the selected texture
        m_currentBindings = rhi->newShaderResourceBindings();
        m_currentBindings->setBindings({
            QRhiShaderResourceBinding::uniformBuffer(
                0,
                QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage,
                resources->texturePipelineSet().uniformBuffer
            ),
            QRhiShaderResourceBinding::sampledTexture(
                1,
                QRhiShaderResourceBinding::FragmentStage,
                texData.texture,
                texData.sampler
            )
        });

        if (!m_currentBindings->create()) {
            qWarning() << "RhiHelperTexture::draw: Failed to create shader resource bindings";
            delete m_currentBindings;
            m_currentBindings = nullptr;
            return;
        }

        m_lastTextureIndex = textureIndex;
    }

    // Set pipeline and resources
    cb->setGraphicsPipeline(pipeline);
    cb->setShaderResources(m_currentBindings);

    // Bind vertex buffer
    const QRhiCommandBuffer::VertexInput vbufBindings[] = {
        { m_vertexBuffer, 0 }
    };
    cb->setVertexInput(0, 1, vbufBindings);

    // Draw
    cb->draw(size());
}

void RhiHelperTexture::releaseBuffer()
{
    if (m_vertexBuffer) {
        delete m_vertexBuffer;
        m_vertexBuffer = nullptr;
        m_vertexBufferSize = 0;
    }

    if (m_currentBindings) {
        delete m_currentBindings;
        m_currentBindings = nullptr;
        m_lastTextureIndex = -1;
    }
}
