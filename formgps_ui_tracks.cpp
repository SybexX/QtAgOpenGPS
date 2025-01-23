// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
//
// GUI to backend tracks interface
#include "formgps.h"
#include "qmlutil.h"
#include "aogproperty.h"


void FormGPS::tracks_select(int index)
{
    //reset to generate new reference
    trk.curve.isCurveValid = false;
    trk.curve.lastHowManyPathsAway = 98888;
    trk.ABLine.isABValid = false;
    trk.curve.desList.clear();

    FileSaveTracks();

    //We assume that QML will always pass us a valid index that is
    //visible, or -1
    trk.setIdx(index);
    yt.ResetCreatedYouTurn(makeUTurnCounter);
}

void FormGPS::tracks_start_new(int mode)
{
    trk.newTrack = CTrk();
    trk.setNewMode((TrackMode)mode);
    trk.setNewName("");
}

void FormGPS::tracks_mark_start(double easting, double northing, double heading)
{
    //mark "A" location for AB Line or AB curve, or center for waterPivot
    switch(trk.getNewMode()) {
    case TrackMode::AB:
        trk.curve.desList.clear();
        trk.ABLine.isMakingABLine = true;
        trk.ABLine.desPtA.easting = easting;
        trk.ABLine.desPtA.northing = northing;

        trk.ABLine.desLineEndA.easting = trk.ABLine.desPtA.easting - (sin(heading) * 1000);
        trk.ABLine.desLineEndA.northing = trk.ABLine.desPtA.northing - (cos(heading) * 1000);

        trk.ABLine.desLineEndB.easting = trk.ABLine.desPtA.easting + (sin(heading) * 1000);
        trk.ABLine.desLineEndB.northing = trk.ABLine.desPtA.northing + (cos(heading) * 1000);

        break;

    case TrackMode::Curve:
        trk.curve.desList.clear();
        trk.curve.isMakingCurve = true;
        break;

    case TrackMode::waterPivot:
        //record center
        trk.newTrack.ptA.easting = easting;
        trk.newTrack.ptA.northing = northing;
        trk.setNewName("Piv");

    default:
        return;
    }
}

void FormGPS::tracks_mark_end(int refSide, double easting, double northing)
{
    //mark "B" location for AB Line or AB curve, or NOP for waterPivot
    int cnt;
    double aveLineHeading = 0;
    trk.newRefSide = refSide;

    switch(trk.getNewMode()) {
    case TrackMode::AB:
        trk.newTrack.ptB.easting = easting;
        trk.newTrack.ptB.northing = northing;

        trk.ABLine.desHeading = atan2(easting - trk.ABLine.desPtA.easting,
                                  northing - trk.ABLine.desPtA.northing);
        if (trk.ABLine.desHeading < 0) trk.ABLine.desHeading += glm::twoPI;

        trk.newTrack.heading = trk.ABLine.desHeading;

        trk.setNewName("AB " + locale.toString(glm::toDegrees(trk.ABLine.desHeading), 'g', 1) + QChar(0x00B0));

        //after we're sure we want this, we'll shift it over
        break;

    case TrackMode::Curve:
        trk.newTrack.curvePts.clear();

        cnt = trk.curve.desList.count();
        if (cnt > 3)
        {
            //make sure point distance isn't too big
            trk.curve.MakePointMinimumSpacing(trk.curve.desList, 1.6);
            trk.curve.CalculateHeadings(trk.curve.desList);

            trk.newTrack.ptA = Vec2(trk.curve.desList[0].easting,
                                     trk.curve.desList[0].northing);
            trk.newTrack.ptB = Vec2(trk.curve.desList[trk.curve.desList.count() - 1].easting,
                                     trk.curve.desList[trk.curve.desList.count() - 1].northing);

            //calculate average heading of line
            double x = 0, y = 0;
            for (Vec3 &pt : trk.curve.desList)
            {
                x += cos(pt.heading);
                y += sin(pt.heading);
            }
            x /= trk.curve.desList.count();
            y /= trk.curve.desList.count();
            aveLineHeading = atan2(y, x);
            if (aveLineHeading < 0) aveLineHeading += glm::twoPI;

            trk.newTrack.heading = aveLineHeading;

            //build the tail extensions
            trk.curve.AddFirstLastPoints(trk.curve.desList,bnd);
            trk.curve.SmoothABDesList(4);
            trk.curve.CalculateHeadings(trk.curve.desList);

            //write out the Curve Points
            for (Vec3 &item : trk.curve.desList)
            {
                trk.newTrack.curvePts.append(item);
            }

            trk.setNewName("Cu " + locale.toString(glm::toDegrees(aveLineHeading), 'g', 1) + QChar(0x00B0));

            double dist;

            if (trk.newRefSide > 0)
            {
                dist = (tool.width - tool.overlap) * 0.5 + tool.offset;
                trk.NudgeRefCurve(trk.newTrack, dist);
            }
            else if (trk.newRefSide < 0)
            {
                dist = (tool.width - tool.overlap) * -0.5 + tool.offset;
                trk.NudgeRefCurve(trk.newTrack, dist);
            }
            //else no nudge, center ref line

       }
        else
        {
            trk.curve.isMakingCurve = false;
            trk.curve.desList.clear();
        }
        break;

    case TrackMode::waterPivot:
        //Do nothing here.  pivot point is already established.
        break;

    default:
        return;
    }

}

void FormGPS::tracks_finish_new(QString name)
{
    double dist;
    trk.newTrack.name = name;

    switch(trk.getNewMode()) {
    case TrackMode::AB:
        if (!trk.ABLine.isMakingABLine) return; //do not add line if it stopped

        if (trk.newRefSide > 0)
        {
            dist = (tool.width - tool.overlap) * 0.5 + tool.offset;
            trk.NudgeRefABLine(trk.newTrack, dist);

        }
        else if (trk.newRefSide < 0)
        {
            dist = (tool.width - tool.overlap) * -0.5 + tool.offset;
            trk.NudgeRefABLine(trk.newTrack, dist);
        }

        trk.ABLine.isMakingABLine = false;
        break;

    case TrackMode::Curve:
        if (!trk.curve.isMakingCurve) return; //do not add line if it failed.
        trk.curve.isMakingCurve = false;
        break;

    case TrackMode::waterPivot:
        break;

    default:
        return;

    }

    trk.gArr.append(trk.newTrack);
    trk.setIdx(trk.gArr.count() - 1);
    trk.reloadModel();

}

void FormGPS::tracks_cancel_new()
{
    trk.ABLine.isMakingABLine = false;
    trk.curve.isMakingCurve = false;
    if(trk.newTrack.mode == TrackMode::Curve) {
        trk.curve.desList.clear();
    }
    trk.newTrack.mode = 0;

    //don't need to do anything else
}

void FormGPS::tracks_pause(bool pause)
{
    if (trk.newTrack.mode == TrackMode::Curve) {
        //turn off isMakingCurve when paused, or turn it on
        //when unpausing
        trk.curve.isMakingCurve = !pause;
    }
}

void FormGPS::tracks_add_point(double easting, double northing, double heading)
{
    trk.AddPathPoint(Vec3(easting, northing, heading));
}

void FormGPS::tracks_ref_nudge(double dist_m)
{
    trk.NudgeRefTrack(dist_m);
}

void FormGPS::tracks_nudge_zero()
{
    trk.NudgeDistanceReset();
}

void FormGPS::tracks_nudge_center()
{
    trk.SnapToPivot();
}

void FormGPS::tracks_nudge(double dist_m)
{
    trk.NudgeTrack(dist_m);
}

void FormGPS::tracks_delete(int index)
{
    trk.gArr.removeAt(index);
}

void FormGPS::tracks_swapAB(int idx)
{
    if (idx >= 0 && idx < trk.gArr.count()) {
        if (trk.gArr[idx].mode == TrackMode::AB)
        {
            Vec2 bob = trk.gArr[idx].ptA;
            trk.gArr[idx].ptA = trk.gArr[idx].ptB;
            trk.gArr[idx].ptB = bob;

            trk.gArr[idx].heading += M_PI;
            if (trk.gArr[idx].heading < 0) trk.gArr[idx].heading += glm::twoPI;
            if (trk.gArr[idx].heading > glm::twoPI) trk.gArr[idx].heading -= glm::twoPI;
        }
        else
        {
            int cnt = trk.gArr[idx].curvePts.count();
            if (cnt > 0)
            {
                QVector<Vec3> arr;
                arr.reserve(trk.gArr[idx].curvePts.count());
                std::reverse_copy(trk.gArr[idx].curvePts.begin(),
                                  trk.gArr[idx].curvePts.end(), std::back_inserter(arr));

                trk.gArr[idx].curvePts.clear();

                trk.gArr[idx].heading += M_PI;
                if (trk.gArr[idx].heading < 0) trk.gArr[idx].heading += glm::twoPI;
                if (trk.gArr[idx].heading > glm::twoPI) trk.gArr[idx].heading -= glm::twoPI;

                for (int i = 1; i < cnt; i++)
                {
                    Vec3 pt3 = arr[i];
                    pt3.heading += M_PI;
                    if (pt3.heading > glm::twoPI) pt3.heading -= glm::twoPI;
                    if (pt3.heading < 0) pt3.heading += glm::twoPI;
                    trk.gArr[idx].curvePts.append(pt3);
                }

                Vec2 temp = trk.gArr[idx].ptA;

                trk.gArr[idx].ptA =trk.gArr[idx].ptB;
                trk.gArr[idx].ptB = temp;
            }
        }
    }
    trk.reloadModel();
}

void FormGPS::tracks_changeName(int index, QString new_name)
{
    if (index >=0 && index < trk.gArr.count() ) {
        trk.gArr[index].name = new_name;
    }
    trk.reloadModel();
}

void FormGPS::tracks_setVisible(int index, bool isVisible)
{
    trk.gArr[index].isVisible = isVisible;
    trk.reloadModel();
}

    /*
void FormGPS::update_current_ABline_from_qml()
{
    //AOGInterface currentABLine property changed; sync our
 0   //local ABLine.numABLineSelected with it.

    QObject *aog = qmlItem(qml_root, "aog"); //TODO save this in formgps.h

    //the property will be -1 if nothing is selected, ABLine uses base 1
    //so add one to it
    ABLine.numABLineSelected = aog->property("currentABLine").toInt() + 1;
    if (ABLine.numABLineSelected > ABLine.lineArr.count())
        ABLine.numABLineSelected = 0;

    ABLine.isABValid = false; //recalculate the closest line to us
    ABLine.moveDistance = 0;

    if (ABLine.numABLineSelected > 0) {
        ABLine.abHeading = ABLine.lineArr[ABLine.numABLineSelected-1].heading;
        ABLine.refPoint1 = ABLine.lineArr[ABLine.numABLineSelected-1].origin;
        ABLine.SetABLineByHeading();
        ABLine.isBtnABLineOn = true;
        ABLine.isABLineSet = true;
        ABLine.isABLineLoaded = true;
        ABLine.isABLineBeingSet = false;
        aog->setProperty("currentABLine_heading", ABLine.abHeading);
    } else {
        ABLine.isBtnABLineOn = false;
        ABLine.isABLineSet = false;
        ABLine.isABLineLoaded = false;
        ABLine.isABLineBeingSet = false;
    }

    int selectedItem = aog->property("currentABCurve").toInt();
    //reset to generate new reference
    curve.isCurveValid = false;
    curve.moveDistance = 0;
    curve.desList.clear();

    FileSaveCurveLines(); // in case a new one was added

    if (selectedItem > -1 && selectedItem <= curve.curveArr.count())
    {
        int idx = selectedItem;
        curve.numCurveLineSelected = idx + 1;

        curve.aveLineHeading = curve.curveArr[idx].aveHeading;
        curve.refList.clear();
        for (int i = 0; i < curve.curveArr[idx].curvePts.count(); i++)
        {
            curve.refList.append(curve.curveArr[idx].curvePts[i]);
        }
        curve.isCurveSet = true;
        yt.ResetYouTurn();
    }
    else
    {
        curve.numCurveLineSelected = 0;
        curve.isOkToAddDesPoints = false;
        curve.isCurveSet = false;
        curve.refList.clear();
        curve.isCurveSet = false;
        //DisableYouTurnButtons();
        //done in QML
        curve.isBtnCurveOn = false;
    }

    if (ABLine.numABLineSelected == 0 && curve.numCurveLineSelected == 0 && (bool)ct.isContourBtnOn == false) {
        isBtnAutoSteerOn = false;
    }
}

void FormGPS::update_ABlines_in_qml()
{
    QObject *linesInterface = qmlItem(qml_root,"linesInterface");

    QList<QVariant> list;
    QMap<QString, QVariant>line;

    for(int i=0; i < ABLine.lineArr.count(); i++) {
        line.clear();
        line["index"] = i;
        line["name"] = ABLine.lineArr[i].Name;
        line["easting"] = ABLine.lineArr[i].origin.easting;
        line["northing"] = ABLine.lineArr[i].origin.northing;
        line["heading"] = ABLine.lineArr[i].heading;
        line["visible"] = ABLine.lineArr[i].isVisible;
        list.append(line);
    }

    linesInterface->setProperty("abLinesList",list);

    list.clear();
    for(int i=0; i < curve.curveArr.count(); i++) {
        line.clear();
        line["index"] = i;
        line["name"] = curve.curveArr[i].Name;
        line["visible"] = curve.curveArr[i].isVisible;
        list.append(line);
    }

    linesInterface->setProperty("abCurvesList",list);
}

void FormGPS::add_new_ABline(QString name, double easting, double northing, double heading)
{
    qDebug() << name << easting << northing << heading;
    CABLines line;
    line.origin = Vec2(easting, northing);
    line.heading = heading;
    line.Name = name;
    ABLine.lineArr.append(line);
    FileSaveABLines();
    update_ABlines_in_qml();
}

void FormGPS::start_newABLine(bool start_or_cancel, double easting, double northing, double heading)
{
    if (!start_or_cancel) {
        ABLine.isABLineBeingSet = false;
        return;
    }

    ABLine.desPoint1.easting = easting + cos(heading) * (double)property_setVehicle_toolOffset;
    ABLine.desPoint2.northing = northing + sin(heading) * (double)property_setVehicle_toolOffset;

    ABLine.desHeading = heading;

    ABLine.desPoint2.easting = 99999;
    ABLine.desPoint2.northing = 99999;

    ABLine.isABLineBeingSet = true;

    ABLine.desHeading = heading;
    ABLine.desP1.easting = ABLine.desPoint1.easting - (sin(ABLine.desHeading) * (double)property_setAB_lineLength);
    ABLine.desP1.northing = ABLine.desPoint1.northing - (cos(ABLine.desHeading) * (double)property_setAB_lineLength);
    ABLine.desP2.easting = ABLine.desPoint1.easting + (sin(ABLine.desHeading) * (double)property_setAB_lineLength);
    ABLine.desP2.northing = ABLine.desPoint1.northing + (cos(ABLine.desHeading) * (double)property_setAB_lineLength);
}

void FormGPS::delete_ABLine(int which_one)
{
    QObject *linesInterface = qmlItem(qml_root,"linesInterface");

    if ((which_one < 0) || (which_one >= ABLine.lineArr.count()))
        return;

    ABLine.lineArr.removeAt(which_one);

    linesInterface->setProperty("currentABLine", -1);

    update_ABlines_in_qml();
    update_current_ABline_from_qml();
    FileSaveABLines();
}

void FormGPS::swap_heading_ABLine(int which_one)
{
    if ((which_one < 0) || (which_one >= ABLine.lineArr.count()))
        return;

    double heading = ABLine.lineArr[which_one].heading + M_PI;
    if (heading > glm::twoPI) //if over 360
        heading -= glm::twoPI;

    ABLine.lineArr[which_one].heading = heading;
    update_ABlines_in_qml();
    FileSaveABLines();
}

void FormGPS::change_name_ABLine(int which_one, QString name)
{
    qDebug() << "changing name of " << which_one << " to " << name;
    if (which_one > -1) {
        ABLine.lineArr[which_one].Name = name;
        update_ABlines_in_qml();
        FileSaveABLines();
    }
}
*/
