// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
//
// Layers properties for FieldViewItem - grouped property for QML
// Contains coverage layer data for scene graph rendering

#ifndef LAYERSPROPERTIES_H
#define LAYERSPROPERTIES_H

#include <QObject>
#include <QProperty>
#include <QBindable>
#include <QHash>
#include <QVector>
#include <QtQml/qqmlregistration.h>

#include "layertypes.h"
#include "simpleproperty.h"

class LayersProperties : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(Layers)

    // Global visibility toggle for all layers
    Q_PROPERTY(bool visible READ visible WRITE setVisible NOTIFY visibleChanged BINDABLE bindableVisible)

    // Number of sections for pending triangle building
    Q_PROPERTY(int sectionCount READ sectionCount WRITE setSectionCount NOTIFY sectionCountChanged)

public:
    explicit LayersProperties(QObject *parent = nullptr);

    // Visibility accessors
    bool visible() const { return m_visible.value(); }
    void setVisible(bool value) { m_visible.setValue(value); }
    QBindable<bool> bindableVisible() { return &m_visible; }

    // ===== Layer Management =====

    // Get all layer IDs
    QList<int> layerIds() const { return m_layers.keys(); }

    // Check if a layer exists
    bool hasLayer(int layerId) const { return m_layers.contains(layerId); }

    // Get layer by ID (returns nullptr if not found)
    const CoverageLayer* layer(int layerId) const;

    // Get all layers (for iteration in scene graph)
    const QHash<int, CoverageLayer>& layers() const { return m_layers; }

    // ===== Layer Operations (called by LayerService or QML) =====

    // Add a new layer
    Q_INVOKABLE void addLayer(int layerId, const QString &name = QString());

    // Remove a layer
    Q_INVOKABLE void removeLayer(int layerId);

    // Clear all layers
    Q_INVOKABLE void clearAllLayers();

    // ===== Triangle Operations =====

    // Add triangles to a layer
    void addTriangles(int layerId, const QVector<CoverageTriangle> &triangles);

    // Add a single triangle from QML (convenience method)
    Q_INVOKABLE void addTriangle(int layerId,
                                  const QVector3D &v0,
                                  const QVector3D &v1,
                                  const QVector3D &v2,
                                  const QColor &color);

    // Clear triangles from a layer
    Q_INVOKABLE void clearTriangles(int layerId);

    // Get triangle count for a layer
    Q_INVOKABLE int triangleCount(int layerId) const;

    // ===== Spatial Queries =====

    // Quick check if layer's bounding box intersects the query area
    Q_INVOKABLE bool layerIntersectsArea(int layerId, const QRectF &area) const;
    Q_INVOKABLE bool layerIntersectsArea(int layerId, qreal x, qreal y, qreal width, qreal height) const;

    // Get triangles within a bounding area (linear scan)
    QVector<CoverageTriangle> getTrianglesInArea(int layerId, const QRectF &area) const;
    QVector<CoverageTriangle> getTrianglesInArea(int layerId, qreal x, qreal y, qreal width, qreal height) const;

    // Get layer's bounding box
    Q_INVOKABLE QRectF layerBoundingBox(int layerId) const;

    // ===== Layer Properties =====

    // Set layer visibility
    void setLayerVisible(int layerId, bool visible);
    bool isLayerVisible(int layerId) const;

    // Set layer alpha (multiplied with triangle alpha during rendering)
    void setLayerAlpha(int layerId, float alpha);
    float layerAlpha(int layerId) const;

    // Set alpha on all triangles in a layer (e.g., for day/night mode switch)
    void setTrianglesAlpha(int layerId, float alpha);

    // ===== Section Management =====

    // Section count - when changed, flushes pending and reconfigures all layers
    int sectionCount() const { return m_sectionCount; }
    void setSectionCount(int count);

    // Add two vertices for a section (triangle strip style)
    // Returns number of triangles added (0 or 2)
    int addSectionVertices(int layerId, int sectionIndex,
                           const QVector3D &left, const QVector3D &right,
                           const QColor &color);

    int addZoneVertices(int layerId, int zoneIndex, int startSection, int endSection,
                        const QVector3D &left, const QVector3D &right,
                        const QColor &color);

    // Check if a section has pending vertices (is currently drawing)
    bool isSectionPending(int layerId, int sectionIndex) const;

    // Check if a zone has pending vertices (is currently drawing)
    bool isZonePending(int layerId, int zoneIndex) const;

    // Flush pending sections for a layer (e.g., when lifting tool)
    void flushPendingSections(int layerId);

    // Flush a single pending section for a layer (when one section turns off)
    void flushPendingSection(int layerId, int sectionIndex);

    // Flush all pending sections across all layers
    void flushAllPendingSections();

signals:
    void visibleChanged();
    void sectionCountChanged();
    void layerAdded(int layerId);
    void layerRemoved(int layerId);
    void trianglesChanged(int layerId);       // Triangles appended (incremental update)
    void trianglesInvalidated(int layerId);   // Triangles cleared/modified (full rebuild needed)
    void layerVisibilityChanged(int layerId, bool visible);
    void layerAlphaChanged(int layerId, float alpha);

private:
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(LayersProperties, bool, m_visible, true, &LayersProperties::visibleChanged)

    // Layer storage
    QHash<int, CoverageLayer> m_layers;

    // Section count for pending triangle building
    int m_sectionCount = 0;
};

#endif // LAYERSPROPERTIES_H
