// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
//
// Texture factory implementation

#include "texturefactory.h"
#include <QQuickWindow>
#include <QImage>

// Resource paths must use URL format:
// - LOCAL_QML: "local:/path" for filesystem access during development
// - Normal:    ":/AOG/path" for compiled Qt resources
#ifdef LOCAL_QML
#define TEXTURE_PREFIX "local:/images/textures/"
#else
#define TEXTURE_PREFIX ":/AOG/images/textures/"
#endif

TextureFactory::TextureFactory(QQuickWindow *window)
    : m_window(window)
{
}

TextureFactory::~TextureFactory()
{
    clear();
}

QSGTexture *TextureFactory::texture(TextureId id)
{
    // Return cached texture if available
    auto it = m_textures.find(id);
    if (it != m_textures.end()) {
        return it.value();
    }

    // Load and cache the texture
    QSGTexture *tex = loadTexture(id);
    if (tex) {
        m_textures.insert(id, tex);
    }
    return tex;
}

bool TextureFactory::isLoaded(TextureId id) const
{
    return m_textures.contains(id);
}

void TextureFactory::preload()
{
    if (!m_window) return;
    texture(TextureId::Floor);
    texture(TextureId::Font);
    texture(TextureId::HydLift);
    texture(TextureId::Tractor);
    texture(TextureId::QuestionMark);
    texture(TextureId::FrontWheels);
    texture(TextureId::Tractor4WDFront);
    texture(TextureId::Tractor4WDRear);
    texture(TextureId::Harvester);
    texture(TextureId::ToolWheels);
    texture(TextureId::Tire);
}

int TextureFactory::textureWidth(TextureId id)
{
    return textureSize(id).width();
}

int TextureFactory::textureHeight(TextureId id)
{
    return textureSize(id).height();
}

QSize TextureFactory::textureSize(TextureId id)
{
    // Load texture if not already loaded (which also stores the size)
    if (!m_textureSizes.contains(id)) {
        texture(id);
    }
    return m_textureSizes.value(id, QSize(0, 0));
}

void TextureFactory::clear()
{
    for (QSGTexture *tex : std::as_const(m_textures)) {
        delete tex;
    }
    m_textures.clear();
    m_textureSizes.clear();
}

void TextureFactory::setWindow(QQuickWindow *window)
{
    if (m_window != window) {
        // Clear textures when window changes - they're tied to the old context
        clear();
        m_window = window;
    }
}

QSGTexture *TextureFactory::loadTexture(TextureId id)
{
    if (!m_window) {
        return nullptr;
    }

    QString path = texturePath(id);
    QImage image(path);

    if (image.isNull()) {
        qWarning() << "TextureFactory: Failed to load texture:" << path;
        return nullptr;
    }

    // Convert to RGBA format for consistent texture handling
    if (image.format() != QImage::Format_RGBA8888 &&
        image.format() != QImage::Format_RGBA8888_Premultiplied) {
        image = image.convertToFormat(QImage::Format_RGBA8888);
    }

    // Store the image size before creating texture
    m_textureSizes.insert(id, image.size());

    QSGTexture *tex = m_window->createTextureFromImage(image);
    if (tex) {
        // Default texture settings - can be customized per texture if needed
        tex->setHorizontalWrapMode(QSGTexture::ClampToEdge);
        tex->setVerticalWrapMode(QSGTexture::ClampToEdge);
        tex->setFiltering(QSGTexture::Linear);
        tex->setMipmapFiltering(QSGTexture::None);

        // Special case: Floor texture needs repeat wrapping
        if (id == TextureId::Floor) {
            tex->setHorizontalWrapMode(QSGTexture::Repeat);
            tex->setVerticalWrapMode(QSGTexture::Repeat);
        }
    }

    return tex;
}

QString TextureFactory::texturePath(TextureId id)
{
    switch (id) {
    case TextureId::Floor:
        return QStringLiteral(TEXTURE_PREFIX "floor.png");
    case TextureId::Font:
        return QStringLiteral(TEXTURE_PREFIX "Font.png");
    case TextureId::HydLift:
        return QStringLiteral(TEXTURE_PREFIX "Lift.png");
    case TextureId::Tractor:
        return QStringLiteral(TEXTURE_PREFIX "z_Tractor.png");
    case TextureId::QuestionMark:
        return QStringLiteral(TEXTURE_PREFIX "z_QuestionMark.png");
    case TextureId::FrontWheels:
        return QStringLiteral(TEXTURE_PREFIX "FrontWheels.png");
    case TextureId::Tractor4WDFront:
        return QStringLiteral(TEXTURE_PREFIX "Tractor4WDFront.png");
    case TextureId::Tractor4WDRear:
        return QStringLiteral(TEXTURE_PREFIX "Tractor4WDRear.png");
    case TextureId::Harvester:
        return QStringLiteral(TEXTURE_PREFIX "Harvester.png");
    case TextureId::ToolWheels:
        return QStringLiteral(TEXTURE_PREFIX "z_Tool.png");
    case TextureId::Tire:
        return QStringLiteral(TEXTURE_PREFIX "z_Tire.png");
    }

    return QString();
}
