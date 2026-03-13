// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
//
#include "rhiresources.h"
#include <QFile>
#include <QImage>
#include <QDebug>
#include <rhi/qrhi.h>

#ifdef LOCAL_QML
#define PREFIX "local:"
#else
#define PREFIX ":/AOG"
#endif

// RhiPipeline implementation
void RhiPipeline::destroy()
{
    delete pipeline;
    delete bindings;
    delete uniformBuffer;
    pipeline = nullptr;
    bindings = nullptr;
    uniformBuffer = nullptr;
}

// RhiPipelineSet implementation
void RhiPipelineSet::destroy()
{
    delete points;
    delete lines;
    delete lineStrip;
    delete triangles;
    delete triangleStrip;
    delete bindings;
    delete uniformBuffer;

    points = nullptr;
    lines = nullptr;
    lineStrip = nullptr;
    triangles = nullptr;
    triangleStrip = nullptr;
    bindings = nullptr;
    uniformBuffer = nullptr;
}

QRhiGraphicsPipeline* RhiPipelineSet::getPipeline(QRhiGraphicsPipeline::Topology topology) const
{
    switch (topology) {
    case QRhiGraphicsPipeline::Points:
        return points;
    case QRhiGraphicsPipeline::Lines:
        return lines;
    case QRhiGraphicsPipeline::LineStrip:
        return lineStrip;
    case QRhiGraphicsPipeline::Triangles:
        return triangles;
    case QRhiGraphicsPipeline::TriangleStrip:
        return triangleStrip;
    case QRhiGraphicsPipeline::TriangleFan:
        return triangleStrip; // Fallback - not all backends support TriangleFan
    default:
        qWarning() << "Unsupported topology:" << topology;
        return nullptr;
    }
}

// RhiTextureData implementation
void RhiTextureData::destroy()
{
    delete texture;
    delete sampler;
    texture = nullptr;
    sampler = nullptr;
}

// RhiResources implementation
RhiResources::RhiResources()
{
    m_textures.resize(RHI_TEXTURE_COUNT);
}

RhiResources::~RhiResources()
{
    destroy();
}

bool RhiResources::initialize(QRhi *rhi, QRhiRenderPassDescriptor *renderPass)
{
    if (m_initialized) {
        qWarning() << "RhiResources already initialized";
        return false;
    }

    if (!rhi || !renderPass) {
        qWarning() << "Invalid QRhi or render pass descriptor";
        return false;
    }

    m_rhi = rhi;
    m_renderPass = renderPass;

    qDebug() << "Initializing RhiResources...";
    qDebug() << "  Backend:" << m_rhi->backend();
    qDebug() << "  Driver:" << m_rhi->driverInfo();

    // Initialize textures first (needed for pipelines that use them)
    if (!initializeTextures()) {
        qWarning() << "Failed to initialize textures";
        destroy();
        return false;
    }

    // Initialize pipelines
    if (!initializePipelines()) {
        qWarning() << "Failed to initialize pipelines";
        destroy();
        return false;
    }

    m_initialized = true;
    qDebug() << "RhiResources initialized successfully";
    return true;
}

bool RhiResources::initializeWithoutPipelines(QRhi *rhi)
{
    if (m_initialized) {
        qWarning() << "RhiResources already initialized";
        return false;
    }

    if (!rhi) {
        qWarning() << "Invalid QRhi";
        return false;
    }

    m_rhi = rhi;

    qDebug() << "Initializing RhiResources (without pipelines)...";
    qDebug() << "  Backend:" << m_rhi->backend();
    qDebug() << "  Driver:" << m_rhi->driverInfo();

    // Initialize textures first
    if (!initializeTextures()) {
        qWarning() << "Failed to initialize textures";
        destroy();
        return false;
    }

    // Mark as initialized even without pipelines
    m_initialized = true;
    qDebug() << "RhiResources initialized (pipelines deferred)";
    return true;
}

bool RhiResources::ensurePipelines(QRhiRenderPassDescriptor *renderPass)
{
    if (!m_rhi) {
        qWarning() << "Cannot create pipelines without QRhi instance";
        return false;
    }

    if (!renderPass) {
        qWarning() << "Cannot create pipelines without render pass descriptor";
        return false;
    }

    // If pipelines already created, verify they're compatible
    if (m_colorPipelineSet.isValid()) {
        if (m_renderPass != renderPass) {
            qWarning() << "Render pass changed - pipelines may be incompatible!";
            qWarning() << "Consider destroying and recreating RhiResources";
        }
        return true; // Already created
    }

    qDebug() << "Creating pipelines with render pass descriptor...";
    m_renderPass = renderPass;

    // Initialize pipelines
    if (!initializePipelines()) {
        qWarning() << "Failed to initialize pipelines";
        return false;
    }

    qDebug() << "Pipelines created successfully";
    return true;
}

void RhiResources::destroy()
{
    if (!m_initialized) {
        return;
    }

    qDebug() << "Destroying RhiResources...";

    // Destroy pipeline sets
    m_colorPipelineSet.destroy();
    m_colorsPipelineSet.destroy();
    m_texturePipelineSet.destroy();

    // Destroy textures
    for (auto &tex : m_textures) {
        tex.destroy();
    }

    m_rhi = nullptr;
    m_renderPass = nullptr;
    m_initialized = false;
}

RhiTextureData &RhiResources::texture(int index)
{
    Q_ASSERT(index >= 0 && index < m_textures.size());
    return m_textures[index];
}

const RhiTextureData &RhiResources::texture(int index) const
{
    Q_ASSERT(index >= 0 && index < m_textures.size());
    return m_textures[index];
}

QShader RhiResources::loadShader(const QString &name)
{
    QFile f(QString(PREFIX "/shaders/%1.qsb").arg(name));
    if (!f.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open shader file:" << name;
        qWarning() << "  Path:" << f.fileName();
        return QShader();
    }

    QShader shader = QShader::fromSerialized(f.readAll());
    if (!shader.isValid()) {
        qWarning() << "Failed to deserialize shader:" << name;
        return QShader();
    }

    qDebug() << "  Loaded shader:" << name;
    return shader;
}

bool RhiResources::initializePipelines()
{
    qDebug() << "Initializing pipelines...";

    if (!createColorPipeline()) {
        qWarning() << "Failed to create color pipeline";
        return false;
    }

    if (!createColorsPipeline()) {
        qWarning() << "Failed to create colors pipeline";
        return false;
    }

    if (!createTexturePipeline()) {
        qWarning() << "Failed to create texture pipeline";
        return false;
    }

    qDebug() << "  All pipelines created successfully";
    return true;
}

bool RhiResources::createColorPipeline()
{
    qDebug() << "  Creating color pipeline...";

    // Create uniform buffer
    m_colorPipelineSet.uniformBuffer = m_rhi->newBuffer(
        QRhiBuffer::Dynamic,
        QRhiBuffer::UniformBuffer,
        sizeof(ColorUniforms)
    );

    if (!m_colorPipelineSet.uniformBuffer->create()) {
        qWarning() << "Failed to create color uniform buffer";
        return false;
    }

    // Create shader resource bindings
    m_colorPipelineSet.bindings = m_rhi->newShaderResourceBindings();
    m_colorPipelineSet.bindings->setBindings({
        QRhiShaderResourceBinding::uniformBuffer(
            0,
            QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage,
            m_colorPipelineSet.uniformBuffer
        )
    });

    if (!m_colorPipelineSet.bindings->create()) {
        qWarning() << "Failed to create color shader resource bindings";
        return false;
    }

    // Load shaders
    QShader vertShader = loadShader("color_vshader.vert");
    QShader fragShader = loadShader("color_fshader.frag");

    if (!vertShader.isValid() || !fragShader.isValid()) {
        qWarning() << "Failed to load color shaders";
        return false;
    }

    // Vertex input layout (vec3 position)
    QRhiVertexInputLayout inputLayout;
    inputLayout.setBindings({
        { 3 * sizeof(float) }  // Stride for vec3
    });
    inputLayout.setAttributes({
        { 0, 0, QRhiVertexInputAttribute::Float3, 0 }  // binding, location, format, offset
    });

    // Create pipeline for Lines topology (primary use case for color pipeline)
    m_colorPipelineSet.lines = m_rhi->newGraphicsPipeline();
    m_colorPipelineSet.lines->setShaderStages({
        { QRhiShaderStage::Vertex, vertShader },
        { QRhiShaderStage::Fragment, fragShader }
    });
    m_colorPipelineSet.lines->setVertexInputLayout(inputLayout);
    m_colorPipelineSet.lines->setShaderResourceBindings(m_colorPipelineSet.bindings);
    m_colorPipelineSet.lines->setRenderPassDescriptor(m_renderPass);
    m_colorPipelineSet.lines->setTopology(QRhiGraphicsPipeline::Lines);
    m_colorPipelineSet.lines->setDepthTest(true);
    m_colorPipelineSet.lines->setDepthWrite(true);
    if (!m_colorPipelineSet.lines->create()) {
        qWarning() << "Failed to create color lines graphics pipeline";
        return false;
    }

    // Create pipeline for Triangles topology
    m_colorPipelineSet.triangles = m_rhi->newGraphicsPipeline();
    m_colorPipelineSet.triangles->setShaderStages({
        { QRhiShaderStage::Vertex, vertShader },
        { QRhiShaderStage::Fragment, fragShader }
    });
    m_colorPipelineSet.triangles->setVertexInputLayout(inputLayout);
    m_colorPipelineSet.triangles->setShaderResourceBindings(m_colorPipelineSet.bindings);
    m_colorPipelineSet.triangles->setRenderPassDescriptor(m_renderPass);
    m_colorPipelineSet.triangles->setTopology(QRhiGraphicsPipeline::Triangles);
    m_colorPipelineSet.triangles->setDepthTest(true);
    m_colorPipelineSet.triangles->setDepthWrite(true);
    if (!m_colorPipelineSet.triangles->create()) {
        qWarning() << "Failed to create color triangles graphics pipeline";
        return false;
    }

    qDebug() << "    Color pipeline created";
    return true;
}

bool RhiResources::createColorsPipeline()
{
    qDebug() << "  Creating colors (interpolated) pipeline...";

    // Create uniform buffer
    m_colorsPipelineSet.uniformBuffer = m_rhi->newBuffer(
        QRhiBuffer::Dynamic,
        QRhiBuffer::UniformBuffer,
        sizeof(ColorsUniforms)
    );

    if (!m_colorsPipelineSet.uniformBuffer->create()) {
        qWarning() << "Failed to create colors uniform buffer";
        return false;
    }

    // Create shader resource bindings
    m_colorsPipelineSet.bindings = m_rhi->newShaderResourceBindings();
    m_colorsPipelineSet.bindings->setBindings({
        QRhiShaderResourceBinding::uniformBuffer(
            0,
            QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage,
            m_colorsPipelineSet.uniformBuffer
        )
    });

    if (!m_colorsPipelineSet.bindings->create()) {
        qWarning() << "Failed to create colors shader resource bindings";
        return false;
    }

    // Load shaders
    QShader vertShader = loadShader("colors_vshader.vert");
    QShader fragShader = loadShader("colors_fshader.frag");

    if (!vertShader.isValid() || !fragShader.isValid()) {
        qWarning() << "Failed to load colors shaders";
        return false;
    }

    // Vertex input layout (vec3 position + vec4 color)
    QRhiVertexInputLayout inputLayout;
    inputLayout.setBindings({
        { 7 * sizeof(float) }  // Stride for vec3 + vec4
    });
    inputLayout.setAttributes({
        { 0, 0, QRhiVertexInputAttribute::Float3, 0 },                      // position
        { 0, 1, QRhiVertexInputAttribute::Float4, 3 * sizeof(float) }       // color
    });

    // Create pipeline for Triangles topology (primary use case for colors pipeline)
    m_colorsPipelineSet.triangles = m_rhi->newGraphicsPipeline();
    m_colorsPipelineSet.triangles->setShaderStages({
        { QRhiShaderStage::Vertex, vertShader },
        { QRhiShaderStage::Fragment, fragShader }
    });
    m_colorsPipelineSet.triangles->setVertexInputLayout(inputLayout);
    m_colorsPipelineSet.triangles->setShaderResourceBindings(m_colorsPipelineSet.bindings);
    m_colorsPipelineSet.triangles->setRenderPassDescriptor(m_renderPass);
    m_colorsPipelineSet.triangles->setTopology(QRhiGraphicsPipeline::Triangles);
    m_colorsPipelineSet.triangles->setDepthTest(true);
    m_colorsPipelineSet.triangles->setDepthWrite(true);
    if (!m_colorsPipelineSet.triangles->create()) {
        qWarning() << "Failed to create colors triangles graphics pipeline";
        return false;
    }

    // Create pipeline for TriangleStrip topology
    m_colorsPipelineSet.triangleStrip = m_rhi->newGraphicsPipeline();
    m_colorsPipelineSet.triangleStrip->setShaderStages({
        { QRhiShaderStage::Vertex, vertShader },
        { QRhiShaderStage::Fragment, fragShader }
    });
    m_colorsPipelineSet.triangleStrip->setVertexInputLayout(inputLayout);
    m_colorsPipelineSet.triangleStrip->setShaderResourceBindings(m_colorsPipelineSet.bindings);
    m_colorsPipelineSet.triangleStrip->setRenderPassDescriptor(m_renderPass);
    m_colorsPipelineSet.triangleStrip->setTopology(QRhiGraphicsPipeline::TriangleStrip);
    m_colorsPipelineSet.triangleStrip->setDepthTest(true);
    m_colorsPipelineSet.triangleStrip->setDepthWrite(true);
    if (!m_colorsPipelineSet.triangleStrip->create()) {
        qWarning() << "Failed to create colors triangleStrip graphics pipeline";
        return false;
    }

    qDebug() << "    Colors pipeline created";
    return true;
}

bool RhiResources::createTexturePipeline()
{
    qDebug() << "  Creating texture pipeline...";

    // Create uniform buffer
    m_texturePipelineSet.uniformBuffer = m_rhi->newBuffer(
        QRhiBuffer::Dynamic,
        QRhiBuffer::UniformBuffer,
        sizeof(TextureUniforms)
    );

    if (!m_texturePipelineSet.uniformBuffer->create()) {
        qWarning() << "Failed to create texture uniform buffer";
        return false;
    }

    // Create shader resource bindings (texture will be set dynamically per draw)
    // For now, use the font texture as default
    m_texturePipelineSet.bindings = m_rhi->newShaderResourceBindings();
    m_texturePipelineSet.bindings->setBindings({
        QRhiShaderResourceBinding::uniformBuffer(
            0,
            QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage,
            m_texturePipelineSet.uniformBuffer
        ),
        QRhiShaderResourceBinding::sampledTexture(
            1,
            QRhiShaderResourceBinding::FragmentStage,
            m_textures[RHI_FONT].texture,
            m_textures[RHI_FONT].sampler
        )
    });

    if (!m_texturePipelineSet.bindings->create()) {
        qWarning() << "Failed to create texture shader resource bindings";
        return false;
    }

    // Load shaders
    QShader vertShader = loadShader("colortex_vshader.vert");
    QShader fragShader = loadShader("colortex_fshader.frag");

    if (!vertShader.isValid() || !fragShader.isValid()) {
        qWarning() << "Failed to load texture shaders";
        return false;
    }

    // Vertex input layout (vec3 position + vec2 texcoord)
    QRhiVertexInputLayout inputLayout;
    inputLayout.setBindings({
        { 5 * sizeof(float) }  // Stride for vec3 + vec2
    });
    inputLayout.setAttributes({
        { 0, 0, QRhiVertexInputAttribute::Float3, 0 },                      // position
        { 0, 1, QRhiVertexInputAttribute::Float2, 3 * sizeof(float) }       // texcoord
    });

    // Blending for textures (especially font rendering)
    QRhiGraphicsPipeline::TargetBlend blend;
    blend.enable = true;
    blend.srcColor = QRhiGraphicsPipeline::SrcAlpha;
    blend.dstColor = QRhiGraphicsPipeline::OneMinusSrcAlpha;
    blend.srcAlpha = QRhiGraphicsPipeline::One;
    blend.dstAlpha = QRhiGraphicsPipeline::OneMinusSrcAlpha;

    // Create pipeline for Triangles topology (primary use case for texture pipeline)
    m_texturePipelineSet.triangles = m_rhi->newGraphicsPipeline();
    m_texturePipelineSet.triangles->setShaderStages({
        { QRhiShaderStage::Vertex, vertShader },
        { QRhiShaderStage::Fragment, fragShader }
    });
    m_texturePipelineSet.triangles->setVertexInputLayout(inputLayout);
    m_texturePipelineSet.triangles->setShaderResourceBindings(m_texturePipelineSet.bindings);
    m_texturePipelineSet.triangles->setRenderPassDescriptor(m_renderPass);
    m_texturePipelineSet.triangles->setTopology(QRhiGraphicsPipeline::Triangles);
    m_texturePipelineSet.triangles->setTargetBlends({ blend });
    m_texturePipelineSet.triangles->setDepthTest(true);
    m_texturePipelineSet.triangles->setDepthWrite(true);
    if (!m_texturePipelineSet.triangles->create()) {
        qWarning() << "Failed to create texture triangles graphics pipeline";
        return false;
    }

    // Create pipeline for TriangleStrip topology
    m_texturePipelineSet.triangleStrip = m_rhi->newGraphicsPipeline();
    m_texturePipelineSet.triangleStrip->setShaderStages({
        { QRhiShaderStage::Vertex, vertShader },
        { QRhiShaderStage::Fragment, fragShader }
    });
    m_texturePipelineSet.triangleStrip->setVertexInputLayout(inputLayout);
    m_texturePipelineSet.triangleStrip->setShaderResourceBindings(m_texturePipelineSet.bindings);
    m_texturePipelineSet.triangleStrip->setRenderPassDescriptor(m_renderPass);
    m_texturePipelineSet.triangleStrip->setTopology(QRhiGraphicsPipeline::TriangleStrip);
    m_texturePipelineSet.triangleStrip->setTargetBlends({ blend });
    m_texturePipelineSet.triangleStrip->setDepthTest(true);
    m_texturePipelineSet.triangleStrip->setDepthWrite(true);
    if (!m_texturePipelineSet.triangleStrip->create()) {
        qWarning() << "Failed to create texture triangleStrip graphics pipeline";
        return false;
    }

    qDebug() << "    Texture pipeline created";
    return true;
}

bool RhiResources::initializeTextures()
{
    qDebug() << "Initializing textures...";

    // Texture file paths matching glutils.cpp order
    struct TextureInfo {
        QString path;
        RhiTextures index;
    };

    const TextureInfo textureFiles[] = {
        { PREFIX "/images/textures/floor.png", RHI_FLOOR },
        { PREFIX "/images/textures/Font.png", RHI_FONT },
        { PREFIX "/images/textures/Lift.png", RHI_HYDLIFT },
        { PREFIX "/images/textures/z_Tractor.png", RHI_TRACTOR },
        { PREFIX "/images/textures/z_QuestionMark.png", RHI_QUESTION_MARK },
        { PREFIX "/images/textures/FrontWheels.png", RHI_FRONT_WHEELS },
        { PREFIX "/images/textures/Tractor4WDFront.png", RHI_TRACTOR_4WD_FRONT },
        { PREFIX "/images/textures/Tractor4WDRear.png", RHI_TRACTOR_4WD_REAR },
        { PREFIX "/images/textures/Harvester.png", RHI_HARVESTER },
        { PREFIX "/images/textures/z_Tool.png", RHI_TOOLWHEELS },
        { PREFIX "/images/textures/z_Tire.png", RHI_TIRE }
    };

    for (const auto &info : textureFiles) {
        QImage img(info.path);
        if (img.isNull()) {
            qWarning() << "Failed to load texture:" << info.path;
            return false;
        }

        // Convert to RGBA8 format
        img = img.convertToFormat(QImage::Format_RGBA8888);

        // Save font texture dimensions
        if (info.index == RHI_FONT) {
            m_fontTextureWidth = img.width();
            m_fontTextureHeight = img.height();
            qDebug() << "  Font texture size:" << m_fontTextureWidth << "x" << m_fontTextureHeight;
        }

        // Create texture
        QRhiTexture *tex = m_rhi->newTexture(
            QRhiTexture::RGBA8,
            img.size(),
            1,
            QRhiTexture::UsedAsTransferSource
        );

        if (!tex->create()) {
            qWarning() << "Failed to create texture for:" << info.path;
            delete tex;
            return false;
        }

        // Create sampler
        QRhiSampler *sampler = m_rhi->newSampler(
            QRhiSampler::Linear,
            QRhiSampler::Linear,
            QRhiSampler::None,
            QRhiSampler::ClampToEdge,
            QRhiSampler::ClampToEdge
        );

        if (!sampler->create()) {
            qWarning() << "Failed to create sampler for:" << info.path;
            delete tex;
            delete sampler;
            return false;
        }

        // Upload texture data (will be done in first frame)
        // For now, just store the image data to be uploaded later
        // Note: In actual implementation, you'll need to upload via QRhiResourceUpdateBatch

        m_textures[info.index].texture = tex;
        m_textures[info.index].sampler = sampler;

        qDebug() << "  Loaded texture:" << info.path;
    }

    qDebug() << "  All textures loaded successfully";
    return true;
}
