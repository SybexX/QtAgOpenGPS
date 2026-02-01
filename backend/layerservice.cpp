// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
//
// Layer service implementation

#include "layerservice.h"
#include "layersproperties.h"

// Static members
LayerService* LayerService::s_instance = nullptr;
QMutex LayerService::s_mutex;

LayerService::LayerService(QObject *parent)
    : QObject(parent)
{
    // Create owned LayersProperties
    m_layersProperties = new LayersProperties(this);

    // Create default layer
    ensureDefaultLayer();
}

LayerService::~LayerService()
{
}

LayerService* LayerService::instance()
{
    QMutexLocker locker(&s_mutex);
    if (!s_instance) {
        s_instance = new LayerService();
    }
    return s_instance;
}

LayerService* LayerService::create(QQmlEngine *qmlEngine, QJSEngine *jsEngine)
{
    Q_UNUSED(jsEngine)

    // Ensure singleton instance exists
    LayerService *service = instance();

    // QML engine takes ownership
    QJSEngine::setObjectOwnership(service, QJSEngine::CppOwnership);

    return service;
}

void LayerService::ensureDefaultLayer()
{
    if (m_defaultLayerId < 0) {
        m_defaultLayerId = createLayer(QStringLiteral("Default"));
    }
}

int LayerService::createLayer(const QString &name)
{
    int layerId = m_nextLayerId++;
    QString layerName = name.isEmpty() ? QString("Layer %1").arg(layerId) : name;

    m_layersProperties->addLayer(layerId, layerName);

    emit layerCreated(layerId, layerName);
    emit layerCountChanged();

    return layerId;
}

void LayerService::deleteLayer(int layerId)
{
    // Don't delete the default layer
    if (layerId == m_defaultLayerId) {
        clearLayer(layerId);
        return;
    }

    if (m_layersProperties->hasLayer(layerId)) {
        m_layersProperties->removeLayer(layerId);
        emit layerDeleted(layerId);
        emit layerCountChanged();
    }
}

void LayerService::clearLayer(int layerId)
{
    if (!m_layersProperties->hasLayer(layerId)) {
        return;
    }

    m_layersProperties->clearTriangles(layerId);
    emit layerCleared(layerId);
}

void LayerService::clearAllLayers()
{
    for (int layerId : m_layersProperties->layerIds()) {
        m_layersProperties->clearTriangles(layerId);
    }
    emit allLayersCleared();
}

bool LayerService::hasLayer(int layerId) const
{
    return m_layersProperties->hasLayer(layerId);
}

void LayerService::addTriangle(int layerId, const CoverageTriangle &triangle)
{
    if (!m_layersProperties->hasLayer(layerId)) {
        return;
    }

    QVector<CoverageTriangle> triangles;
    triangles.append(triangle);
    m_layersProperties->addTriangles(layerId, triangles);

    emit trianglesAdded(layerId, 1);
}

void LayerService::addTriangles(int layerId, const QVector<CoverageTriangle> &triangles)
{
    if (triangles.isEmpty()) {
        return;
    }

    if (!m_layersProperties->hasLayer(layerId)) {
        return;
    }

    m_layersProperties->addTriangles(layerId, triangles);
    emit trianglesAdded(layerId, triangles.count());
}

void LayerService::addTriangle(int layerId,
                                const QVector3D &v0,
                                const QVector3D &v1,
                                const QVector3D &v2,
                                const QColor &color)
{
    CoverageTriangle triangle(v0, v1, v2, color);
    addTriangle(layerId, triangle);
}

void LayerService::addTriangleToDefault(const CoverageTriangle &triangle)
{
    ensureDefaultLayer();
    addTriangle(m_defaultLayerId, triangle);
}

void LayerService::addTrianglesToDefault(const QVector<CoverageTriangle> &triangles)
{
    ensureDefaultLayer();
    addTriangles(m_defaultLayerId, triangles);
}

int LayerService::triangleCount(int layerId) const
{
    return m_layersProperties->triangleCount(layerId);
}

int LayerService::totalTriangleCount() const
{
    int total = 0;
    for (int layerId : m_layersProperties->layerIds()) {
        total += m_layersProperties->triangleCount(layerId);
    }
    return total;
}

void LayerService::setLayerVisible(int layerId, bool visible)
{
    if (!m_layersProperties->hasLayer(layerId)) {
        return;
    }

    if (m_layersProperties->isLayerVisible(layerId) != visible) {
        m_layersProperties->setLayerVisible(layerId, visible);
        emit layerVisibilityChanged(layerId, visible);
    }
}

bool LayerService::isLayerVisible(int layerId) const
{
    return m_layersProperties->isLayerVisible(layerId);
}

void LayerService::setLayerAlpha(int layerId, float alpha)
{
    if (!m_layersProperties->hasLayer(layerId)) {
        return;
    }

    alpha = qBound(0.0f, alpha, 1.0f);

    if (!qFuzzyCompare(m_layersProperties->layerAlpha(layerId), alpha)) {
        m_layersProperties->setLayerAlpha(layerId, alpha);
        emit layerAlphaChanged(layerId, alpha);
    }
}

float LayerService::layerAlpha(int layerId) const
{
    return m_layersProperties->layerAlpha(layerId);
}

void LayerService::setLayerName(int layerId, const QString &name)
{
    // LayersProperties doesn't store name separately, so we just emit signal
    // In a full implementation, we'd add name storage to LayersProperties
    emit layerNameChanged(layerId, name);
}

QString LayerService::layerName(int layerId) const
{
    const CoverageLayer *layer = m_layersProperties->layer(layerId);
    if (layer) {
        return layer->name;
    }
    return QString();
}

bool LayerService::isSectionPending(int sectionIndex, int layerId) const
{
    if (layerId < 0) {
        layerId = m_defaultLayerId;
    }
    return m_layersProperties->isSectionPending(layerId, sectionIndex);
}

int LayerService::addSectionVertices(int sectionIndex,
                                      const QVector3D &left, const QVector3D &right,
                                      const QColor &color, int layerId)
{
    if (layerId < 0) {
        layerId = m_defaultLayerId;
    }
    return m_layersProperties->addSectionVertices(layerId, sectionIndex, left, right, color);
}

void LayerService::flushPendingSection(int sectionIndex, int layerId)
{
    if (layerId < 0) {
        layerId = m_defaultLayerId;
    }
    m_layersProperties->flushPendingSection(layerId, sectionIndex);
}

void LayerService::flushPendingSections(int layerId)
{
    if (layerId < 0) {
        layerId = m_defaultLayerId;
    }
    m_layersProperties->flushPendingSections(layerId);
}
