#ifndef CVEHICLE_H
#define CVEHICLE_H

#include "csection.h"
#include "vec2.h"
#include "vec3.h"
#include "common.h"
#include <QObject>
#include <QMatrix4x4>
#include <QtQml/qqmlregistration.h>
#include <QQmlEngine>
#include <QJSEngine>
#include <QProperty>
#include <QBindable>
#include <QMutex>
#include "simpleproperty.h"

#include <QOpenGLBuffer>

class QOpenGLFunctions;
class VehicleProperties;
class CYouTurn;
class CTool;
class CBoundary;
class CNMEA;
class CCamera;
class CABCurve;
class CABLine;
class CContour;
class CTrack;

Q_MOC_INCLUDE("vehicleproperties.h")

class CVehicle: public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(VehicleInterface)
    QML_SINGLETON

    // ===== Scene Graph Properties =====
    Q_PROPERTY(VehicleProperties* vehicleProperties READ vehicleProperties CONSTANT)

    // ===== QML PROPERTIES - Qt 6.8 QProperty + BINDABLE + NOTIFY =====
    Q_PROPERTY(bool isHydLiftOn READ isHydLiftOn WRITE setIsHydLiftOn NOTIFY isHydLiftOnChanged BINDABLE bindableIsHydLiftOn)
    Q_PROPERTY(bool hydLiftDown READ hydLiftDown WRITE setHydLiftDown NOTIFY hydLiftDownChanged BINDABLE bindableHydLiftDown)
    Q_PROPERTY(bool isChangingDirection READ isChangingDirection WRITE setIsChangingDirection NOTIFY isChangingDirectionChanged BINDABLE bindableIsChangingDirection)
    Q_PROPERTY(int leftTramState READ leftTramState WRITE setLeftTramState NOTIFY leftTramStateChanged BINDABLE bindableLeftTramState)
    Q_PROPERTY(int rightTramState READ rightTramState WRITE setRightTramState NOTIFY rightTramStateChanged BINDABLE bindableRightTramState)
    Q_PROPERTY(bool isReverse READ isReverse WRITE setIsReverse NOTIFY isReverseChanged BINDABLE bindableIsReverse)
    Q_PROPERTY(QList<QVariant> vehicleList READ vehicleList WRITE setVehicleList NOTIFY vehicleListChanged BINDABLE bindableVehicleList)
    Q_PROPERTY(bool isInFreeDriveMode READ isInFreeDriveMode WRITE setIsInFreeDriveMode NOTIFY isInFreeDriveModeChanged BINDABLE bindableIsInFreeDriveMode)
    Q_PROPERTY(double driveFreeSteerAngle READ driveFreeSteerAngle WRITE setDriveFreeSteerAngle NOTIFY driveFreeSteerAngleChanged BINDABLE bindableDriveFreeSteerAngle)

public:
    // C++ singleton access (strict singleton pattern - same as CTrack)
    static CVehicle *instance();
    static CVehicle *create (QQmlEngine *qmlEngine, QJSEngine *jsEngine);

    // Scene graph properties accessor
    VehicleProperties* vehicleProperties() const { return m_vehicleProperties; }

    bool isSteerAxleAhead;
    bool isPivotBehindAntenna;

    // Phase 6.0.24 Problem 18: Initialize geometry variables with safe defaults
    // Critical: If SettingsManager not ready at startup, DrawVehicle uses these values
    // Safe defaults prevent division by zero in OpenGL projection calculations
    double antennaHeight = 1.0;
    double antennaPivot = 2.0;
    double wheelbase = 3.0;
    double antennaOffset = 0.0;
    double panicStopSpeed = 0.0;
    int vehicleType = 0;

    //min vehicle speed allowed before turning shit off
    double slowSpeedCutoff = 0;

    //autosteer values
    // Phase 6.0.24 Problem 18: Initialize autosteer parameters with safe defaults
    double goalPointLookAhead = 1.0;
    double goalPointLookAheadHold = 1.0;
    double goalPointLookAheadMult = 1.0;
    double uturnCompensation = 0.0;

    double stanleyDistanceErrorGain = 1.0;
    double stanleyHeadingErrorGain = 1.0;
    double minLookAheadDistance = 2.0;
    double maxSteerAngle = 45.0;
    double maxSteerSpeed = 20.0;
    double minSteerSpeed = 1.0;
    double maxAngularVelocity = 1.0;
    double trackWidth = 2.0;  // Phase 6.0.24 Problem 18: Safe default for OpenGL rendering

    double stanleyIntegralDistanceAwayTriggerAB = 0.0;
    double stanleyIntegralGainAB = 0.0;
    double purePursuitIntegralGain = 0.0;

    double modeXTE = 0.0;
    int modeTime = 0;

    double functionSpeedLimit = 20.0;  // Phase 6.0.24 Problem 18: Safe default speed limit


    //from pn or main form:
    int ringCounter = 0;


    //tram indicator vars

    //storage for the cos and sin of heading
    double cosSectionHeading = 1.0, sinSectionHeading = 0.0;
    Vec3 pivotAxlePos;
    Vec3 steerAxlePos;
    Vec3 hitchPos;
    Vec2 guidanceLookPos;

    //Current fix positions
    double fixEasting = 0.0;
    double fixNorthing = 3.0;

    // autosteer variables for sending serial
    short int guidanceLineSteerAngle = 0;
    short int distanceDisplay = 0;

    //how far travelled since last section was added, section points
    double sectionTriggerDistance = 0, sectionTriggerStepDistance = 0;
    Vec2 prevSectionPos;

    CSection sections[MAXSECTIONS];

    //tally counters for display
    double totalSquareMeters = 0, totalUserSquareMeters = 0, userSquareMetersAlarm = 0;

    double distancePivotToTurnLine = 0.0;  // Phase 6.0.24 Problem 18: Initialize for U-turn calculations

    int modeTimeCounter = 0;
    double goalDistance = 0;

    //from Position.Designer.cs

    void loadSettings();
    void saveSettings();

    double UpdateGoalPointDistance();
    void DrawVehicle(QOpenGLFunctions *gl, QMatrix4x4 modelview, QMatrix4x4 projection,
                     double steerAngle,
                     double markLeft,
                     double markRight, double camSetDistance,
                     QRect viewport);

    //C++ code should use Qt 6.8 QProperty setters above
    //QML bindings work automatically with BINDABLE functions
    // Legacy setters replaced by Qt 6.8 QProperty pattern
    SIMPLE_BINDABLE_PROPERTY (double,modeActualXTE)
    SIMPLE_BINDABLE_PROPERTY (double,modeActualHeadingError)
    SIMPLE_BINDABLE_PROPERTY (QPoint,screenCoord)
    SIMPLE_BINDABLE_PROPERTY (QRect,screenBounding)
    SIMPLE_BINDABLE_PROPERTY (short int, guidanceLineDistanceOff)
    SIMPLE_BINDABLE_PROPERTY (double, avgPivDistance)
    SIMPLE_BINDABLE_PROPERTY (double, avgSpeed)
    SIMPLE_BINDABLE_PROPERTY (double, fixHeading)

signals:
    //void setLookAheadGoal(double);
    // Qt 6.8 QProperty NOTIFY signals for Q_OBJECT_BINDABLE_PROPERTY migration
    void isHydLiftOnChanged();
    void hydLiftDownChanged();
    void isChangingDirectionChanged();
    void isReverseChanged();
    void leftTramStateChanged();
    void rightTramStateChanged();
    void vehicleListChanged();
    void isInFreeDriveModeChanged();
    void driveFreeSteerAngleChanged();

    // Thread-safe vehicle management signals (Phase 1 architecture)
    void vehicle_saveas(QString vehicle_name);
    void vehicle_load(QString vehicle_name);
    void vehicle_delete(QString vehicle_name);
    void vehicle_update_list();

public slots:
    //Q_INVOKABLE wrappers for QML that emit thread-safe signals
    Q_INVOKABLE void requestVehicleSaveas(QString vehicle_name) { emit vehicle_saveas(vehicle_name); }
    Q_INVOKABLE void requestVehicleLoad(QString vehicle_name) { emit vehicle_load(vehicle_name); }
    Q_INVOKABLE void requestVehicleDelete(QString vehicle_name) { emit vehicle_delete(vehicle_name); }
    Q_INVOKABLE void requestVehicleUpdateList() { emit vehicle_update_list(); }

public:
    // ===== Qt 6.8 QProperty READ/WRITE/BINDABLE Methods =====
    // Hydraulic Lift Properties
    bool isHydLiftOn() const; // Qt 6.8 FIX: Moved to .cpp
    void setIsHydLiftOn(bool value); // Qt 6.8 FIX: Moved to .cpp
    QBindable<bool> bindableIsHydLiftOn(); // Qt 6.8 FIX: Moved to .cpp

    bool hydLiftDown() const; // Qt 6.8 FIX: Moved to .cpp
    void setHydLiftDown(bool value); // Qt 6.8 FIX: Moved to .cpp
    QBindable<bool> bindableHydLiftDown(); // Qt 6.8 FIX: Moved to .cpp

    // Direction Properties
    bool isChangingDirection() const; // Qt 6.8 FIX: Moved to .cpp
    void setIsChangingDirection(bool value); // Qt 6.8 FIX: Moved to .cpp
    QBindable<bool> bindableIsChangingDirection(); // Qt 6.8 FIX: Moved to .cpp

    bool isReverse() const; // Qt 6.8 FIX: Moved to .cpp
    void setIsReverse(bool value); // Qt 6.8 FIX: Moved to .cpp
    QBindable<bool> bindableIsReverse(); // Qt 6.8 FIX: Moved to .cpp

    // Tram State Properties
    int leftTramState() const; // Qt 6.8 FIX: Moved to .cpp
    void setLeftTramState(int value); // Qt 6.8 FIX: Moved to .cpp
    QBindable<int> bindableLeftTramState(); // Qt 6.8 FIX: Moved to .cpp

    int rightTramState() const; // Qt 6.8 FIX: Moved to .cpp
    void setRightTramState(int value); // Qt 6.8 FIX: Moved to .cpp
    QBindable<int> bindableRightTramState(); // Qt 6.8 FIX: Moved to .cpp

    // Vehicle List Property
    QList<QVariant> vehicleList() const; // Qt 6.8 FIX: Moved to .cpp
    void setVehicleList(const QList<QVariant>& value); // Qt 6.8 FIX: Moved to .cpp
    QBindable<QList<QVariant>> bindableVehicleList(); // Qt 6.8 FIX: Moved to .cpp

    bool isInFreeDriveMode() const;
    void setIsInFreeDriveMode(bool new_mode);
    QBindable<bool> bindableIsInFreeDriveMode();

    double driveFreeSteerAngle() const;
    void setDriveFreeSteerAngle(double new_angle);
    QBindable<double> bindableDriveFreeSteerAngle();

    // Legacy compatibility methods
    void setLeftTramIndicator(int value) { setLeftTramState(value); }
    void setRightTramIndicator(int value) { setRightTramState(value); }

public slots:
    void AverageTheSpeed(double newSpeed);
    //void settingsChanged(); //notify us that settings changed so buffers need to be redone.

private:
    // Private constructor for strict singleton pattern
    explicit CVehicle(QObject* parent = nullptr);

    ~CVehicle() override=default;

    static CVehicle *s_instance;
    static QMutex s_mutex;
    static bool s_cpp_created;

    // Scene graph properties
    VehicleProperties *m_vehicleProperties = nullptr;

    // Qt 6.8 MIGRATION: Lazy initialization flag
    mutable bool m_settingsLoaded = false;
    void ensureSettingsLoaded() const {
        if (!m_settingsLoaded) {
            const_cast<CVehicle*>(this)->loadSettings();
            m_settingsLoaded = true;
        }
    }

    // ===== Qt 6.8 Q_OBJECT_BINDABLE_PROPERTY Private Members =====
    Q_OBJECT_BINDABLE_PROPERTY(CVehicle, bool, m_isHydLiftOn, &CVehicle::isHydLiftOnChanged)
    Q_OBJECT_BINDABLE_PROPERTY(CVehicle, bool, m_hydLiftDown, &CVehicle::hydLiftDownChanged)
    Q_OBJECT_BINDABLE_PROPERTY(CVehicle, bool, m_isChangingDirection, &CVehicle::isChangingDirectionChanged)
    Q_OBJECT_BINDABLE_PROPERTY(CVehicle, bool, m_isReverse, &CVehicle::isReverseChanged)
    Q_OBJECT_BINDABLE_PROPERTY(CVehicle, int, m_leftTramState, &CVehicle::leftTramStateChanged)
    Q_OBJECT_BINDABLE_PROPERTY(CVehicle, int, m_rightTramState, &CVehicle::rightTramStateChanged)
    Q_OBJECT_BINDABLE_PROPERTY(CVehicle, QList<QVariant>, m_vehicleList, &CVehicle::vehicleListChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(CVehicle, bool, m_isInFreeDriveMode, false, &CVehicle::isInFreeDriveModeChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(CVehicle, double, m_driveFreeSteerAngle, 0, &CVehicle::driveFreeSteerAngleChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(CVehicle, double, m_modeActualXTE, 0, &CVehicle::modeActualXTEChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(CVehicle, double, m_modeActualHeadingError, 0, &CVehicle::modeActualHeadingErrorChanged)
    Q_OBJECT_BINDABLE_PROPERTY(CVehicle, QPoint, m_screenCoord, &CVehicle::screenCoordChanged)
    Q_OBJECT_BINDABLE_PROPERTY(CVehicle, QRect, m_screenBounding, &CVehicle::screenBoundingChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(CVehicle, short int, m_guidanceLineDistanceOff, 0, &CVehicle::guidanceLineDistanceOffChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(CVehicle, double, m_avgPivDistance, 32000, &CVehicle::avgPivDistanceChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(CVehicle, double, m_avgSpeed, 0, &CVehicle::avgSpeedChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(CVehicle, double, m_fixHeading, 0, &CVehicle::fixHeadingChanged)

};

#endif // CVEHICLE_H
