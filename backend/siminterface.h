// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
#ifndef SIMINTERFACE_H
#define SIMINTERFACE_H

#include <QObject>
#include <QPropertyBinding>
#include <QQmlEngine>
#include <QMutex>
#include <QTimer>

#include "simpleproperty.h"

class SimInterface : public QObject
{
    Q_OBJECT
    QML_SINGLETON
    QML_ELEMENT

private:
    explicit SimInterface(QObject *parent = nullptr);
    ~SimInterface() override = default;

    // Prevent copying
    SimInterface(const SimInterface &) = delete;
    SimInterface &operator=(const SimInterface &) = delete;

    static SimInterface *s_instance;
    static QMutex s_mutex;
    static bool s_cpp_created;

    QTimer simTimer;

public:
    double altitude = 300;

    double latitude = 0, longitude = 0;
    double headingTrue = 0.0, steerangleAve = 0.0;
    double steerAngleScrollBar = 0;

    bool isAccelForward = false, isAccelBack = false;

    static SimInterface *instance();
    static SimInterface *create(QQmlEngine *qmlEngine, QJSEngine *jsEngine);

    Q_INVOKABLE bool isRunning();

    // Properties will be added here
    SIMPLE_BINDABLE_PROPERTY(double, steerAngle)
    SIMPLE_BINDABLE_PROPERTY(double, steerAngleActual)
    SIMPLE_BINDABLE_PROPERTY(double, stepDistance)

    Q_INVOKABLE void startUp();
    Q_INVOKABLE void shutDown();

    Q_INVOKABLE void speedup();
    Q_INVOKABLE void slowdown();
    Q_INVOKABLE void stop();
    Q_INVOKABLE void forward();
    Q_INVOKABLE void reverse();
    Q_INVOKABLE void reset();

    Q_INVOKABLE void rotate();

public slots:
    void onTimeout();

signals:
    void newPosition(double vtgSpeed,
                     double headingTrue,
                     double latitude,
                     double longitude, double hdop,
                     double altitude,
                     double satellitesTracked);

private:
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(SimInterface, double, m_steerAngle, 0, &SimInterface::steerAngleChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(SimInterface, double, m_steerAngleActual, 0, &SimInterface::steerAngleActualChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(SimInterface, double, m_stepDistance, 0, &SimInterface::stepDistanceChanged)

    void CalculateNewPositionFromBearingDistance(double lat, double lng, double bearing, double distance);

};

#endif // SIMINTERFACE_H
