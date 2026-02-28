#ifndef CABCURVE_H
#define CABCURVE_H

#include <QObject>
#include <QVector>
#include <QFuture>
#include <QFutureWatcher>
#include "vec2.h"
#include "vec3.h"

class QOpenGLFunctions;
class QMatrix4x4;
class CVehicle;
class CYouTurn;
class CTram;
class CCamera;
class CBoundary;
class CNMEA;
class CAHRS;
class CTrk;

class CCurveLines
{
public:
    QVector<Vec3> curvePts;
    double aveHeading = 3;
    QString Name = "aa";
    bool isVisible = true;
};



class CABCurve : public QObject
{
    Q_OBJECT
private:
    int A, B, C;
    int rA, rB;

    int counter2;

public:
    //flag for starting stop adding points
    bool isBtnTrackOn, isMakingCurve;

    double distanceFromCurrentLinePivot;
    double distanceFromRefLine;

    bool isHeadingSameWay = true;
    bool lastIsHeadingSameWay = true;      // Phase 6.0.43: Track previous direction for conditional reconstruction

    int howManyPathsAway = 0;              // Phase 6.0.43 BUG FIX: Changed from double to int for type consistency
    int lastHowManyPathsAway = 98888;      // Phase 6.0.43: Track previous parallel line for conditional reconstruction

    Vec2 refPoint1 = Vec2(1, 1), refPoint2 = Vec2(2, 2);

    Vec2 boxC = Vec2(1, 1), boxD = Vec2(2, 3);
    int currentLocationIndex;

    //pure pursuit values
    Vec2 goalPointCu = Vec2(0, 0);

    Vec2 radiusPointCu = Vec2(0, 0);
    double steerAngleCu, rEastCu, rNorthCu, ppRadiusCu, manualUturnHeading;

    bool isSmoothWindowOpen, isLooping;
    QVector<Vec3> smooList;

    //the list of points of curve to drive on
    QVector<Vec3> curList;

    QFutureWatcher<QVector<Vec3>> m_buildWatcher;
    QFuture<QVector<Vec3>> m_buildFuture;
    bool m_findGlobalNearestCurvePoint = true;
    int m_lastClosestIndex = 0;

    //side guidelines
    QVector<QVector<Vec3>> guideArr;
    QFutureWatcher<QVector<QVector<Vec3>>> m_guideWatcher;
    QFuture<QVector<QVector<Vec3>>> m_guideFuture;

    //the current curve reference line.
    //CTrk refCurve;

    bool isCurveValid, isLateralTriggered;

    double lastSecond = 0;

    QVector<Vec3> desList;
    QString desName = "**";

    double pivotDistanceError, pivotDistanceErrorLast, pivotDerivative, pivotDerivativeSmoothed, lastCurveDistance = 10000;

    double inty;

    explicit CABCurve(QObject *parent = 0);

    void BuildCurveCurrentList(Vec3 pivot,
                               double secondsSinceStart,
                               const CVehicle &vehicle,
                               const CTrk &track,
                               const CBoundary &bnd,
                               const CYouTurn &yt);

    static void BuildNewOffsetList(QPromise<QVector<Vec3>> &promise,
                                   double distAway, CTrk track,
                                   QVector<Vec2> fenceLineEar);

    void GetCurrentCurveLine(Vec3 pivot,
                             Vec3 steer,
                             bool isBtnAutoSteerOn,
                             CVehicle &vehicle,
                             CTrk &track,
                             CYouTurn &yt,
                             const CAHRS &ahrs,
                             CNMEA &pn);


    void DrawCurveNew(QOpenGLFunctions *gl, const QMatrix4x4 &mvp);

    void DrawCurve(QOpenGLFunctions *gl, const QMatrix4x4 &mvp,
                   bool isFontOn, double camSetDistance,
                   const CTrk &track,
                   CYouTurn &yt);

    //void drawTram(QOpenGLFunctions *gl, const QMatrix4x4 &mvp);

    void BuildTram(CBoundary &bnd, CTram &tram, const CTrk &track);
    void SmoothAB(int smPts, const CTrk &track);
    void SmoothABDesList(int smPts);
    void CalculateHeadings(QVector<Vec3> &xList);
    void MakePointMinimumSpacing(QVector<Vec3> &xList, double minDistance);
    void SaveSmoothList(CTrk &track);
    void MoveABCurve(double dist);
    bool PointOnLine(Vec3 pt1, Vec3 pt2, Vec3 pt);
    void AddFirstLastPoints(QVector<Vec3> &xList);

    static void BuildCurveGuidelines(QPromise<QVector<QVector<Vec3>>> &promise,
                                      double distAway, int numPasses,
                                      bool isHeadingSameWay, double toolOffset,
                                      CTrk track, QVector<Vec2> fenceLineEar);
    static void AddGuidelineExtensions(QVector<Vec3> &guideLine);

    static QVector<Vec3> ResampleCurveToUniformSpacing(
        const QVector<Vec3> &originalList, double targetSpacing);
    int findNearestGlobalCurvePoint(const Vec3 &refPoint, int increment = 1);
    int findNearestLocalCurvePoint(const Vec3 &refPoint, int startIndex,
        double minSearchDistance, bool reverseSearchDirection);

    CABCurve &operator=(CABCurve &src)
    {
        A=src.A;
        B=src.B;
        C=src.C;
        rA=src.rA;
        rB=src.rB;
        counter2 = src.counter2;
        isBtnTrackOn = src.isBtnTrackOn;
        isMakingCurve = src.isMakingCurve;

        distanceFromCurrentLinePivot = src.distanceFromCurrentLinePivot;
        distanceFromRefLine = src.distanceFromRefLine;

        isHeadingSameWay = src.isHeadingSameWay;
        lastIsHeadingSameWay = src.lastIsHeadingSameWay;  // Phase 6.0.43

        howManyPathsAway = src.howManyPathsAway;
        lastHowManyPathsAway = src.lastHowManyPathsAway;  // Phase 6.0.43

        refPoint1 = src.refPoint1;
        refPoint2 = src.refPoint2;

        boxC = src.boxC;
        boxD = src.boxD;
        currentLocationIndex = src.currentLocationIndex;

        goalPointCu = src.goalPointCu;

        radiusPointCu = src.radiusPointCu;
        steerAngleCu = src.steerAngleCu;
        rEastCu = src.rEastCu;
        rNorthCu = src.rNorthCu;
        ppRadiusCu = src.ppRadiusCu;
        manualUturnHeading = src.manualUturnHeading;

        isSmoothWindowOpen = src.isSmoothWindowOpen;
        isLooping = src.isLooping;

        smooList = src.smooList;
        curList = src.curList;

        m_findGlobalNearestCurvePoint = src.m_findGlobalNearestCurvePoint;
        m_lastClosestIndex = src.m_lastClosestIndex;

        isCurveValid = src.isCurveValid;
        isLateralTriggered = src.isLateralTriggered;

        lastSecond = src.lastSecond;

        desList = src.desList;
        desName = src.desName;

        pivotDistanceError = src.pivotDistanceError;
        pivotDistanceErrorLast = src.pivotDistanceErrorLast;
        pivotDerivative = src.pivotDerivative;
        pivotDerivativeSmoothed = src.pivotDerivativeSmoothed;
        lastCurveDistance = src.lastCurveDistance;

        inty = src.inty;

        return *this;
    }

signals:

public slots:
    void onBuildFinished();
    void onGuideFinished();
};

#endif // CABCURVE_H
