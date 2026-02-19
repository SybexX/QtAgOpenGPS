#ifndef CBOUNDARY_H
#define CBOUNDARY_H

#include "vec2.h"
#include "vec3.h"
#include <QVector>
#include <QVector3D>
#include <QSharedPointer>
#include <QOpenGLBuffer>
#include "cboundarylist.h"
#include "mainwindowstate.h"
#include <QObject>


class QOpenGLFunctions;
class QMatrix4x4;
class CVehicle;
class CABLine;
class CYouTurn;
class ModuleComm;
class CTool;
class CPGN_EF;

class CBoundary : public QObject
{
    Q_OBJECT
private:
    const double scanWidth = 1.0;
    const double boxLength = 2000;


    bool bufferCurrent = false;
    bool backBufferCurrent = false;

    Vec2 prevBoundaryPos;

    quint64 m_lastFingerprint = 0;
    quint64 calculateFingerprint() const;

public:
    //area of boundaries
    QVector<CBoundaryList> bndList;

    QVector<Vec3> bndBeingMadePts;

    // isBndBeingMade in BoundaryInterface as property
    //bool isBndBeingMade = false;

    bool isOkToAddPoints = false;

    int closestFenceNum;

    //point at the farthest boundary segment from pivotAxle
    Vec3 closestFencePt = Vec3(-10000,-10000, 9);

    // the list of possible bounds points
    QVector<Vec3> turnClosestList;
    int turnSelected, closestTurnNum;
    double iE = 0, iN = 0;

    //point at the farthest turn segment from pivotAxle
    Vec3 closestTurnPt = Vec3(-10000, -10000, 9);
    Vec3 closePt;

    bool isToolInHeadland, isToolOuterPointsInHeadland, isSectionControlledByHeadland;


    CBoundary(QObject *parent = 0);
    void loadSettings();

    //CFence.cs
    bool IsPointInsideFenceArea(Vec3 testPoint) const ;
    bool IsPointInsideFenceArea(Vec2 testPoint) const;
    void DrawFenceLines(Vec3 pivot, QOpenGLFunctions *g, const QMatrix4x4 &mvp);

    //CTurn.sh
    int IsPointInsideTurnArea(Vec3 pt) const;
    void FindClosestTurnPoint(const CABLine &abline, Vec3 fromPt);
    void BuildTurnLines();

    //CHead.cs
    void SetHydPosition(SectionState::State autoBtnState, CPGN_EF &p_239, CVehicle &vehicle); //TODO sounds, p_239
    bool IsPointInsideHeadArea(Vec2 pt) const;

    /*
    void findClosestBoundaryPoint(Vec2 fromPt, double headAB);
    void resetBoundaries();
    */
    //void drawClosestPoint(QOpenGLFunctions *g, const QMatrix4x4 &mvp);
    //void drawBoundaryLineOnBackBuffer(QOpenGLFunctions *gl, const QMatrix4x4 &mvp);

    void AddCurrentPoint(double min_dist);
    void UpdateFieldBoundaryGUIAreas();
    bool CalculateMinMax();
    bool loadBoundary(const QString &field_path);
    static double getSavedFieldArea(const QString &boundarytxt_path);

    void updateInterface();


public slots:
    // methods to be used by GUI.
    void calculateArea();
    void updateList();
    void start();
    void stop();
    void addPoint();
    void deleteLastPoint();
    void pause();
    void record();
    void reset();
    void deleteBoundary(int which_boundary);
    void setDriveThrough(int which_boundary, bool drive_thru);
    void deleteAll();

    void loadBoundaryFromKML(QString filename);
    void addBoundaryOSMPoint(double latitude, double longitude);


signals:
    void TimedMessage(int timeout, QString title, QString message);
    void soundHydLiftChange(bool);

    void saveBoundaryRequested();

};

#endif // CBOUNDARY_H
