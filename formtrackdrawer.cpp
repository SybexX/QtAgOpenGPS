// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later

#include <QApplication>
#include <QWidget>
#include <utility>
#include "formtrackdrawer.h"
#include "qmlutil.h"
#include "cboundary.h"
#include "ctrack.h"
#include "glm.h"
#include "qmlutil.h"
#include <QMatrix4x4>
#include <QOpenGLFunctions>
#include "glutils.h"
#include "classes/settingsmanager.h"
#include <QTime>
#include "mainwindowstate.h"
#include "backend.h"
#include "trackinterface.h"
#include "models/fencelinemodel.h"
#include "cvehicle.h"

void CalculateHeadings(QVector<Vec3> &xList);
void MakePointMinimumSpacing(QVector<Vec3> &xList, double minDistance);

void AddFirstLastPoints(QVector<Vec3> &xList)
{
    if (xList.count() < 2) return;

    int ptCnt = xList.count() - 1;
    Vec3 start(xList[0]);

    for (int i = 1; i < 300; i++)
    {
        Vec3 pt(xList[ptCnt]);
        pt.easting += (sin(pt.heading) * i);
        pt.northing += (cos(pt.heading) * i);
        xList.append(pt);
    }

    start = Vec3(xList[0]);

    for (int i = 1; i < 300; i++)
    {
        Vec3 pt(start);
        pt.easting -= (sin(pt.heading) * i);
        pt.northing -= (cos(pt.heading) * i);
        xList.insert(0, pt);
    }
}

FormTrackDrawer::FormTrackDrawer(QObject *parent)
    : QObject{parent}
{
    isA = true;

    connect(TrackInterface::instance(), &TrackInterface::load, this, &FormTrackDrawer::load);
    connect(TrackInterface::instance(), &TrackInterface::updateLines, this, &FormTrackDrawer::updateLines);
    connect(TrackInterface::instance(), &TrackInterface::mouseClicked, this, &FormTrackDrawer::mouseClicked);
    connect(TrackInterface::instance(), &TrackInterface::mouseDragged, this, &FormTrackDrawer::mouseDragged);
    connect(TrackInterface::instance(), &TrackInterface::cycleForward, this, &FormTrackDrawer::btnCycleForward_Click);
    connect(TrackInterface::instance(), &TrackInterface::cycleBackward, this, &FormTrackDrawer::btnCycleBackward_Click);
    connect(TrackInterface::instance(), &TrackInterface::deleteTrack, this, &FormTrackDrawer::btnDeleteTrack_Click);
    connect(TrackInterface::instance(), &TrackInterface::cancelTouch, this, &FormTrackDrawer::btnCancelTouch_Click);
    connect(TrackInterface::instance(), &TrackInterface::createABLine, this, &FormTrackDrawer::createABLine);
    connect(TrackInterface::instance(), &TrackInterface::createCurve, this, &FormTrackDrawer::createCurve);
    connect(TrackInterface::instance(), &TrackInterface::createBoundaryCurve, this, &FormTrackDrawer::createBoundaryCurve);
    connect(TrackInterface::instance(), &TrackInterface::cancelTrackCreation, this, &FormTrackDrawer::cancelTrackCreation);
    connect(TrackInterface::instance(), &TrackInterface::saveExit, this, &FormTrackDrawer::saveExit);
    connect(TrackInterface::instance(), &TrackInterface::alength, this, &FormTrackDrawer::alength_Click);
    connect(TrackInterface::instance(), &TrackInterface::blength, this, &FormTrackDrawer::blength_Click);
    connect(TrackInterface::instance(), &TrackInterface::ashrink, this, &FormTrackDrawer::ashrink_Click);
    connect(TrackInterface::instance(), &TrackInterface::bshrink, this, &FormTrackDrawer::bshrink_Click);
    connect(TrackInterface::instance(), &TrackInterface::setTrackVisible, this, &FormTrackDrawer::setTrackVisible_Click);
}

QVector3D FormTrackDrawer::mouseClickToField(int mouseX, int mouseY) {
    QMatrix4x4 modelview;
    QMatrix4x4 projection;

    int width = TrackInterface::instance()->viewportWidth();
    int height = TrackInterface::instance()->viewportHeight();

    projection.setToIdentity();
    projection.perspective(58, 1.0f, 1.0f, 20000);

    modelview.setToIdentity();
    modelview.translate(0, 0, -(double)Backend::instance()->m_currentField.maxDistance * (double)TrackInterface::instance()->zoom());

    modelview.translate(-(double)Backend::instance()->m_currentField.centerX + (double)TrackInterface::instance()->sX() * (double)Backend::instance()->m_currentField.maxDistance,
                        -(double)Backend::instance()->m_currentField.centerY + (double)TrackInterface::instance()->sY() * (double)Backend::instance()->m_currentField.maxDistance,
                        0);

    float x = mouseX;
    float y = height - mouseY;

    QVector3D worldpoint_near = QVector3D({x, y, 0}).unproject(modelview, projection, QRect(0, 0, width, height));
    QVector3D worldpoint_far = QVector3D({x, y, 1}).unproject(modelview, projection, QRect(0, 0, width, height));
    QVector3D direction = worldpoint_far - worldpoint_near;
    double lambda = -(worldpoint_near.z()) / direction.z();

    float mouseEasting = worldpoint_near.x() + lambda * direction.x();
    float mouseNorthing = worldpoint_near.y() + lambda * direction.y();

    return QVector3D({(float)mouseEasting, (float)mouseNorthing, 0});
}

void FormTrackDrawer::setup_matrices(QMatrix4x4 &modelview, QMatrix4x4 &projection) {
    projection.setToIdentity();
    projection.perspective(58, 1.0f, 1.0f, 20000);

    modelview.setToIdentity();
    modelview.translate(0, 0, -(double)Backend::instance()->m_currentField.maxDistance * (double)TrackInterface::instance()->zoom());

    modelview.translate(-(double)Backend::instance()->m_currentField.centerX + (double)TrackInterface::instance()->sX() * (double)Backend::instance()->m_currentField.maxDistance,
                        -(double)Backend::instance()->m_currentField.centerY + (double)TrackInterface::instance()->sY() * (double)Backend::instance()->m_currentField.maxDistance,
                        0);
}

void FormTrackDrawer::load() {
    if (!bnd || !track) return;

    TrackInterface::instance()->set_showa(false);
    TrackInterface::instance()->set_showb(false);

    start = 99999;
    end = 99999;
    isA = true;

    updateLines();
}

void FormTrackDrawer::close() {
    if (!bnd || !track) return;

    TrackInterface::instance()->set_showa(false);
    TrackInterface::instance()->set_showb(false);

    start = 99999;
    end = 99999;
}

void FormTrackDrawer::updateLines() {
    if (!bnd || !track) return;

    QVector<FenceLineModel::FenceLine> boundaries;
    QMatrix4x4 modelview;
    QMatrix4x4 projection;

    QVector3D s, p;

    setup_matrices(modelview, projection);

    int width = TrackInterface::instance()->viewportWidth();
    int height = TrackInterface::instance()->viewportHeight();

    for (int j = 0; j < bnd->bndList.count(); j++) {
        FenceLineModel::FenceLine line;
        line.index = j;

        if (j == bndSelect)
            line.color = QColor::fromRgbF(0.75f, 0.75f, 0.75f);
        else
            line.color = QColor::fromRgbF(0.0f, 0.25f, 0.10f);

        line.width = 4;

        QVariantList linepoints;
        for (int i = 0; i < bnd->bndList[j].fenceLine.count(); i++) {
            p = QVector3D(bnd->bndList[j].fenceLine[i].easting,
                          bnd->bndList[j].fenceLine[i].northing,
                          0);
            s = p.project(modelview, projection, QRect(0, 0, width, height));
            QPoint linepoint = QPoint(s.x(), height - s.y());
            linepoints.append(linepoint);
        }
        line.points = linepoints;
        boundaries.append(line);
    }

    TrackInterface::instance()->boundaryLineModel()->setFenceLines(boundaries);

    if (width > 0) {
        p = QVector3D(CVehicle::instance()->pivotAxlePos.easting,
                      CVehicle::instance()->pivotAxlePos.northing,
                      0);
        s = p.project(modelview, projection, QRect(0, 0, width, height));
        TrackInterface::instance()->set_vehiclePoint(QPoint(s.x(), height - s.y()));
    }

    TrackInterface::instance()->set_trackCount(track->gArr.count());
    TrackInterface::instance()->set_currentTrackIndex(track->idx());

    updateABPoints();

    emit saveTracks();
}

void FormTrackDrawer::mouseClicked(int mouseX, int mouseY) {
    if (!bnd || !track) return;

    QVector3D fieldCoords = mouseClickToField(mouseX, mouseY);
    double clickEasting = fieldCoords.x();
    double clickNorthing = fieldCoords.y();

    if (isA) {
        double minDist = glm::DOUBLE_MAX;
        start = 99999;
        end = 99999;

        for (int j = 0; j < bnd->bndList.count(); j++) {
            for (int i = 0; i < bnd->bndList[j].fenceLine.count(); i++) {
                double dist = ((clickEasting - bnd->bndList[j].fenceLine[i].easting) * (clickEasting - bnd->bndList[j].fenceLine[i].easting))
                            + ((clickNorthing - bnd->bndList[j].fenceLine[i].northing) * (clickNorthing - bnd->bndList[j].fenceLine[i].northing));
                if (dist < minDist) {
                    minDist = dist;
                    bndSelect = j;
                    start = i;
                }
            }
        }

        isA = false;
        TrackInterface::instance()->set_showa(true);
        TrackInterface::instance()->set_sliceCount(1);
    } else {
        double minDist = glm::DOUBLE_MAX;
        int j = bndSelect;

        for (int i = 0; i < bnd->bndList[j].fenceLine.count(); i++) {
            double dist = ((clickEasting - bnd->bndList[j].fenceLine[i].easting) * (clickEasting - bnd->bndList[j].fenceLine[i].easting))
                        + ((clickNorthing - bnd->bndList[j].fenceLine[i].northing) * (clickNorthing - bnd->bndList[j].fenceLine[i].northing));
            if (dist < minDist) {
                minDist = dist;
                end = i;
            }
        }

        if (start == end) {
            start = 99999;
            end = 99999;
            emit Backend::instance()->timedMessage(3000, tr("Line Error"), tr("Start Point = End Point"));
            return;
        }

        TrackInterface::instance()->set_showb(true);
        TrackInterface::instance()->set_sliceCount(2);
        isA = true;
    }

    updateABPoints();
}

void FormTrackDrawer::updateABPoints() {
    if (!bnd) return;

    QMatrix4x4 modelview, projection;
    QVector3D p, s;

    setup_matrices(modelview, projection);

    int width = TrackInterface::instance()->viewportWidth();
    int height = TrackInterface::instance()->viewportHeight();

    if (start != 99999) {
        p = QVector3D(bnd->bndList[bndSelect].fenceLine[start].easting, bnd->bndList[bndSelect].fenceLine[start].northing, 0);
        s = p.project(modelview, projection, QRect(0, 0, width, height));
        TrackInterface::instance()->set_apoint(QPoint(s.x(), height - s.y()));
        TrackInterface::instance()->set_showa(true);
    }

    if (end != 99999) {
        p = QVector3D(bnd->bndList[bndSelect].fenceLine[end].easting, bnd->bndList[bndSelect].fenceLine[end].northing, 0);
        s = p.project(modelview, projection, QRect(0, 0, width, height));
        TrackInterface::instance()->set_bpoint(QPoint(s.x(), height - s.y()));
        TrackInterface::instance()->set_showb(true);
    }
}

void FormTrackDrawer::updateCurrentTrackLine(int idx) {
    if (!track || idx < 0 || idx >= track->gArr.count()) return;

    QMatrix4x4 modelview, projection;
    QVector3D p, s;

    setup_matrices(modelview, projection);

    int width = TrackInterface::instance()->viewportWidth();
    int height = TrackInterface::instance()->viewportHeight();

    QVariantList trackLine;
    QVariantList refLine;
    CTrk &t = track->gArr[idx];

    if (t.curvePts.count() > 0) {
        for (const Vec3 &pt : std::as_const(t.curvePts)) {
            p = QVector3D(pt.easting, pt.northing, 0);
            s = p.project(modelview, projection, QRect(0, 0, width, height));
            trackLine.append(QPoint(s.x(), height - s.y()));
        }
        p = QVector3D(t.ptA.easting, t.ptA.northing, 0);
        s = p.project(modelview, projection, QRect(0, 0, width, height));
        QPoint ptA = QPoint(s.x(), height - s.y());
        TrackInterface::instance()->set_apoint(ptA);
        TrackInterface::instance()->set_showa(true);

        p = QVector3D(t.ptB.easting, t.ptB.northing, 0);
        s = p.project(modelview, projection, QRect(0, 0, width, height));
        QPoint ptB = QPoint(s.x(), height - s.y());
        TrackInterface::instance()->set_bpoint(ptB);
        TrackInterface::instance()->set_showb(true);

        TrackInterface::instance()->set_refLine(QVariantList());
    } else {
        p = QVector3D(t.ptA.easting, t.ptA.northing, 0);
        s = p.project(modelview, projection, QRect(0, 0, width, height));
        QPoint ptA = QPoint(s.x(), height - s.y());
        trackLine.append(ptA);

        p = QVector3D(t.ptB.easting, t.ptB.northing, 0);
        s = p.project(modelview, projection, QRect(0, 0, width, height));
        QPoint ptB = QPoint(s.x(), height - s.y());
        trackLine.append(ptB);

        TrackInterface::instance()->set_apoint(ptA);
        TrackInterface::instance()->set_bpoint(ptB);
        TrackInterface::instance()->set_showa(true);
        TrackInterface::instance()->set_showb(true);

        if (t.mode == TrackMode::AB) {
            p = QVector3D(t.endPtA.easting, t.endPtA.northing, 0);
            s = p.project(modelview, projection, QRect(0, 0, width, height));
            refLine.append(QPoint(s.x(), height - s.y()));

            p = QVector3D(t.endPtB.easting, t.endPtB.northing, 0);
            s = p.project(modelview, projection, QRect(0, 0, width, height));
            refLine.append(QPoint(s.x(), height - s.y()));
        } else {
            TrackInterface::instance()->set_refLine(QVariantList());
        }
    }

    TrackInterface::instance()->set_currentTrackLine(trackLine);
    if (!refLine.isEmpty()) {
        TrackInterface::instance()->set_refLine(refLine);
    }
}

void FormTrackDrawer::mouseDragged(int fromX, int fromY, int mouseX, int mouseY) {
    Q_UNUSED(fromX);
    Q_UNUSED(fromY);
    Q_UNUSED(mouseX);
    Q_UNUSED(mouseY);
}

void FormTrackDrawer::btnCycleForward_Click() {
    if (!track) return;

    int count = track->gArr.count();
    int currentIdx = track->idx();

    if (count > 0) {
        currentIdx++;
        if (currentIdx >= count) {
            currentIdx = 0;
        }
    } else {
        currentIdx = -1;
    }

    track->setIdx(currentIdx);
    TrackInterface::instance()->set_currentTrackIndex(currentIdx);

    if (currentIdx >= 0 && currentIdx < count) {
        TrackInterface::instance()->set_isTrackVisible(track->gArr[currentIdx].isVisible);
        updateCurrentTrackLine(currentIdx);
    } else {
        TrackInterface::instance()->set_isTrackVisible(false);
        TrackInterface::instance()->set_currentTrackLine(QVariantList());
        TrackInterface::instance()->set_refLine(QVariantList());
        TrackInterface::instance()->set_showa(false);
        TrackInterface::instance()->set_showb(false);
    }

    updateLines();
}

void FormTrackDrawer::btnCycleBackward_Click() {
    if (!track) return;

    int count = track->gArr.count();
    int currentIdx = track->idx();

    if (count > 0) {
        currentIdx--;
        if (currentIdx < 0) {
            currentIdx = count - 1;
        }
    } else {
        currentIdx = -1;
    }

    track->setIdx(currentIdx);
    TrackInterface::instance()->set_currentTrackIndex(currentIdx);
    
    if (currentIdx >= 0 && currentIdx < count) {
        TrackInterface::instance()->set_isTrackVisible(track->gArr[currentIdx].isVisible);
        updateCurrentTrackLine(currentIdx);
    } else {
        TrackInterface::instance()->set_isTrackVisible(false);
        TrackInterface::instance()->set_currentTrackLine(QVariantList());
        TrackInterface::instance()->set_refLine(QVariantList());
        TrackInterface::instance()->set_showa(false);
        TrackInterface::instance()->set_showb(false);
    }
    
    updateLines();
}

void FormTrackDrawer::btnDeleteTrack_Click() {
    if (!track) return;

    int currentIdx = track->idx();
    int count = track->gArr.count();

    if (currentIdx < 0 || currentIdx >= count) return;

    track->gArr.removeAt(currentIdx);

    if (track->gArr.count() > 0) {
        track->setIdx(0);
    } else {
        track->setIdx(-1);
    }

    track->reloadModel();
    TrackInterface::instance()->set_currentTrackIndex(track->idx());
    TrackInterface::instance()->set_trackCount(track->gArr.count());

    if (track->gArr.count() > 0) {
        updateCurrentTrackLine(0);
    } else {
        TrackInterface::instance()->set_currentTrackLine(QVariantList());
        TrackInterface::instance()->set_refLine(QVariantList());
        TrackInterface::instance()->set_isTrackVisible(false);
        TrackInterface::instance()->set_showa(false);
        TrackInterface::instance()->set_showb(false);
    }

    emit saveTracks();
}

void FormTrackDrawer::btnCancelTouch_Click() {
    start = 99999;
    end = 99999;
    isA = true;

    TrackInterface::instance()->set_showa(false);
    TrackInterface::instance()->set_showb(false);
    TrackInterface::instance()->set_sliceCount(0);
}

void FormTrackDrawer::createABLine() {
    if (!track || !bnd || start == 99999 || end == 99999) return;

    int s = start;
    int e = end;

    if ((abs(s - e)) > (bnd->bndList[bndSelect].fenceLine.count() * 0.5)) {
        if (s < e) {
            int tmp = s;
            s = e;
            e = tmp;
        }
    } else {
        if (s > e) {
            int tmp = s;
            s = e;
            e = tmp;
        }
    }

    Vec3 ptA(bnd->bndList[bndSelect].fenceLine[s]);
    Vec3 ptB(bnd->bndList[bndSelect].fenceLine[e]);

    double abHead = atan2(
        bnd->bndList[bndSelect].fenceLine[e].easting - bnd->bndList[bndSelect].fenceLine[s].easting,
        bnd->bndList[bndSelect].fenceLine[e].northing - bnd->bndList[bndSelect].fenceLine[s].northing);
    if (abHead < 0) abHead += glm::twoPI;

    CTrk newTrack;
    newTrack.mode = (int)TrackMode::AB;
    newTrack.heading = abHead;
    newTrack.ptA = Vec2(ptA.easting, ptA.northing);
    newTrack.ptB = Vec2(ptB.easting, ptB.northing);
    newTrack.isVisible = true;
    newTrack.nudgeDistance = 0;

    QString name = TrackInterface::instance()->trackName();
    if (name.isEmpty()) {
        double headingDeg = glm::toDegrees(newTrack.heading);
        name = QString("AB %1\u00B0").arg(headingDeg, 0, 'f', 1);
    }
    newTrack.name = name;

    track->gArr.append(newTrack);
    track->setIdx(track->gArr.count() - 1);

    start = 99999;
    end = 99999;

    TrackInterface::instance()->set_showa(false);
    TrackInterface::instance()->set_showb(false);
    TrackInterface::instance()->set_sliceCount(0);
    TrackInterface::instance()->set_trackCount(track->gArr.count());
    TrackInterface::instance()->set_currentTrackIndex(track->idx());
    TrackInterface::instance()->set_trackName("");
    TrackInterface::instance()->set_isTrackVisible(true);

    track->reloadModel();
    updateCurrentTrackLine(track->idx());
    emit saveTracks();
}

void FormTrackDrawer::createCurve() {
    if (!track || !bnd || start == 99999 || end == 99999) return;

    int s = start;
    int e = end;
    bool isLoop = false;
    int limit = e;

    if ((abs(s - e)) > (bnd->bndList[bndSelect].fenceLine.count() * 0.5)) {
        if (s < e) {
            int tmp = s;
            s = e;
            e = tmp;
        }
        isLoop = true;
        if (s < e) {
            limit = e;
            e = 0;
        } else {
            limit = e;
            e = bnd->bndList[bndSelect].fenceLine.count();
        }
    } else {
        if (s > e) {
            int tmp = s;
            s = e;
            e = tmp;
        }
    }

    CTrk newTrack;
    newTrack.mode = (int)TrackMode::Curve;
    newTrack.isVisible = true;
    newTrack.nudgeDistance = 0;

    double x = 0, y = 0;

    if (s < e) {
        for (int i = s; i <= e; i++) {
            Vec3 pt = bnd->bndList[bndSelect].fenceLine[i];
            newTrack.curvePts.append(pt);

            x += cos(pt.heading);
            y += sin(pt.heading);

            if (isLoop && i == bnd->bndList[bndSelect].fenceLine.count() - 1) {
                i = -1;
                isLoop = false;
                e = limit;
            }
        }
    } else {
        for (int i = s; i >= e; i--) {
            Vec3 pt = bnd->bndList[bndSelect].fenceLine[i];
            newTrack.curvePts.append(pt);

            x += cos(pt.heading);
            y += sin(pt.heading);

            if (isLoop && i == 0) {
                i = bnd->bndList[bndSelect].fenceLine.count() - 1;
                isLoop = false;
                e = limit;
            }
        }
    }

    MakePointMinimumSpacing(newTrack.curvePts, 1.6);
    CalculateHeadings(newTrack.curvePts);

    x /= newTrack.curvePts.count();
    y /= newTrack.curvePts.count();
    newTrack.heading = atan2(y, x);
    if (newTrack.heading < 0) newTrack.heading += glm::twoPI;

    newTrack.ptA = Vec2(newTrack.curvePts[0].easting, newTrack.curvePts[0].northing);
    newTrack.ptB = Vec2(newTrack.curvePts.last().easting, newTrack.curvePts.last().northing);

    int cnt = newTrack.curvePts.count();
    if (cnt > 3) {
        AddFirstLastPoints(newTrack.curvePts);
        CalculateHeadings(newTrack.curvePts);
    }

    QString name = TrackInterface::instance()->trackName();
    if (name.isEmpty()) {
        name = QString("Cu %1\u00B0").arg(glm::toDegrees(newTrack.heading), 0, 'g', 1);
    }
    newTrack.name = name;

    track->gArr.append(newTrack);
    track->setIdx(track->gArr.count() - 1);

    start = 99999;
    end = 99999;

    TrackInterface::instance()->set_showa(false);
    TrackInterface::instance()->set_showb(false);
    TrackInterface::instance()->set_sliceCount(0);
    TrackInterface::instance()->set_trackCount(track->gArr.count());
    TrackInterface::instance()->set_currentTrackIndex(track->idx());
    TrackInterface::instance()->set_trackName("");
    TrackInterface::instance()->set_isTrackVisible(true);

    track->reloadModel();
    updateCurrentTrackLine(track->idx());
    emit saveTracks();
}

void FormTrackDrawer::cancelTrackCreation() {
    btnCancelTouch_Click();
}

void FormTrackDrawer::saveExit() {
    if (!track) return;

    emit saveTracks();

    if (track->gArr.count() > 0) {
        track->setIdx(0);
    } else {
        track->setIdx(-1);
    }
}

void FormTrackDrawer::alength_Click() {
    if (!track) return;

    int idx = track->idx();
    if (idx < 0 || idx >= track->gArr.count()) return;

    CTrk &t = track->gArr[idx];
    if (t.curvePts.count() < 2) return;

    Vec3 start = t.curvePts[0];
    for (int i = 1; i < 50; i++) {
        Vec3 pt = start;
        pt.easting -= (sin(pt.heading) * i);
        pt.northing -= (cos(pt.heading) * i);
        t.curvePts.insert(0, pt);
    }
    track->reloadModel();
    updateCurrentTrackLine(idx);
    emit saveTracks();
}

void FormTrackDrawer::blength_Click() {
    if (!track) return;

    int idx = track->idx();
    if (idx < 0 || idx >= track->gArr.count()) return;

    CTrk &t = track->gArr[idx];
    if (t.curvePts.count() < 2) return;

    int ptCnt = t.curvePts.count() - 1;
    for (int i = 1; i < 50; i++) {
        Vec3 pt(t.curvePts[ptCnt]);
        pt.easting += (sin(pt.heading) * i);
        pt.northing += (cos(pt.heading) * i);
        t.curvePts.append(pt);
    }
    track->reloadModel();
    updateCurrentTrackLine(idx);
    emit saveTracks();
}

void FormTrackDrawer::createBoundaryCurve() {
    if (!track || !bnd) return;

    for (int q = 0; q < bnd->bndList.count(); q++) {
        QVector<Vec3> desList;

        for (int i = 0; i < bnd->bndList[bndSelect].fenceLine.count(); i++) {
            Vec3 pt = bnd->bndList[bndSelect].fenceLine[i];
            desList.append(pt);
        }

        CTrk newTrack;
        newTrack.ptA = Vec2(desList[0].easting, desList[0].northing);
        newTrack.ptB = Vec2(desList.last().easting, desList.last().northing);

        Vec3 pt3 = desList[0];
        desList.append(pt3);

        int cnt = desList.count();
        if (cnt > 3) {
            pt3 = desList[0];
            desList.append(pt3);

            MakePointMinimumSpacing(desList, 1.6);
            CalculateHeadings(desList);

            newTrack.name = "Boundary Curve";
            if (q > 0) newTrack.name = QString("Inner Boundary Curve %1").arg(q);

            newTrack.heading = 0;
            newTrack.mode = (int)TrackMode::bndCurve;
            newTrack.isVisible = true;
            newTrack.nudgeDistance = 0;

            for (const Vec3 &item : desList) {
                newTrack.curvePts.append(item);
            }

            track->gArr.append(newTrack);
        }
    }

    start = 99999;
    end = 99999;

    TrackInterface::instance()->set_showa(false);
    TrackInterface::instance()->set_showb(false);
    TrackInterface::instance()->set_sliceCount(0);
    TrackInterface::instance()->set_trackCount(track->gArr.count());
    TrackInterface::instance()->set_currentTrackIndex(track->idx());
    TrackInterface::instance()->set_trackName("");
    TrackInterface::instance()->set_isTrackVisible(true);

    track->reloadModel();
    updateCurrentTrackLine(track->idx());
    emit saveTracks();
}

void FormTrackDrawer::ashrink_Click() {
    if (!track) return;

    int idx = track->idx();
    if (idx < 0 || idx >= track->gArr.count()) return;

    CTrk &t = track->gArr[idx];
    if (t.curvePts.count() > 8) {
        t.curvePts.remove(0, 5);
    }
    track->reloadModel();
    updateCurrentTrackLine(idx);
    emit saveTracks();
}

void FormTrackDrawer::bshrink_Click() {
    if (!track) return;

    int idx = track->idx();
    if (idx < 0 || idx >= track->gArr.count()) return;

    CTrk &t = track->gArr[idx];
    if (t.curvePts.count() > 8) {
        t.curvePts.remove(t.curvePts.count() - 5, 5);
    }
    track->reloadModel();
    updateCurrentTrackLine(idx);
    emit saveTracks();
}

void FormTrackDrawer::setTrackVisible_Click(bool visible) {
    if (!track) return;

    int idx = track->idx();
    if (idx < 0 || idx >= track->gArr.count()) return;

    track->gArr[idx].isVisible = visible;
    TrackInterface::instance()->set_isTrackVisible(visible);
    track->reloadModel();
    updateCurrentTrackLine(idx);
    updateLines();
    emit saveTracks();
}
