#ifndef CTRACK_H
#define CTRACK_H

#include <QObject>
#include <QVector>
#include <QAbstractListModel>
#include <QtQml/qqmlregistration.h>
#include <QQmlEngine>
#include <QJSEngine>
#include <QProperty>
#include <QBindable>
#include "vec3.h"
#include "vec2.h"
#include "setter.h"
#include "cabcurve.h"
#include "cabline.h"
#include "tracksproperties.h"

class QOpenGLFunctions;
class CVehicle;
class CABLine;
class CABCurve;
class CBoundary;
class CYouTurn;
class CAHRS;
class CGuidance;
class CNMEA;
class CCamera;
class CTram;
class FormGPS;  // Forward declaration for optional injection

enum TrackMode {
    None = 0,
    AB = 2,
    Curve = 4,
    bndTrackOuter = 8,
    bndTrackInner = 16,
    bndCurve = 32,
    waterPivot = 64
};//, Heading, Circle, Spiral

class CTrk
{
public:
    QVector<Vec3> curvePts;
    double heading;
    QString name;
    bool isVisible;
    Vec2 ptA;
    Vec2 ptB;
    Vec2 endPtA;
    Vec2 endPtB;
    int mode;
    double nudgeDistance;

    CTrk();
    CTrk(const CTrk &orig);
};

class CTrack : public QAbstractListModel
{
    Q_OBJECT
    // QML registration handled manually in main.cpp
       // ===== QML PROPERTIES - Qt 6.8 Unified Rectangle Pattern =====
    Q_PROPERTY(TracksProperties* properties READ properties NOTIFY propertiesChanged)
    Q_PROPERTY(int idx READ idx WRITE setIdx NOTIFY idxChanged BINDABLE bindableIdx)
    Q_PROPERTY(QObject* model READ getModel CONSTANT)
    Q_PROPERTY(int newRefSide READ newRefSide WRITE setNewRefSide NOTIFY newRefSideChanged BINDABLE bindableNewRefSide)
    // Qt 6.8 Unified Rectangle Pattern - All properties with NOTIFY + BINDABLE
    Q_PROPERTY(bool isAutoTrack READ isAutoTrack WRITE setIsAutoTrack NOTIFY isAutoTrackChanged BINDABLE bindableIsAutoTrack)
    Q_PROPERTY(bool isAutoSnapToPivot READ isAutoSnapToPivot WRITE setIsAutoSnapToPivot NOTIFY isAutoSnapToPivotChanged BINDABLE bindableIsAutoSnapToPivot)
    Q_PROPERTY(bool isAutoSnapped READ isAutoSnapped WRITE setIsAutoSnapped NOTIFY isAutoSnappedChanged BINDABLE bindableIsAutoSnapped)
    Q_PROPERTY(int howManyPathsAway READ howManyPathsAway WRITE setHowManyPathsAway NOTIFY howManyPathsAwayChanged BINDABLE bindableHowManyPathsAway)
    Q_PROPERTY(int mode READ mode WRITE setMode NOTIFY modeChanged BINDABLE bindableMode)
    Q_PROPERTY(int newMode READ newMode WRITE setNewMode NOTIFY newModeChanged BINDABLE bindableNewMode)
    Q_PROPERTY(QString newName READ newName WRITE setNewName NOTIFY newNameChanged BINDABLE bindableNewName)
    Q_PROPERTY(double newHeading READ newHeading WRITE setNewHeading NOTIFY newHeadingChanged BINDABLE bindableNewHeading)
    Q_PROPERTY(int count READ count WRITE setCount NOTIFY countChanged BINDABLE bindableCount)
    Q_PROPERTY(QString currentName READ currentName WRITE setCurrentName NOTIFY currentNameChanged BINDABLE bindableCurrentName)


public:
    // Main data members (public for external access)
    QVector<CTrk> gArr;
    CABCurve curve;
    CABLine ABLine;
    CTrk newTrack;
    int autoTrack3SecTimer;

    // Membres et méthodes publiques nécessaires pour accès externe
    bool isLine;
    QVector<Vec2> designRefLine;

    // CTrack interface (publiques pour accès via singleton)
    TracksProperties *properties() const { return m_tracksProperties; }
    void updateInterface();

    int FindClosestRefTrack(Vec3 pivot, const CVehicle &vehicle);
    void SwitchToClosestRefTrack(Vec3 pivot, const CVehicle &vehicle);
    void BuildCurrentLine(Vec3 pivot,
                          double secondsSinceStart, bool isBtnAutoSteerOn,
                          CYouTurn &yt,
                          CVehicle &vehicle,
                          const CBoundary &bnd,
                          const CAHRS &ahrs,
                          CGuidance &gyd,
                          CNMEA &pn);
    void ResetCurveLine();
    void AddPathPoint(Vec3 point);
    void DrawTrackNew(QOpenGLFunctions *gl, const QMatrix4x4 &mvp);
    void DrawTrack(QOpenGLFunctions *gl, const QMatrix4x4 &mvp,
                   bool isFontOn,
                   bool isRateMapOn,
                   double camSetDistance,
                   CYouTurn &yt,
                   const CGuidance &gyd);
    void DrawTrackGoalPoint(QOpenGLFunctions *gl, const QMatrix4x4 &mvp);
    int getHowManyPathsAway();
    int getMode() { if (idx() >=0) return gArr[idx()].mode; else return 0; }

    void reloadModel() {
        //force QML to reload the model to reflect changes
        //that may have been made in C++ code.
        beginResetModel();
        endResetModel();
        emit modelChanged(); //not sure if this is necessary
    }


    enum RoleNames {
        index = Qt::UserRole,
        NameRole,
        IsVisibleRole,
        ModeRole,
        ptA,
        ptB,
        endPtA,
        endPtB,
        nudgeDistance
    };
    
    ~CTrack();

    void NudgeRefABLine(CTrk &track, double dist);
    void NudgeRefCurve(CTrk &track, double distAway);
    void NudgeTrack(double dist);
    void NudgeDistanceReset();
    void SnapToPivot();
    void NudgeRefTrack(double dist);

    // Qt 6.8 property getters and setters
    int idx() const; // Qt 6.8 FIX: Moved to .cpp
    void setIdx(int new_idx);
    QBindable<int> bindableIdx(); // Qt 6.8 FIX: Moved to .cpp

    bool isAutoTrack() const; // Qt 6.8 FIX: Moved to .cpp
    void setIsAutoTrack(bool value); // Qt 6.8 FIX: Moved to .cpp
    QBindable<bool> bindableIsAutoTrack(); // Qt 6.8 FIX: Moved to .cpp

    bool isAutoSnapToPivot() const; // Qt 6.8 FIX: Moved to .cpp
    void setIsAutoSnapToPivot(bool value); // Qt 6.8 FIX: Moved to .cpp
    QBindable<bool> bindableIsAutoSnapToPivot(); // Qt 6.8 FIX: Moved to .cpp

    bool isAutoSnapped() const; // Qt 6.8 FIX: Moved to .cpp
    void setIsAutoSnapped(bool value); // Qt 6.8 FIX: Moved to .cpp
    QBindable<bool> bindableIsAutoSnapped(); // Qt 6.8 FIX: Moved to .cpp

    QString newName(void);
    void setNewName(QString new_name);
    QBindable<QString> bindableNewName(); // Qt 6.8 FIX: Moved to .cpp

    int newMode(void);
    void setNewMode(int mode);
    QBindable<int> bindableNewMode(); // Qt 6.8 FIX: Moved to .cpp

    int newRefSide(void); // Qt 6.8 FIX: Moved to .cpp
    void setNewRefSide(int which_side);
    QBindable<int> bindableNewRefSide(); // Qt 6.8 FIX: Moved to .cpp

    double newHeading(void);
    void setNewHeading(double new_heading);
    QBindable<double> bindableNewHeading(); // Qt 6.8 FIX: Moved to .cpp

    QString getCurrentName(void);

    // Additional property getters/setters/bindables - Qt 6.8 FIX: Moved to .cpp
    int howManyPathsAway() const;
    void setHowManyPathsAway(int value);
    QBindable<int> bindableHowManyPathsAway();

    int mode() const;
    void setMode(int value);
    QBindable<int> bindableMode();

    int count() const;
    void setCount(int value);
    QBindable<int> bindableCount();

    QString currentName() const;
    void setCurrentName(const QString& value);
    QBindable<QString> bindableCurrentName();

    QObject *getModel() { return this;}

    Q_INVOKABLE QString getTrackName(int index);
    Q_INVOKABLE bool getTrackVisible(int index);
    Q_INVOKABLE double getTrackNudge(int index);

    // QML model interface
    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    virtual QVariant data(const QModelIndex &index, int role) const override;

    void update_ab_refline();

protected:
    // QML model interface
    virtual QHash<int, QByteArray> roleNames() const override;


public:
    // Constructor declaration (implementation in .cpp)
    explicit CTrack(QObject* parent = nullptr);
    

signals:
    void propertiesChanged();
    void resetCreatedYouTurn();
    void saveTracks();

    void idxChanged();
    void modeChanged();
    void modelChanged();
    void isAutoSnapToPivotChanged();
    void isAutoSnappedChanged();
    void isAutoTrackChanged();
    void howManyPathsAwayChanged();
    void newModeChanged();
    void newNameChanged();
    void newRefSideChanged();
    void newHeadingChanged();
    void countChanged();
    void currentNameChanged();

public slots:
    //Qt 6.8 FIX: Q_INVOKABLE required for QML access with Q_PROPERTY architecture
    Q_INVOKABLE void select(int index);
    Q_INVOKABLE void next();
    Q_INVOKABLE void prev();

    Q_INVOKABLE void start_new(int mode);
    Q_INVOKABLE void mark_start(double easting, double northing, double heading);
    Q_INVOKABLE void mark_end(int refSide, double easting,
                  double northing);

    Q_INVOKABLE void finish_new(QString name);

    Q_INVOKABLE void cancel_new();
    Q_INVOKABLE void pause(bool pause);
    Q_INVOKABLE void add_point(double easting, double northing, double heading);

    Q_INVOKABLE void delete_track(int index);
    Q_INVOKABLE void changeName(int index, QString new_name);
    Q_INVOKABLE void swapAB(int index);
    Q_INVOKABLE void setVisible(int index, bool isVisible);
    Q_INVOKABLE void copy(int index, QString new_name);

    Q_INVOKABLE void ref_nudge(double dist_m);
    Q_INVOKABLE void nudge_zero();
    Q_INVOKABLE void nudge_center();
    Q_INVOKABLE void nudge(double dist_m);

private:
    // Used by QML model interface
    QHash<int, QByteArray> m_roleNames;
    TracksProperties *m_tracksProperties = nullptr;


    // ===== Qt 6.8 Q_OBJECT_BINDABLE_PROPERTY Private Members =====
    Q_OBJECT_BINDABLE_PROPERTY(CTrack, int, m_idx, &CTrack::idxChanged)
    Q_OBJECT_BINDABLE_PROPERTY(CTrack, bool, m_isAutoTrack, &CTrack::isAutoTrackChanged)
    Q_OBJECT_BINDABLE_PROPERTY(CTrack, bool, m_isAutoSnapToPivot, &CTrack::isAutoSnapToPivotChanged)
    Q_OBJECT_BINDABLE_PROPERTY(CTrack, bool, m_isAutoSnapped, &CTrack::isAutoSnappedChanged)
    Q_OBJECT_BINDABLE_PROPERTY(CTrack, QString, m_newName, &CTrack::newNameChanged)
    Q_OBJECT_BINDABLE_PROPERTY(CTrack, int, m_newMode, &CTrack::newModeChanged)
    Q_OBJECT_BINDABLE_PROPERTY(CTrack, int, m_newRefSide, &CTrack::newRefSideChanged)
    Q_OBJECT_BINDABLE_PROPERTY(CTrack, double, m_newHeading, &CTrack::newHeadingChanged)
    Q_OBJECT_BINDABLE_PROPERTY(CTrack, int, m_howManyPathsAway, &CTrack::howManyPathsAwayChanged)
    Q_OBJECT_BINDABLE_PROPERTY(CTrack, int, m_mode, &CTrack::modeChanged)
    Q_OBJECT_BINDABLE_PROPERTY(CTrack, int, m_count, &CTrack::countChanged)
    Q_OBJECT_BINDABLE_PROPERTY(CTrack, QString, m_currentName, &CTrack::currentNameChanged)

    void initializeModel() {
        // Initialize role names hash
        m_roleNames[index] = "index";
        m_roleNames[NameRole] = "name";
        m_roleNames[IsVisibleRole] = "visible";
        m_roleNames[ModeRole] = "mode";
        m_roleNames[ptA] = "ptA";
        m_roleNames[ptB] = "ptB";
        m_roleNames[endPtA] = "endPtA";
        m_roleNames[endPtB] = "endPtB";
        m_roleNames[nudgeDistance] = "nudgeDistance";
    }

};

#endif // CTRACK_H
