// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
//
// Layer service - backend singleton for managing coverage layers
// Provides layer creation, triangle storage, and updates to scene graph

#ifndef LAYERSERVICE_H
#define LAYERSERVICE_H

#include <QObject>
#include <QMutex>
#include <QQmlEngine>
#include <QtQml>
#include <QHash>
#include <QVector>

#include "layertypes.h"
#include "layersproperties.h"

/**
 * @brief LayerService - Singleton service for managing coverage layers
 *
 * This service provides:
 * - Layer creation, deletion, and clearing
 * - Triangle addition to layers
 * - Visibility and alpha control per layer
 * - A LayersProperties instance for binding to FieldViewItem
 *
 * Usage in QML:
 *   FieldViewItem {
 *       layers: LayerService.layers  // Bind to show coverage
 *   }
 *   FieldViewItem {
 *       layers: null  // No coverage in this view
 *   }
 */
class LayerService : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    // LayersProperties for QML binding to FieldViewItem
    Q_PROPERTY(LayersProperties* layers READ layers CONSTANT)

    // Number of layers
    Q_PROPERTY(int layerCount READ layerCount NOTIFY layerCountChanged)

    // Default layer ID (for simple use cases)
    Q_PROPERTY(int defaultLayerId READ defaultLayerId CONSTANT)

public:
    // Singleton access
    static LayerService* instance();
    static LayerService* create(QQmlEngine *qmlEngine, QJSEngine *jsEngine);

    // LayersProperties for QML binding
    LayersProperties* layers() const { return m_layersProperties; }

    // ===== Layer Management =====

    // Create a new layer with given name
    // Returns the layer ID
    Q_INVOKABLE int createLayer(const QString &name = QString());

    // Delete a layer by ID
    Q_INVOKABLE void deleteLayer(int layerId);

    // Clear all triangles from a layer (keep the layer)
    Q_INVOKABLE void clearLayer(int layerId);

    // Clear all layers
    Q_INVOKABLE void clearAllLayers();

    // Get layer count
    int layerCount() const { return m_layersProperties ? m_layersProperties->layers().count() : 0; }

    // Get default layer ID
    int defaultLayerId() const { return m_defaultLayerId; }

    // Check if a layer exists
    Q_INVOKABLE bool hasLayer(int layerId) const;

    // ===== Triangle Operations =====

    // Add a single triangle to a layer
    void addTriangle(int layerId, const CoverageTriangle &triangle);

    // Add multiple triangles to a layer
    void addTriangles(int layerId, const QVector<CoverageTriangle> &triangles);

    // Convenience: Add triangle by vertices and color
    Q_INVOKABLE void addTriangle(int layerId,
                                  const QVector3D &v0,
                                  const QVector3D &v1,
                                  const QVector3D &v2,
                                  const QColor &color);

    // Convenience: Add to default layer
    void addTriangleToDefault(const CoverageTriangle &triangle);
    void addTrianglesToDefault(const QVector<CoverageTriangle> &triangles);

    // Get triangle count for a layer
    Q_INVOKABLE int triangleCount(int layerId) const;

    // Get total triangle count across all layers
    Q_INVOKABLE int totalTriangleCount() const;

    // ===== Layer Properties =====

    // Set layer visibility
    Q_INVOKABLE void setLayerVisible(int layerId, bool visible);
    Q_INVOKABLE bool isLayerVisible(int layerId) const;

    // Set layer alpha
    Q_INVOKABLE void setLayerAlpha(int layerId, float alpha);
    Q_INVOKABLE float layerAlpha(int layerId) const;

    // Set layer name
    Q_INVOKABLE void setLayerName(int layerId, const QString &name);
    Q_INVOKABLE QString layerName(int layerId) const;

    // ===== Section Management =====

    // Check if a section has pending vertices (is currently drawing)
    // layerId defaults to the default layer (-1 means use default)
    bool isSectionPending(int sectionIndex, int layerId = -1) const;

    // Check if a zone has pending vertices (is currently drawing)
    bool isZonePending(int zoneIndex, int layerId = -1) const;

    // Add two vertices for a section (triangle strip style)
    // Returns number of triangles added (0 or 2)
    // layerId defaults to the default layer (-1 means use default)
    int addSectionVertices(int sectionIndex,
                           const QVector3D &left, const QVector3D &right,
                           const QColor &color, int layerId = -1);

    int addZoneVertices(int zoneIndex, int startSection, int endSection,
                         const QVector3D &left, const QVector3D &right,
                         const QColor &color, int layerId = -1);

    // Flush a single pending section for a layer (when one section turns off)
    // layerId defaults to the default layer (-1 means use default)
    void flushPendingSection(int sectionIndex, int layerId = -1);

    // Flush all pending sections for a layer
    // layerId defaults to the default layer (-1 means use default)
    void flushPendingSections(int layerId = -1);

signals:
    void layerCountChanged();
    void layerCreated(int layerId, const QString &name);
    void layerDeleted(int layerId);
    void layerCleared(int layerId);
    void allLayersCleared();
    void trianglesAdded(int layerId, int count);
    void layerVisibilityChanged(int layerId, bool visible);
    void layerAlphaChanged(int layerId, float alpha);
    void layerNameChanged(int layerId, const QString &name);

private:
    // Private constructor for singleton pattern
    explicit LayerService(QObject *parent = nullptr);
    ~LayerService();

    // Prevent copying
    LayerService(const LayerService &) = delete;
    LayerService &operator=(const LayerService &) = delete;

    static LayerService *s_instance;
    static QMutex s_mutex;

    // Owned LayersProperties for QML binding
    LayersProperties *m_layersProperties = nullptr;

    int m_nextLayerId = 0;
    int m_defaultLayerId = -1;

    // Ensure default layer exists
    void ensureDefaultLayer();
};

#endif // LAYERSERVICE_H
