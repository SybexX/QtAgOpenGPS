// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
//
// Layers properties implementation

#include "layersproperties.h"

LayersProperties::LayersProperties(QObject *parent)
    : QObject(parent)
{
}

const CoverageLayer* LayersProperties::layer(int layerId) const
{
    auto it = m_layers.constFind(layerId);
    if (it != m_layers.constEnd()) {
        return &it.value();
    }
    return nullptr;
}

void LayersProperties::addLayer(int layerId, const QString &name)
{
    if (m_layers.contains(layerId)) {
        return; // Layer already exists
    }

    CoverageLayer newLayer(layerId, name);
    // Configure with current section count
    if (m_sectionCount > 0) {
        newLayer.configureSections(m_sectionCount);
    }
    m_layers.insert(layerId, newLayer);
    emit layerAdded(layerId);
}

void LayersProperties::removeLayer(int layerId)
{
    if (m_layers.remove(layerId) > 0) {
        emit layerRemoved(layerId);
    }
}

void LayersProperties::clearAllLayers()
{
    QList<int> ids = m_layers.keys();
    m_layers.clear();
    for (int id : ids) {
        emit layerRemoved(id);
    }
}

void LayersProperties::addTriangles(int layerId, const QVector<CoverageTriangle> &triangles)
{
    auto it = m_layers.find(layerId);
    if (it == m_layers.end()) {
        return; // Layer doesn't exist
    }

    it->triangles.append(triangles);
    it->updateBoundingBox();
    emit trianglesChanged(layerId);
}

void LayersProperties::addTriangle(int layerId,
                                    const QVector3D &v0,
                                    const QVector3D &v1,
                                    const QVector3D &v2,
                                    const QColor &color)
{
    auto it = m_layers.find(layerId);
    if (it == m_layers.end()) {
        return; // Layer doesn't exist
    }

    CoverageTriangle tri(v0, v1, v2, color);
    it->triangles.append(tri);
    it->updateBoundingBox();
    emit trianglesChanged(layerId);
}

void LayersProperties::clearTriangles(int layerId)
{
    auto it = m_layers.find(layerId);
    if (it == m_layers.end()) {
        return;
    }

    it->triangles.clear();
    it->boundingBox = QRectF();
    emit trianglesInvalidated(layerId);
}

int LayersProperties::triangleCount(int layerId) const
{
    auto it = m_layers.constFind(layerId);
    if (it != m_layers.constEnd()) {
        return it->triangles.count();
    }
    return 0;
}

bool LayersProperties::layerIntersectsArea(int layerId, const QRectF &area) const
{
    auto it = m_layers.constFind(layerId);
    if (it != m_layers.constEnd()) {
        return it->intersects(area);
    }
    return false;
}

bool LayersProperties::layerIntersectsArea(int layerId, qreal x, qreal y, qreal width, qreal height) const
{
    return layerIntersectsArea(layerId, QRectF(x, y, width, height));
}

QVector<CoverageTriangle> LayersProperties::getTrianglesInArea(int layerId, const QRectF &area) const
{
    auto it = m_layers.constFind(layerId);
    if (it != m_layers.constEnd()) {
        return it->getTrianglesInArea(area);
    }
    return QVector<CoverageTriangle>();
}

QVector<CoverageTriangle> LayersProperties::getTrianglesInArea(int layerId, qreal x, qreal y, qreal width, qreal height) const
{
    return getTrianglesInArea(layerId, QRectF(x, y, width, height));
}

QRectF LayersProperties::layerBoundingBox(int layerId) const
{
    auto it = m_layers.constFind(layerId);
    if (it != m_layers.constEnd()) {
        return it->boundingBox;
    }
    return QRectF();
}

void LayersProperties::setLayerVisible(int layerId, bool visible)
{
    auto it = m_layers.find(layerId);
    if (it == m_layers.end()) {
        return;
    }

    if (it->visible != visible) {
        it->visible = visible;
        emit layerVisibilityChanged(layerId, visible);
    }
}

bool LayersProperties::isLayerVisible(int layerId) const
{
    auto it = m_layers.constFind(layerId);
    if (it != m_layers.constEnd()) {
        return it->visible;
    }
    return false;
}

void LayersProperties::setLayerAlpha(int layerId, float alpha)
{
    auto it = m_layers.find(layerId);
    if (it == m_layers.end()) {
        return;
    }

    // Clamp alpha to valid range
    alpha = qBound(0.0f, alpha, 1.0f);

    if (!qFuzzyCompare(it->alpha, alpha)) {
        it->alpha = alpha;
        emit layerAlphaChanged(layerId, alpha);
    }
}

float LayersProperties::layerAlpha(int layerId) const
{
    auto it = m_layers.constFind(layerId);
    if (it != m_layers.constEnd()) {
        return it->alpha;
    }
    return 0.596f; // Default AOG alpha
}

void LayersProperties::setTrianglesAlpha(int layerId, float alpha)
{
    auto it = m_layers.find(layerId);
    if (it == m_layers.end()) {
        return;
    }

    // Clamp alpha to valid range
    alpha = qBound(0.0f, alpha, 1.0f);

    // Update alpha on all triangles in this layer
    for (CoverageTriangle &tri : it->triangles) {
        QColor c = tri.color;
        c.setAlphaF(alpha);
        tri.color = c;
    }

    // Signal that existing triangles were modified so LayersNode will rebuild geometry
    emit trianglesInvalidated(layerId);
}

void LayersProperties::setSectionCount(int count)
{
    if (m_sectionCount == count) {
        return;
    }

    // Flush all pending sections before reconfiguring
    flushAllPendingSections();

    m_sectionCount = count;

    // Reconfigure all layers for new section count
    for (auto it = m_layers.begin(); it != m_layers.end(); ++it) {
        it->configureSections(count);
    }

    emit sectionCountChanged();
}

int LayersProperties::addSectionVertices(int layerId, int sectionIndex,
                                          const QVector3D &left, const QVector3D &right,
                                          const QColor &color)
{
    auto it = m_layers.find(layerId);
    if (it == m_layers.end()) {
        return 0;
    }

    int added = it->addSectionVertices(sectionIndex, left, right, color);

    if (added > 0) {
        emit trianglesChanged(layerId);
    }

    return added;
}

int LayersProperties::addZoneVertices(int layerId, int zoneIndex, int startSection, int endSection,
                                      const QVector3D &left, const QVector3D &right,
                                      const QColor &color)
{
    auto it = m_layers.find(layerId);
    if (it == m_layers.end()) {
        return 0;
    }

    int added = it->addZoneVertices(zoneIndex, startSection, endSection, left, right, color);

    if (added > 0) {
        emit trianglesChanged(layerId);
    }

    return added;
}

bool LayersProperties::isSectionPending(int layerId, int sectionIndex) const
{
    auto it = m_layers.constFind(layerId);
    if (it != m_layers.constEnd()) {
        if (sectionIndex >= 0 && sectionIndex < it->pendingSections.size()) {
            return it->pendingSections[sectionIndex].hasPrevious;
        }
    }
    return false;
}

bool LayersProperties::isZonePending(int layerId, int zoneIndex) const
{
    auto it = m_layers.constFind(layerId);
    if (it != m_layers.constEnd()) {
        if (zoneIndex >= 0 && zoneIndex < it->pendingZones.size()) {
            return it->pendingZones[zoneIndex].hasPrevious;
        }
    }
    return false;
}

void LayersProperties::flushPendingSections(int layerId)
{
    auto it = m_layers.find(layerId);
    if (it != m_layers.end()) {
        it->flushPendingSections();
    }
}

void LayersProperties::flushPendingSection(int layerId, int sectionIndex)
{
    auto it = m_layers.find(layerId);
    if (it != m_layers.end()) {
        it->flushPendingSection(sectionIndex);
    }
}

void LayersProperties::flushAllPendingSections()
{
    for (auto it = m_layers.begin(); it != m_layers.end(); ++it) {
        it->flushPendingSections();
    }
}
