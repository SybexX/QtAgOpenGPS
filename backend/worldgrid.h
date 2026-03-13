// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
//
// WorldGrid singleton - manages world grid rendering and field bounds

#ifndef WORLDGRID_H
#define WORLDGRID_H

#include <QObject>
#include <QMatrix4x4>
#include <QQmlEngine>
#include <QMutex>
#include <QColor>
#include <QOpenGLBuffer>
#include "simpleproperty.h"

class QOpenGLFunctions;
class QOpenGLShaderProgram;
class CCamera;

class WorldGrid : public QObject
{
    Q_OBJECT
    QML_SINGLETON
    QML_ELEMENT

private:
    explicit WorldGrid(QObject *parent = nullptr);
    ~WorldGrid() override;

    // Prevent copying
    WorldGrid(const WorldGrid &) = delete;
    WorldGrid &operator=(const WorldGrid &) = delete;

    static WorldGrid *s_instance;
    static QMutex s_mutex;
    static bool s_cpp_created;

    // OpenGL resources
    QOpenGLShaderProgram *m_fieldShader = nullptr;
    QOpenGLBuffer m_fieldBuffer;
    QOpenGLBuffer m_rateMapBuffer;
    QOpenGLBuffer m_gridBuffer;
    int m_gridBufferCount = 0;

    // Buffer state tracking
    bool m_fieldBufferCurrent = false;
    bool m_rateMapBufferCurrent = false;

    double m_lastGridZoom = -1;
    double m_lastCount = -1;
    double m_lastNorthingMax = 0;
    double m_lastNorthingMin = 0;
    double m_lastEastingMax = 0;
    double m_lastEastingMin = 0;

    void invalidateBuffers();

public:
    // Singleton access
    static WorldGrid *instance();
    static WorldGrid *create(QQmlEngine *qmlEngine, QJSEngine *jsEngine);

    // Drawing methods
    void DrawFieldSurface(QOpenGLFunctions *gl, const QMatrix4x4 &mvp,
                          bool isTextureOn, QColor fieldColor, double _gridZoom);
    void DrawWorldGrid(QOpenGLFunctions *gl, QMatrix4x4 modelview, QMatrix4x4 projection,
                       double _gridZoom, QColor gridColor);
    void destroyGLBuffers();

    // ===== QML PROPERTIES =====
    SIMPLE_BINDABLE_PROPERTY(double, northingMax)
    SIMPLE_BINDABLE_PROPERTY(double, northingMin)
    SIMPLE_BINDABLE_PROPERTY(double, eastingMax)
    SIMPLE_BINDABLE_PROPERTY(double, eastingMin)
    SIMPLE_BINDABLE_PROPERTY(double, gridSize)
    SIMPLE_BINDABLE_PROPERTY(double, count)
    SIMPLE_BINDABLE_PROPERTY(bool, isGeoMap)
    SIMPLE_BINDABLE_PROPERTY(double, northingMaxGeo)
    SIMPLE_BINDABLE_PROPERTY(double, northingMinGeo)
    SIMPLE_BINDABLE_PROPERTY(double, eastingMaxGeo)
    SIMPLE_BINDABLE_PROPERTY(double, eastingMinGeo)


    SIMPLE_BINDABLE_PROPERTY(double, fixEasting)
    SIMPLE_BINDABLE_PROPERTY(double, fixNorthing)

public slots:
    void checkZoomWorldGrid(double northing, double easting);

signals:
    //automatic from SIMPLE_BINDABLE_PROPERTY

private:
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(WorldGrid, double, m_northingMax, 0, &WorldGrid::northingMaxChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(WorldGrid, double, m_northingMin, 0, &WorldGrid::northingMinChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(WorldGrid, double, m_eastingMax, 0, &WorldGrid::eastingMaxChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(WorldGrid, double, m_eastingMin, 0, &WorldGrid::eastingMinChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(WorldGrid, double, m_gridSize, 6000, &WorldGrid::gridSizeChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(WorldGrid, double, m_count, 40, &WorldGrid::countChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(WorldGrid, bool, m_isGeoMap, false, &WorldGrid::isGeoMapChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(WorldGrid, double, m_northingMaxGeo, 300, &WorldGrid::northingMaxGeoChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(WorldGrid, double, m_northingMinGeo, -300, &WorldGrid::northingMinGeoChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(WorldGrid, double, m_eastingMaxGeo, 300, &WorldGrid::eastingMaxGeoChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(WorldGrid, double, m_eastingMinGeo, -300, &WorldGrid::eastingMinGeoChanged)

    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(WorldGrid, double, m_fixEasting, 0, &WorldGrid::fixEastingChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(WorldGrid, double, m_fixNorthing, 0, &WorldGrid::fixNorthingChanged)
};

#endif // WORLDGRID_H
