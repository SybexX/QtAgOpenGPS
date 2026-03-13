// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
//
#ifndef RHIRESOURCES_H
#define RHIRESOURCES_H

#include <rhi/qrhi.h>
#include <QVector>
#include <QMatrix4x4>
#include <QColor>
#include <QVector3D>
#include <QVector4D>
#include <QVector2D>

// Forward declarations for QRhi classes
class QRhi;
class QRhiBuffer;
class QRhiTexture;
class QRhiSampler;
class QRhiGraphicsPipeline;
class QRhiShaderResourceBindings;
class QRhiRenderPassDescriptor;
class QRhiCommandBuffer;

// Texture enum matching glutils.h
enum RhiTextures {
    RHI_FLOOR,
    RHI_FONT,
    RHI_HYDLIFT,
    RHI_TRACTOR,
    RHI_QUESTION_MARK,
    RHI_FRONT_WHEELS,
    RHI_TRACTOR_4WD_FRONT,
    RHI_TRACTOR_4WD_REAR,
    RHI_HARVESTER,
    RHI_TOOLWHEELS,
    RHI_TIRE,
    RHI_TEXTURE_COUNT
};

// Pipeline resource bundle
struct RhiPipeline {
    QRhiGraphicsPipeline *pipeline = nullptr;
    QRhiShaderResourceBindings *bindings = nullptr;
    QRhiBuffer *uniformBuffer = nullptr;

    void destroy();
    bool isValid() const { return pipeline != nullptr; }
};

// Pipeline variant for different topologies
// Shares shaders and bindings, but has different topology
struct RhiPipelineSet {
    QRhiShaderResourceBindings *bindings = nullptr;
    QRhiBuffer *uniformBuffer = nullptr;

    // Different pipelines for different topologies
    QRhiGraphicsPipeline *points = nullptr;
    QRhiGraphicsPipeline *lines = nullptr;
    QRhiGraphicsPipeline *lineStrip = nullptr;
    QRhiGraphicsPipeline *triangles = nullptr;
    QRhiGraphicsPipeline *triangleStrip = nullptr;

    void destroy();
    bool isValid() const { return bindings != nullptr; }

    // Get pipeline for specific topology
    QRhiGraphicsPipeline* getPipeline(QRhiGraphicsPipeline::Topology topology) const;
};

// Texture resource bundle
struct RhiTextureData {
    QRhiTexture *texture = nullptr;
    QRhiSampler *sampler = nullptr;

    void destroy();
    bool isValid() const { return texture != nullptr; }
};

/**
 * @brief Central resource manager for QRhi-based rendering
 *
 * This class manages all QRhi resources including:
 * - Graphics pipelines (shaders, vertex layouts, render state)
 * - Uniform buffers for shader parameters
 * - Textures and samplers
 * - Shader resource bindings
 *
 * Resources are created once during initialization and reused
 * throughout the application lifetime.
 */
class RhiResources
{
public:
    RhiResources();
    ~RhiResources();

    /**
     * @brief Initialize all QRhi resources
     * @param rhi QRhi instance from scene graph
     * @param renderPass Render pass descriptor from scene graph's render target
     * @return true if initialization succeeded
     *
     * For QSGRenderNode usage:
     * - Get QRhi from QQuickWindow::rendererInterface()
     * - Get render pass from the scene graph's swap chain render target
     * - Call this in the first render() call or in sync()
     */
    bool initialize(QRhi *rhi, QRhiRenderPassDescriptor *renderPass);

    /**
     * @brief Initialize resources with delayed pipeline creation
     * @param rhi QRhi instance from scene graph
     * @return true if initialization succeeded
     *
     * Use this when you don't have a render pass descriptor yet.
     * Textures and buffers will be created, but pipelines must be
     * created later with ensurePipelines().
     */
    bool initializeWithoutPipelines(QRhi *rhi);

    /**
     * @brief Ensure pipelines are created with the given render pass
     * @param renderPass Render pass descriptor (typically from scene graph)
     * @return true if pipelines were created or already exist
     *
     * Safe to call multiple times - will only create once.
     * Use with initializeWithoutPipelines() for QSGRenderNode.
     */
    bool ensurePipelines(QRhiRenderPassDescriptor *renderPass);

    /**
     * @brief Destroy all resources
     * Call before QRhi is destroyed
     */
    void destroy();

    /**
     * @brief Check if resources are initialized
     */
    bool isInitialized() const { return m_rhi != nullptr; }

    // Accessors
    QRhi *rhi() const { return m_rhi; }

    // Pipeline set accessors (support multiple topologies)
    RhiPipelineSet &colorPipelineSet() { return m_colorPipelineSet; }
    RhiPipelineSet &colorsPipelineSet() { return m_colorsPipelineSet; }
    RhiPipelineSet &texturePipelineSet() { return m_texturePipelineSet; }

    // Convenience methods to get pipeline for specific topology
    QRhiGraphicsPipeline* getColorPipeline(QRhiGraphicsPipeline::Topology topology) {
        return m_colorPipelineSet.getPipeline(topology);
    }
    QRhiGraphicsPipeline* getColorsPipeline(QRhiGraphicsPipeline::Topology topology) {
        return m_colorsPipelineSet.getPipeline(topology);
    }
    QRhiGraphicsPipeline* getTexturePipeline(QRhiGraphicsPipeline::Topology topology) {
        return m_texturePipelineSet.getPipeline(topology);
    }

    // Texture accessors
    RhiTextureData &texture(int index);
    const RhiTextureData &texture(int index) const;

    // Font texture dimensions (for text rendering)
    int fontTextureWidth() const { return m_fontTextureWidth; }
    int fontTextureHeight() const { return m_fontTextureHeight; }

private:
    // Initialization helpers
    bool initializePipelines();
    bool initializeTextures();

    // Pipeline creation helpers
    bool createColorPipeline();
    bool createColorsPipeline();
    bool createTexturePipeline();

    // Helper to create a single pipeline variant with specific topology
    QRhiGraphicsPipeline* createPipelineVariant(
        const QShader &vertShader,
        const QShader &fragShader,
        QRhiShaderResourceBindings *bindings,
        const QRhiVertexInputLayout &inputLayout,
        QRhiGraphicsPipeline::Topology topology,
        bool enableBlending = false);

    // Shader loading
    QShader loadShader(const QString &name);

    // Core QRhi instance (not owned)
    QRhi *m_rhi = nullptr;
    QRhiRenderPassDescriptor *m_renderPass = nullptr;

    // Pipeline sets (support multiple topologies)
    RhiPipelineSet m_colorPipelineSet;      // Single color rendering
    RhiPipelineSet m_colorsPipelineSet;     // Per-vertex color interpolation
    RhiPipelineSet m_texturePipelineSet;    // Textured rendering

    // Textures
    QVector<RhiTextureData> m_textures;

    // Font texture dimensions
    int m_fontTextureWidth = 0;
    int m_fontTextureHeight = 0;

    // Initialization state
    bool m_initialized = false;
};

// Uniform buffer structures (must match shader layouts with std140)

/**
 * @brief Uniform buffer for simple color shader
 * Matches the layout in color shaders
 */
struct ColorUniforms {
    QMatrix4x4 mvpMatrix;    // 64 bytes
    QVector4D color;         // 16 bytes
    float pointSize;         // 4 bytes
    float _pad[3];           // 12 bytes padding for alignment

    ColorUniforms() : pointSize(1.0f) {
        _pad[0] = _pad[1] = _pad[2] = 0.0f;
    }
};

/**
 * @brief Uniform buffer for interpolated color shader
 * Matches the layout in colors shaders
 */
struct ColorsUniforms {
    QMatrix4x4 mvpMatrix;    // 64 bytes
    float pointSize;         // 4 bytes
    float _pad[3];           // 12 bytes padding

    ColorsUniforms() : pointSize(1.0f) {
        _pad[0] = _pad[1] = _pad[2] = 0.0f;
    }
};

/**
 * @brief Uniform buffer for texture shader
 * Matches the layout in texture shaders
 */
struct TextureUniforms {
    QMatrix4x4 mvpMatrix;    // 64 bytes
    QVector4D color;         // 16 bytes
    int useColor;            // 4 bytes
    float _pad[3];           // 12 bytes padding

    TextureUniforms() : useColor(0) {
        color = QVector4D(1, 1, 1, 1);
        _pad[0] = _pad[1] = _pad[2] = 0.0f;
    }
};

// Vertex structures

/**
 * @brief Vertex with position and color
 * Used with colorsPipeline
 */
struct ColorVertex {
    QVector3D vertex;
    QVector4D color;
};

/**
 * @brief Vertex with position and texture coordinate
 * Used with texturePipeline
 */
struct VertexTexcoord {
    QVector3D vertex;
    QVector2D texcoord;
};

#endif // RHIRESOURCES_H
