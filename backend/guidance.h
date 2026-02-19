#ifndef GUIDANCE_H
#define GUIDANCE_H

#include <QObject>
#include <QVector>
#include "vec3.h"

class CVehicle;
class CAHRS;
class CYouTurn;
class CABLine;
class CABCurve;

class Guidance
{
    Q_GADGET

public:
    int sA, sB, C, pA, pB;

    double distanceFromCurrentLineSteer, distanceFromCurrentLinePivot;
    double steerAngleGu;
    Q_PROPERTY(double rEastSteer MEMBER rEastSteer)
    Q_PROPERTY(double rNorthSteer MEMBER rNorthSteer)
    Q_PROPERTY(double rEastPivot MEMBER rEastPivot)
    Q_PROPERTY(double rNorthPivot MEMBER rNorthPivot)
    double rEastSteer, rNorthSteer, rEastPivot, rNorthPivot;

    double inty, xTrackSteerCorrection = 0;
    double steerHeadingError, steerHeadingErrorDegrees;

    double distSteerError, lastDistSteerError, derivativeDistError;

    double pivotDistanceError, stanleyModeMultiplier;

    int counter;

    Guidance();

    void DoSteerAngleCalc(bool isBtnAutoSteerOn,
                          CVehicle &vehicle,
                          const CAHRS &ahrs
                         );

    void StanleyGuidanceABLine(Vec3 curPtA, Vec3 curPtB,
                               Vec3 pivot, Vec3 steer,
                               bool isBtnAutoSteerOn,
                               CVehicle &vehicle,
                               CABLine &CABLine,
                               const CAHRS &ahrs,
                               CYouTurn &yt);


    void StanleyGuidanceCurve(Vec3 pivot, Vec3 steer,
                              QVector<Vec3> &curList,
                              bool isBtnAutoSteerOn,
                              CVehicle &vehicle,
                              CABCurve &curve,
                              const CAHRS &ahrs);
};

#endif // GUIDANCE_H
