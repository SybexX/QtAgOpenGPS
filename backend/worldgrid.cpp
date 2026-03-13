// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later

#include "worldgrid.h"
#include "glutils.h"
#include "glm.h"
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QCoreApplication>
#include <QLoggingCategory>
#include "backend.h"
#include "settingsmanager.h"

Q_LOGGING_CATEGORY(worldgrid_log, "worldgrid.qtagopengps")

WorldGrid *WorldGrid::s_instance = nullptr;
QMutex WorldGrid::s_mutex;
bool WorldGrid::s_cpp_created = false;

struct SurfaceVertex {
    QVector3D vertex;
    QVector2D textureCoord;
};

WorldGrid::WorldGrid(QObject *parent)
    : QObject(parent)
{
    m_northingMaxGeo = 300;
    m_northingMinGeo = -300;
    m_eastingMaxGeo = 300;
    m_eastingMinGeo = -300;
    //react to changes in fix properties, usually bound by CNMEA
    m_fixEasting.subscribe([&]() {
        checkZoomWorldGrid(m_fixEasting, m_fixNorthing);
    });

    m_fixNorthing.subscribe([&]() {
        checkZoomWorldGrid(m_fixEasting, m_fixNorthing);
    });
}

WorldGrid::~WorldGrid()
{
    // RAII should destroy our buffers for us
    // Not too worried about leaking since this is a singleton that
    // lives for the full lifetime of QtAOG
}

WorldGrid *WorldGrid::instance()
{
    QMutexLocker locker(&s_mutex);
    if (!s_instance) {
        s_instance = new WorldGrid();
        qDebug(worldgrid_log) << "WorldGrid singleton created by C++ code.";
        s_cpp_created = true;

        // Ensure cleanup on app exit
        QObject::connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit,
                         s_instance, []() {
                             delete s_instance;
                             s_instance = nullptr;
                         });
    }
    return s_instance;
}

WorldGrid *WorldGrid::create(QQmlEngine *qmlEngine, QJSEngine *jsEngine)
{
    Q_UNUSED(jsEngine)

    QMutexLocker locker(&s_mutex);

    if (!s_instance) {
        s_instance = new WorldGrid();
        qDebug(worldgrid_log) << "WorldGrid singleton created by QML engine.";
    } else if (s_cpp_created) {
        qmlEngine->setObjectOwnership(s_instance, QQmlEngine::CppOwnership);
    }

    return s_instance;
}

void WorldGrid::invalidateBuffers()
{
    if ((m_lastEastingMax != m_eastingMax) ||
        (m_lastEastingMin != m_eastingMin) ||
        (m_lastNorthingMax != m_northingMax) ||
        (m_lastNorthingMin != m_northingMin) ||
        (m_lastCount != m_count)) {
        m_lastGridZoom = -1;
        m_fieldBufferCurrent = false;
        m_rateMapBufferCurrent = false;
        m_lastCount = -1;
    }
}

void WorldGrid::DrawFieldSurface(QOpenGLFunctions *gl, const QMatrix4x4 &mvp,
                                 bool isTextureOn, QColor fieldColor,
                                 double _gridZoom)
{
    // We can save a lot of time by keeping this grid buffer on the GPU unless it needs to
    // be altered.

    // Adjust bitmap zoom based on cam zoom
    if (_gridZoom> 100) m_count = 4;
    else if (_gridZoom> 80) m_count = 8;
    else if (_gridZoom> 50) m_count = 16;
    else if (_gridZoom> 20) m_count = 32;
    else if (_gridZoom> 10) m_count = 64;
    else m_count = 80;

    if (isTextureOn)
    {
        // Check to see if we need to regenerate the buffers
        invalidateBuffers();

        if (!m_fieldBufferCurrent) {
            SurfaceVertex field[] = {
                { QVector3D(static_cast<float>(m_eastingMin), static_cast<float>(m_northingMax), -0.10f),
                  QVector2D(0, 0) },
                { QVector3D(static_cast<float>(m_eastingMax), static_cast<float>(m_northingMax), -0.10f),
                  QVector2D(static_cast<float>(m_count), 0.0f) },
                { QVector3D(static_cast<float>(m_eastingMin), static_cast<float>(m_northingMin), -0.10f),
                  QVector2D(0, static_cast<float>(m_count)) },
                { QVector3D(static_cast<float>(m_eastingMax), static_cast<float>(m_northingMin), -0.10f),
                  QVector2D(static_cast<float>(m_count), static_cast<float>(m_count)) }
            };

            if (m_fieldBuffer.isCreated())
                m_fieldBuffer.destroy();
            m_fieldBuffer.create();
            m_fieldBuffer.bind();
            m_fieldBuffer.allocate(field, sizeof(SurfaceVertex) * 4);
            m_fieldBuffer.release();

            m_fieldBufferCurrent = true;
            m_lastCount = m_count;
        }

        // Now draw the field surface
        texture[Textures::FLOOR]->bind();
        glDrawArraysTexture(gl, mvp, GL_TRIANGLE_STRIP, m_fieldBuffer, GL_FLOAT, 4, true, fieldColor);
        texture[Textures::FLOOR]->release();
    }
    else
    {
        GLHelperOneColor field;

        field.append(QVector3D(static_cast<float>(m_eastingMin), static_cast<float>(m_northingMax), -0.10f));
        field.append(QVector3D(static_cast<float>(m_eastingMax), static_cast<float>(m_northingMax), -0.10f));
        field.append(QVector3D(static_cast<float>(m_eastingMin), static_cast<float>(m_northingMin), -0.10f));
        field.append(QVector3D(static_cast<float>(m_eastingMax), static_cast<float>(m_northingMin), -0.10f));

        field.draw(gl, mvp, fieldColor, GL_TRIANGLE_STRIP, 1.0f);
    }

    if (m_isGeoMap)
    {
        QColor geomap_color = QColor::fromRgbF(0.6f, 0.6f, 0.6f, 0.5f);

        SurfaceVertex field[] = {
            { QVector3D(static_cast<float>(m_eastingMin), static_cast<float>(m_northingMax), -0.05f),
              QVector2D(0, 0) },
            { QVector3D(static_cast<float>(m_eastingMax), static_cast<float>(m_northingMax), -0.05f),
              QVector2D(static_cast<float>(m_count), 0.0f) },
            { QVector3D(static_cast<float>(m_eastingMin), static_cast<float>(m_northingMin), -0.05f),
              QVector2D(0, static_cast<float>(m_count)) },
            { QVector3D(static_cast<float>(m_eastingMax), static_cast<float>(m_northingMin), -0.05f),
              QVector2D(static_cast<float>(m_count), static_cast<float>(m_count)) }
        };
        Q_UNUSED(field)
        Q_UNUSED(geomap_color)

        // TODO: bind bing map texture
        // TODO: glDrawArraysTexture(gl, mvp, GL_TRIANGLE_STRIP, m_fieldBuffer, GL_FLOAT, 4, true, geomap_color);
        // TODO: unbind texture
    }
}

void WorldGrid::DrawWorldGrid(QOpenGLFunctions *gl, QMatrix4x4 modelview, QMatrix4x4 projection,
                              double _gridZoom, QColor gridColor)
{
    //modelview.rotate(static_cast<float>(-m_gridRotation), 0.0f, 0.0f, 1.0f);
    QMatrix4x4 mvp = projection * modelview;

    GLHelperOneColor gldraw;

    for (double num = glm::roundMidAwayFromZero(m_eastingMin / _gridZoom) * _gridZoom; num < m_eastingMax; num += _gridZoom)
    {
        if (num < m_eastingMin) continue;
        gldraw.append(QVector3D(static_cast<float>(num), static_cast<float>(m_northingMax), 0.1f));
        gldraw.append(QVector3D(static_cast<float>(num), static_cast<float>(m_northingMin), 0.1f));
    }

    for (double num2 = glm::roundMidAwayFromZero(m_northingMin / _gridZoom) * _gridZoom; num2 < m_northingMax; num2 += _gridZoom)
    {
        if (num2 < m_northingMin) continue;
        gldraw.append(QVector3D(static_cast<float>(m_eastingMax), static_cast<float>(num2), 0.1f));
        gldraw.append(QVector3D(static_cast<float>(m_eastingMin), static_cast<float>(num2), 0.1f));
    }

    gldraw.draw(gl, mvp, gridColor, GL_LINES, 1.0f);
}

void WorldGrid::checkZoomWorldGrid(double northing, double easting)
{
    double n = glm::roundMidAwayFromZero(northing / (m_gridSize / m_count * 2)) * (m_gridSize / m_count * 2);
    double e = glm::roundMidAwayFromZero(easting / (m_gridSize / m_count * 2)) * (m_gridSize / m_count * 2);

    m_northingMax = n + m_gridSize;
    m_northingMin = n - m_gridSize;
    m_eastingMax = e + m_gridSize;
    m_eastingMin = e - m_gridSize;

    // Check to see if we need to regenerate all the buffers for OpenGL
    invalidateBuffers();
}

void WorldGrid::destroyGLBuffers()
{
    // Assume valid OpenGL context
    delete m_fieldShader;
    m_fieldShader = nullptr;

    m_fieldBuffer.destroy();
}
