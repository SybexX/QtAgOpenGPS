#include "cabcurve.h"
#include "glutils.h"
#include "glm.h"
#include <QOpenGLFunctions>
#include <QMatrix4x4>
#include <memory> // C++17 smart pointers
#include <QVector>
#include <QFuture>
#include <QtConcurrent/QtConcurrent>
#include "vec3.h"
#include "vec2.h"
#include "cvehicle.h"
#include "cyouturn.h"
#include "cboundary.h"
#include "ctram.h"
#include "cnmea.h"
#include "cahrs.h"
#include "ctrack.h"
#include "backend/backend.h"
#include "backend/modulecomm.h"
#include "mainwindowstate.h"
#include "classes/settingsmanager.h"
#include "backend/camera.h"

CABCurve::CABCurve(QObject *parent) : QObject(parent)
{
    connect(&m_buildWatcher, &QFutureWatcher<QVector<Vec3>>::finished,
            this, &CABCurve::onBuildFinished);
    connect(&m_guideWatcher, &QFutureWatcher<QVector<QVector<Vec3>>>::finished,
            this, &CABCurve::onGuideFinished);
}

void CABCurve::onBuildFinished()
{
    if (m_buildWatcher.isCanceled())
        return;
    curList = m_buildWatcher.result();
    m_findGlobalNearestCurvePoint = true;
}

void CABCurve::waitForBuildFinished()
{
    if (m_buildWatcher.isRunning())
    {
        m_buildWatcher.waitForFinished();
    }
}

QVector<Vec3> CABCurve::BuildOffsetListSync(double distAway, CTrk track,
                                           QVector<Vec2> fenceLineEar)
{
    QVector<Vec3> result;

    try
    {
        if (track.mode == (int)TrackMode::AB)
        {
            Vec2 nudgePtA = track.ptA;
            Vec2 nudgePtB = track.ptB;

            Vec2 point1 = Vec2((cos(-track.heading) * distAway) + nudgePtA.easting,
                               (sin(-track.heading) * distAway) + nudgePtA.northing);

            Vec2 point2 = Vec2((cos(-track.heading) * distAway) + nudgePtB.easting,
                               (sin(-track.heading) * distAway) + nudgePtB.northing);

            double abLength = SettingsManager::instance()->ab_lineLength();

            double easting1 = point1.easting - (sin(track.heading) * abLength);
            double northing1 = point1.northing - (cos(track.heading) * abLength);

            result.append(Vec3(easting1, northing1, track.heading));

            double easting2 = point2.easting + (sin(track.heading) * abLength);
            double northing2 = point2.northing + (cos(track.heading) * abLength);
            result.append(Vec3(easting2, northing2, track.heading));
        }
        else if (track.mode == (int)TrackMode::waterPivot)
        {
            double Angle = glm::twoPI / qMin(qMax(ceil(glm::twoPI / (2 * acos(1 - (0.02 / fabs(distAway))))), 50.0), 500.0);

            Vec3 centerPos = Vec3(track.ptA.easting, track.ptA.northing, 0);
            double rotation = 0;

            while (rotation < glm::twoPI)
            {
                rotation += Angle;
                result.append(Vec3(centerPos.easting + distAway * sin(rotation), centerPos.northing + distAway * cos(rotation), 0));
            }

            if (result.count() > 1)
            {
                int cnt = result.count();
                for (int i = 0; i < (cnt - 1); i++)
                {
                    result[i].heading = atan2(result[i + 1].easting - result[i].easting, result[i + 1].northing - result[i].northing);
                    if (result[i].heading < 0) result[i].heading += glm::twoPI;
                    if (result[i].heading >= glm::twoPI) result[i].heading -= glm::twoPI;
                }

                result[cnt - 1].heading = atan2(result[0].easting - result[cnt - 1].easting, result[0].northing - result[cnt - 1].northing);
            }
        }
        else
        {
            double tool_width = SettingsManager::instance()->vehicle_toolWidth();
            double tool_overlap = SettingsManager::instance()->vehicle_toolOverlap();
            double step = (tool_width - tool_overlap) * 0.48;
            if (step > 4) step = 4;
            if (step < 1) step = 1;

            double distSqAway = (distAway * distAway) - 0.01;

            Vec3 point;
            int refCount = track.curvePts.count();
            for (int i = 0; i < refCount; i++)
            {
                if (i < 0 || i >= track.curvePts.count()) continue;
                point = Vec3(
                    track.curvePts[i].easting + (sin(glm::PIBy2 + track.curvePts[i].heading) * distAway),
                    track.curvePts[i].northing + (cos(glm::PIBy2 + track.curvePts[i].heading) * distAway),
                    track.curvePts[i].heading);
                bool Add = true;

                for (int t = 0; t < refCount && t < track.curvePts.count(); t++)
                {
                    if (t < 0 || t >= track.curvePts.count()) continue;
                    double dist = ((point.easting - track.curvePts[t].easting) * (point.easting - track.curvePts[t].easting))
                        + ((point.northing - track.curvePts[t].northing) * (point.northing - track.curvePts[t].northing));
                    if (dist < distSqAway)
                    {
                        Add = false;
                        break;
                    }
                }

                if (Add)
                {
                    if (result.count() > 0)
                    {
                        double dist = ((point.easting - result[result.count() - 1].easting) * (point.easting - result[result.count() - 1].easting))
                            + ((point.northing - result[result.count() - 1].northing) * (point.northing - result[result.count() - 1].northing));
                        if (dist > step)
                            result.append(point);
                    }
                    else
                    {
                        result.append(point);
                    }
                }
            }

            if (result.count() > 1)
            {
                int cnt = result.count();
                QVector<Vec3> arr(cnt);
                for (int i = 0; i < cnt; i++) arr[i] = result[i];
                result.clear();

                for (int i = 0; i < cnt - 1; i++)
                {
                    result.append(arr[i]);

                    double dist = glm::Distance(arr[i], arr[i + 1]);
                    if (dist > 2.5)
                    {
                        int loopTimes = (int)(dist / step);
                        if (loopTimes < 1) loopTimes = 1;

                        for (int j = 1; j < loopTimes; j++)
                        {
                            Vec3 pos = Vec3(glm::Catmull(j / (double)(loopTimes), arr[i], arr[i + 1], arr[i + 2], arr[i + 3]));
                            result.append(pos);
                        }
                    }
                }

                result.append(arr[cnt - 2]);
                result.append(arr[cnt - 1]);

                cnt = result.count();
                QVector<Vec3> arr2(cnt);
                for (int i = 0; i < cnt; i++) arr2[i] = result[i];
                cnt--;
                result.clear();
                result.append(Vec3(arr2[0]));

                for (int i = 1; i < cnt; i++)
                {
                    Vec3 pt3 = Vec3(arr2[i]);
                    if (i + 1 < arr2.count())
                    {
                        pt3.heading = atan2(arr2[i + 1].easting - arr2[i - 1].easting, arr2[i + 1].northing - arr2[i - 1].northing);
                    }
                    if (pt3.heading < 0) pt3.heading += glm::twoPI;
                    result.append(pt3);
                }

                int k = arr2.count() - 1;
                Vec3 pt33 = Vec3(arr2[k]);
                if (k - 1 >= 0)
                {
                    pt33.heading = atan2(arr2[k].easting - arr2[k - 1].easting, arr2[k].northing - arr2[k - 1].northing);
                }
                if (pt33.heading < 0) pt33.heading += glm::twoPI;
                result.append(pt33);

                if (fenceLineEar.count() > 0)
                {
                    int numAPts = 1;
                    Vec2 newPt1, newPt2, lastPt = Vec2(result[0].easting, result[0].northing);
                    for (int i = 1; i < result.count(); i++)
                    {
                        Vec2 pt = Vec2(result[i].easting, result[i].northing);
                        double dist = glm::Distance(lastPt, pt);
                        if (dist > 5)
                        {
                            numAPts++;
                            lastPt = pt;
                        }
                    }

                    QVector<Vec2> arr2D(numAPts);
                    numAPts = 1;
                    arr2D[0] = Vec2(result[0].easting, result[0].northing);
                    lastPt = arr2D[0];
                    for (int i = 1; i < result.count(); i++)
                    {
                        Vec2 pt = Vec2(result[i].easting, result[i].northing);
                        double dist = glm::Distance(lastPt, pt);
                        if (dist > 5)
                        {
                            arr2D[numAPts] = pt;
                            numAPts++;
                            lastPt = pt;
                        }
                    }

                    for (int i = 0; i < numAPts; i++)
                    {
                        Vec2 pointE = Vec2(arr2D[i].easting, arr2D[i].northing);
                        for (int j = 0; j < fenceLineEar.count(); j += 2)
                        {
                            double intersectionX, intersectionZ;
                            int res = glm::GetLineIntersection(fenceLineEar[j].easting, fenceLineEar[j].northing,
                                                         fenceLineEar[j + 1].easting, fenceLineEar[j + 1].northing,
                                                         pointE.easting - 1, pointE.northing,
                                                         pointE.easting + 1, pointE.northing,
                                                         intersectionX, intersectionZ);
                            if (res == 1)
                            {
                                pointE.easting = intersectionX;
                                pointE.northing = intersectionZ;
                                break;
                            }
                        }
                        result[i].easting = pointE.easting;
                        result[i].northing = pointE.northing;
                    }
                }
            }
        }
    }
    catch (...)
    {
        result.clear();
    }

    return result;
}

void CABCurve::onGuideFinished()
{
    if (m_guideWatcher.isCanceled())
        return;
    guideArr = m_guideWatcher.result();
}

void CABCurve::BuildCurveCurrentList(Vec3 pivot,
                                     double secondsSinceStart,
                                     const CVehicle &vehicle,
                                     const CTrk &track,
                                     const CBoundary &bnd,
                                     const CYouTurn &yt)
{
    double minDistA = 1000000, minDistB;

    double tool_width = SettingsManager::instance()->vehicle_toolWidth();
    double tool_overlap = SettingsManager::instance()->vehicle_toolOverlap();
    double tool_offset = SettingsManager::instance()->vehicle_toolOffset();

    //move the ABLine over based on the overlap amount set in vehicle
    double widthMinusOverlap = tool_width - tool_overlap;

    // CONDITION 1: Time-gated recalculation of nearest point and howManyPathsAway
    if (!isCurveValid || ((secondsSinceStart - lastSecond) > 0.66
        && (!MainWindowState::instance()->isBtnAutoSteerOn() || ModuleComm::instance()->steerSwitchHigh())))
    {
        lastSecond = secondsSinceStart;

        if (track.mode != (int)TrackMode::waterPivot)
        {
            int refCount = track.curvePts.count();
            if (refCount < 2)
            {
                curList.clear();
                return;
            }

            //close call hit
            int cc = 0, dd;

            for (int j = 0; j < refCount && j < track.curvePts.count(); j += 10)
            {
                double dist = ((CVehicle::instance()->guidanceLookPos.easting - track.curvePts[j].easting)
                               * (CVehicle::instance()->guidanceLookPos.easting - track.curvePts[j].easting))
                              + ((CVehicle::instance()->guidanceLookPos.northing - track.curvePts[j].northing)
                                 * (CVehicle::instance()->guidanceLookPos.northing - track.curvePts[j].northing));
                if (dist < minDistA)
                {
                    minDistA = dist;
                    cc = j;
                }
            }

            minDistA = minDistB = 1000000;

            dd = cc + 7;
            if (dd > track.curvePts.count() - 1) dd = track.curvePts.count();
            cc -= 7;
            if (cc < 0) cc = 0;

            //find the closest 2 points to current close call
            for (int j = cc; j < dd; j++)
            {
                if (j < 0 || j >= track.curvePts.count()) continue;
                double dist = ((CVehicle::instance()->guidanceLookPos.easting - track.curvePts[j].easting)
                               * (CVehicle::instance()->guidanceLookPos.easting - track.curvePts[j].easting))
                              + ((CVehicle::instance()->guidanceLookPos.northing - track.curvePts[j].northing)
                                 * (CVehicle::instance()->guidanceLookPos.northing - track.curvePts[j].northing));
                if (dist < minDistA)
                {
                    minDistB = minDistA;
                    rB = rA;
                    minDistA = dist;
                    rA = j;
                }
                else if (dist < minDistB)
                {
                    minDistB = dist;
                    rB = j;
                }
            }

            // Guard against rA/rB being out of bounds due to async track changes
            if (rA >= track.curvePts.count() || rB >= track.curvePts.count())
            {
                curList.clear();
                return;
            }

            if (rA > rB) {
                C = rA;
                rA = rB;
                rB = C;
            }

            //same way as line creation or not
            isHeadingSameWay = M_PI - fabs(fabs(pivot.heading - track.curvePts[rA].heading) - M_PI) < glm::PIBy2;

            //which side of the closest point are we on is next
            //calculate endpoints of reference line based on closest point
            refPoint1.easting = track.curvePts[rA].easting - (sin(track.curvePts[rA].heading) * 300.0);
            refPoint1.northing = track.curvePts[rA].northing - (cos(track.curvePts[rA].heading) * 300.0);

            refPoint2.easting = track.curvePts[rA].easting + (sin(track.curvePts[rA].heading) * 300.0);
            refPoint2.northing = track.curvePts[rA].northing + (cos(track.curvePts[rA].heading) * 300.0);

            //x2-x1
            double dx = refPoint2.easting - refPoint1.easting;
            //z2-z1
            double dz = refPoint2.northing - refPoint1.northing;

            //how far are we away from the reference line at 90 degrees - 2D cross product and distance
            distanceFromRefLine = ((dz * CVehicle::instance()->guidanceLookPos.easting) -
                                   (dx * CVehicle::instance()->guidanceLookPos.northing) +
                                   (refPoint2.easting * refPoint1.northing) -
                                   (refPoint2.northing * refPoint1.easting))
                                  / sqrt((dz * dz) + (dx * dx));
        }
        else //pivot guide list
        {
            //cross product
            isHeadingSameWay = ((CVehicle::instance()->pivotAxlePos.easting - track.ptA.easting) * (CVehicle::instance()->steerAxlePos.northing - track.ptA.northing)
                                - (CVehicle::instance()->pivotAxlePos.northing - track.ptA.northing) * (CVehicle::instance()->steerAxlePos.easting - track.ptA.easting)) < 0;

            //pivot circle center
            distanceFromRefLine = -glm::Distance(CVehicle::instance()->guidanceLookPos, track.ptA);
        }

        distanceFromRefLine -= (0.5 * widthMinusOverlap);

        double RefDist = (distanceFromRefLine +
                          (isHeadingSameWay ? tool_offset : -tool_offset)
                          - track.nudgeDistance) / widthMinusOverlap;

        if (RefDist < 0) howManyPathsAway = (int)(RefDist - 0.5);
        else howManyPathsAway = (int)(RefDist + 0.5);
    }

    // CONDITION 2: Reconstruct curve only when howManyPathsAway or direction changes
    if (!isCurveValid || howManyPathsAway != lastHowManyPathsAway ||
        (isHeadingSameWay != lastIsHeadingSameWay && tool_offset != 0))
    {
        isCurveValid = true;
        lastHowManyPathsAway = howManyPathsAway;
        lastIsHeadingSameWay = isHeadingSameWay;
        double distAway = widthMinusOverlap * howManyPathsAway +
                          (isHeadingSameWay ? -tool_offset : tool_offset)
                          + track.nudgeDistance;

        distAway += (0.5 * widthMinusOverlap);

        // Cancel previous async build if running
        if (m_buildFuture.isRunning())
        {
            m_buildFuture.cancel();
            m_buildFuture.waitForFinished();
        }

        // Copy boundary data for thread safety
        QVector<Vec2> fenceLineEar;
        if (bnd.bndList.count() > 0)
            fenceLineEar = bnd.bndList[0].fenceLineEar;

        // Launch async build for all modes
        m_buildFuture = QtConcurrent::run<QVector<Vec3>>(
            &CABCurve::BuildNewOffsetList, distAway, CTrk(track), fenceLineEar);
        m_buildWatcher.setFuture(m_buildFuture);

        m_findGlobalNearestCurvePoint = true;

        // Build side guidelines if enabled
        bool isSideGuideLines = SettingsManager::instance()->menu_isSideGuideLines();
        double camDist = Camera::instance()->camSetDistance();

        if (isSideGuideLines && camDist > tool_width * -400)
        {
            int numPasses = SettingsManager::instance()->as_numGuideLines();

            if (m_guideFuture.isRunning())
            {
                m_guideFuture.cancel();
                m_guideFuture.waitForFinished();
            }

            m_guideFuture = QtConcurrent::run<QVector<QVector<Vec3>>>(
                &CABCurve::BuildCurveGuidelines, distAway, numPasses,
                isHeadingSameWay, tool_offset, CTrk(track), fenceLineEar);
            m_guideWatcher.setFuture(m_guideFuture);
        }
        else
        {
            if (m_guideFuture.isRunning())
            {
                m_guideFuture.cancel();
                m_guideFuture.waitForFinished();
            }
            guideArr.clear();
        }
    }
}

void CABCurve::BuildNewOffsetList(QPromise<QVector<Vec3>> &promise,
                                  double distAway, CTrk track,
                                  QVector<Vec2> fenceLineEar)
{
    QVector<Vec3> newCurList;

    try
    {
        if (track.mode == (int)TrackMode::AB)
        {
            Vec2 nudgePtA = track.ptA;
            Vec2 nudgePtB = track.ptB;

            Vec2 point1 = Vec2((cos(-track.heading) * distAway) + nudgePtA.easting,
                               (sin(-track.heading) * distAway) + nudgePtA.northing);

            Vec2 point2 = Vec2((cos(-track.heading) * distAway) + nudgePtB.easting,
                               (sin(-track.heading) * distAway) + nudgePtB.northing);

            double abLength = SettingsManager::instance()->ab_lineLength();

            double easting1 = point1.easting - (sin(track.heading) * abLength);
            double northing1 = point1.northing - (cos(track.heading) * abLength);

            newCurList.append(Vec3(easting1, northing1, track.heading));

            double easting2 = point2.easting + (sin(track.heading) * abLength);
            double northing2 = point2.northing + (cos(track.heading) * abLength);
            newCurList.append(Vec3(easting2, northing2, track.heading));
        }
        else if (track.mode == (int)TrackMode::waterPivot)
        {
            double Angle = glm::twoPI / qMin(qMax(ceil(glm::twoPI / (2 * acos(1 - (0.02 / fabs(distAway))))), 50.0), 500.0);

            Vec3 centerPos = Vec3(track.ptA.easting, track.ptA.northing, 0);
            double rotation = 0;

            while (rotation < glm::twoPI)
            {
                rotation += Angle;
                newCurList.append(Vec3(centerPos.easting + distAway * sin(rotation), centerPos.northing + distAway * cos(rotation), 0));
            }

            if (newCurList.count() > 1)
            {
                int cnt = newCurList.count();
                for (int i = 0; i < (cnt - 1); i++)
                {
                    newCurList[i].heading = atan2(newCurList[i + 1].easting - newCurList[i].easting, newCurList[i + 1].northing - newCurList[i].northing);
                    if (newCurList[i].heading < 0) newCurList[i].heading += glm::twoPI;
                    if (newCurList[i].heading >= glm::twoPI) newCurList[i].heading -= glm::twoPI;
                }

                newCurList[cnt - 1].heading = atan2(newCurList[0].easting - newCurList[cnt - 1].easting, newCurList[0].northing - newCurList[cnt - 1].northing);
            }
        }
        else
        {
            double tool_width = SettingsManager::instance()->vehicle_toolWidth();
            double tool_overlap = SettingsManager::instance()->vehicle_toolOverlap();
            double step = (tool_width - tool_overlap) * 0.48;
            if (step > 4) step = 4;
            if (step < 1) step = 1;

            double distSqAway = (distAway * distAway) - 0.01;

            Vec3 point;

            int refCount = track.curvePts.count();
            for (int i = 0; i < refCount && i < track.curvePts.count(); i++)
            {
                if (promise.isCanceled())
                    break;
                if (i < 0 || i >= track.curvePts.count()) continue;
                point = Vec3(
                    track.curvePts[i].easting + (sin(glm::PIBy2 + track.curvePts[i].heading) * distAway),
                    track.curvePts[i].northing + (cos(glm::PIBy2 + track.curvePts[i].heading) * distAway),
                    track.curvePts[i].heading);
                bool Add = true;

                for (int t = 0; t < refCount && t < track.curvePts.count(); t++)
                {
                    double dist = ((point.easting - track.curvePts[t].easting) * (point.easting - track.curvePts[t].easting))
                        + ((point.northing - track.curvePts[t].northing) * (point.northing - track.curvePts[t].northing));
                    if (dist < distSqAway)
                    {
                        Add = false;
                        break;
                    }
                }

                if (Add)
                {
                    if (newCurList.count() > 0)
                    {
                        double dist = ((point.easting - newCurList[newCurList.count() - 1].easting) * (point.easting - newCurList[newCurList.count() - 1].easting))
                            + ((point.northing - newCurList[newCurList.count() - 1].northing) * (point.northing - newCurList[newCurList.count() - 1].northing));
                        if (dist > step)
                            newCurList.append(point);
                    }
                    else newCurList.append(point);
                }
            }

            int cnt = newCurList.count();
            if (cnt > 6 && !promise.isCanceled())
            {
                QVector<Vec3> arr(newCurList);
                newCurList.clear();

                for (int i = 0; i < (arr.count() - 1); i++)
                {
                    if (promise.isCanceled())
                        break;
                    arr[i].heading = atan2(arr[i + 1].easting - arr[i].easting, arr[i + 1].northing - arr[i].northing);
                    if (arr[i].heading < 0) arr[i].heading += glm::twoPI;
                    if (arr[i].heading >= glm::twoPI) arr[i].heading -= glm::twoPI;
                }

                arr[arr.count() - 1].heading = arr[arr.count() - 2].heading;

                cnt = arr.count();
                double distance;

                //add the first point of loop - it will be p1
                newCurList.append(arr[0]);

                for (int i = 0; i < cnt - 3; i++)
                {
                    if (promise.isCanceled())
                        break;
                    // add p1
                    newCurList.append(arr[i + 1]);

                    distance = glm::Distance(arr[i + 1], arr[i + 2]);

                    if (distance > step)
                    {
                        int loopTimes = (int)(distance / step + 1);
                        for (int j = 1; j < loopTimes; j++)
                        {
                            Vec3 pos = Vec3(glm::Catmull(j / (double)(loopTimes), arr[i], arr[i + 1], arr[i + 2], arr[i + 3]));
                            newCurList.append(pos);
                        }
                    }
                }

                newCurList.append(arr[cnt - 2]);
                newCurList.append(arr[cnt - 1]);

                //to calc heading based on next and previous points to give an average heading.
                cnt = newCurList.count();
                arr = newCurList;
                cnt--;
                newCurList.clear();

                newCurList.append(Vec3(arr[0]));

                //middle points
                for (int i = 1; i < cnt; i++)
                {
                    Vec3 pt3 = Vec3(arr[i]);
                    pt3.heading = atan2(arr[i + 1].easting - arr[i - 1].easting, arr[i + 1].northing - arr[i - 1].northing);
                    if (pt3.heading < 0) pt3.heading += glm::twoPI;
                    newCurList.append(pt3);
                }

                int k = arr.count() - 1;
                Vec3 pt33 = Vec3(arr[k]);
                pt33.heading = atan2(arr[k].easting - arr[k - 1].easting, arr[k].northing - arr[k - 1].northing);
                if (pt33.heading < 0) pt33.heading += glm::twoPI;
                newCurList.append(pt33);

                if (!promise.isCanceled() && fenceLineEar.count() > 0 && !(track.mode == (int)TrackMode::bndCurve))
                {
                    int ptCnt = newCurList.count() - 1;

                    bool isAdding = false;
                    //end
                    while (glm::IsPointInPolygon(fenceLineEar, newCurList[newCurList.count() - 1]))
                    {
                        if (promise.isCanceled())
                            break;
                        isAdding = true;
                        for (int i = 1; i < 10; i++)
                        {
                            Vec3 pt = Vec3(newCurList[ptCnt]);
                            pt.easting += (sin(pt.heading) * i * 2);
                            pt.northing += (cos(pt.heading) * i * 2);
                            newCurList.append(pt);
                        }
                        ptCnt = newCurList.count() - 1;
                    }

                    if (isAdding)
                    {
                        Vec3 pt = Vec3(newCurList[newCurList.count() - 1]);
                        for (int i = 1; i < 5; i++)
                        {
                            pt.easting += (sin(pt.heading) * 2);
                            pt.northing += (cos(pt.heading) * 2);
                            newCurList.append(pt);
                        }
                    }

                    isAdding = false;

                    //and the beginning
                    pt33 = Vec3(newCurList[0]);

                    while (glm::IsPointInPolygon(fenceLineEar, newCurList[0]))
                    {
                        if (promise.isCanceled())
                            break;
                        isAdding = true;
                        pt33 = Vec3(newCurList[0]);

                        for (int i = 1; i < 10; i++)
                        {
                            Vec3 pt = Vec3(pt33);
                            pt.easting -= (sin(pt.heading) * i * 2);
                            pt.northing -= (cos(pt.heading) * i * 2);
                            newCurList.insert(0, pt);
                        }
                    }

                    if (isAdding)
                    {
                        Vec3 pt = Vec3(newCurList[0]);
                        for (int i = 1; i < 5; i++)
                        {
                            pt.easting -= (sin(pt.heading) * 2);
                            pt.northing -= (cos(pt.heading) * 2);
                            newCurList.insert(0, pt);
                        }
                    }
                }
            }
        }
    }
    catch (...)
    {
        //exception building offset curve
    }

    // Resample to uniform spacing to prevent lookahead jumping
    if (newCurList.count() > 2)
    {
        double targetSpacing = 1.0; // 1 meter uniform spacing
        newCurList = ResampleCurveToUniformSpacing(newCurList, targetSpacing);
    }

    promise.addResult(newCurList);
}

void CABCurve::AddGuidelineExtensions(QVector<Vec3> &guideLine)
{
    if (guideLine.isEmpty())
        return;

    Vec3 startExt;
    startExt.easting = guideLine[0].easting - (sin(guideLine[0].heading) * 2000.0);
    startExt.northing = guideLine[0].northing - (cos(guideLine[0].heading) * 2000.0);
    startExt.heading = guideLine[0].heading;
    guideLine.prepend(startExt);

    int last = guideLine.count() - 1;
    Vec3 endExt;
    endExt.easting = guideLine[last].easting + (sin(guideLine[last].heading) * 2000.0);
    endExt.northing = guideLine[last].northing + (cos(guideLine[last].heading) * 2000.0);
    endExt.heading = guideLine[last].heading;
    guideLine.append(endExt);
}

void CABCurve::BuildCurveGuidelines(QPromise<QVector<QVector<Vec3>>> &promise,
                                     double distAway, int numPasses,
                                     bool isHeadingSameWay, double toolOffset,
                                     CTrk track, QVector<Vec2> fenceLineEar)
{
    QVector<QVector<Vec3>> newGuideLL;

    double tool_width = SettingsManager::instance()->vehicle_toolWidth();
    double tool_overlap = SettingsManager::instance()->vehicle_toolOverlap();
    double widthMinusOverlap = tool_width - tool_overlap;

    bool isBndExist = fenceLineEar.count() > 0;

    bool isSwitch = isHeadingSameWay;

    int refCount = track.curvePts.count();
    if (refCount < 2)
    {
        promise.addResult(newGuideLL);
        return;
    }

    // Left side guidelines
    for (int numGuides = -1; numGuides > -numPasses; numGuides--)
    {
        if (promise.isCanceled())
            break;

        QVector<Vec3> newGuideList;

        double nextGuideDist = 0;
        if (isHeadingSameWay)
        {
            nextGuideDist = widthMinusOverlap * numGuides + track.nudgeDistance;
            nextGuideDist += (isSwitch ? toolOffset * 2 : 0);
            isSwitch = !isSwitch;
        }
        else
        {
            nextGuideDist = widthMinusOverlap * -numGuides + track.nudgeDistance;
            nextGuideDist += (isSwitch ? 0 : -toolOffset * 2);
            isSwitch = !isSwitch;
        }

        nextGuideDist += distAway;

        double step = widthMinusOverlap * 0.48;
        if (step > 4) step = 4;
        if (step < 1) step = 1;

        double distSqAway = (nextGuideDist * nextGuideDist) - 0.01;

        for (int i = 0; i < refCount; i++)
        {
            if (promise.isCanceled())
                break;

            Vec3 point(
                track.curvePts[i].easting + (sin(glm::PIBy2 + track.curvePts[i].heading) * nextGuideDist),
                track.curvePts[i].northing + (cos(glm::PIBy2 + track.curvePts[i].heading) * nextGuideDist),
                track.curvePts[i].heading);
            bool add = true;

            for (int t = 0; t < refCount; t++)
            {
                double dist = ((point.easting - track.curvePts[t].easting) * (point.easting - track.curvePts[t].easting))
                    + ((point.northing - track.curvePts[t].northing) * (point.northing - track.curvePts[t].northing));
                if (dist < distSqAway)
                {
                    add = false;
                    break;
                }
            }

            if (add)
            {
                if (newGuideList.count() > 0)
                {
                    double dist = ((point.easting - newGuideList.last().easting) * (point.easting - newGuideList.last().easting))
                        + ((point.northing - newGuideList.last().northing) * (point.northing - newGuideList.last().northing));
                    if (dist > step)
                    {
                        if (!isBndExist || glm::IsPointInPolygon(fenceLineEar, point))
                            newGuideList.append(point);
                    }
                }
                else
                {
                    if (!isBndExist || glm::IsPointInPolygon(fenceLineEar, point))
                        newGuideList.append(point);
                }
            }
        }

        if (newGuideList.isEmpty())
            continue;

        AddGuidelineExtensions(newGuideList);
        newGuideLL.append(newGuideList);
    }

    // Right side guidelines
    for (int numGuides = 1; numGuides < numPasses; numGuides++)
    {
        if (promise.isCanceled())
            break;

        QVector<Vec3> newGuideList;

        double nextGuideDist = 0;
        if (isHeadingSameWay)
        {
            nextGuideDist = widthMinusOverlap * numGuides + track.nudgeDistance;
            nextGuideDist += (isSwitch ? toolOffset * 2 : 0);
            isSwitch = !isSwitch;
        }
        else
        {
            nextGuideDist = widthMinusOverlap * -numGuides + track.nudgeDistance;
            nextGuideDist += (isSwitch ? 0 : -toolOffset * 2);
            isSwitch = !isSwitch;
        }

        nextGuideDist += distAway;

        double step = widthMinusOverlap * 0.48;
        if (step > 4) step = 4;
        if (step < 1) step = 1;

        double distSqAway = (nextGuideDist * nextGuideDist) - 0.01;

        for (int i = 0; i < refCount; i++)
        {
            if (promise.isCanceled())
                break;

            Vec3 point(
                track.curvePts[i].easting + (sin(glm::PIBy2 + track.curvePts[i].heading) * nextGuideDist),
                track.curvePts[i].northing + (cos(glm::PIBy2 + track.curvePts[i].heading) * nextGuideDist),
                track.curvePts[i].heading);
            bool add = true;

            for (int t = 0; t < refCount; t++)
            {
                double dist = ((point.easting - track.curvePts[t].easting) * (point.easting - track.curvePts[t].easting))
                    + ((point.northing - track.curvePts[t].northing) * (point.northing - track.curvePts[t].northing));
                if (dist < distSqAway)
                {
                    add = false;
                    break;
                }
            }

            if (add)
            {
                if (newGuideList.count() > 0)
                {
                    double dist = ((point.easting - newGuideList.last().easting) * (point.easting - newGuideList.last().easting))
                        + ((point.northing - newGuideList.last().northing) * (point.northing - newGuideList.last().northing));
                    if (dist > step)
                    {
                        if (!isBndExist || glm::IsPointInPolygon(fenceLineEar, point))
                            newGuideList.append(point);
                    }
                }
                else
                {
                    if (!isBndExist || glm::IsPointInPolygon(fenceLineEar, point))
                        newGuideList.append(point);
                }
            }
        }

        if (newGuideList.isEmpty())
            continue;

        AddGuidelineExtensions(newGuideList);
        newGuideLL.append(newGuideList);
    }

    promise.addResult(newGuideLL);
}

void CABCurve::GetCurrentCurveLine(Vec3 pivot,
                                   Vec3 steer,
                                   bool isBtnAutoSteerOn,
                                   CVehicle &vehicle,
                                   CTrk &track,
                                   CYouTurn &yt,
                                   const CAHRS &ahrs,
                                   CNMEA &pn)
{
    double purePursuitGain = SettingsManager::instance()->vehicle_purePursuitIntegralGainAB();
    double wheelBase = SettingsManager::instance()->vehicle_wheelbase();
    double maxSteerAngle = SettingsManager::instance()->vehicle_maxSteerAngle();
    bool vehicle_isStanleyUsed = SettingsManager::instance()->vehicle_isStanleyUsed();
    double as_sideHillCompensation = SettingsManager::instance()->as_sideHillCompensation();

    if (track.curvePts.count() == 0 || track.curvePts.count() < 5)
    {
        if (track.mode != (int)TrackMode::waterPivot)
        {
            return;
        }
    }

    double dist, dx, dz;
    double minDistA = 1000000, minDistB = 1000000;
    //int ptCount = curList.count();

    if (curList.count() > 0)
    {
        if (yt.isYouTurnTriggered && yt.DistanceFromYouTurnLine(pn))//do the pure pursuit from youTurn
        {
            //now substitute what it thinks are AB line values with auto turn values
            steerAngleCu = yt.steerAngleYT;
            distanceFromCurrentLinePivot = yt.distanceFromCurrentLine;

            goalPointCu = yt.goalPointYT;
            radiusPointCu.easting = yt.radiusPointYT.easting;
            radiusPointCu.northing = yt.radiusPointYT.northing;
            ppRadiusCu = yt.ppRadiusYT;
            CVehicle::instance()->set_modeActualXTE((distanceFromCurrentLinePivot));
        }
        else if (vehicle_isStanleyUsed)//Stanley
        {
            Backend::instance()->gyd().StanleyGuidanceCurve(pivot, steer, curList, isBtnAutoSteerOn, *CVehicle::instance(), *this, ahrs);
        }
        else// Pure Pursuit ------------------------------------------
        {
            //update based on autosteer settings and distance from line
            double goalPointDistance = CVehicle::instance()->UpdateGoalPointDistance();

            bool ReverseHeading = CVehicle::instance()->isReverse() ? !isHeadingSameWay : isHeadingSameWay;

            //If is a curve
            if (track.mode <= (int)TrackMode::Curve)
            {
                int cc;

                if (m_findGlobalNearestCurvePoint)
                {
                    cc = findNearestGlobalCurvePoint(pivot, 10);
                    m_findGlobalNearestCurvePoint = false;
                }
                else
                {
                    cc = findNearestLocalCurvePoint(pivot, currentLocationIndex, goalPointDistance, ReverseHeading);
                }

                minDistA = minDistB = glm::DOUBLE_MAX;

                int dd = cc + 8;
                if (dd > curList.count() - 1) dd = curList.count();
                cc -= 8;
                if (cc < 0) cc = 0;

                //find the closest 2 points to current close call
                for (int j = cc; j < dd; j++)
                {
                    dist = glm::DistanceSquared(pivot, curList[j]);
                    if (dist < minDistA)
                    {
                        minDistB = minDistA;
                        B = A;
                        minDistA = dist;
                        A = j;
                    }
                    else if (dist < minDistB)
                    {
                        minDistB = dist;
                        B = j;
                    }
                }

                //just need to make sure the points continue ascending or heading switches all over the place
                if (A > B) {
                    C = A;
                    A = B;
                    B = C;
                }

                currentLocationIndex = A;

                if (A > curList.count() - 1 || B > curList.count() - 1)
                    return;
            }
            else //it's TrackMode::waterPivot
            {
                if (m_findGlobalNearestCurvePoint)
                {
                    A = findNearestGlobalCurvePoint(pivot);
                    m_findGlobalNearestCurvePoint = false;
                }
                else
                {
                    A = findNearestLocalCurvePoint(pivot, currentLocationIndex, goalPointDistance, ReverseHeading);
                }

                currentLocationIndex = A;

                if (A > curList.count() - 1)
                    return;

                //initial forward Test if pivot InRange AB
                if (A == curList.count() - 1) B = 0;
                else B = A + 1;

                if (glm::InRangeBetweenAB(curList[A].easting, curList[A].northing,
                                         curList[B].easting, curList[B].northing, pivot.easting, pivot.northing))
                    goto SegmentFound;

                A = currentLocationIndex;
                //step back one
                if (A == 0)
                {
                    A = curList.count() - 1;
                    B = 0;
                }
                else
                {
                    A = A - 1;
                    B = A + 1;
                }

                if (glm::InRangeBetweenAB(curList[A].easting, curList[A].northing,
                                         curList[B].easting, curList[B].northing, pivot.easting, pivot.northing))
                    goto SegmentFound;

                //realy really lost
                return;

            }

            SegmentFound:

            //get the distance from currently active AB line

            dx = curList[B].easting - curList[A].easting;
            dz = curList[B].northing - curList[A].northing;

            if (fabs(dx) < glm::DOUBLE_EPSILON && fabs(dz) < glm::DOUBLE_EPSILON) return;

            //abHeading = atan2(dz, dx);

            //how far from current AB Line is fix
            distanceFromCurrentLinePivot = ((dz * pivot.easting) - (dx * pivot.northing) + (curList[B].easting
                                                                                            * curList[A].northing) - (curList[B].northing * curList[A].easting))
                                           / sqrt((dz * dz) + (dx * dx));

            //integral slider is set to 0
            if (purePursuitGain != 0 && !CVehicle::instance()->isReverse())
            {
                pivotDistanceError = distanceFromCurrentLinePivot * 0.2 + pivotDistanceError * 0.8;

                if (counter2++ > 4)
                {
                    pivotDerivative = pivotDistanceError - pivotDistanceErrorLast;
                    pivotDistanceErrorLast = pivotDistanceError;
                    counter2 = 0;
                    pivotDerivative *= 2;

                    //limit the derivative
                    //if (pivotDerivative > 0.03) pivotDerivative = 0.03;
                    //if (pivotDerivative < -0.03) pivotDerivative = -0.03;
                    //if (fabs(pivotDerivative) < 0.01) pivotDerivative = 0;
                }

                //pivotErrorTotal = pivotDistanceError + pivotDerivative;

                if (isBtnAutoSteerOn && CVehicle::instance()->avgSpeed() > 2.5 && fabs(pivotDerivative) < 0.1)
                {
                    //if over the line heading wrong way, rapidly decrease integral
                    if ((inty < 0 && distanceFromCurrentLinePivot < 0) || (inty > 0 && distanceFromCurrentLinePivot > 0))
                    {
                        inty += pivotDistanceError * purePursuitGain * -0.04;
                    }
                    else
                    {
                        if (fabs(distanceFromCurrentLinePivot) > 0.02)
                        {
                            inty += pivotDistanceError * purePursuitGain * -0.02;
                            if (inty > 0.2) inty = 0.2;
                            else if (inty < -0.2) inty = -0.2;
                        }
                    }
                }
                else inty *= 0.95;
            }
            else inty = 0;

            // ** Pure pursuit ** - calc point on ABLine closest to current position
            double U = (((pivot.easting - curList[A].easting) * dx)
                        + ((pivot.northing - curList[A].northing) * dz))
                       / ((dx * dx) + (dz * dz));

            rEastCu = curList[A].easting + (U * dx);
            rNorthCu = curList[A].northing + (U * dz);
            manualUturnHeading = curList[A].heading;

            int count = ReverseHeading ? 1 : -1;
            Vec3 start(rEastCu, rNorthCu, 0);
            double distSoFar = 0;

            for (int i = ReverseHeading ? B : A; i < curList.count() && i >= 0;)
            {
                // used for calculating the length squared of next segment.
                double tempDist = glm::Distance(start, curList[i]);

                //will we go too far?
                if ((tempDist + distSoFar) > goalPointDistance)
                {
                    double j = (goalPointDistance - distSoFar) / tempDist; // the remainder to yet travel

                    goalPointCu.easting = (((1 - j) * start.easting) + (j * curList[i].easting));
                    goalPointCu.northing = (((1 - j) * start.northing) + (j * curList[i].northing));
                    break;
                }
                else distSoFar += tempDist;
                start = curList[i];
                i += count;
                if (i < 0) i = curList.count() - 1;
                if (i > curList.count() - 1) i = 0;
            }

            if (track.mode <= (int)TrackMode::Curve)
            {
                if (isBtnAutoSteerOn && !CVehicle::instance()->isReverse())
                {
                    if (isHeadingSameWay)
                    {
                        if (glm::Distance(goalPointCu, curList[(curList.count() - 1)]) < 0.5)
                        {
                            emit Backend::instance()->timedMessage(2000,tr("Guidance Stopped"), tr("Past end of curve"));
                            MainWindowState::instance()->set_isBtnAutoSteerOn(false);
                        }
                    }
                    else
                    {
                        if (glm::Distance(goalPointCu, curList[0]) < 0.5)
                        {
                            emit Backend::instance()->timedMessage(2000,tr("Guidance Stopped"), tr("Past end of curve"));
                            MainWindowState::instance()->set_isBtnAutoSteerOn(false);
                        }
                    }
                }
            }

            //calc "D" the distance from pivot axle to lookahead point
            double goalPointDistanceSquared = glm::DistanceSquared(goalPointCu.northing, goalPointCu.easting, pivot.northing, pivot.easting);

            //calculate the the delta x in local coordinates and steering angle degrees based on wheelbase
            //double localHeading = glm::twoPI - mf.fixHeading;

            double localHeading;
            if (ReverseHeading) localHeading = glm::twoPI - CVehicle::instance()->fixHeading() + inty;
            else localHeading = glm::twoPI - CVehicle::instance()->fixHeading() - inty;

            ppRadiusCu = goalPointDistanceSquared / (2 * (((goalPointCu.easting - pivot.easting) * cos(localHeading)) + ((goalPointCu.northing - pivot.northing) * sin(localHeading))));

            steerAngleCu = glm::toDegrees(atan(2 * (((goalPointCu.easting - pivot.easting) * cos(localHeading))
                                                        + ((goalPointCu.northing - pivot.northing) * sin(localHeading))) * wheelBase / goalPointDistanceSquared));

            if (ahrs.imuRoll != 88888)
                steerAngleCu += ahrs.imuRoll * -as_sideHillCompensation; /* gyd.sideHillCompFactor*/

            if (steerAngleCu < -maxSteerAngle) steerAngleCu = -maxSteerAngle;
            if (steerAngleCu > maxSteerAngle) steerAngleCu = maxSteerAngle;

            if (!isHeadingSameWay)
                distanceFromCurrentLinePivot *= -1.0;

            //used for acquire/hold mode
            CVehicle::instance()->set_modeActualXTE ( (distanceFromCurrentLinePivot)) ;

            double steerHeadingError = (pivot.heading - curList[A].heading);
            //Fix the circular error
            if (steerHeadingError > M_PI)
                steerHeadingError -= M_PI;
            else if (steerHeadingError < -M_PI)
                steerHeadingError += M_PI;

            if (steerHeadingError > glm::PIBy2)
                steerHeadingError -= M_PI;
            else if (steerHeadingError < -glm::PIBy2)
                steerHeadingError += M_PI;

            CVehicle::instance()->set_modeActualHeadingError (glm::toDegrees(steerHeadingError));

            //Convert to centimeters
            CVehicle::instance()->set_guidanceLineDistanceOff ( (short)glm::roundMidAwayFromZero(distanceFromCurrentLinePivot * 1000.0));
            CVehicle::instance()->guidanceLineSteerAngle = (short)(steerAngleCu * 100);
        }
    }
    else
    {
        //invalid distance so tell AS module
        distanceFromCurrentLinePivot = 32000;
        CVehicle::instance()->set_guidanceLineDistanceOff (32000);
    }
}

void CABCurve::DrawCurveNew(QOpenGLFunctions *gl, const QMatrix4x4 &mvp)
{
    GLHelperOneColor gldraw;

    for (int h = 0; h < desList.count(); h++) gldraw.append(QVector3D(desList[h].easting, desList[h].northing, 0));
    gl->glLineWidth(5);
    gldraw.draw(gl, mvp, QColor::fromRgbF(0.95f, 0.42f, 0.750f), GL_LINE_STRIP, 4.0f); //TODO is 4 pixels right?  check main form call
    gl->glLineWidth(1);
}

void CABCurve::DrawCurve(QOpenGLFunctions *gl, const QMatrix4x4 &mvp,
                         bool isFontOn,
                         double camSetDistance,
                         const CTrk &track,
                         CYouTurn &yt)
{
    //double tool_toolWidth = SettingsManager::instance()->getValue("Vehicle_toolWidth;
    //double tool_toolOverlap = SettingsManager::instance()->getValue("Vehicle_toolOverlap;


    GLHelperColors gldraw_colors;
    GLHelperOneColor gldraw;
    ColorVertex cv;
    QColor color;

    double lineWidth = SettingsManager::instance()->display_lineWidth();
    bool vehicle_isStanleyUsed = SettingsManager::instance()->vehicle_isStanleyUsed();

    if (desList.count() > 0)
    {
        for (int h = 0; h < desList.count(); h++)
            gldraw.append(QVector3D(desList[h].easting, desList[h].northing, 0));
        gl->glLineWidth(lineWidth);
        gldraw.draw(gl,mvp,QColor::fromRgbF(0.95f, 0.42f, 0.750f),GL_LINE_STRIP,lineWidth);
        gl->glLineWidth(1);
    }

    int ptCount = track.curvePts.count();
    if (track.mode != TrackMode::waterPivot)
    {
        if (track.curvePts.count() == 0) return;

        gldraw.clear();
        //cv.color = QVector4D(0.96, 0.2f, 0.2f, 1.0f);
        for (int h = 0; h < ptCount; h++) {
            gldraw.append(QVector3D(track.curvePts[h].easting, track.curvePts[h].northing, 0));
        }
        gl->glLineWidth(lineWidth);
        gldraw.draw(gl,mvp,QColor::fromRgbF(0.96, 0.2f, 0.2f), GL_LINES, 4.0);
        gl->glLineWidth(1);
        if (isFontOn)
        {
            color.setRgbF(0.40f, 0.90f, 0.95f);
            drawText3D(gl, mvp, track.curvePts[0].easting, track.curvePts[0].northing, "&A", 1.0, true, color);
            drawText3D(gl, mvp, track.curvePts[track.curvePts.count() - 1].easting, track.curvePts[track.curvePts.count() - 1].northing, "&B", 1.0, true, color);
        }

        //just draw ref and smoothed line if smoothing window is open
        if (isSmoothWindowOpen)
        {
            if (smooList.count() == 0) return;

            gldraw.clear();

            for (int h = 0; h < smooList.count(); h++)
                gldraw.append(QVector3D(smooList[h].easting, smooList[h].northing, 0));
            gl->glLineWidth(lineWidth);
            gldraw.draw(gl,mvp,QColor::fromRgbF(0.930f, 0.92f, 0.260f),GL_LINES,lineWidth);
            gl->glLineWidth(1);
        }
        else //normal. Smoothing window is not open.
        {
            if (curList.count() > 0)
            {
                color.setRgbF(0.95f, 0.2f, 0.95f);
                gldraw.clear();

                for (int h = 0; h < curList.count(); h++)
                    gldraw.append(QVector3D(curList[h].easting, curList[h].northing, 0));

                //ablines and curves are a line - the rest a loop

                if(track.mode <= (int)TrackMode::Curve){
                    gl->glLineWidth(lineWidth);
                    gldraw.draw(gl,mvp,color,GL_LINE_STRIP,lineWidth);
                    gl->glLineWidth(1);}
                else{
                    gl->glLineWidth(lineWidth);
                    gldraw.draw(gl,mvp,color,GL_LINE_LOOP,lineWidth);
                    gl->glLineWidth(1);}

                if (!vehicle_isStanleyUsed && camSetDistance > -200)
                {
                    gldraw.clear();
                    //Draw lookahead Point
                    color.setRgbF(1.0f, 0.95f, 0.195f);
                    gl->glLineWidth(lineWidth);
                    gldraw.append(QVector3D(goalPointCu.easting, goalPointCu.northing, 0.0));
                    gldraw.draw(gl,mvp,color,GL_POINTS,8.0f);
                    gl->glLineWidth(1);
                }

                yt.DrawYouTurn(gl,mvp);

                gldraw.clear();
                for (int h = 0; h < curList.count(); h++)
                    gldraw.append(QVector3D(curList[h].easting, curList[h].northing, 0));
                gl->glLineWidth(lineWidth);
                gldraw.draw(gl, mvp, QColor::fromRgbF(0.920f, 0.6f, 0.950f), GL_POINTS, 3.0f);
                gl->glLineWidth(1);
            }
        }
    }

    else
    {
        if (curList.count() > 0)
        {
            gldraw.clear();
            gldraw.append(QVector3D(track.ptA.easting, track.ptA.northing, 0));
            for (int h = 0; h < curList.count(); h++)
                gldraw.append(QVector3D(curList[h].easting, curList[h].northing, 0));
            gl->glLineWidth(lineWidth);
            gldraw.draw(gl, mvp, QColor::fromRgbF(0.95f, 0.2f, 0.95f),GL_LINE_STRIP, lineWidth);
            gl->glLineWidth(1);

            if (!vehicle_isStanleyUsed && camSetDistance > -200)
            {
                //Draw lookahead Point
                gldraw.clear();
                gldraw.append(QVector3D(goalPointCu.easting, goalPointCu.northing, 0.0));
                gldraw.draw(gl, mvp, QColor::fromRgbF(1.0f, 0.95f, 0.195f), GL_POINTS, 8.0f);
            }
        }
    }
}

void CABCurve::BuildTram(CBoundary &bnd, CTram &tram, const CTrk &track)
{
    double halfWheelTrack = SettingsManager::instance()->vehicle_trackWidth() * 0.5;
    double tram_width = SettingsManager::instance()->tram_width();
    int tram_passes = SettingsManager::instance()->tram_passes();

    //if all or bnd only then make outer loop pass
    if (tram.generateMode != 1)
    {
        tram.BuildTramBnd(bnd);
    }
    else
    {
        tram.tramBndOuterArr.clear();
        tram.tramBndInnerArr.clear();
    }

    tram.tramList.clear();
    tram.tramArr->clear();

    if (tram.generateMode == 2) return;

    bool isBndExist = bnd.bndList.count() != 0;

    int refCount = track.curvePts.count();

    int cntr = 0;
    if (isBndExist)
    {
        if (tram.generateMode == 1)
            cntr = 0;
        else
            cntr = 1;
    }
    double widd = 0;

    for (int i = cntr; i <= tram_passes; i++)
    {
        tram.tramArr = QSharedPointer<QVector<Vec2>>(new QVector<Vec2>());
        tram.tramList.append(tram.tramArr);

        widd = tram_width * 0.5 - halfWheelTrack;
        widd += (tram_width * i);

        double distSqAway = widd * widd * 0.999999;

        for (int j = 0; j < refCount; j += 1)
        {
            Vec2 point(
                (sin(glm::PIBy2 + track.curvePts[j].heading) *
                 widd) + track.curvePts[j].easting,
                (cos(glm::PIBy2 + track.curvePts[j].heading) *
                 widd) + track.curvePts[j].northing
                );

            bool Add = true;
            for (int t = 0; t < refCount; t++)
            {
                //distance check to be not too close to ref line
                double dist = ((point.easting - track.curvePts[t].easting) * (point.easting - track.curvePts[t].easting))
                              + ((point.northing - track.curvePts[t].northing) * (point.northing - track.curvePts[t].northing));
                if (dist < distSqAway)
                {
                    Add = false;
                    break;
                }
            }
            if (Add)
            {
                //a new point only every 2 meters
                double dist = (tram.tramArr->count() > 0 ? ((point.easting - (*tram.tramArr)[tram.tramArr->count() - 1].easting) * (point.easting - (*tram.tramArr)[tram.tramArr->count() - 1].easting))
                                                               + ((point.northing - (*tram.tramArr)[tram.tramArr->count() - 1].northing) * (point.northing - (*tram.tramArr)[tram.tramArr->count() - 1].northing)) : 3.0);
                if (dist > 2)
                {
                    //if inside the boundary, add
                    if (!isBndExist || glm::IsPointInPolygon(bnd.bndList[0].fenceLineEar,point))
                    {
                        tram.tramArr->append(point);
                    }
                }
            }
        }
    }

    for (int i = cntr; i <= tram_passes; i++)
    {
        tram.tramArr = QSharedPointer<QVector<Vec2>>(new QVector<Vec2>());
        tram.tramList.append(tram.tramArr);

        widd = tram_width * 0.5 + halfWheelTrack;
        widd += (tram_width * i);

        double distSqAway = widd * widd * 0.999999;

        for (int j = 0; j < refCount; j += 1)
        {
            Vec2 point(
                sin(glm::PIBy2 + track.curvePts[j].heading) *
                        widd + track.curvePts[j].easting,
                cos(glm::PIBy2 + track.curvePts[j].heading) *
                        widd + track.curvePts[j].northing
                );

            bool Add = true;
            for (int t = 0; t < refCount; t++)
            {
                //distance check to be not too close to ref line
                double dist = ((point.easting - track.curvePts[t].easting) * (point.easting - track.curvePts[t].easting))
                              + ((point.northing - track.curvePts[t].northing) * (point.northing - track.curvePts[t].northing));
                if (dist < distSqAway)
                {
                    Add = false;
                    break;
                }
            }
            if (Add)
            {
                //a new point only every 2 meters
                double dist = tram.tramArr->count() > 0 ? ((point.easting - (*tram.tramArr)[tram.tramArr->count() - 1].easting) * (point.easting - (*tram.tramArr)[tram.tramArr->count() - 1].easting))
                                                              + ((point.northing - (*tram.tramArr)[tram.tramArr->count() - 1].northing) * (point.northing - (*tram.tramArr)[tram.tramArr->count() - 1].northing)) : 3.0;
                if (dist > 2)
                {
                    //if inside the boundary, add
                    if (!isBndExist || glm::IsPointInPolygon(bnd.bndList[0].fenceLineEar,point))
                    {
                        tram.tramArr->append(point);
                    }
                }
            }
        }
    }
}

void CABCurve::SmoothAB(int smPts, const CTrk &track)
{
    //count the reference list of original curve
    int cnt = track.curvePts.count();

    //just go back if not very long
    if (cnt < 200) return;

    //the temp array - C++17 RAII automatic cleanup
    auto arr = std::make_unique<Vec3[]>(cnt);

    //read the points before and after the setpoint
    for (int s = 0; s < smPts / 2; s++)
    {
        arr[s].easting = track.curvePts[s].easting;
        arr[s].northing = track.curvePts[s].northing;
        arr[s].heading = track.curvePts[s].heading;
    }

    for (int s = cnt - (smPts / 2); s < cnt; s++)
    {
        arr[s].easting = track.curvePts[s].easting;
        arr[s].northing = track.curvePts[s].northing;
        arr[s].heading = track.curvePts[s].heading;
    }

    //average them - center weighted average
    for (int i = smPts / 2; i < cnt - (smPts / 2); i++)
    {
        for (int j = -smPts / 2; j < smPts / 2; j++)
        {
            arr[i].easting += track.curvePts[j + i].easting;
            arr[i].northing += track.curvePts[j + i].northing;
        }
        arr[i].easting /= smPts;
        arr[i].northing /= smPts;
        arr[i].heading = track.curvePts[i].heading;
    }

    //make a list to draw
    smooList.clear();
    for (int i = 0; i < cnt; i++)
    {
        smooList.append(arr[i]);
    }

    // ✅ C++17 RAII: automatic cleanup, no manual delete needed
}

void CABCurve::SmoothABDesList(int smPts)
{
    //count the reference list of original curve
    int cnt = desList.count();

    //the temp array - C++17 RAII automatic cleanup
    auto arr = std::make_unique<Vec3[]>(cnt);

    //read the points before and after the setpoint
    for (int s = 0; s < smPts / 2; s++)
    {
        arr[s].easting = desList[s].easting;
        arr[s].northing = desList[s].northing;
        arr[s].heading = desList[s].heading;
    }

    for (int s = cnt - (smPts / 2); s < cnt; s++)
    {
        arr[s].easting = desList[s].easting;
        arr[s].northing = desList[s].northing;
        arr[s].heading = desList[s].heading;
    }

    //average them - center weighted average
    for (int i = smPts / 2; i < cnt - (smPts / 2); i++)
    {
        for (int j = -smPts / 2; j < smPts / 2; j++)
        {
            arr[i].easting += desList[j + i].easting;
            arr[i].northing += desList[j + i].northing;
        }
        arr[i].easting /= smPts;
        arr[i].northing /= smPts;
        arr[i].heading = desList[i].heading;
    }

    //make a list to draw
    desList.clear();
    for (int i = 0; i < cnt; i++)
    {
        desList.append(arr[i]);
    }

    // ✅ C++17 RAII: automatic cleanup, no manual delete needed
}

void CABCurve::CalculateHeadings(QVector<Vec3> &xList)
{
    //to calc heading based on next and previous points to give an average heading.
    int cnt = xList.count();
    if (cnt > 3)
    {
        QVector<Vec3> arr = xList;
        cnt--;
        xList.clear();

        Vec3 pt3(arr[0]);
        pt3.heading = atan2(arr[1].easting - arr[0].easting, arr[1].northing - arr[0].northing);
        if (pt3.heading < 0) pt3.heading += glm::twoPI;
        xList.append(pt3);

        //middle points
        for (int i = 1; i < cnt; i++)
        {
            Vec3 pt3 = arr[i];
            pt3.heading = atan2(arr[i + 1].easting - arr[i - 1].easting, arr[i + 1].northing - arr[i - 1].northing);
            if (pt3.heading < 0) pt3.heading += glm::twoPI;
            xList.append(pt3);
        }
    }
}

void CABCurve::MakePointMinimumSpacing(QVector<Vec3> &xList, double minDistance)
{
    int cnt = xList.count();
    if (cnt > 3)
    {
        //make sure point distance isn't too big
        for (int i = 0; i < cnt - 1; i++)
        {
            int j = i + 1;
            //if (j == cnt) j = 0;
            double distance = glm::Distance(xList[i], xList[j]);
            if (distance > minDistance)
            {
                Vec3 pointB((xList[i].easting + xList[j].easting) / 2.0,
                            (xList[i].northing + xList[j].northing) / 2.0,
                            xList[i].heading);

                xList.insert(j, pointB);
                cnt = xList.count();
                i = -1;
            }
        }
    }
}

void CABCurve::SaveSmoothList(CTrk &track)
{
    //oops no smooth list generated
    int cnt = smooList.size();
    if (cnt == 0) return;

    //eek
    track.curvePts.clear();

    //copy to an array to calculate all the new headings
    QVector<Vec3> arr = smooList;

    //calculate new headings on smoothed line
    for (int i = 1; i < cnt - 1; i++)
    {
        arr[i].heading = atan2(arr[i + 1].easting - arr[i].easting, arr[i + 1].northing - arr[i].northing);
        if (arr[i].heading < 0) arr[i].heading += glm::twoPI;
        track.curvePts.append(arr[i]);
    }
}

bool CABCurve::PointOnLine(Vec3 pt1, Vec3 pt2, Vec3 pt)
{
    Vec2 r(0, 0);
    if (pt1.northing == pt2.northing && pt1.easting == pt2.easting) { pt1.northing -= 0.00001; }

    double U = ((pt.northing - pt1.northing) * (pt2.northing - pt1.northing)) + ((pt.easting - pt1.easting) * (pt2.easting - pt1.easting));

    double Udenom = pow(pt2.northing - pt1.northing, 2) + pow(pt2.easting - pt1.easting, 2);

    U /= Udenom;

    r.northing = pt1.northing + (U * (pt2.northing - pt1.northing));
    r.easting = pt1.easting + (U * (pt2.easting - pt1.easting));

    double minx, maxx, miny, maxy;

    minx = fmin(pt1.northing, pt2.northing);
    maxx = fmax(pt1.northing, pt2.northing);

    miny = fmin(pt1.easting, pt2.easting);
    maxy = fmax(pt1.easting, pt2.easting);
    return (r.northing >= minx && r.northing <= maxx && (r.easting >= miny && r.easting <= maxy));

}

void CABCurve::AddFirstLastPoints(QVector<Vec3> &xList)
{
    //No longer are we clipping to the boundary. I don't think it's necessary.

    int ptCnt = xList.count() - 1;
    Vec3 start(xList[0]);

    for (int i = 1; i < 300; i++)
    {
        Vec3 pt(xList[ptCnt]);
        pt.easting += (sin(pt.heading) * i);
        pt.northing += (cos(pt.heading) * i);
        xList.append(pt);
    }

    //and the beginning
    start = Vec3(xList[0]);

    for (int i = 1; i < 300; i++)
    {
        Vec3 pt(start);
        pt.easting -= (sin(pt.heading) * i);
        pt.northing -= (cos(pt.heading) * i);
        xList.insert(0, pt);
    }
}

QVector<Vec3> CABCurve::ResampleCurveToUniformSpacing(
    const QVector<Vec3> &originalList, double targetSpacing)
{
    if (originalList.count() < 2)
        return originalList;

    QVector<Vec3> resampledList;
    // Always add the first point
    resampledList.append(originalList[0]);

    double accumulatedDistance = 0;
    int sourceIndex = 1;

    while (sourceIndex < originalList.count())
    {
        double segmentLength = glm::Distance(originalList[sourceIndex - 1], originalList[sourceIndex]);

        if (segmentLength < 0.001) // Skip duplicate points
        {
            sourceIndex++;
            continue;
        }

        accumulatedDistance += segmentLength;

        // Add points at uniform intervals
        while (accumulatedDistance >= targetSpacing && sourceIndex < originalList.count())
        {
            // Calculate how far back we need to go on this segment
            double overshoot = accumulatedDistance - targetSpacing;
            double ratio = 1.0 - (overshoot / segmentLength);

            Vec3 newPoint(
                originalList[sourceIndex - 1].easting + ratio * (originalList[sourceIndex].easting - originalList[sourceIndex - 1].easting),
                originalList[sourceIndex - 1].northing + ratio * (originalList[sourceIndex].northing - originalList[sourceIndex - 1].northing),
                originalList[sourceIndex - 1].heading
            );

            resampledList.append(newPoint);
            accumulatedDistance -= targetSpacing;
        }

        sourceIndex++;
    }

    // Always add the final point if we didn't generate enough samples
    if (resampledList.count() == 1)
    {
        resampledList.append(originalList[originalList.count() - 1]);
    }

    // Recalculate headings for the resampled points
    for (int i = 0; i < resampledList.count() - 1; i++)
    {
        double newHeading = atan2(
            resampledList[i + 1].easting - resampledList[i].easting,
            resampledList[i + 1].northing - resampledList[i].northing);

        if (newHeading < 0)
            newHeading += glm::twoPI;

        resampledList[i] = Vec3(
            resampledList[i].easting,
            resampledList[i].northing,
            newHeading
        );
    }

    // Set last point heading same as previous
    if (resampledList.count() > 1)
    {
        resampledList[resampledList.count() - 1] = Vec3(
            resampledList[resampledList.count() - 1].easting,
            resampledList[resampledList.count() - 1].northing,
            resampledList[resampledList.count() - 2].heading
        );
    }

    return resampledList;
}

int CABCurve::findNearestGlobalCurvePoint(const Vec3 &refPoint, int increment)
{
    double minDist = glm::DOUBLE_MAX;
    int minDistIndex = 0;

    for (int i = 0; i < curList.count(); i += increment)
    {
        double dist = glm::DistanceSquared(refPoint, curList[i]);
        if (dist < minDist)
        {
            minDist = dist;
            minDistIndex = i;
        }
    }
    return minDistIndex;
}

int CABCurve::findNearestLocalCurvePoint(const Vec3 &refPoint, int startIndex,
    double minSearchDistance, bool reverseSearchDirection)
{
    double minDist = glm::DistanceSquared(refPoint, curList[(startIndex + curList.count()) % curList.count()]);
    int minDistIndex = startIndex;

    int directionMultiplier = reverseSearchDirection ? 1 : -1;
    double distSoFar = 0;
    Vec3 start = curList[startIndex];

    // First: search forward in the direction of travel
    int offset = 1;

    while (offset < curList.count())
    {
        int pointIndex = (startIndex + (offset * directionMultiplier) + curList.count()) % curList.count();
        double dist = glm::DistanceSquared(refPoint, curList[pointIndex]);

        if (dist < minDist)
        {
            minDist = dist;
            minDistIndex = pointIndex;
        }

        distSoFar += glm::Distance(start, curList[pointIndex]);
        start = curList[pointIndex];

        offset++;

        if (distSoFar > minSearchDistance)
        {
            break;
        }
    }

    // Continue traversing forward until the distance starts growing
    while (offset < curList.count())
    {
        int pointIndex = (startIndex + (offset * directionMultiplier) + curList.count()) % curList.count();
        double dist = glm::DistanceSquared(refPoint, curList[pointIndex]);
        if (dist < minDist)
        {
            minDist = dist;
            minDistIndex = pointIndex;
        }
        else
        {
            break;
        }
        offset++;
    }

    // Only check backwards if we haven't found a good point forward
    double backwardSearchLimit = 3.0;
    distSoFar = 0;
    start = curList[startIndex];

    for (offset = 1; offset < curList.count() && distSoFar < backwardSearchLimit; offset++)
    {
        int pointIndex = (startIndex + (offset * (-directionMultiplier)) + curList.count()) % curList.count();

        distSoFar += glm::Distance(start, curList[pointIndex]);
        start = curList[pointIndex];

        if (distSoFar >= backwardSearchLimit)
            break;

        double dist = glm::DistanceSquared(refPoint, curList[pointIndex]);

        // Only accept backwards point if it's significantly closer (20% threshold)
        if (dist < minDist * 0.8)
        {
            minDist = dist;
            minDistIndex = pointIndex;
        }
        else
        {
            break;
        }
    }

    return minDistIndex;
}
