#ifndef RECORDEDPATH_H
#define RECORDEDPATH_H

#include <QObject>
#include <QQmlEngine>
#include <QProperty>
#include <QVector>
#include <QMutex>
#include "recordedpathmodel.h"
#include "simpleproperty.h"
#include "vec3.h"
#include "vec2.h"
#include "btnenum.h"

class QOpenGLFunctions;
class QMatrix4x4;
class CVehicle;
class CYouTurn;
class RecordedPathProperties;

Q_MOC_INCLUDE("recordedpathproperties.h")

class CRecPathPt
{
public:
    double easting;
    double northing;
    double heading;
    double speed;
    bool autoBtnState;

    //constructor
    CRecPathPt(): easting(0), northing(0), heading(0), speed(0), autoBtnState(btnStates::Off)
    {
    }

    CRecPathPt(double _easting, double _northing, double _heading, double _speed,
                        bool _autoBtnState)
    {
        easting = _easting;
        northing = _northing;
        heading = _heading;
        speed = _speed;
        autoBtnState = _autoBtnState;
    }
};

class RecordedPath : public QObject
{
    Q_OBJECT
    QML_SINGLETON
    QML_ELEMENT
private:
    explicit RecordedPath(QObject *parent = nullptr);
    ~RecordedPath() override = default;

    // Prevent copying
    RecordedPath(const RecordedPath &) = delete;
    RecordedPath &operator=(const RecordedPath &) = delete;

    static RecordedPath *s_instance;
    static QMutex s_mutex;
    static bool s_cpp_created;

    int A, B, C;
    int counter2;

public:
    static RecordedPath *instance();
    static RecordedPath *create(QQmlEngine *qmlEngine, QJSEngine *jsEngine);

    // Model-based recorded path list
    Q_PROPERTY(RecordedPathModel* model READ model CONSTANT)
    RecordedPathModel* model() const { return m_model; }

    SIMPLE_BINDABLE_PROPERTY(bool, isDrivingRecordedPath)
    SIMPLE_BINDABLE_PROPERTY(QString, currentPathName)

    //the recorded path from driving around
    QVector<CRecPathPt> recList;

    int recListCount;

    //the dubins path to get there
    QVector<CRecPathPt> shuttleDubinsList;

    int shuttleListCount;

    //list of vec3 points of Dubins shortest path between 2 points - To be converted to RecPt
    QVector<Vec3> shortestDubinsList;

    //generated reference line
    Vec2 refPoint1 = Vec2(1, 1), refPoint2 = Vec2(2, 2);

    double distanceFromRefLine, distanceFromCurrentLinePivot;

    int currentPositonIndex;

    //pure pursuit values
    Vec3 pivotAxlePosRP = Vec3(0, 0, 0);

    Vec3 homePos;
    Vec2 goalPointRP = Vec2(0, 0);
    double steerAngleRP, rEastRP, rNorthRP, ppRadiusRP;
    Vec2 radiusPointRP = Vec2(0,0);

    bool isEndOfTheRecLine, isRecordOn;
    bool isFollowingDubinsToPath, isFollowingRecPath, isFollowingDubinsHome;

    double pivotDistanceError, pivotDistanceErrorLast, pivotDerivative, pivotDerivativeSmoothed;

    //derivative counters

    double inty;
    double steerAngleSmoothed, pivotErrorTotal;
    double distSteerError, lastDistSteerError, derivativeDistError;

    int resumeState;

    int starPathIndx = 0;

    bool trig;
    double north;
    int pathCount = 0;

    RecordedPathProperties *m_recordedPathProperties = nullptr;

    Q_PROPERTY(RecordedPathProperties* properties READ properties CONSTANT)
    RecordedPathProperties *properties() const { return m_recordedPathProperties; }
    void updateInterface();

    bool StartDrivingRecordedPath(CVehicle &vehicle, const CYouTurn &yt);
    void UpdatePosition(const CYouTurn &yt, bool isBtnAutoSteerOn);
    void StopDrivingRecordedPath();
    void GetDubinsPath(CVehicle &vehicle, Vec3 goal, const CYouTurn &yt);
    void PurePursuitRecPath(CVehicle &vehicle, int ptCount);
    void PurePursuitDubins(CVehicle &vehicle, const CYouTurn &yt, bool isAutoSteerButtonOn, int ptCount);

    void DrawRecordedLine(QOpenGLFunctions *gl, const QMatrix4x4 &mvp);
    void DrawDubins(QOpenGLFunctions *gl, const QMatrix4x4 &mvp);

signals:
    // Signals for actions that C++ code can connect to
    void updateLines();
    void open(QString name);
    void remove(QString name);
    void startDriving();
    void stopDriving();
    void clear();

    void setSimStepDistance(double speed);
    void stoppedDriving();

private:
    RecordedPathModel *m_model;

    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(RecordedPath, bool, m_isDrivingRecordedPath, false, &RecordedPath::isDrivingRecordedPathChanged)
    Q_OBJECT_BINDABLE_PROPERTY(RecordedPath, QString, m_currentPathName, &RecordedPath::currentPathNameChanged)

public slots:
};

#endif // RECORDEDPATH_H
