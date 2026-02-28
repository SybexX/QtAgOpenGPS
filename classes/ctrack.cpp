#include <QOpenGLFunctions>
#include <QMatrix4x4>
#include <QVector>
#include <QFuture>
#include <QtConcurrent/QtConcurrent>
#include <vector>
#include "glutils.h"
#include "ctrack.h"
#include "cvehicle.h"
#include "glm.h"
#include "cabcurve.h"
#include "cabline.h"
#include "classes/settingsmanager.h"
#include "cyouturn.h"
#include "cboundary.h"
#include "ctram.h"
#include "cahrs.h"
#include "backend/backend.h"

// Approche SomcoSoftware : Qt gère le singleton automatiquement

CTrk::CTrk()
{
    heading = 3;
}

CTrk::CTrk(const CTrk &orig)
{
    curvePts = orig.curvePts;
    heading = orig.heading;
    name = orig.name;
    isVisible = orig.isVisible;
    ptA = orig.ptA;
    ptB = orig.ptB;
    endPtA = orig.endPtA;
    endPtB = orig.endPtB;
    mode = orig.mode;
    nudgeDistance = orig.nudgeDistance;
}

CTrack::CTrack(QObject* parent) : QAbstractListModel(parent)
{
    qDebug() << "🏗️ CTrack constructor called, parent:" << parent;
    // Initialize role names
    m_roleNames[index] = "index";
    m_roleNames[NameRole] = "name";
    m_roleNames[IsVisibleRole] = "isVisible";
    m_roleNames[ModeRole] = "mode";
    m_roleNames[ptA] = "ptA";
    m_roleNames[ptB] = "ptB";
    m_roleNames[endPtA] = "endPtA";
    m_roleNames[endPtB] = "endPtB";
    m_roleNames[nudgeDistance] = "nudgeDistance";

    // Initialize Qt 6.8 Q_OBJECT_BINDABLE_PROPERTY members
    m_idx = -1;  // Will be set again by setIdx call
    m_newMode = 0;
    m_newRefSide = 0;
    m_howManyPathsAway = 0;
    m_mode = 0;
    m_count = 0;
    m_isAutoTrack = false;
    m_isAutoSnapToPivot = false;  // Match C# default
    m_isAutoSnapped = false;
    m_newHeading = 0.0;
    m_newName = "";
    m_currentName = "";

    // PHASE 6.0.42.9 BUG #0 FIX: Initialize auto-track timer
    // C# auto-initializes int to 0, but C++ leaves uninitialized (garbage value)
    // Garbage value could be negative → timer never reaches >= 1 threshold
    // This was the ROOT CAUSE preventing auto-track from working
    autoTrack3SecTimer = 0;

    setIdx(-1);

    m_tracksProperties = new TracksProperties(this);
}

CTrack::~CTrack()
{
    setIdx(-1);
    gArr.clear();
    reloadModel();
}

int CTrack::FindClosestRefTrack(Vec3 pivot, const CVehicle &vehicle)
{
    if (idx() < 0 || gArr.count() == 0) return -1;

    //only 1 track
    if (gArr.count() == 1) return idx();

    int trak = -1;
    int cntr = 0;

    //Count visible
    for (int i = 0; i < gArr.count(); i++)
    {
        if (gArr[i].isVisible)
        {
            cntr++;
            trak = i;
        }
    }

    //only 1 track visible of the group
    if (cntr == 1) return trak;

    //no visible tracks
    if (cntr == 0) return -1;

    //determine if any aligned reasonably close
    std::vector<bool> isAlignedArr(gArr.count());
    for (int i = 0; i < gArr.count(); i++)
    {
        if (gArr[i].mode == (int)TrackMode::Curve) isAlignedArr[i] = true;
        else
        {
            double diff = M_PI - fabs(fabs(pivot.heading - gArr[i].heading) - M_PI);
            if ((diff < 1) || (diff > 2.14))
                isAlignedArr[i] = true;
            else
                isAlignedArr[i] = false;
        }
    }

    double minDistA = glm::DOUBLE_MAX;
    double dist;

    Vec2 endPtA, endPtB;

    for (int i = 0; i < gArr.count(); i++)
    {
        if (!isAlignedArr[i]) continue;
        if (!gArr[i].isVisible) continue;

        if (gArr[i].mode == (int)TrackMode::AB)
        {
            double abHeading = gArr[i].heading;

            endPtA.easting = gArr[i].ptA.easting - (sin(abHeading) * 2000);
            endPtA.northing = gArr[i].ptA.northing - (cos(abHeading) * 2000);

            endPtB.easting = gArr[i].ptB.easting + (sin(abHeading) * 2000);
            endPtB.northing = gArr[i].ptB.northing + (cos(abHeading) * 2000);

            //x2-x1
            double dx = endPtB.easting - endPtA.easting;
            //z2-z1
            double dy = endPtB.northing - endPtA.northing;

            dist = ((dy * CVehicle::instance()->steerAxlePos.easting) - (dx * CVehicle::instance()->steerAxlePos.northing) +
                    (endPtB.easting * endPtA.northing) - (endPtB.northing * endPtA.easting))
                   / sqrt((dy * dy) + (dx * dx));

            dist *= dist;

            if (dist < minDistA)
            {
                minDistA = dist;
                trak = i;
            }
        }
        else
        {
            for (int j = 0; j < gArr[i].curvePts.count(); j ++)
            {

                dist = glm::DistanceSquared(gArr[i].curvePts[j], pivot);

                if (dist < minDistA)
                {
                    minDistA = dist;
                    trak = i;
                }
            }
        }
    }

    return trak;
}

void CTrack::SwitchToClosestRefTrack(Vec3 pivot, const CVehicle &vehicle)
{
    int new_idx;

    new_idx = FindClosestRefTrack(pivot, *CVehicle::instance());
    if (new_idx >= 0 && new_idx != idx()) {
        setIdx(new_idx);
        curve.isCurveValid = false;
        ABLine.isABValid = false;
    }
}

void CTrack::NudgeTrack(double dist)
{
    if (idx() > -1)
    {
        if (gArr[idx()].mode == (int)TrackMode::AB)
        {
            ABLine.isABValid = false;
            ABLine.lastHowManyPathsAway = 98888;  // Phase 6.0.43: Reset sentinel for conditional reconstruction
            ABLine.lastSecond = 0;
            gArr[idx()].nudgeDistance += ABLine.isHeadingSameWay ? dist : -dist;
        }
        else
        {
            curve.isCurveValid = false;
            curve.lastHowManyPathsAway = 98888;
            curve.lastSecond = 0;
            gArr[idx()].nudgeDistance += curve.isHeadingSameWay ? dist : -dist;
        }

        //if (gArr[idx()].nudgeDistance > 0.5 * mf.tool.width) gArr[idx()].nudgeDistance -= mf.tool.width;
        //else if (gArr[idx()].nudgeDistance < -0.5 * mf.tool.width) gArr[idx()].nudgeDistance += mf.tool.width;
    }
}

void CTrack::NudgeDistanceReset()
{
    if (idx() > -1 && gArr.count() > 0)
    {
        if (gArr[idx()].mode == (int)TrackMode::AB)
        {
            ABLine.isABValid = false;
            ABLine.lastHowManyPathsAway = 98888;  // Phase 6.0.43: Reset sentinel for conditional reconstruction
            ABLine.lastSecond = 0;
        }
        else
        {
            curve.isCurveValid = false;
            curve.lastHowManyPathsAway = 98888;
            curve.lastSecond = 0;
        }

        gArr[idx()].nudgeDistance = 0;
    }
}

void CTrack::SnapToPivot()
{
    //if (isBtnGuidanceOn)

    if (idx() > -1)
    {
        if (gArr[idx()].mode == (int)(TrackMode::AB))
        {
            NudgeTrack(ABLine.distanceFromCurrentLinePivot);

        }
        else
        {
            NudgeTrack(curve.distanceFromCurrentLinePivot);
        }

    }
}

void CTrack::NudgeRefTrack(double dist)
{
    if (idx() > -1)
    {
        if (gArr[idx()].mode == (int)TrackMode::AB)
        {
            ABLine.isABValid = false;
            ABLine.lastHowManyPathsAway = 98888;  // Phase 6.0.43: Reset sentinel for conditional reconstruction
            ABLine.lastSecond = 0;
            NudgeRefABLine( gArr[idx()], ABLine.isHeadingSameWay ? dist : -dist);
        }
        else
        {
            curve.isCurveValid = false;
            curve.lastHowManyPathsAway = 98888;
            curve.lastSecond = 0;
            NudgeRefCurve( gArr[idx()], curve.isHeadingSameWay ? dist : -dist);
        }
    }
}

void CTrack::NudgeRefABLine(CTrk &track, double dist)
{
    double head = track.heading;

    track.ptA.easting += (sin(head+glm::PIBy2) * (dist));
    track.ptA.northing += (cos(head + glm::PIBy2) * (dist));

    track.ptB.easting += (sin(head + glm::PIBy2) * (dist));
    track.ptB.northing += (cos(head + glm::PIBy2) * (dist));
}

void CTrack::NudgeRefCurve(CTrk &track, double distAway)
{
    curve.isCurveValid = false;
    curve.lastHowManyPathsAway = 98888;
    curve.lastSecond = 0;

    QVector<Vec3> curList;

    double distSqAway = (distAway * distAway) - 0.01;
    Vec3 point;

    for (int i = 0; i < track.curvePts.count(); i++)
    {
        point = Vec3(
            track.curvePts[i].easting + (sin(glm::PIBy2 + track.curvePts[i].heading) * distAway),
            track.curvePts[i].northing + (cos(glm::PIBy2 + track.curvePts[i].heading) * distAway),
            track.curvePts[i].heading);
        bool Add = true;

        for (int t = 0; t < track.curvePts.count(); t++)
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
            if (curList.count() > 0)
            {
                double dist = ((point.easting - curList[curList.count() - 1].easting) * (point.easting - curList[curList.count() - 1].easting))
                + ((point.northing - curList[curList.count() - 1].northing) * (point.northing - curList[curList.count() - 1].northing));
                if (dist > 1.0)
                    curList.append(point);
            }
            else curList.append(point);
        }
    }

    int cnt = curList.count();
    if (cnt > 6)
    {
        QVector<Vec3> arr = curList;
        curList.clear();

        for (int i = 0; i < (arr.count() - 1); i++)
        {
            arr[i].heading = atan2(arr[i + 1].easting - arr[i].easting, arr[i + 1].northing - arr[i].northing);
            if (arr[i].heading < 0) arr[i].heading += glm::twoPI;
            if (arr[i].heading >= glm::twoPI) arr[i].heading -= glm::twoPI;
        }

        arr[arr.count() - 1].heading = arr[arr.count() - 2].heading;

        //replace the array
        cnt = arr.count();
        double distance;
        double spacing = 1.2;

        //add the first point of loop - it will be p1
        curList.append(arr[0]);

        for (int i = 0; i < cnt - 3; i++)
        {
            // add p2
            curList.append(arr[i + 1]);

            distance = glm::Distance(arr[i + 1], arr[i + 2]);

            if (distance > spacing)
            {
                int loopTimes = (int)(distance / spacing + 1);
                for (int j = 1; j < loopTimes; j++)
                {
                    Vec3 pos(glm::Catmull(j / (double)(loopTimes), arr[i], arr[i + 1], arr[i + 2], arr[i + 3]));
                    curList.append(pos);
                }
            }
        }

        curList.append(arr[cnt - 2]);
        curList.append(arr[cnt - 1]);

        curve.CalculateHeadings(curList);

        track.curvePts = curList;
        //track.curvePts.clear();

        //for (auto item: curList)
        //{
        //    track.curvePts.append(new vec3(item));
        //}

        //for (int i = 0; i < cnt; i++)
        //{
        //    arr[i].easting += cos(arr[i].heading) * (dist);
        //    arr[i].northing -= sin(arr[i].heading) * (dist);
        //    track.curvePts.append(arr[i]);
        //}
    }
}

void CTrack::DrawTrackNew(QOpenGLFunctions *gl, const QMatrix4x4 &mvp)
{
    GLHelperOneColor gldraw;
    QColor color;
    double lineWidth = SettingsManager::instance()->display_lineWidth();

    if (ABLine.isMakingABLine && newMode() == TrackMode::AB) {
        ABLine.DrawABLineNew(gl, mvp);

        gldraw.append(QVector3D(designRefLine[0].easting, designRefLine[0].northing, 0.0));
        gldraw.append(QVector3D(designRefLine[1].easting, designRefLine[1].northing, 0.0));
        color.setRgbF(0.930f, 0.4f, 0.4f);
        gldraw.draw(gl, mvp, color, GL_LINES, lineWidth);

    } else if (curve.isMakingCurve && newMode() == TrackMode::Curve) {
        curve.DrawCurveNew(gl, mvp);
    }
}

void CTrack::DrawTrack(QOpenGLFunctions *gl,
                       const QMatrix4x4 &mvp,
                       bool isFontOn, bool isRateMapOn,
                       double camSetDistance,
                       CYouTurn &yt)
{
    if (idx() >= 0) {
        if (gArr[idx()].mode == TrackMode::AB)
            ABLine.DrawABLines(gl, mvp, isFontOn, isRateMapOn, camSetDistance, gArr[idx()], yt);
        else if (gArr[idx()].mode == TrackMode::Curve)
            curve.DrawCurve(gl, mvp, isFontOn, camSetDistance, gArr[idx()], yt);
    }
}

void CTrack::DrawTrackGoalPoint(QOpenGLFunctions *gl,
                                const QMatrix4x4 &mvp)
{
    GLHelperOneColor gldraw1;
    QColor color;

    if (idx() >= 0) {
        color.setRgbF(0.98, 0.98, 0.098);
        if (gArr[idx()].mode == TrackMode::AB) {
            gldraw1.append(QVector3D(ABLine.goalPointAB.easting, ABLine.goalPointAB.northing, 0));
            gldraw1.draw(gl,mvp,QColor::fromRgbF(0,0,0),GL_POINTS,16);
            gldraw1.draw(gl,mvp,color,GL_POINTS,10);
        } else if (gArr[idx()].mode == TrackMode::Curve) {
            gldraw1.append(QVector3D(curve.goalPointCu.easting, curve.goalPointCu.northing, 0));
            gldraw1.draw(gl,mvp,QColor::fromRgbF(0,0,0),GL_POINTS,16);
            gldraw1.draw(gl,mvp,color,GL_POINTS,10);
        }
    }
}

void CTrack::BuildCurrentLine(Vec3 pivot, double secondsSinceStart,
                              bool isBtnAutoSteerOn,
                              CYouTurn &yt,
                              CVehicle &vehicle,
                              const CBoundary &bnd,
                              const CAHRS &ahrs,
                              CNMEA &pn)
{
    if (gArr.count() > 0 && idx() > -1)
    {
        if (gArr[idx()].mode == TrackMode::AB)
        {
            ABLine.BuildCurrentABLineList(pivot,secondsSinceStart,gArr[idx()],yt,*CVehicle::instance());

            ABLine.GetCurrentABLine(pivot, CVehicle::instance()->steerAxlePos,isBtnAutoSteerOn,*CVehicle::instance(),yt,ahrs,pn);

            // Update QML property for parallel line number display
            setHowManyPathsAway(ABLine.howManyPathsAway);
        }
        else
        {
            //build new current ref line if required
            curve.BuildCurveCurrentList(pivot, secondsSinceStart,*CVehicle::instance(),gArr[idx()],bnd,yt);

            curve.GetCurrentCurveLine(pivot, CVehicle::instance()->steerAxlePos,isBtnAutoSteerOn,*CVehicle::instance(),gArr[idx()],yt,ahrs,pn);

            // Update QML property for parallel line number display
            setHowManyPathsAway(curve.howManyPathsAway);
        }
    }
    // Qt 6.8 Q_OBJECT_BINDABLE_PROPERTY: howManyPathsAway automatically emits when changed
}

void CTrack::ResetCurveLine()
{
    if (idx() >=0 && gArr[idx()].mode == TrackMode::Curve) {
        curve.curList.clear();
        setIdx(-1);
    }
}

void CTrack::AddPathPoint(Vec3 point)
{
    if (curve.isMakingCurve) {
        curve.desList.append(point);
    } else if (ABLine.isMakingABLine && !ABLine.isDesPtBSet) {
        //set the B point to current so we can draw a preview line
        ABLine.desPtB.easting = point.easting;
        ABLine.desPtB.northing = point.northing;

        update_ab_refline();
    }
}

int CTrack::getHowManyPathsAway()
{
    if (idx() >= 0) {
        if (gArr[idx()].mode == TrackMode::AB)
            return ABLine.howManyPathsAway;
        else
            return curve.howManyPathsAway;
    }

    return 0;
}

void CTrack::setIdx(int new_idx)
{
    // Allow -1 for deselection or valid indices within bounds
    if (new_idx == -1 || (new_idx >= 0 && new_idx < gArr.count())) {
        m_idx = new_idx;
        // Qt 6.8 Q_OBJECT_BINDABLE_PROPERTY automatically emits idxChanged()
        // Manually emit related signals that depend on idx
        emit modeChanged();
        emit currentNameChanged();
    }
}

int CTrack::newMode(void)
{
    return m_newMode;
}

void CTrack::setNewMode(int new_mode)
{
    m_newMode = new_mode;
    newTrack.mode = new_mode;
}

QString CTrack::newName(void)
{
    return m_newName;
}

void CTrack::setNewName(QString new_name)
{
    m_newName = new_name;
    if (m_newMode == TrackMode::AB)
        ABLine.desName = new_name;
    else
        curve.desName = new_name;
}




QString CTrack::getCurrentName(void)
{
    if (idx() > -1) {
        return gArr[idx()].name;
    } else {
        return "";
    }
}

// ===== Qt 6.8 Q_OBJECT_BINDABLE_PROPERTY Implementations =====

// idx property
int CTrack::idx() const { return m_idx; }
QBindable<int> CTrack::bindableIdx() { return QBindable<int>(&m_idx); }

// isAutoTrack property
bool CTrack::isAutoTrack() const { return m_isAutoTrack; }
void CTrack::setIsAutoTrack(bool value) { m_isAutoTrack = value; }
QBindable<bool> CTrack::bindableIsAutoTrack() { return QBindable<bool>(&m_isAutoTrack); }

// isAutoSnapToPivot property
bool CTrack::isAutoSnapToPivot() const { return m_isAutoSnapToPivot; }
void CTrack::setIsAutoSnapToPivot(bool value) { m_isAutoSnapToPivot = value; }
QBindable<bool> CTrack::bindableIsAutoSnapToPivot() { return QBindable<bool>(&m_isAutoSnapToPivot); }

// isAutoSnapped property
bool CTrack::isAutoSnapped() const { return m_isAutoSnapped; }
void CTrack::setIsAutoSnapped(bool value) { m_isAutoSnapped = value; }
QBindable<bool> CTrack::bindableIsAutoSnapped() { return QBindable<bool>(&m_isAutoSnapped); }

// Bindables for properties with existing getters/setters
QBindable<QString> CTrack::bindableNewName() { return QBindable<QString>(&m_newName); }
QBindable<int> CTrack::bindableNewMode() { return QBindable<int>(&m_newMode); }
QBindable<double> CTrack::bindableNewHeading() { return QBindable<double>(&m_newHeading); }

// newRefSide property (getter and bindable only, setter exists with business logic)
int CTrack::newRefSide(void) { return m_newRefSide; }
QBindable<int> CTrack::bindableNewRefSide() { return QBindable<int>(&m_newRefSide); }

// howManyPathsAway property
int CTrack::howManyPathsAway() const { return m_howManyPathsAway; }
void CTrack::setHowManyPathsAway(int value) { m_howManyPathsAway = value; }
QBindable<int> CTrack::bindableHowManyPathsAway() { return QBindable<int>(&m_howManyPathsAway); }

// mode property (depends on current track idx)
int CTrack::mode() const {
    if (idx() >= 0) return gArr[idx()].mode;
    else return 0;
}
void CTrack::setMode(int value) { m_mode = value; }
QBindable<int> CTrack::bindableMode() { return QBindable<int>(&m_mode); }

// count property
int CTrack::count() const { return m_count; }
void CTrack::setCount(int value) { m_count = value; }
QBindable<int> CTrack::bindableCount() { return QBindable<int>(&m_count); }

// currentName property (depends on current track idx)
QString CTrack::currentName() const {
    if (idx() > -1) {
        return gArr[idx()].name;
    } else {
        return "";
    }
}
void CTrack::setCurrentName(const QString& value) { m_currentName = value; }
QBindable<QString> CTrack::bindableCurrentName() { return QBindable<QString>(&m_currentName); }

int CTrack::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return gArr.size();
}

QVariant CTrack::data(const QModelIndex &index, int role) const
{
    int row = index.row();
    if(row < 0 || row >= gArr.size()) {
        return QVariant();
    }

    const CTrk &trk = gArr.at(row);
    switch(role) {
    case RoleNames::index:
        return row;
    case RoleNames::NameRole:
        return trk.name;
    case RoleNames::ModeRole:
        return trk.mode;
    case RoleNames::IsVisibleRole:
        return trk.isVisible;
    case RoleNames::ptA:
        return QVector2D(trk.ptA.easting, trk.ptA.northing);
    case RoleNames::ptB:
        return QVector2D(trk.ptB.easting, trk.ptB.northing);
    case RoleNames::endPtA:
        return QVector2D(trk.endPtA.easting, trk.endPtA.northing);
    case RoleNames::endPtB:
        return QVector2D(trk.endPtB.easting, trk.endPtB.northing);
    case RoleNames::nudgeDistance:
        return trk.nudgeDistance;
    }

    return QVariant();
}

QHash<int, QByteArray> CTrack::roleNames() const
{
    return m_roleNames;
}

QString CTrack::getTrackName(int index)
{
    if (index < 0) return "";
    return gArr[index].name;
}

bool CTrack::getTrackVisible(int index)
{
    if (index < 0) return false;

    return gArr[index].isVisible;
}

double CTrack::getTrackNudge(int index)
{
    if (index < 0) return 0;

    return gArr[index].nudgeDistance;
}

void CTrack::update_ab_refline()
{
    double dist;
    double heading90;
    double vehicle_toolWidth = SettingsManager::instance()->vehicle_toolWidth();
    double vehicle_toolOffset = SettingsManager::instance()->vehicle_toolOffset();
    double vehicle_toolOverlap = SettingsManager::instance()->vehicle_toolOverlap();

    ABLine.desHeading = atan2(ABLine.desPtB.easting - ABLine.desPtA.easting,
                              ABLine.desPtB.northing - ABLine.desPtA.northing);
    if (ABLine.desHeading < 0) ABLine.desHeading += glm::twoPI;

    heading90 = ABLine.desHeading + glm::PIBy2;

    // update the ABLine desLineA and B and the design reference lines

    if (newRefSide() > 0) {
        // right side
        dist = (vehicle_toolWidth - vehicle_toolOverlap) * 0.5 + vehicle_toolOffset;
    } else if (newRefSide() < 0) {
        // left side
        dist = (vehicle_toolWidth - vehicle_toolOverlap) * -0.5 + vehicle_toolOffset;
    }

    ABLine.desLineEndA.easting = ABLine.desPtA.easting - (sin(ABLine.desHeading) * 1000);
    ABLine.desLineEndA.northing = ABLine.desPtA.northing - (cos(ABLine.desHeading) * 1000);

    designRefLine[0].easting = ABLine.desLineEndA.easting + sin(heading90) * dist;
    designRefLine[0].northing = ABLine.desLineEndA.northing + cos(heading90) * dist;

    ABLine.desLineEndB.easting = ABLine.desPtA.easting + (sin(ABLine.desHeading) * 1000);
    ABLine.desLineEndB.northing = ABLine.desPtA.northing + (cos(ABLine.desHeading) * 1000);

    designRefLine[1].easting = ABLine.desLineEndB.easting + sin(heading90) * dist;
    designRefLine[1].northing = ABLine.desLineEndB.northing + cos(heading90) * dist;
}

void CTrack::select(int index)
{
    //reset to generate new reference
    curve.isCurveValid = false;
    curve.lastHowManyPathsAway = 98888;
    ABLine.isABValid = false;
    ABLine.lastHowManyPathsAway = 98888;  // Phase 6.0.43: Reset sentinel for conditional reconstruction
    curve.desList.clear();

    emit saveTracks(); //Do we really need to do this here?

    //We assume that QML will always pass us a valid index that is
    //visible, or -1
    setIdx(index);
    emit resetCreatedYouTurn();
    //yt.ResetCreatedYouTurn();
}

void CTrack::next()
{
    if (idx() < 0) return;

    int visible_count = 0;
    for(CTrk &track : gArr) {
        if (track.isVisible) visible_count++;
    }

    if (visible_count == 0) return; //no visible tracks to choose

    // Qt 6.8 FIX: Use direct member access to avoid binding loop
    m_idx = (m_idx + 1) % gArr.count();
    while (!gArr[m_idx].isVisible)
        m_idx = (m_idx + 1) % gArr.count();
}

void CTrack::prev()
{
    if (idx() < 0) return;

    int visible_count = 0;
    for(CTrk &track : gArr) {
        if (track.isVisible) visible_count++;
    }

    if (visible_count == 0) return; //no visible tracks to choose

    int newIdx = idx() - 1;
    if (newIdx < 0) newIdx = gArr.count() - 1;
    setIdx(newIdx);
    while (!gArr[idx()].isVisible) {
        newIdx = idx() - 1;
        if (newIdx < 0) newIdx = gArr.count() - 1;
        setIdx(newIdx);
    }
}

void CTrack::start_new(int mode)
{
    newTrack = CTrk();
    newTrack.nudgeDistance = 0;
    setNewMode((TrackMode)mode);
    setNewName("");
    designRefLine.clear();
    designRefLine.append(Vec2());
    designRefLine.append(Vec2());
}

void CTrack::mark_start(double easting, double northing, double heading)
{
    //mark "A" location for AB Line or AB curve, or center for waterPivot
    switch(newMode()) {
    case TrackMode::AB:
        curve.desList.clear();
        newTrack.ptA.easting = easting;
        newTrack.ptA.northing = northing;
        ABLine.isMakingABLine = true;
        ABLine.desPtA.easting = easting;
        ABLine.desPtA.northing = northing;
        //temporarily set the B point based on current heading
        ABLine.desPtB.easting = easting + sin(heading) * 1000;
        ABLine.desPtB.northing = easting + cos(heading) * 1000;

        ABLine.isDesPtBSet = false;

        update_ab_refline();

        break;

    case TrackMode::Curve:
        curve.desList.clear();
        curve.isMakingCurve = true;
        break;

    case TrackMode::waterPivot:
        //record center
        newTrack.ptA.easting = easting;
        newTrack.ptA.northing = northing;
        setNewName("Piv");

    default:
        return;
    }
}

void CTrack::mark_end(int refSide, double easting, double northing)
{
    QLocale locale;
    double vehicle_toolWidth = SettingsManager::instance()->vehicle_toolWidth();
    double vehicle_toolOffset = SettingsManager::instance()->vehicle_toolOffset();
    double vehicle_toolOverlap = SettingsManager::instance()->vehicle_toolOverlap();


    //mark "B" location for AB Line or AB curve, or NOP for waterPivot
    int cnt;
    double aveLineHeading = 0;

    setNewRefSide(refSide);

    switch(newMode()) {
    case TrackMode::AB:
        newTrack.ptB.easting = easting;
        newTrack.ptB.northing = northing;

        //set desPtB in ABLine just so display updates.
        ABLine.desPtB.easting = easting;
        ABLine.desPtB.northing = northing;
        ABLine.isDesPtBSet = true;

        update_ab_refline();

        newTrack.heading = ABLine.desHeading;

        setNewName("AB " + locale.toString(glm::toDegrees(ABLine.desHeading), 'f', 1) + QChar(0x00B0));

        //after we're sure we want this, we'll shift it over
        break;

    case TrackMode::Curve:
        newTrack.curvePts.clear();

        cnt = curve.desList.count();
        if (cnt > 3)
        {
            //make sure point distance isn't too big
            curve.MakePointMinimumSpacing(curve.desList, 1.6);
            curve.CalculateHeadings(curve.desList);

            newTrack.ptA = Vec2(curve.desList[0].easting,
                                curve.desList[0].northing);
            newTrack.ptB = Vec2(curve.desList[curve.desList.count() - 1].easting,
                                curve.desList[curve.desList.count() - 1].northing);

            //calculate average heading of line
            double x = 0, y = 0;
            for (Vec3 &pt : curve.desList)
            {
                x += cos(pt.heading);
                y += sin(pt.heading);
            }
            x /= curve.desList.count();
            y /= curve.desList.count();
            aveLineHeading = atan2(y, x);
            if (aveLineHeading < 0) aveLineHeading += glm::twoPI;

            newTrack.heading = aveLineHeading;

            //build the tail extensions
            curve.AddFirstLastPoints(curve.desList);
            curve.SmoothABDesList(4);
            curve.CalculateHeadings(curve.desList);

            //write out the Curve Points
            for (Vec3 &item : curve.desList)
            {
                newTrack.curvePts.append(item);
            }

            setNewName("Cu " + locale.toString(glm::toDegrees(aveLineHeading), 'g', 1) + QChar(0x00B0));

            double dist;

            if (newRefSide() > 0)
            {
                dist = (vehicle_toolWidth - vehicle_toolOverlap) * 0.5 + vehicle_toolOffset;
                NudgeRefCurve(newTrack, dist);
            }
            else if (newRefSide() < 0)
            {
                dist = (vehicle_toolWidth - vehicle_toolOverlap) * -0.5 + vehicle_toolOffset;
                NudgeRefCurve(newTrack, dist);
            }
            //else no nudge, center ref line

        }
        else
        {
            curve.isMakingCurve = false;
            curve.desList.clear();
        }
        break;

    case TrackMode::waterPivot:
        //Do nothing here.  pivot point is already established.
        break;

    default:
        return;
    }

}

void CTrack::finish_new(QString name)
{
    double vehicle_toolWidth = SettingsManager::instance()->vehicle_toolWidth();
    double vehicle_toolOffset = SettingsManager::instance()->vehicle_toolOffset();
    double vehicle_toolOverlap = SettingsManager::instance()->vehicle_toolOverlap();

    double dist;
    newTrack.name = name;

    switch(newMode()) {
    case TrackMode::AB:
        if (!ABLine.isMakingABLine) return; //do not add line if it stopped

        if (newRefSide() > 0)
        {
            dist = (vehicle_toolWidth - vehicle_toolOverlap) * 0.5 + vehicle_toolOffset;
            NudgeRefABLine(newTrack, dist);

        }
        else if (newRefSide() < 0)
        {
            dist = (vehicle_toolWidth - vehicle_toolOverlap) * -0.5 + vehicle_toolOffset;
            NudgeRefABLine(newTrack, dist);
        }

        ABLine.isMakingABLine = false;
        break;

    case TrackMode::Curve:
        if (!curve.isMakingCurve) return; //do not add line if it failed.
        curve.isMakingCurve = false;
        break;

    case TrackMode::waterPivot:
        break;

    default:
        return;

    }

    newTrack.isVisible = true;
    gArr.append(newTrack);

    //emit saveTracks();

    //save tracks and activate the latest one
    select(gArr.count() - 1);
    reloadModel();

}

void CTrack::cancel_new()
{
    ABLine.isMakingABLine = false;
    curve.isMakingCurve = false;
    if(newTrack.mode == TrackMode::Curve) {
        curve.desList.clear();
    }
    newTrack.mode = 0;

    //don't need to do anything else
    }

void CTrack::pause(bool pause)
{
    if (newTrack.mode == TrackMode::Curve) {
        //turn off isMakingCurve when paused, or turn it on
        //when unpausing
        curve.isMakingCurve = !pause;
    }
}

void CTrack::add_point(double easting, double northing, double heading)
{
    AddPathPoint(Vec3(easting, northing, heading));
}

void CTrack::ref_nudge(double dist_m)
{
    NudgeRefTrack(dist_m);
}

void CTrack::nudge_zero()
{
    NudgeDistanceReset();
}

void CTrack::nudge_center()
{
    SnapToPivot();
}

void CTrack::nudge(double dist_m)
{
    NudgeTrack(dist_m);
}

void CTrack::delete_track(int index)
{
    //if we are using the track we are deleting, cancel
    //autosteer
    if (idx() == index) setIdx(-1);

    //otherwise we'll have to adjust the current index after
    //deleting this track.
    // Qt 6.8 FIX: Use direct member access to avoid binding loop
    if (m_idx > index) m_idx = m_idx - 1;

    gArr.removeAt(index);
    reloadModel();
}

void CTrack::swapAB(int idx)
{
    if (idx >= 0 && idx < gArr.count()) {
        if (gArr[idx].mode == TrackMode::AB)
        {
            Vec2 bob = gArr[idx].ptA;
            gArr[idx].ptA = gArr[idx].ptB;
            gArr[idx].ptB = bob;

            gArr[idx].heading += M_PI;
            if (gArr[idx].heading < 0) gArr[idx].heading += glm::twoPI;
            if (gArr[idx].heading > glm::twoPI) gArr[idx].heading -= glm::twoPI;
        }
        else
        {
            int cnt = gArr[idx].curvePts.count();
            if (cnt > 0)
            {
                QVector<Vec3> arr;
                arr.reserve(gArr[idx].curvePts.count());
                std::reverse_copy(gArr[idx].curvePts.begin(),
                                  gArr[idx].curvePts.end(), std::back_inserter(arr));

                gArr[idx].curvePts.clear();

                gArr[idx].heading += M_PI;
                if (gArr[idx].heading < 0) gArr[idx].heading += glm::twoPI;
                if (gArr[idx].heading > glm::twoPI) gArr[idx].heading -= glm::twoPI;

                for (int i = 1; i < cnt; i++)
                {
                    Vec3 pt3 = arr[i];
                    pt3.heading += M_PI;
                    if (pt3.heading > glm::twoPI) pt3.heading -= glm::twoPI;
                    if (pt3.heading < 0) pt3.heading += glm::twoPI;
                    gArr[idx].curvePts.append(pt3);
                }

                Vec2 temp = gArr[idx].ptA;

                gArr[idx].ptA =gArr[idx].ptB;
                gArr[idx].ptB = temp;
            }
        }
    }
    reloadModel();
}

void CTrack::changeName(int index, QString new_name)
{
    if (index >=0 && index < gArr.count() ) {
        gArr[index].name = new_name;
    }
    reloadModel();
}

void CTrack::setVisible(int index, bool isVisible)
{
    if (index >=0 && index <= gArr.count() ) {
        gArr[index].isVisible = isVisible;
    }
    reloadModel();
}

void CTrack::copy(int index, QString new_name)
{
    CTrk new_track = CTrk(gArr[index]);
    new_track.name = new_name;

    gArr.append(new_track);
    reloadModel();
}

// Qt 6.8 QProperty getter/setter implementations
double CTrack::newHeading(void)
{
    return m_newHeading;
}

void CTrack::setNewHeading(double new_heading)
{
    m_newHeading = new_heading;
    newTrack.heading = new_heading;

    // Preserve original business logic
    QLocale locale;
    if (m_newMode == TrackMode::AB) {
        if (new_heading != ABLine.desHeading) {
            //calculate the B point 10 meters from the A point

            mark_end(m_newRefSide, ABLine.desPtA.easting + sin(new_heading) * 10,
                     ABLine.desPtA.northing + cos(new_heading) * 10);

            //update the new line heading as well as recalculate the ref line
            setNewName("A+ " + locale.toString(glm::toDegrees(ABLine.desHeading), 'f', 1) + QChar(0x00B0));
        }
    }
}

void CTrack::setNewRefSide(int which_side)
{
    if (m_newRefSide != which_side) {
        m_newRefSide = which_side;
        if (m_newMode == TrackMode::AB)
            update_ab_refline();
    }
}




void CTrack::updateInterface()
{
    TracksProperties *props = m_tracksProperties;

    // Design preview: show when making a new track (even before idx is valid)
    QVector<QVector3D> des;
    if (ABLine.isMakingABLine) {
        if (ABLine.isDesPtBSet) {
            des = { QVector3D(ABLine.desLineEndA.easting, ABLine.desLineEndA.northing, 0),
                    QVector3D(ABLine.desLineEndB.easting, ABLine.desLineEndB.northing, 0) };
        }
    } else if (curve.isMakingCurve) {
        des.reserve(curve.desList.count());
        for (const Vec3 &v : curve.desList)
            des.append(QVector3D(v.easting, v.northing, 0));
    }
    props->set_newTrack(des);

    if (MainWindowState::instance()->isContourBtnOn()) {
        // contourLine - the current contour line being recorded/played
        QVector<QVector3D> ctLine;
        ctLine.reserve(contour.ctList.count());
        for (const Vec3 &v : contour.ctList)
            ctLine.append(QVector3D(v.easting, v.northing, 0));
        props->set_contourLine(ctLine);

        // stripPointsNearby - dense points near vehicle (nearest point +/- 70 points)
        QVector<QVector3D> stripPtsNearby;
        // stripPointsSparse - sparse points (every ~1m) for entire strip - from CContour
        QVector<QVector3D> stripPtsSparse;
        int stripNum = contour.getStripNum();
        if (stripNum >= 0 && stripNum < contour.stripList.count()) {
            QSharedPointer<QVector<Vec3>> strip = contour.stripList[stripNum];
            if (strip) {
                int stripCount = strip->count();
                int closestPt = contour.getClosestPointIndex();

                // Nearby: closest point +/- 70 points
                int nearbyStart = qMax(0, closestPt - 70);
                int nearbyEnd = qMin(stripCount, closestPt + 70);
                stripPtsNearby.reserve(nearbyEnd - nearbyStart);
                for (int i = nearbyStart; i < nearbyEnd; i++) {
                    const Vec3 &v = (*strip)[i];
                    stripPtsNearby.append(QVector3D(v.easting, v.northing, 0));
                }

                // Sparse: get from CContour (updated incrementally, ~1m spacing)
                const QVector<Vec3> &sparsePts = contour.getStripSparsePoints();
                stripPtsSparse.reserve(sparsePts.count());
                for (const Vec3 &v : sparsePts) {
                    stripPtsSparse.append(QVector3D(v.easting, v.northing, 0));
                }
            }
        }
        props->set_stripPointsNearby(stripPtsNearby);
        props->set_stripPointsSparse(stripPtsSparse);

        // contourCurrentPoint - current position on the strip (pt 0 or last)
        QVector3D currentPt;
        if (contour.ptList && !contour.ptList->isEmpty()) {
            const Vec3 &v = contour.ptList->first();
            currentPt = QVector3D(v.easting, v.northing, 0);
        }
        props->set_contourCurrentPoint(currentPt);

        // contourGoalPoint - goal point for pure display
        props->set_contourGoalPoint(QVector3D(contour.goalPointCT.easting, contour.goalPointCT.northing, 0));

        // isContourOn - whether contour guidance is active
        props->set_isContourOn(contour.isContourOn);

        // clear out ABLine or Curve properties
        props->set_showRefFlags(false);
        props->set_refLine({});
        props->set_currentLine({});
    } else {
        //clear out contour properties
        props->set_isContourOn(false);
        props->set_contourLine({});
        props->set_stripPointsNearby({});
        props->set_stripPointsSparse({});
        props->set_contourCurrentPoint(QVector3D());
        props->set_contourGoalPoint(QVector3D());

        // Current track: only show if a valid track is selected
        int i = idx();
        if (i < 0 || i >= gArr.count()) {
            props->set_showRefFlags(false);
            props->set_refLine({});
            props->set_currentLine({});
            return;
        }

        const CTrk &trk = gArr[i];
        int trackMode = trk.mode;

        // Reference line and A/B flag positions
        QVector<QVector3D> ref;
        if (trackMode == TrackMode::AB || trackMode == TrackMode::bndTrackOuter
            || trackMode == TrackMode::bndTrackInner) {
            ref = { QVector3D(trk.endPtA.easting, trk.endPtA.northing, 0),
                    QVector3D(trk.endPtB.easting, trk.endPtB.northing, 0) };
            props->set_aRefFlag(QVector3D(trk.ptA.easting, trk.ptA.northing, 0));
            props->set_bRefFlag(QVector3D(trk.ptB.easting, trk.ptB.northing, 0));
            props->set_showRefFlags(ABLine.isABValid);
        } else {
            // Curve modes: curvePts are the reference
            ref.reserve(trk.curvePts.count());
            for (const Vec3 &v : trk.curvePts)
                ref.append(QVector3D(v.easting, v.northing, 0));
            if (!trk.curvePts.isEmpty()) {
                props->set_aRefFlag(QVector3D(trk.curvePts.first().easting,
                                              trk.curvePts.first().northing, 0));
                props->set_bRefFlag(QVector3D(trk.curvePts.last().easting,
                                              trk.curvePts.last().northing, 0));
            }
            props->set_showRefFlags(curve.isCurveValid);
        }
        props->set_refLine(ref);

        // Current (active parallel) guidance line
        QVector<QVector3D> cur;
        if (trackMode == TrackMode::AB || trackMode == TrackMode::bndTrackOuter
            || trackMode == TrackMode::bndTrackInner) {
            if (ABLine.isABValid) {
                cur = { QVector3D(ABLine.currentLinePtA.easting, ABLine.currentLinePtA.northing, 0),
                        QVector3D(ABLine.currentLinePtB.easting, ABLine.currentLinePtB.northing, 0) };
            }
        } else {
            cur.reserve(curve.curList.count());
            for (const Vec3 &v : curve.curList)
                cur.append(QVector3D(v.easting, v.northing, 0));
        }
        props->set_currentLine(cur);

        bool isABMode = (trackMode == TrackMode::AB
                         || trackMode == TrackMode::bndTrackOuter
                         || trackMode == TrackMode::bndTrackInner);

        // Shadow outline quad (AB modes only)
        {
            QVector<QVector3D> shadow;
            if (isABMode && ABLine.isABValid) {
                double toolWidth   = SettingsManager::instance()->vehicle_toolWidth();
                double toolOverlap = SettingsManager::instance()->vehicle_toolOverlap();
                double toolOffset  = SettingsManager::instance()->vehicle_toolOffset();
                double wmo = toolWidth - toolOverlap;
                double soff = ABLine.isHeadingSameWay ? toolOffset : -toolOffset;

                double angle = ABLine.abHeading + glm::PIBy2;
                double sinHR = sin(angle) * (wmo * 0.5 + soff);
                double cosHR = cos(angle) * (wmo * 0.5 + soff);
                double sinHL = sin(angle) * (wmo * 0.5 - soff);
                double cosHL = cos(angle) * (wmo * 0.5 - soff);

                shadow = {
                    QVector3D(ABLine.currentLinePtA.easting - sinHL,
                              ABLine.currentLinePtA.northing - cosHL, 0),
                    QVector3D(ABLine.currentLinePtA.easting + sinHR,
                              ABLine.currentLinePtA.northing + cosHR, 0),
                    QVector3D(ABLine.currentLinePtB.easting + sinHR,
                              ABLine.currentLinePtB.northing + cosHR, 0),
                    QVector3D(ABLine.currentLinePtB.easting - sinHL,
                              ABLine.currentLinePtB.northing - cosHL, 0),
                };
            }
            props->set_shadowQuad(shadow);
        }

        // Side guide lines (AB modes only)
        {
            QVector<QVector3D> guides;
            bool sideGuideOn = SettingsManager::instance()->menu_isSideGuideLines();
            if (isABMode && ABLine.isABValid && sideGuideOn) {
                double toolWidth   = SettingsManager::instance()->vehicle_toolWidth()
                                     - SettingsManager::instance()->vehicle_toolOverlap();
                double toolOffset  = SettingsManager::instance()->vehicle_toolOffset() * 2.0;

                double cosH = cos(-ABLine.abHeading);
                double sinH = sin(-ABLine.abHeading);

                auto addLine = [&](double offsetX, double offsetZ) {
                    guides.append(QVector3D(
                        cosH * offsetX - sinH * offsetZ + ABLine.currentLinePtA.easting,
                        sinH * offsetX + cosH * offsetZ + ABLine.currentLinePtA.northing, 0));
                    guides.append(QVector3D(
                        cosH * offsetX - sinH * offsetZ + ABLine.currentLinePtB.easting,
                        sinH * offsetX + cosH * offsetZ + ABLine.currentLinePtB.northing, 0));
                };

                if (ABLine.isHeadingSameWay) {
                    addLine(toolWidth + toolOffset, 0);
                    addLine(-toolWidth + toolOffset, 0);
                    addLine(toolWidth * 2.0, 0);
                    addLine(-toolWidth * 2.0, 0);
                } else {
                    addLine(toolWidth - toolOffset, 0);
                    addLine(-toolWidth - toolOffset, 0);
                    addLine(toolWidth * 2.0, 0);
                    addLine(-toolWidth * 2.0, 0);
                }
            }
            props->set_sideGuideLines(guides);
        }

        // Lookahead / goal point
        {
            QVector<QVector3D> pts;
            bool stanley = SettingsManager::instance()->vehicle_isStanleyUsed();
            if (!stanley) {
                if (isABMode && ABLine.isABValid) {
                    pts.append(QVector3D(ABLine.goalPointAB.easting, ABLine.goalPointAB.northing, 0));
                    pts.append(QVector3D(Backend::instance()->gyd().rEastSteer, Backend::instance()->gyd().rNorthSteer, 0));
                    pts.append(QVector3D(Backend::instance()->gyd().rEastPivot, Backend::instance()->gyd().rNorthPivot, 0));
                }
                else if (curve.isCurveValid && !curve.curList.isEmpty()) {
                    pts.append(QVector3D(curve.goalPointCu.easting, curve.goalPointCu.northing, 0));
                    pts.append(QVector3D(Backend::instance()->gyd().rEastSteer, Backend::instance()->gyd().rNorthSteer, 0));
                    pts.append(QVector3D(Backend::instance()->gyd().rEastPivot, Backend::instance()->gyd().rNorthPivot, 0));
                }
            }
            props->set_lookaheadPoints(pts);
        }

        // Pure pursuit radius circle
        {
            QVector<QVector3D> circle;
            if (qAbs(ABLine.ppRadiusAB) < 50.0 && qAbs(ABLine.ppRadiusAB) > 0.1) {
                const int segs = 100;
                for (int j = 0; j <= segs; ++j) {
                    float a = j * 2.0f * static_cast<float>(M_PI) / segs;
                    circle.append(QVector3D(
                        ABLine.radiusPointAB.easting + ABLine.ppRadiusAB * cos(a),
                        ABLine.radiusPointAB.northing + ABLine.ppRadiusAB * sin(a), 0));
                }
            }
            props->set_pursuitCircle(circle);
        }

        // Smoothed curve (curve modes only)
        {
            QVector<QVector3D> smoo;
            if (!isABMode && curve.isSmoothWindowOpen) {
                smoo.reserve(curve.smooList.count());
                for (const Vec3 &v : curve.smooList)
                    smoo.append(QVector3D(v.easting, v.northing, 0));
            }
            props->set_smoothedCurve(smoo);
        }

        // Curve vertex dots flag
        props->set_showCurrentLineDots(!isABMode && !curve.curList.isEmpty());

        // YouTurn points
        {
            QObject *ytObj = Backend::instance()->yt();
            if (ytObj) {
                CYouTurn *yt = qobject_cast<CYouTurn*>(ytObj);
                if (yt && yt->ytList.count() >= 3) {
                    QVector<QVector3D> ytPts;
                    ytPts.reserve(yt->ytList.count());
                    for (const Vec3 &v : yt->ytList)
                        ytPts.append(QVector3D(v.easting, v.northing, 0));
                    props->set_youTurnPoints(ytPts);
                } else {
                    props->set_youTurnPoints({});
                }
            }
        }
    }

}
