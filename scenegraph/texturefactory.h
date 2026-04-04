// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
//
// Texture factory for scene graph rendering - creates and caches QSGTextures

#ifndef TEXTUREFACTORY_H
#define TEXTUREFACTORY_H

#include <QSGTexture>
#include <QHash>
#include <QSize>

class QQuickWindow;

// Texture identifiers matching the old glutils.h enum
enum class TextureId {
    Floor,
    Font,
    HydLift,
    Tractor,
    QuestionMark,
    FrontWheels,
    Tractor4WDFront,
    Tractor4WDRear,
    Harvester,
    ToolWheels,
    Tire
};

class TextureFactory
{
public:
    explicit TextureFactory(QQuickWindow *window);
    ~TextureFactory();

    // Get a texture by ID, creating it on demand if needed
    // Returns nullptr if texture cannot be loaded
    QSGTexture *texture(TextureId id);

    // Get texture dimensions (loads texture if not already loaded)
    int textureWidth(TextureId id);
    int textureHeight(TextureId id);
    QSize textureSize(TextureId id);

    // Check if a texture is already loaded
    bool isLoaded(TextureId id) const;

    // Preload all vehicle textures (call after construction to avoid runtime delays)
    void preload();

    // Clear all cached textures (call when window changes or on cleanup)
    void clear();

    // Set the window (needed if window changes)
    void setWindow(QQuickWindow *window);

private:
    // Load a texture from the resource path
    QSGTexture *loadTexture(TextureId id);

    // Get the resource path for a texture
    static QString texturePath(TextureId id);

    QQuickWindow *m_window = nullptr;
    QHash<TextureId, QSGTexture *> m_textures;
    QHash<TextureId, QSize> m_textureSizes;
};

#endif // TEXTUREFACTORY_H
