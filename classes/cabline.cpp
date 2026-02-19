#include "cabline.h"
#include "vec2.h"
#include "glm.h"
#include "cvehicle.h"
#include "cboundary.h"
#include "cyouturn.h"
#include "ctram.h"
#include "cahrs.h"
#include "ctrack.h"
#include "backend/backend.h"
#include <QOpenGLFunctions>
#include <QColor>
#include "glutils.h"
#include "cnmea.h"
#include "classes/settingsmanager.h"

//??? why does CABLine refer to mf.ABLine? Isn't there only one instance and
//thus was can just use "this."  If this is wrong, we'll remove this and fix it.

CABLine::CABLine(QObject *parent) : QObject(parent)
{
    //abLength = property_setAB_lineLength; ?
    abLength = 2000;
}

void CABLine::BuildCurrentABLineList(Vec3 pivot,
                                     double secondsSinceStart,
                                     CTrk &track,
                                     const CYouTurn &yt,
                                     const CVehicle &vehicle)
{
    double tool_width = SettingsManager::instance()->vehicle_toolWidth();
    double tool_overlap = SettingsManager::instance()->vehicle_toolOverlap();
    double tool_offset = SettingsManager::instance()->vehicle_toolOffset();

    double dx, dy;

    abHeading = track.heading;

    // ═════════════════════════════════════════════════════════════════════════════════
    // PHASE 6.0.43 BUG FIX: Calculate howManyPathsAway EVERY FRAME (not just on timeout)
    // Original bug: Calculation was trapped inside CONDITION 1, preventing real-time updates
    // ═════════════════════════════════════════════════════════════════════════════════

    // Extend AB line points (needed every frame for howManyPathsAway calculation)
    track.endPtA.easting = track.ptA.easting - (sin(abHeading) * abLength);
    track.endPtA.northing = track.ptA.northing - (cos(abHeading) * abLength);

    track.endPtB.easting = track.ptB.easting + (sin(abHeading) * abLength);
    track.endPtB.northing = track.ptB.northing + (cos(abHeading) * abLength);

    refNudgePtA = track.endPtA; refNudgePtB = track.endPtB;

    if (track.nudgeDistance != 0)
    {
        refNudgePtA.easting += (sin(abHeading + glm::PIBy2) * track.nudgeDistance);
        refNudgePtA.northing += (cos(abHeading + glm::PIBy2) * track.nudgeDistance);

        refNudgePtB.easting += (sin(abHeading + glm::PIBy2) * track.nudgeDistance);
        refNudgePtB.northing += (cos(abHeading + glm::PIBy2) * track.nudgeDistance);
    }

    widthMinusOverlap = tool_width - tool_overlap;

    // PHASE 6.0.43 CRITICAL FIX: Use UN-NUDGED track.endPtA/B for distance calculation
    // The nudge is applied ONLY in RefDist calculation below (line 87)
    // Using nudged points here causes double-nudge bug!
    dx = track.endPtB.easting - track.endPtA.easting;
    dy = track.endPtB.northing - track.endPtA.northing;

    distanceFromRefLine = ((dy * CVehicle::instance()->guidanceLookPos.easting) - (dx * CVehicle::instance()->guidanceLookPos.northing) +
                           (track.endPtB.easting * track.endPtA.northing) -
                           (track.endPtB.northing * track.endPtA.easting)) /
                          sqrt((dy * dy) + (dx * dx));

    distanceFromRefLine -= (0.5 * widthMinusOverlap);

    isLateralTriggered = false;

    isHeadingSameWay = M_PI - fabs(fabs(pivot.heading - abHeading) - M_PI) < glm::PIBy2;

    if (yt.isYouTurnTriggered && !yt.isGoingStraightThrough) isHeadingSameWay = !isHeadingSameWay;

    // Calculate which parallel line the vehicle is on (Phase 6.0.43: includes nudgeDistance)
    // THIS MUST BE CALCULATED EVERY FRAME FOR AUTO-SNAP TO WORK!
    double RefDist = (distanceFromRefLine
                      + (isHeadingSameWay ? tool_offset : -tool_offset)
                      - track.nudgeDistance) / widthMinusOverlap;

    if (RefDist < 0)
        howManyPathsAway = (int)(RefDist - 0.5);
    else
        howManyPathsAway = (int)(RefDist + 0.5);

    // ✅ PHASE 6.0.43: C# CONDITION 1 - Update timeout (used for other purposes)
    // C# CABLine.cs line 84: if (!isABValid || ((secondsSinceStart - lastSecond) > 0.66))
    if (!isABValid || ((secondsSinceStart - lastSecond) > 0.66))
    {
        lastSecond = secondsSinceStart;
    }

    // ✅ PHASE 6.0.43: C# CONDITION 2 - Reconstruct line if howManyPathsAway or direction changed
    // C# CABLine.cs line 124: if (!isABValid || howManyPathsAway != lastHowManyPathsAway ||
    //                             (isHeadingSameWay != lastIsHeadingSameWay && tool.offset != 0))
    if (!isABValid ||
        howManyPathsAway != lastHowManyPathsAway ||
        (isHeadingSameWay != lastIsHeadingSameWay && tool_offset != 0))
    {
        isABValid = true;
        lastHowManyPathsAway = howManyPathsAway;
        lastIsHeadingSameWay = isHeadingSameWay;

        widthMinusOverlap = tool_width - tool_overlap;

        // Calculate distance to parallel line (Phase 6.0.43: includes nudgeDistance)
        double distAway = widthMinusOverlap * howManyPathsAway;
        distAway += (isHeadingSameWay ? -tool_offset : tool_offset);
        distAway += track.nudgeDistance;
        distAway += (0.5 * widthMinusOverlap);

        shadowOffset = isHeadingSameWay ? tool_offset : -tool_offset;

        // Points on original AB line (without nudge for reconstruction)
        Vec2 point1((cos(-abHeading) * distAway) + track.ptA.easting,
                    (sin(-abHeading) * distAway) + track.ptA.northing);
        Vec2 point2((cos(-abHeading) * distAway) + track.ptB.easting,
                    (sin(-abHeading) * distAway) + track.ptB.northing);

        // Create the new current line extent points based on parallel offset
        currentLinePtA.easting = point1.easting - (sin(abHeading) * abLength);
        currentLinePtA.northing = point1.northing - (cos(abHeading) * abLength);

        currentLinePtB.easting = point2.easting + (sin(abHeading) * abLength);
        currentLinePtB.northing = point2.northing + (cos(abHeading) * abLength);

        currentLinePtA.heading = abHeading;
        currentLinePtB.heading = abHeading;
    }

    // Phase 6.0.43 BUG FIX: DO NOT modify howManyPathsAway here!
    // The +1 adjustment for display is now handled in TrackNum.qml (UI layer)
    // Modifying it here breaks CONDITION 2 comparison in subsequent frames
}

void CABLine::GetCurrentABLine(Vec3 pivot, Vec3 steer,
                               bool isBtnAutoSteerOn,
                               CVehicle &vehicle,
                               CYouTurn &yt,
                               const CAHRS &ahrs,
                               CNMEA &pn
                               )
{
    double dx, dy;
    double purePursuitIntegralGain = SettingsManager::instance()->vehicle_purePursuitIntegralGainAB();
    double wheelBase = SettingsManager::instance()->vehicle_wheelbase();
    double maxSteerAngle = SettingsManager::instance()->vehicle_maxSteerAngle();
    bool vehicle_isStanleyUsed = SettingsManager::instance()->vehicle_isStanleyUsed();
    double as_sideHillCompensation = SettingsManager::instance()->as_sideHillCompensation();

    //Check uturn first
    if (yt.isYouTurnTriggered && yt.DistanceFromYouTurnLine(pn))//do the pure pursuit from youTurn
    {
        //now substitute what it thinks are AB line values with auto turn values
        steerAngleAB = yt.steerAngleYT;
        distanceFromCurrentLinePivot = yt.distanceFromCurrentLine;

        goalPointAB = yt.goalPointYT;
        radiusPointAB.easting = yt.radiusPointYT.easting;
        radiusPointAB.northing = yt.radiusPointYT.northing;
        ppRadiusAB = yt.ppRadiusYT;

        CVehicle::instance()->modeTimeCounter = 0;
        CVehicle::instance()->set_modeActualXTE ( (distanceFromCurrentLinePivot));
    }

    //Stanley
    else if (vehicle_isStanleyUsed)
        Backend::instance()->gyd().StanleyGuidanceABLine(currentLinePtA, currentLinePtB, pivot, steer, isBtnAutoSteerOn, *CVehicle::instance(),*this, ahrs,yt);

    //Pure Pursuit
    else
    {
        //get the distance from currently active AB line
        //x2-x1
        dx = currentLinePtB.easting - currentLinePtA.easting;
        //z2-z1
        dy = currentLinePtB.northing - currentLinePtA.northing;

        //save a copy of dx,dy in youTurn
        yt.dxAB = dx; yt.dyAB = dy;

        //how far from current AB Line is fix
        distanceFromCurrentLinePivot = ((dy * pivot.easting) - (dx * pivot.northing) + (currentLinePtB.easting
                                                                                        * currentLinePtA.northing) - (currentLinePtB.northing * currentLinePtA.easting))
                                       / sqrt((dy * dy) + (dx * dx));

        //integral slider is set to 0
        if (purePursuitIntegralGain != 0 && !CVehicle::instance()->isReverse())
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

            if (isBtnAutoSteerOn
                && fabs(pivotDerivative) < (0.1)
                && CVehicle::instance()->avgSpeed() > 2.5
                && !yt.isYouTurnTriggered)
            //&& fabs(pivotDistanceError) < 0.2)

            {
                //if over the line heading wrong way, rapidly decrease integral
                if ((inty < 0 && distanceFromCurrentLinePivot < 0) || (inty > 0 && distanceFromCurrentLinePivot > 0))
                {
                    inty += pivotDistanceError * purePursuitIntegralGain * -0.04;
                }
                else
                {
                    if (fabs(distanceFromCurrentLinePivot) > 0.02)
                    {
                        inty += pivotDistanceError * purePursuitIntegralGain * -0.02;
                        if (inty > 0.2) inty = 0.2;
                        else if (inty < -0.2) inty = -0.2;
                    }
                }
            }
            else inty *= 0.95;
        }
        else inty = 0;

        //Subtract the two headings, if > 1.57 its going the opposite heading as refAB
        abFixHeadingDelta = (fabs(CVehicle::instance()->fixHeading() - abHeading));
        if (abFixHeadingDelta >= M_PI) abFixHeadingDelta = fabs(abFixHeadingDelta - glm::twoPI);

        // ** Pure pursuit ** - calc point on ABLine closest to current position
        double U = (((pivot.easting - currentLinePtA.easting) * dx)
                    + ((pivot.northing - currentLinePtA.northing) * dy))
                   / ((dx * dx) + (dy * dy));

        //point on AB line closest to pivot axle point
        rEastAB = currentLinePtA.easting + (U * dx);
        rNorthAB = currentLinePtA.northing + (U * dy);

        //update base on autosteer settings and distance from line
        double goalPointDistance = CVehicle::instance()->UpdateGoalPointDistance();

        if (CVehicle::instance()->isReverse() ? isHeadingSameWay : !isHeadingSameWay)
        {
            goalPointAB.easting = rEastAB - (sin(abHeading) * goalPointDistance);
            goalPointAB.northing = rNorthAB - (cos(abHeading) * goalPointDistance);
        }
        else
        {
            goalPointAB.easting = rEastAB + (sin(abHeading) * goalPointDistance);
            goalPointAB.northing = rNorthAB + (cos(abHeading) * goalPointDistance);
        }

        //calc "D" the distance from pivot axle to lookahead point
        double goalPointDistanceDSquared
            = glm::DistanceSquared(goalPointAB.northing, goalPointAB.easting, pivot.northing, pivot.easting);

        //calculate the the new x in local coordinates and steering angle degrees based on wheelbase
        double localHeading;

        if (isHeadingSameWay) localHeading = glm::twoPI - CVehicle::instance()->fixHeading() + inty;
        else localHeading = glm::twoPI - CVehicle::instance()->fixHeading() - inty;

        ppRadiusAB = goalPointDistanceDSquared / (2 * (((goalPointAB.easting - pivot.easting) * cos(localHeading))
                                                       + ((goalPointAB.northing - pivot.northing) * sin(localHeading))));

        steerAngleAB = glm::toDegrees(atan(2 * (((goalPointAB.easting - pivot.easting) * cos(localHeading))
                                                    + ((goalPointAB.northing - pivot.northing) * sin(localHeading))) * wheelBase
                                               / goalPointDistanceDSquared));

        if (ahrs.imuRoll != 88888)
            steerAngleAB += ahrs.imuRoll * -as_sideHillCompensation; /*mf.gyd.sideHillCompFactor;*/

        //steerAngleAB *= 1.4;

        if (steerAngleAB < -maxSteerAngle) steerAngleAB = -maxSteerAngle;

        if (steerAngleAB > maxSteerAngle) steerAngleAB = maxSteerAngle;

        //limit circle size for display purpose
        if (ppRadiusAB < -500) ppRadiusAB = -500;
        if (ppRadiusAB > 500) ppRadiusAB = 500;

        radiusPointAB.easting = pivot.easting + (ppRadiusAB * cos(localHeading));
        radiusPointAB.northing = pivot.northing + (ppRadiusAB * sin(localHeading));

        //if (mf.isConstantContourOn)
        //{
        //    //angular velocity in rads/sec  = 2PI * m/sec * radians/meters

        //    //clamp the steering angle to not exceed safe angular velocity
        //    if famf.setAngVel) > 1000)
        //    {
        //        //mf.setAngVel = mf.setAngVel < 0 ? -mf.CVehicle::instance()->maxAngularVelocity : mf.CVehicle::instance()->maxAngularVelocity;
        //        mf.setAngVel = mf.setAngVel < 0 ? -1000 : 1000;
        //    }
        //}

        //distance is negative if on left, positive if on right
        if (!isHeadingSameWay)
            distanceFromCurrentLinePivot *= -1.0;

        //used for acquire/hold mode
        CVehicle::instance()->set_modeActualXTE ( (distanceFromCurrentLinePivot));

        double steerHeadingError = (pivot.heading - abHeading);
        //Fix the circular error
        if (steerHeadingError > M_PI)
            steerHeadingError -= M_PI;
        else if (steerHeadingError < -M_PI)
            steerHeadingError += M_PI;

        if (steerHeadingError > glm::PIBy2)
            steerHeadingError -= M_PI;
        else if (steerHeadingError < -glm::PIBy2)
            steerHeadingError += M_PI;

        CVehicle::instance()->set_modeActualHeadingError ( glm::toDegrees(steerHeadingError));

        //Convert to millimeters
        CVehicle::instance()->set_guidanceLineDistanceOff((short)glm::roundMidAwayFromZero(distanceFromCurrentLinePivot * 1000.0));
        CVehicle::instance()->guidanceLineSteerAngle = (short)(steerAngleAB * 100);
    }

    //mf.setAngVel = 0.277777 * mf.avgSpeed * (Math.Tan(glm::toRadians(steerAngleAB))) / mf.CVehicle::instance()->wheelbase;
    //mf.setAngVel = glm::toDegrees(mf.setAngVel);
}
void CABLine::DrawABLineNew(QOpenGLFunctions *gl, const QMatrix4x4 &mvp)
{
    GLHelperOneColor gldraw;
    QColor color;
    double lineWidth = SettingsManager::instance()->display_lineWidth();
    gl->glLineWidth(lineWidth);
    gldraw.append(QVector3D(desLineEndA.easting, desLineEndA.northing, 0.0));
    gldraw.append(QVector3D(desLineEndB.easting, desLineEndB.northing, 0.0));
    gldraw.draw(gl, mvp, QColor::fromRgbF(0.95f, 0.70f, 0.50f), GL_LINES, lineWidth);
    gl->glLineWidth(1);
    color.setRgbF(0.2f, 0.950f, 0.20f);
    drawText3D(gl,mvp, desPtA.easting, desPtA.northing, "&A", 1.0, true, color);
    if (isDesPtBSet)
        drawText3D(gl,mvp, desPtB.easting, desPtB.northing, "&B", 1.0, true, color);
}

void CABLine::DrawABLines(QOpenGLFunctions *gl, const QMatrix4x4 &mvp,
                          bool isFontOn,
                          bool isRateMapOn,
                          double camSetDistance,
                          const CTrk &track,
                          CYouTurn &yt)
{
    double tool_toolWidth = SettingsManager::instance()->vehicle_toolWidth();
    double tool_toolOverlap = SettingsManager::instance()->vehicle_toolOverlap();
    double tool_toolOffset = SettingsManager::instance()->vehicle_toolOffset();
    bool isStanleyUsed = SettingsManager::instance()->vehicle_isStanleyUsed();
    bool isSideGuideLines = SettingsManager::instance()->menu_isSideGuideLines();
    double lineWidth = SettingsManager::instance()->display_lineWidth();
    widthMinusOverlap = tool_toolWidth - tool_toolOverlap;

    GLHelperOneColor gldraw;
    GLHelperColors gldraw1;
    ColorVertex cv;
    QColor color;

    //Draw AB Points
    cv.color = QVector4D(0.95f, 0.0f, 0.0f, 1.0);
    cv.vertex = QVector3D(track.ptB.easting, track.ptB.northing, 0.0);
    gldraw1.append(cv);
    cv.color = QVector4D(0.0f, 0.90f, 0.95f, 1.0);
    cv.vertex = QVector3D(track.ptA.easting, track.ptA.northing, 0.0);
    gldraw1.append(cv);
    //cv.color = QVector4D(0.00990f, 0.990f, 0.095f,1.0);
    //cv.vertex = QVector3D(bnd.iE, bnd.iN, 0.0);
    //gldraw1.append(cv);
    gldraw1.draw(gl,mvp,GL_POINTS,8.0f);

    if (isFontOn && !isMakingABLine)
    {
        color.setRgbF(0.00990f, 0.990f, 0.095f);
        drawText3D(gl,mvp, track.ptA.easting, track.ptA.northing, "&A", 1.0, true, color);
        drawText3D(gl,mvp, track.ptB.easting, track.ptB.northing, "&B", 1.0, true, color);
    }

    //Draw reference AB line
    color.setRgbF(0.930f, 0.2f, 0.2f);
    gldraw.clear();
    gldraw.append(QVector3D(track.endPtA.easting, track.endPtA.northing, 0));
    gldraw.append(QVector3D(track.endPtB.easting, track.endPtB.northing, 0));

    //TODO: figure out a way to make the line dashed
    gl->glLineWidth(lineWidth);
    gldraw.draw(gl, mvp, color, GL_LINES, 4.0f);
    gl->glLineWidth(1);

    if (!isRateMapOn)
    {
        double sinHR = sin(abHeading + glm::PIBy2) * (widthMinusOverlap * 0.5 + shadowOffset);
        double cosHR = cos(abHeading + glm::PIBy2) * (widthMinusOverlap * 0.5 + shadowOffset);
        double sinHL = sin(abHeading + glm::PIBy2) * (widthMinusOverlap * 0.5 - shadowOffset);
        double cosHL = cos(abHeading + glm::PIBy2) * (widthMinusOverlap * 0.5 - shadowOffset);

        //shadow
        color.setRgbF(0.5, 0.5, 0.5, 0.3);

        gldraw.clear();
        gldraw.append(QVector3D(currentLinePtA.easting - sinHL, currentLinePtA.northing - cosHL, 0));
        gldraw.append(QVector3D(currentLinePtA.easting + sinHR, currentLinePtA.northing + cosHR, 0));
        gldraw.append(QVector3D(currentLinePtB.easting + sinHR, currentLinePtB.northing + cosHR, 0));
        gldraw.append(QVector3D(currentLinePtB.easting - sinHL, currentLinePtB.northing - cosHR, 0));

        gldraw.draw(gl,mvp,color,GL_TRIANGLE_FAN,lineWidth);

        //shadow lines
        color.setRgbF(0.55, 0.55, 0.55, 0.3);
        gldraw.clear();
        gldraw.append(QVector3D(currentLinePtA.easting - sinHL, currentLinePtA.northing - cosHL, 0));
        gldraw.append(QVector3D(currentLinePtA.easting + sinHR, currentLinePtA.northing + cosHR, 0));
        gldraw.append(QVector3D(currentLinePtB.easting + sinHR, currentLinePtB.northing + cosHR, 0));
        gldraw.append(QVector3D(currentLinePtB.easting - sinHL, currentLinePtB.northing - cosHR, 0));

        gldraw.draw(gl,mvp,color,GL_LINE_LOOP,1.0f);

    }

    //draw current AB line
    color.setRgbF(0.95, 0.2f, 0.950f);
    gldraw.clear();
    gl->glLineWidth(lineWidth);
    gldraw.append(QVector3D(currentLinePtA.easting, currentLinePtA.northing, 0));
    gldraw.append(QVector3D(currentLinePtB.easting, currentLinePtB.northing, 0));
    gldraw.draw(gl,mvp,color,GL_LINES,lineWidth);
    gl->glLineWidth(1);

    if (isSideGuideLines && camSetDistance > tool_toolWidth * -120)
    {
        //get the tool offset and width
        double toolOffset = tool_toolOffset * 2;
        double toolWidth = tool_toolWidth - tool_toolOverlap;
        double cosHeading = cos(-abHeading);
        double sinHeading = sin(-abHeading);

        color.setRgbF(0.756f, 0.7650f, 0.7650f);
        gldraw.clear();

        /*
                for (double i = -2.5; i < 3; i++)
                {
                    GL.Vertex3((cosHeading * ((mf.tool.toolWidth - mf.tool.toolOverlap) * (howManyPathsAway + i))) + refPoint1.easting, (sinHeading * ((mf.tool.toolWidth - mf.tool.toolOverlap) * (howManyPathsAway + i))) + refPoint1.northing, 0);
                    GL.Vertex3((cosHeading * ((mf.tool.toolWidth - mf.tool.toolOverlap) * (howManyPathsAway + i))) + refPoint2.easting, (sinHeading * ((mf.tool.toolWidth - mf.tool.toolOverlap) * (howManyPathsAway + i))) + refPoint2.northing, 0);
                }
                */

        if (isHeadingSameWay)
        {
            gldraw.append(QVector3D((cosHeading * (toolWidth + toolOffset)) + currentLinePtA.easting, (sinHeading * (toolWidth + toolOffset)) + currentLinePtA.northing, 0));
            gldraw.append(QVector3D((cosHeading * (toolWidth + toolOffset)) + currentLinePtB.easting, (sinHeading * (toolWidth + toolOffset)) + currentLinePtB.northing, 0));
            gldraw.append(QVector3D((cosHeading * (-toolWidth + toolOffset)) + currentLinePtA.easting, (sinHeading * (-toolWidth + toolOffset)) + currentLinePtA.northing, 0));
            gldraw.append(QVector3D((cosHeading * (-toolWidth + toolOffset)) + currentLinePtB.easting, (sinHeading * (-toolWidth + toolOffset)) + currentLinePtB.northing, 0));

            toolWidth *= 2;
            gldraw.append(QVector3D((cosHeading * toolWidth) + currentLinePtA.easting, (sinHeading * toolWidth) + currentLinePtA.northing, 0));
            gldraw.append(QVector3D((cosHeading * toolWidth) + currentLinePtB.easting, (sinHeading * toolWidth) + currentLinePtB.northing, 0));
            gldraw.append(QVector3D((cosHeading * (-toolWidth)) + currentLinePtA.easting, (sinHeading * (-toolWidth)) + currentLinePtA.northing, 0));
            gldraw.append(QVector3D((cosHeading * (-toolWidth)) + currentLinePtB.easting, (sinHeading * (-toolWidth)) + currentLinePtB.northing, 0));
        }

        else
        {
            gldraw.append(QVector3D((cosHeading * (toolWidth - toolOffset)) + currentLinePtA.easting, (sinHeading * (toolWidth - toolOffset)) + currentLinePtA.northing, 0));
            gldraw.append(QVector3D((cosHeading * (toolWidth - toolOffset)) + currentLinePtB.easting, (sinHeading * (toolWidth - toolOffset)) + currentLinePtB.northing, 0));
            gldraw.append(QVector3D((cosHeading * (-toolWidth - toolOffset)) + currentLinePtA.easting, (sinHeading * (-toolWidth - toolOffset)) + currentLinePtA.northing, 0));
            gldraw.append(QVector3D((cosHeading * (-toolWidth - toolOffset)) + currentLinePtB.easting, (sinHeading * (-toolWidth - toolOffset)) + currentLinePtB.northing, 0));

            toolWidth *= 2;
            gldraw.append(QVector3D((cosHeading * toolWidth) + currentLinePtA.easting, (sinHeading * toolWidth) + currentLinePtA.northing, 0));
            gldraw.append(QVector3D((cosHeading * toolWidth) + currentLinePtB.easting, (sinHeading * toolWidth) + currentLinePtB.northing, 0));
            gldraw.append(QVector3D((cosHeading * (-toolWidth)) + currentLinePtA.easting, (sinHeading * (-toolWidth)) + currentLinePtA.northing, 0));
            gldraw.append(QVector3D((cosHeading * (-toolWidth)) + currentLinePtB.easting, (sinHeading * (-toolWidth)) + currentLinePtB.northing, 0));
        }
        gl->glLineWidth(lineWidth);
        gldraw.draw(gl,mvp,color,GL_LINES,lineWidth);
        gl->glLineWidth(1);
    }

    if (!isStanleyUsed && camSetDistance > -200)
    {
        //Draw lookahead Point
        gldraw.clear();
        color.setRgbF(1.0f, 1.0f, 0.0f);
        gldraw.append(QVector3D(goalPointAB.easting, goalPointAB.northing, 0.0));
        gldraw.append(QVector3D(Backend::instance()->gyd().rEastSteer, Backend::instance()->gyd().rNorthSteer, 0.0));
        gldraw.append(QVector3D(Backend::instance()->gyd().rEastPivot, Backend::instance()->gyd().rNorthPivot, 0.0));
        gldraw.draw(gl,mvp,color,GL_POINTS,8.0f);

        if (ppRadiusAB < 50 && ppRadiusAB > -50)
        {
            const int numSegments = 200;
            double theta = glm::twoPI / numSegments;
            double c = cos(theta);//precalculate the sine and cosine
            double s = sin(theta);
            //double x = ppRadiusAB;//we start at angle = 0
            double x = 0;//we start at angle = 0
            double y = 0;

            gldraw.clear();
            color.setRgbF(0.53f, 0.530f, 0.950f);
            for (int ii = 0; ii < numSegments - 15; ii++)
            {
                //glVertex2f(x + cx, y + cy);//output vertex
                gldraw.append(QVector3D(x + radiusPointAB.easting, y + radiusPointAB.northing, 0));//output vertex
                double t = x;//apply the rotation matrix
                x = (c * x) - (s * y);
                y = (s * t) + (c * y);
            }
            gldraw.draw(gl,mvp,color,GL_LINES,2.0f);
        }
    }

    yt.DrawYouTurn(gl,mvp);
}

void CABLine::BuildTram(const CTrk &track, CBoundary &bnd, CTram &tram)
{
    double tramWidth = SettingsManager::instance()->tram_width();
    double tool_halfWidth = (SettingsManager::instance()->vehicle_toolWidth() - SettingsManager::instance()->vehicle_toolOverlap()) / 2.0;
    double halfWheelTrack = SettingsManager::instance()->vehicle_trackWidth() * 0.5;
    int tram_passes = SettingsManager::instance()->tram_passes();

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

    QVector<Vec2> tramRef;

    bool isBndExist = bnd.bndList.count() != 0;

    abHeading = track.heading;

    double hsin = sin(abHeading);
    double hcos = cos(abHeading);

    double len = glm::Distance(track.endPtA, track.endPtB);
    //divide up the AB line into segments
    Vec2 P1;
    for (int i = 0; i < (int)len; i += 4)
    {
        P1.easting = (hsin * i) + track.endPtA.easting;
        P1.northing = (hcos * i) + track.endPtA.northing;
        tramRef.append(P1);
    }

    //create list of list of points of triangle strip of AB Highlight
    double headingCalc = abHeading + glm::PIBy2;
    hsin = sin(headingCalc);
    hcos = cos(headingCalc);

    tram.tramList.clear();
    tram.tramArr->clear();

    //no boundary starts on first pass
    int cntr = 0;
    if (isBndExist)
    {
        if (tram.generateMode == 1)
            cntr = 0;
        else
            cntr = 1;
    }

    double widd = 0;

    for (int i = cntr; i < tram_passes; i++)
    {
        tram.tramArr = QSharedPointer<QVector<Vec2>>(new QVector<Vec2>());
        tram.tramList.append(tram.tramArr);

        widd = (tramWidth * 0.5) - tool_halfWidth - halfWheelTrack;
        widd += (tramWidth * i);

        for (int j = 0; j < tramRef.count(); j++)
        {
            P1.easting = hsin * widd + tramRef[j].easting;
            P1.northing = (hcos * widd) + tramRef[j].northing;

            if (!isBndExist || glm::IsPointInPolygon(bnd.bndList[0].fenceLineEar,P1))
            {
                tram.tramArr->append(P1);
            }
        }
    }

    for (int i = cntr; i < tram_passes; i++)
    {
        tram.tramArr = QSharedPointer<QVector<Vec2>>(new QVector<Vec2>());
        tram.tramList.append(tram.tramArr);

        widd = (tramWidth * 0.5) - tool_halfWidth + halfWheelTrack;
        widd += (tramWidth * i);

        for (int j = 0; j < tramRef.count(); j++)
        {
            P1.easting = (hsin * widd) + tramRef[j].easting;
            P1.northing = (hcos * widd) + tramRef[j].northing;

            if (!isBndExist || glm::IsPointInPolygon(bnd.bndList[0].fenceLineEar,P1))
            {
                tram.tramArr->append(P1);
            }
        }
    }

    tramRef.clear();
    //outside tram

    if (bnd.bndList.count() == 0 || tram_passes != 0)
    {
        //return;
    }
}
