// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
//
// Main event loop save/load files from file manager to QtAOG
#include "formgps.h"
#include <QDir>
#include <algorithm>
#include "cboundarylist.h"
#include "classes/settingsmanager.h"
#include "qmlutil.h"
#include <QString>
#include <QLoggingCategory>
#include "backend.h"
#include "backendaccess.h"
#include "mainwindowstate.h"
#include "boundaryinterface.h"
#include "flagsinterface.h"
#include "worldgrid.h"
#include "siminterface.h"
#include "camera.h"
#include "backend/layerservice.h"
#include "scenegraph/layertypes.h"

Q_LOGGING_CATEGORY (formgps_saveopen_log, "formgps_saveopen.qtagopengps")

enum OPEN_FLAGS {
    LOAD_MAPPING = 1,
    LOAD_HEADLAND = 2,
    LOAD_LINES = 4,
    LOAD_FLAGS = 8
};

QString caseInsensitiveFilename(const QString &directory, const QString &filename)
{
    //A bit of a hack to work with files from AOG that might not have
    //the exact case we are expecting. For example, Boundaries.Txt and
    //Headland.Txt (vs txt).

    QStringList search;
    QDir findDir(directory);

    search << filename;

    findDir.setNameFilters(search); //seems to be case insensitive
    if (findDir.count() > 0)
        return findDir[0];
    else
        return filename;

}

void FormGPS::ExportFieldAs_ISOXMLv3()
{
    //TODO use xml library
}

void FormGPS::ExportFieldAs_ISOXMLv4()
{
    //TODO use xml library

}

void FormGPS::FileSaveHeadLines()
{
#ifdef __ANDROID__
    QString directoryName = androidDirectory + QCoreApplication::applicationName() + "/Fields/" + currentFieldDirectory;
#else
    QString directoryName = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
                            + "/" + QCoreApplication::applicationName() + "/Fields/" + currentFieldDirectory;
#endif

    QDir saveDir(directoryName);
    if (!saveDir.exists()) {
        bool ok = saveDir.mkpath(directoryName);
        if (!ok) {
            qWarning() << "Couldn't create path " << directoryName;
            return;
        }
    }

    QString filename = directoryName + "/" + caseInsensitiveFilename(directoryName, "HeadLines.txt");

    QFile headfile(filename);
    if (!headfile.open(QIODevice::WriteOnly))
    {
        qWarning() << "couldn't open " << filename << "for writing!";
        return;
    }

    QTextStream writer(&headfile);
    writer.setLocale(QLocale::C);
    writer.setRealNumberNotation(QTextStream::FixedNotation);

    int cnt = hdl.tracksArr.count();
    writer << "$HeadLines" << Qt::endl;

    if (cnt > 0)
    {
        for (int i = 0; i < cnt; i++)
        {
            //write out the name
            writer << hdl.tracksArr[i].name << Qt::endl;


            //write out the moveDistance
            writer << hdl.tracksArr[i].moveDistance << Qt::endl;

            //write out the mode
            writer << hdl.tracksArr[i].mode << Qt::endl;

            //write out the A_Point index
            writer << hdl.tracksArr[i].a_point << Qt::endl;

            //write out the points of ref line
            int cnt2 = hdl.tracksArr[i].trackPts.count();

            writer << cnt2 << Qt::endl;
            if (hdl.tracksArr[i].trackPts.count() > 0)
            {
                for (int j = 0; j < cnt2; j++)
                    writer << qSetRealNumberPrecision(3) << hdl.tracksArr[i].trackPts[j].easting <<
                        "," << qSetRealNumberPrecision(3) << hdl.tracksArr[i].trackPts[j].northing <<
                        "," << qSetRealNumberPrecision(5) << hdl.tracksArr[i].trackPts[j].heading << Qt::endl;
            }
        }
    }
    else
    {
        writer << "$HeadLines" << Qt::endl;
        return;
    }

    if (hdl.idx > (hdl.tracksArr.count() - 1)) hdl.idx = hdl.tracksArr.count() - 1;

    headfile.close();
}

void FormGPS::FileLoadHeadLines()
{
#ifdef __ANDROID__
    QString directoryName = androidDirectory + QCoreApplication::applicationName() + "/Fields/" + currentFieldDirectory;
#else
    QString directoryName = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
                            + "/" + QCoreApplication::applicationName() + "/Fields/" + currentFieldDirectory;
#endif

    QDir loadDir(directoryName);
    if (!loadDir.exists()) {
        bool ok = loadDir.mkpath(directoryName);
        if (!ok) {
            qWarning() << "Couldn't create path " << directoryName;
            TimedMessageBox(1000,tr("Cannot create field directory!"),tr("Cannot create field directory at ") + directoryName);
            return;
        }
    }

    QString filename = directoryName + "/" + caseInsensitiveFilename(directoryName, "HeadLines.txt");

    QFile headfile(filename);
    if (!headfile.open(QIODevice::ReadOnly))
    {
        qWarning() << "Couldn't open " << filename << "for reading! Error was" << headfile.errorString();
        TimedMessageBox(1000,tr("Cannot read headlines."),tr("Cannot read headlines.") + tr(" Error was ") + headfile.errorString());
        return;
    }

    QTextStream reader(&headfile);
    reader.setLocale(QLocale::C);

    lock.lockForWrite();
    hdl.tracksArr.clear();
    hdl.idx = -1;

    //get the file of previous AB Lines
    QString line;

    //read header $HeadLies
    line = reader.readLine();

    while (!reader.atEnd())
    {

        hdl.tracksArr.append(CHeadPath());
        hdl.idx = hdl.tracksArr.count() - 1;

        hdl.tracksArr[hdl.idx].name = reader.readLine();

        line = reader.readLine();
        hdl.tracksArr[hdl.idx].moveDistance = line.toDouble();

        line = reader.readLine();
        hdl.tracksArr[hdl.idx].mode = line.toInt();

        line = reader.readLine();
        hdl.tracksArr[hdl.idx].a_point = line.toInt();

        line = reader.readLine();
        int numPoints = (line.toInt());

        if (numPoints > 3)
        {
            hdl.tracksArr[hdl.idx].trackPts.clear();

            hdl.tracksArr[hdl.idx].trackPts.reserve(numPoints);  // Phase 1.1: Pre-allocate
            for (int i = 0; i < numPoints; i++)
            {
                line = reader.readLine();
                // Phase 1.2: Parse without QStringList allocation
                int comma1 = line.indexOf(',');
                int comma2 = line.indexOf(',', comma1 + 1);
                if (comma1 == -1 || comma2 == -1) {
                    qDebug() << "Corrupt file!  Ignoring " << filename << ".";
                    hdl.tracksArr.clear();
                    hdl.idx = -1;
                    TimedMessageBox(1000,tr("Corrupt File!"), tr("Corrupt headline for this field. Deleting lines."));
                    FileSaveHeadLines();
                }
                Vec3 vecPt(QStringView(line).left(comma1).toDouble(),
                           QStringView(line).mid(comma1 + 1, comma2 - comma1 - 1).toDouble(),
                           QStringView(line).mid(comma2 + 1).toDouble());
                hdl.tracksArr[hdl.idx].trackPts.append(vecPt);
            }
        }
        else
        {
            if (hdl.tracksArr.count() > 0)
            {
                hdl.tracksArr.removeAt(hdl.idx);
            }
        }
    }

    hdl.idx = -1;
    lock.unlock();
}

void FormGPS::FileSaveTracks()
{
    BACKEND_TRACK(track);

#ifdef __ANDROID__
    QString directoryName = androidDirectory + QCoreApplication::applicationName() + "/Fields/" + currentFieldDirectory;
#else
    QString directoryName = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
                            + "/" + QCoreApplication::applicationName() + "/Fields/" + currentFieldDirectory;
#endif

    QDir saveDir(directoryName);
    if (!saveDir.exists()) {
        bool ok = saveDir.mkpath(directoryName);
        if (!ok) {
            qWarning() << "Couldn't create path " << directoryName;
            return;
        }
    }

    QString filename = directoryName + "/" + caseInsensitiveFilename(directoryName, "TrackLines.txt");

    int cnt = track.gArr.count();

    QFile curveFile(filename);
    if (!curveFile.open(QIODevice::WriteOnly))
    {
        qWarning() << "couldn't open " << filename << "for writing!";
        return;
    }

    QTextStream writer(&curveFile);
    writer.setLocale(QLocale::C);
    writer.setRealNumberNotation(QTextStream::FixedNotation);

    writer << "$TrackLines" << Qt::endl;
    if (cnt > 0)
    {
        for (int i = 0; i < cnt; i++)
        {
            //write out the name
            writer << track.gArr[i].name << Qt::endl;

            //write out the heading
            writer << track.gArr[i].heading << Qt::endl;

            //A and B
            writer << qSetRealNumberPrecision(3) << track.gArr[i].ptA.easting << ","
                   << qSetRealNumberPrecision(3) << track.gArr[i].ptA.northing << ","
                   << Qt::endl;

            writer << qSetRealNumberPrecision(3) << track.gArr[i].ptB.easting << ","
                   << qSetRealNumberPrecision(3) << track.gArr[i].ptB.northing << ","
                   << Qt::endl;

            //write out the nudgedistance
            writer << track.gArr[i].nudgeDistance << Qt::endl;

            //write out the mode
            writer << track.gArr[i].mode << Qt::endl;

            //visible?
            if (track.gArr[i].isVisible)
                writer << "True" << Qt::endl;
            else
                writer << "False" << Qt::endl;

            //write out the points of ref line
            int cnt2 = track.gArr[i].curvePts.count();

            writer << cnt2 << Qt::endl;
            if (track.gArr[i].curvePts.count() > 0)
            {
                for (int j = 0; j < cnt2; j++)
                    writer << qSetRealNumberPrecision(3) << track.gArr[i].curvePts[j].easting << ","
                           << qSetRealNumberPrecision(3) << track.gArr[i].curvePts[j].northing << ","
                           << qSetRealNumberPrecision(3) << track.gArr[i].curvePts[j].heading
                           << Qt::endl;
            }
        }
    }
    FileSaveABLines();
    FileSaveCurveLines();

    curveFile.close();
}

void FormGPS::FileLoadTracks()
{
    BACKEND_TRACK(track);

    track.gArr.clear();

    //current field directory should already exist
#ifdef __ANDROID__
    QString directoryName = androidDirectory + QCoreApplication::applicationName() + "/Fields/" + currentFieldDirectory;
#else
    QString directoryName = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
                            + "/" + QCoreApplication::applicationName() + "/Fields/" + currentFieldDirectory;
#endif

    QDir loadDir(directoryName);
    if (!loadDir.exists()) {
        bool ok = loadDir.mkpath(directoryName);
        if (!ok) {
            qWarning() << "Couldn't create path " << directoryName;
            TimedMessageBox(1000,tr("Cannot create field directory!"),tr("Cannot create field directory at ") + directoryName);
            return;
        }
    }

    QString filename = directoryName + "/" + caseInsensitiveFilename(directoryName, "TrackLines.txt");

    QFile headfile(filename);
    if (!headfile.open(QIODevice::ReadOnly))
    {
        FileLoadABLines();
        FileLoadCurveLines();
        FileSaveTracks();
        track.reloadModel();
        return;
    }

    QTextStream reader(&headfile);
    reader.setLocale(QLocale::C);


    QString line;

    //read header $CurveLine
    line = reader.readLine();

    lock.lockForWrite();

    while (!reader.atEnd())
    {
        line = reader.readLine();
        if(line.isNull()) break; //no more to read

        track.gArr.append(CTrk());
        track.setIdx(track.gArr.count() - 1);

        //read header $CurveLine
        track.gArr[track.idx()].name = line;

        track.gArr[track.idx()].heading = reader.readLine().toDouble();

        line = reader.readLine();
        // Phase 1.2: Parse without QStringList allocation
        int comma = line.indexOf(',');
        if (comma == -1) {
            TimedMessageBox(1000,tr("Corrupt File!"), tr("Corrupt TracksList.txt. Not all tracks were loaded."));
            track.gArr.pop_back();
            track.setIdx(track.gArr.count() - 1);
            return;
        }

        track.gArr[track.idx()].ptA = Vec2(QStringView(line).left(comma).toDouble(),
                                            QStringView(line).mid(comma + 1).toDouble());

        line = reader.readLine();
        comma = line.indexOf(',');
        if (comma == -1) {
            TimedMessageBox(1000,tr("Corrupt File!"), tr("Corrupt TracksList.txt. Not all tracks were loaded."));
            track.gArr.pop_back();
            track.setIdx(track.gArr.count() - 1);
            return;
        }

        track.gArr[track.idx()].ptB = Vec2(QStringView(line).left(comma).toDouble(),
                                            QStringView(line).mid(comma + 1).toDouble());

        line = reader.readLine();
        track.gArr[track.idx()].nudgeDistance = line.toDouble();

        line = reader.readLine();
        track.gArr[track.idx()].mode = line.toInt();

        line = reader.readLine();
        if (line == "True")
            track.gArr[track.idx()].isVisible = true;
        else
            track.gArr[track.idx()].isVisible = false;

        line = reader.readLine();
        int numPoints = line.toInt();

        if (numPoints > 3)
        {
            track.gArr[track.idx()].curvePts.clear();
            track.gArr[track.idx()].curvePts.reserve(numPoints);  // Phase 1.1: Pre-allocate

            for (int i = 0; i < numPoints; i++)
            {
                line = reader.readLine();
                // Phase 1.2: Parse without QStringList allocation
                int comma1 = line.indexOf(',');
                int comma2 = line.indexOf(',', comma1 + 1);
                if (comma1 == -1 || comma2 == -1) {
                    TimedMessageBox(1000,tr("Corrupt File!"), tr("Corrupt TracksList.txt. Not all tracks were loaded."));
                    track.gArr.pop_back();
                    track.setIdx(track.gArr.count() - 1);
                    return;
                }

                track.gArr[track.idx()].curvePts.append(Vec3(QStringView(line).left(comma1).toDouble(),
                                                       QStringView(line).mid(comma1 + 1, comma2 - comma1 - 1).toDouble(),
                                                       QStringView(line).mid(comma2 + 1).toDouble()));
            }
        }
    }
    track.setIdx(-1);
    lock.unlock();

    track.reloadModel();

}

void FormGPS::FileSaveCurveLines()
{
    BACKEND_TRACK(track);

#ifdef __ANDROID__
    QString directoryName = androidDirectory + QCoreApplication::applicationName() + "/Fields/" + currentFieldDirectory;
#else
    QString directoryName = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
                            + "/" + QCoreApplication::applicationName() + "/Fields/" + currentFieldDirectory;
#endif

    QDir saveDir(directoryName);
    if (!saveDir.exists()) {
        bool ok = saveDir.mkpath(directoryName);
        if (!ok) {
            qWarning() << "Couldn't create path " << directoryName;
            return;
        }
    }

    QString filename = directoryName + "/" + caseInsensitiveFilename(directoryName, "CurveLines.txt");

    int cnt = track.gArr.count();

    QFile curveFile(filename);
    if (!curveFile.open(QIODevice::WriteOnly))
    {
        qWarning() << "couldn't open " << filename << "for writing!";
        return;
    }

    QTextStream writer(&curveFile);
    writer.setLocale(QLocale::C);
    writer.setRealNumberNotation(QTextStream::FixedNotation);

    writer << "$CurveLines" << Qt::endl;

    for (int i = 0; i < cnt; i++)
    {
        if (track.gArr[i].mode != TrackMode::Curve) continue;

        //write out the Name
        writer << track.gArr[i].name << Qt::endl;

        //write out the heading
        writer << track.gArr[i].heading << Qt::endl;

        //write out the points of ref line
        int cnt2 = track.gArr[i].curvePts.count();

        writer << cnt2 << Qt::endl;
        if (track.gArr[i].curvePts.count() > 0)
        {
            for (int j = 0; j < cnt2; j++)
                writer << qSetRealNumberPrecision(3) << track.gArr[i].curvePts[j].easting << ","
                       << qSetRealNumberPrecision(3) << track.gArr[i].curvePts[j].northing << ","
                       << qSetRealNumberPrecision(5) << track.gArr[i].curvePts[j].heading << Qt::endl;
        }
    }

    curveFile.close();
}

void FormGPS::FileLoadCurveLines()
{
    BACKEND_TRACK(track);

    //This method is only used if there is no TrackLines.txt and we are importing the old
    //CurveLines.txtfile

    //current field directory should already exist
#ifdef __ANDROID__
    QString directoryName = androidDirectory + QCoreApplication::applicationName() + "/Fields/" + currentFieldDirectory;
#else
    QString directoryName = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
                            + "/" + QCoreApplication::applicationName() + "/Fields/" + currentFieldDirectory;
#endif

    QDir loadDir(directoryName);
    if (!loadDir.exists()) {
        bool ok = loadDir.mkpath(directoryName);
        if (!ok) {
            qWarning() << "Couldn't create path " << directoryName;
            return;
        }
    }

    QString filename = directoryName + "/" + caseInsensitiveFilename(directoryName, "CurveLines.txt");

    QFile curveFile(filename);
    if (!curveFile.open(QIODevice::ReadOnly))
    {
        TimedMessageBox(1500, tr("Field Error"), (tr("Couldn't open ") + filename + tr("for reading!")));
        return;
    }

    QTextStream reader(&curveFile);
    reader.setLocale(QLocale::C);

    QString line;

    //read header $CurveLine
    line = reader.readLine();

    lock.lockForWrite();
    while (!reader.atEnd())
    {
        line = reader.readLine();
        if(line.isNull()) break; //no more to read

        track.gArr.append(CTrk());

        //read header $CurveLine
        QString nam = reader.readLine();

        if (nam.length() > 4 && nam.mid(0,5) == "Bound")
        {
            track.gArr[track.gArr.count() - 1].name = nam;
            track.gArr[track.gArr.count() - 1].mode = TrackMode::bndCurve;
        }
        else
        {
            if (nam.length() > 2 && nam.mid(0,2) != "Cu")
                track.gArr[track.gArr.count() - 1].name = "Cu " + nam;
            else
                track.gArr[track.gArr.count() - 1].name = nam;

            track.gArr[track.gArr.count() - 1].mode = TrackMode::Curve;
        }

        // get the average heading
        line = reader.readLine();
        track.gArr[track.gArr.count() - 1].heading = line.toDouble();

        line = reader.readLine();
        int numPoints = line.toInt();

        if (numPoints > 1)
        {
            track.gArr[track.gArr.count() - 1].curvePts.clear();
            track.gArr[track.gArr.count() - 1].curvePts.reserve(numPoints);  // Phase 1.1: Pre-allocate

            for (int i = 0; i < numPoints; i++)
            {
                line = reader.readLine();
                // Phase 1.2: Parse without QStringList allocation
                int comma1 = line.indexOf(',');
                int comma2 = line.indexOf(',', comma1 + 1);
                if (comma1 == -1 || comma2 == -1) {
                    qDebug() << "Corrupt CurvesList.txt.";
                    track.gArr.pop_back();
                    track.setIdx(-1);
                    return;
                }
                Vec3 vecPt(QStringView(line).left(comma1).toDouble(),
                           QStringView(line).mid(comma1 + 1, comma2 - comma1 - 1).toDouble(),
                           QStringView(line).mid(comma2 + 1).toDouble());
                track.gArr[track.gArr.count() - 1].curvePts.append(vecPt);
            }
            track.gArr[track.gArr.count() - 1].ptB.easting = track.gArr[track.gArr.count() - 1].curvePts[0].easting;
            track.gArr[track.gArr.count() - 1].ptB.northing = track.gArr[track.gArr.count() - 1].curvePts[0].northing;

            track.gArr[track.gArr.count() - 1].ptB.easting = track.gArr[track.gArr.count() - 1].curvePts[track.gArr[track.gArr.count() - 1].curvePts.count() - 1].easting;
            track.gArr[track.gArr.count() - 1].ptB.northing = track.gArr[track.gArr.count() - 1].curvePts[track.gArr[track.gArr.count() - 1].curvePts.count() - 1].northing;
            track.gArr[track.gArr.count() - 1].isVisible = true;
        }
        else
        {
            if (track.gArr.count() > 0)
            {
                track.gArr.pop_back();
            }
        }
    }
    lock.unlock();
    curveFile.close();
}

void FormGPS::FileSaveABLines()
{
    BACKEND_TRACK(track);

#ifdef __ANDROID__
    QString directoryName = androidDirectory + QCoreApplication::applicationName() + "/Fields/" + currentFieldDirectory;
#else
    QString directoryName = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
                            + "/" + QCoreApplication::applicationName() + "/Fields/" + currentFieldDirectory;
#endif

    QDir saveDir(directoryName);
    if (!saveDir.exists()) {
        bool ok = saveDir.mkpath(directoryName);
        if (!ok) {
            qWarning() << "Couldn't create path " << directoryName;
            return;
        }
    }

    QString filename = directoryName + "/" + caseInsensitiveFilename(directoryName, "ABLines.txt" );

    QFile lineFile(filename);
    if (!lineFile.open(QIODevice::WriteOnly))
    {
        qWarning() << "Couldn't open " << filename << "for writing!";
        return;
    }

    QTextStream writer(&lineFile);
    writer.setLocale(QLocale::C);
    writer.setRealNumberNotation(QTextStream::FixedNotation);

    int cnt = track.gArr.count();

    if (cnt > 0)
    {
        for (CTrk &item : track.gArr)
        {
            if (item.mode == TrackMode::AB)
            {
                //make it culture invariant
                writer << item.name << ","
                       << qSetRealNumberPrecision(8) << glm::toDegrees(item.heading) << ","
                       << qSetRealNumberPrecision(3) << item.ptA.easting << ","
                       << qSetRealNumberPrecision(3) << item.ptA.northing << Qt::endl;
            }
        }
    }

    lineFile.close();
}

void FormGPS::FileLoadABLines()
{
    //This method is only used if TracksLines.txt is not present. This loads the old ABLines.txt
    //into the new unified tracks system.  When importing the old lines files, this method must
    //run before FileLoadCurveLines().

    //current field directory should already exist
    BACKEND_TRACK(track);

#ifdef __ANDROID__
    QString directoryName = androidDirectory + QCoreApplication::applicationName() + "/Fields/" + currentFieldDirectory;
#else
    QString directoryName = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
                            + "/" + QCoreApplication::applicationName() + "/Fields/" + currentFieldDirectory;
#endif

    QDir loadDir(directoryName);
    if (!loadDir.exists()) {
        bool ok = loadDir.mkpath(directoryName);
        if (!ok) {
            qWarning() << "Couldn't create path " << directoryName;
            return;
        }
    }

    QString filename = directoryName + "/" + caseInsensitiveFilename(directoryName, "ABLines.txt");

    QFile linesFile(filename);
    if (!linesFile.open(QIODevice::ReadOnly))
    {
        TimedMessageBox(1500, tr("Field Error"), (tr("Couldn't open ") + filename + tr(" for reading!")));
        return;
    }

    QTextStream reader(&linesFile);
    reader.setLocale(QLocale::C);

    QString line;

    lock.lockForWrite();
    //read all the lines
    for (int i = 0; !reader.atEnd(); i++)
    {

        line = reader.readLine();
        // Phase 1.2: Parse without QStringList allocation
        int comma1 = line.indexOf(',');
        int comma2 = line.indexOf(',', comma1 + 1);
        int comma3 = line.indexOf(',', comma2 + 1);

        if (comma1 == -1 || comma2 == -1 || comma3 == -1) {
            qDebug() << "Corrupt ABLines.txt.";
            return;
        }

        track.gArr.append(CTrk());

        QString name = line.left(comma1);
        if (name.length() > 2 && name.mid(0,2) != "AB")
            track.gArr[i].name = "AB " + name;
        else
            track.gArr[i].name = name;

        track.gArr[i].mode = TrackMode::AB;

        track.gArr[i].heading = glm::toRadians(QStringView(line).mid(comma1 + 1, comma2 - comma1 - 1).toDouble());
        track.gArr[i].ptA.easting = QStringView(line).mid(comma2 + 1, comma3 - comma2 - 1).toDouble();
        track.gArr[i].ptB.northing = QStringView(line).mid(comma3 + 1).toDouble();
        track.gArr[i].ptB.easting = track.gArr[i].ptA.easting + (sin(track.gArr[i].heading) * 100);
        track.gArr[i].ptB.northing = track.gArr[i].ptA.northing + (cos(track.gArr[i].heading) * 100);
        track.gArr[i].isVisible = true;
    }

    lock.unlock();
    linesFile.close();
}

QMap<QString,QVariant> FormGPS::FileFieldInfo(QString filename)
{
    QMap<QString,QVariant> field_info;

    QString directoryName =  filename.left(filename.indexOf("/Field.txt"));
    QString fieldDir = directoryName.mid(filename.lastIndexOf("Fields/") + 7);

    QFile fieldFile(filename);
    if (!fieldFile.open(QIODevice::ReadOnly))
    {
        TimedMessageBox(5000, tr("Field Error"), (fieldDir + tr(" is missing Field.txt! It will be ignored and should probably be deleted.")));
        return field_info;
    }

    QTextStream reader(&fieldFile);
    reader.setLocale(QLocale::C);

    //start to read the file
    QString line;

    //Date time line
    line = reader.readLine();

    //dir header $FieldDir
    line = reader.readLine();

    //read field directory
    line = reader.readLine();

    field_info["name"] = line.trimmed();
    if (field_info["name"] != fieldDir.trimmed()) {
        field_info["name"] = fieldDir;
    }

    //Offset header
    line = reader.readLine();

    //read the Offsets
    line = reader.readLine();
    // Phase 1.2: Parse without QStringList allocation (offsets not used currently)
    // QStringList offs = line.split(',');

    //convergence angle update
    if (!reader.atEnd())
    {
        line = reader.readLine(); //Convergence
        line = reader.readLine();
    }

    //start positions
    if (!reader.atEnd())
    {
        line = reader.readLine(); //eat StartFix
        line = reader.readLine();
        // Phase 1.2: Parse without QStringList allocation
        int comma = line.indexOf(',');
        if (comma != -1) {
            field_info["latitude"] = QStringView(line).left(comma).toDouble();
            field_info["longitude"] = QStringView(line).mid(comma + 1).toDouble();
        }
    }

    fieldFile.close();

    //Boundaries
    filename = QDir(directoryName).filePath(caseInsensitiveFilename(directoryName, "Boundary.txt"));

    double area = CBoundary::getSavedFieldArea(filename);

    if (area>0) {
        field_info["hasBoundary"] = true;
        field_info["boundaryArea"] = area;
    } else {
        field_info["hasBoundary"] = false;
        field_info["boundaryArea"] = (double)-10;
    }

    return field_info;
}

bool FormGPS::FileOpenField(QString fieldDir, int flags)
{
    CNMEA &pn = *Backend::instance()->pn();
    WorldGrid &worldGrid = *WorldGrid::instance();
    BACKEND_TRACK(track);
    Camera &camera = *Camera::instance();

#ifdef __ANDROID__
    QString directoryName = androidDirectory + QCoreApplication::applicationName() + "/Fields/" + fieldDir;
#else
    QString directoryName = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
                            + "/" + QCoreApplication::applicationName() + "/Fields/" + fieldDir;
#endif

    QString filename = directoryName + "/" + caseInsensitiveFilename(directoryName, "Field.txt");

    QFile fieldFile(filename);
    if (!fieldFile.open(QIODevice::ReadOnly))
    {
        TimedMessageBox(1500, tr("Field Error"), (tr("Couldn't open field ") + filename + tr(" for reading!")));
        return false;
    }

    QTextStream reader(&fieldFile);
    reader.setLocale(QLocale::C);

    //close the existing job and reset everything
    JobClose();

    //and open a new job
    JobNew();
    bnd.loadSettings();
    //Saturday, February 11, 2017  -->  7:26:52 AM
    //$FieldDir
    //Bob_Feb11
    //$Offsets
    //533172,5927719,12 - offset easting, northing, zone

    //start to read the file
    QString line;

    //Date time line
    line = reader.readLine();

    //dir header $FieldDir
    line = reader.readLine();

    //read field directory
    line = reader.readLine();

    currentFieldDirectory = fieldDir;
    SettingsManager::instance()->setF_currentDir(currentFieldDirectory);

    //Offset header
    line = reader.readLine();

    //read the Offsets
    line = reader.readLine();
    // Phase 1.2: Parse without QStringList allocation (currently commented out)
    // QStringList offs = line.split(',');
    //pn.utmEast = offs[0].toInt();
    //pn.utmNorth = offs[1].toInt();
    //pn.actualEasting = offs[0].toDouble();
    //pn.actualNorthing = offs[1].toDouble();
    //pn.zone = offs[2].toInt();
    //isFirstFixPositionSet = true;

    //convergence angle update
    if (!reader.atEnd())
    {
        line = reader.readLine(); //Convergence
        line = reader.readLine();
        //pn.convergenceAngle = line.toDouble();
        //TODO lblConvergenceAngle.Text = Math.Round(glm.toDegrees(pn.convergenceAngle), 3).ToString();
    }

    //start positions
    if (!reader.atEnd())
    {
        line = reader.readLine(); //eat StartFix
        line = reader.readLine();
        // Phase 1.2: Parse without QStringList allocation
        int comma = line.indexOf(',');
        if (comma != -1) {
            // Phase 6.3.1: Use PropertyWrapper for safe property access
            pn.setLatStart(QStringView(line).left(comma).toDouble());
            pn.setLonStart(QStringView(line).mid(comma + 1).toDouble());
        }

        // Qt 6.8 TRACK RESTORATION: Load active track index if present
        if (!reader.atEnd())
        {
            line = reader.readLine(); //check for $ActiveTrackIndex
            if (line == "$ActiveTrackIndex")
            {
                line = reader.readLine();
                int activeTrackIndex = line.toInt();
                qDebug() << "🎯 TRACK RESTORE: Found saved active track index:" << activeTrackIndex;
                track.setIdx(activeTrackIndex);
            }
            else
            {
                // No $ActiveTrackIndex found, reset to no active track
                qDebug() << "📍 TRACK RESTORE: No saved track index found, defaulting to -1";
                track.setIdx(-1);
            }
        }
        else
        {
            // File ended, no track index
            qDebug() << "📍 TRACK RESTORE: Field file ended before track index, defaulting to -1";
            track.setIdx(-1);
        }

        if (SimInterface::instance()->isRunning())
        {
            // Phase 6.3.1: Use PropertyWrapper for safe property access
            pn.latitude = pn.latStart();
            pn.longitude = pn.lonStart();

            SettingsManager::instance()->setGps_simLatitude(pn.latStart());
            SettingsManager::instance()->setGps_simLongitude(pn.lonStart());
            SimInterface::instance()->reset();

            pn.SetLocalMetersPerDegree();
        } else {
            // Phase 6.0.4: Use Q_PROPERTY direct access instead of qmlItem
            pn.SetLocalMetersPerDegree();
        }
    }

    fieldFile.close();


    if (flags & LOAD_LINES) {
        // Qt 6.8 TRACK RESTORATION: Save restored track index before FileLoadTracks() overwrites it
        int savedActiveTrackIndex = track.idx();
        qDebug() << "💾 TRACK RESTORE: Saving restored index before track loading:" << savedActiveTrackIndex;

        // ABLine -------------------------------------------------------------------------------------------------
        FileLoadTracks();

        // Qt 6.8 TRACK RESTORATION: Restore the saved track index after loading
        if (savedActiveTrackIndex >= 0 && savedActiveTrackIndex < track.gArr.count()) {
            track.setIdx(savedActiveTrackIndex);
            qDebug() << "✅ TRACK RESTORE: Restored active track index after loading:" << savedActiveTrackIndex;
        } else if (track.gArr.count() > 0) {
            // If saved index is invalid but we have tracks, select first track
            track.setIdx(0);
            qDebug() << "📍 TRACK RESTORE: Invalid saved index, defaulting to first track (0)";
        } else {
            // No tracks available, keep -1
            qDebug() << "❌ TRACK RESTORE: No tracks available, keeping idx = -1";
        }
    }

    if (flags & LOAD_MAPPING) {
        //section patches
        filename = directoryName + "/" + caseInsensitiveFilename(directoryName, "Sections.txt");

        QFile sectionsFile(filename);
        if (!sectionsFile.open(QIODevice::ReadOnly))
        {
            TimedMessageBox(1500, tr("Field Error"), (tr("Couldn't open sections ") + filename + tr(" for reading!")));
        } else
        {
            // Phase 1.3: Parse without lock, lock only for final assignment
            reader.setDevice(&sectionsFile);
            bool isv3 = false;
            QVector3D vecFix;

            // Local temporary storage - no lock needed
            QVector<QSharedPointer<PatchTriangleList>> localPatchList;
            QVector<QSharedPointer<PatchTriangleList>> localTriangleList;
            QVector<QSharedPointer<PatchBoundingBox>> localPatchBoundingBoxList;

            double localWorkedArea = 0.0;

            //read header
            while (!reader.atEnd())
            {
                line = reader.readLine();
                if (line.contains("ect"))
                {
                    isv3 = true;
                    break;
                }

                int verts = line.toInt();

                QSharedPointer<PatchTriangleList> triangleList = QSharedPointer<PatchTriangleList>(new PatchTriangleList);
                QSharedPointer<PatchBoundingBox> boundingbox = QSharedPointer<PatchBoundingBox>(new PatchBoundingBox);

                triangleList->reserve(verts);  // Phase 1.1: Pre-allocate memory
                localPatchList.append(triangleList);
                localTriangleList.append(triangleList);
                localPatchBoundingBoxList.append(boundingbox);

                for (int v = 0; v < verts; v++)
                {
                   line = reader.readLine();
                    // Phase 1.2: Parse without QStringList allocation (10,000+ iterations)
                    int comma1 = line.indexOf(',');
                    int comma2 = line.indexOf(',', comma1 + 1);
                    vecFix.setX(QStringView(line).left(comma1).toDouble());
                    vecFix.setY(QStringView(line).mid(comma1 + 1, comma2 - comma1 - 1).toDouble());
                    vecFix.setZ(QStringView(line).mid(comma2 + 1).toDouble());
                    triangleList->append(vecFix);

                    if (v > 0) {
                        if (vecFix.x() < (*boundingbox).minx) (*boundingbox).minx = vecFix.x();
                        if (vecFix.x() > (*boundingbox).maxx) (*boundingbox).maxx = vecFix.x();
                        if (vecFix.y() < (*boundingbox).miny) (*boundingbox).miny = vecFix.y();
                        if (vecFix.y() > (*boundingbox).maxy) (*boundingbox).maxy = vecFix.y();
                    }
                }

                // Validate and clean up triangle strip
                // Detect and fix triangle fans stored as strips, remove degenerates
                if (triangleList->count() >= 4) {  // Need color + at least 3 vertices
                    int originalCount = triangleList->count();
                    int numVerts = triangleList->count() - 1;  // Exclude color

                    // First pass: check if this is a triangle fan pattern
                    // Count frequency of each unique vertex (using indices as keys)
                    QVector<int> vertexCounts(numVerts, 0);
                    QVector<int> firstOccurrence(numVerts, -1);
                    int uniqueCount = 0;

                    for (int i = 0; i < numVerts; ++i) {
                        const QVector3D& v = (*triangleList)[i + 1];
                        // Check if we've seen this vertex before
                        bool found = false;
                        for (int j = 0; j < uniqueCount; ++j) {
                            if ((*triangleList)[firstOccurrence[j] + 1] == v) {
                                vertexCounts[j]++;
                                found = true;
                                break;
                            }
                        }
                        if (!found) {
                            firstOccurrence[uniqueCount] = i;
                            vertexCounts[uniqueCount] = 1;
                            uniqueCount++;
                        }
                    }

                    // Find if there's a common center point (appears in >25% of vertices)
                    int centerIndex = -1;
                    bool isFanPattern = false;
                    int threshold = numVerts / 4;  // 25% threshold

                    for (int i = 0; i < uniqueCount; ++i) {
                        if (vertexCounts[i] > threshold && vertexCounts[i] > 2) {
                            centerIndex = firstOccurrence[i];
                            isFanPattern = true;
                            break;
                        }
                    }

                    const QVector3D& centerPoint = (centerIndex >= 0) ? (*triangleList)[centerIndex + 1] : QVector3D();

                    PatchTriangleList cleanedList;
                    cleanedList.reserve(triangleList->count());
                    cleanedList.append((*triangleList)[0]);  // Keep color

                    if (isFanPattern) {
                        // This is a triangle fan - extract unique outer vertices
                        //qDebug(formgps_saveopen_log) << "Detected triangle fan pattern with center point, converting to proper strip";

                        QVector<QVector3D> outerVertices;
                        outerVertices.reserve(triangleList->count());

                        // Collect unique outer vertices IN ORDER OF FIRST APPEARANCE
                        // (excluding the center and duplicates)
                        for (int i = 1; i < triangleList->count(); ++i) {
                            const QVector3D& v = (*triangleList)[i];
                            if (v != centerPoint && !outerVertices.contains(v)) {
                                outerVertices.append(v);
                            }
                        }

                        // Create proper triangle strip from fan
                        // Pattern: center, v0, v1, center, v2, center, v3... creates triangles:
                        // (center, v0, v1), (v0, v1, center), (v1, center, v2), (center, v2, center), (v2, center, v3)...
                        // The key is that center must be repeated to keep it in every triangle
                        if (outerVertices.count() >= 2) {
                            cleanedList.append(centerPoint);       // C
                            cleanedList.append(outerVertices[0]);  // v0
                            cleanedList.append(outerVertices[1]);  // v1

                            for (int i = 2; i < outerVertices.count(); ++i) {
                                cleanedList.append(centerPoint);       // C (repeated)
                                cleanedList.append(outerVertices[i]);  // vi
                            }

                            //qDebug(formgps_saveopen_log) << "Converted fan with" << outerVertices.count()
                            //                             << "outer vertices to proper strip with" << cleanedList.count() << "total vertices";
                        }
                    }

                    // Only use cleaned list if we have enough vertices for at least one triangle
                    if (cleanedList.count() >= 4) {
                        *triangleList = cleanedList;
                    }
                    verts = triangleList->count();
                }

                //calculate area of this patch - AbsoluteValue of (Ax(By-Cy) + Bx(Cy-Ay) + Cx(Ay-By)/2)
                verts -= 2;
                if (verts >= 2)
                {
                    for (int j = 1; j < verts; j++)
                    {
                        double temp = 0;
                        temp = (*triangleList)[j].x() * ((*triangleList)[j + 1].y() - (*triangleList)[j + 2].y()) +
                               (*triangleList)[j + 1].x() * ((*triangleList)[j + 2].y() - (*triangleList)[j].y()) +
                               (*triangleList)[j + 2].x() * ((*triangleList)[j].y() - (*triangleList)[j + 1].y());

                        localWorkedArea += fabs((temp * 0.5));
                    }
                }

                //was old version prior to v4
                if (isv3)
                {
                    //Append the current list to the field file
                }
            }

            sectionsFile.close();

            // Phase 1.3: Lock only for final assignment (< 50ms)
            lock.lockForWrite();
            Backend::instance()->currentField_setDistanceUser(0.0);
            tool.triStrip[0].triangleList = localTriangleList.isEmpty() ? QSharedPointer<PatchTriangleList>(new PatchTriangleList) : localTriangleList.last();
            tool.triStrip[0].patchList = localPatchList;
            tool.triStrip[0].patchBoundingBoxList = localPatchBoundingBoxList;
            tool.patchesBufferDirty = true;
            Backend::instance()->currentField_addWorkedAreaTotal(localWorkedArea);
            lock.unlock();

            // Convert triangle strips to individual triangles for Layers system
            // This runs outside the lock since we're only reading localPatchList
            LayerService *layerService = LayerService::instance();
            layerService->clearAllLayers();

            QVector<CoverageTriangle> layerTriangles;
            // Estimate capacity: each patch has (count-3) triangles (count-1 vertices, minus 2 for strip)
            int estimatedTriangles = 0;
            for (const auto &triList : localPatchList) {
                if (triList->count() > 3) {
                    estimatedTriangles += triList->count() - 3;
                }
            }
            layerTriangles.reserve(estimatedTriangles);

            for (const auto &triList : localPatchList) {
                int count = triList->count();
                // Need at least color + 3 vertices for one triangle
                if (count < 4) continue;

                // First element is color encoded as QVector3D(r, g, b)
                const QVector3D &colorVec = (*triList)[0];
                QColor color;
                if (colorVec.x() > 1 || colorVec.y() > 1 || colorVec.z() > 1) {
                    //use RGB byte values instead of float
                    color = QColor::fromRgb(colorVec.x(), colorVec.y(), colorVec.z());
                } else {
                    //use float
                    color = QColor::fromRgbF(colorVec.x(), colorVec.y(), colorVec.z());
                }
                if (SettingsManager::instance()->display_isDayMode())
                    color.setAlpha(152);
                else
                    color.setAlpha(76);

                // Convert triangle strip to individual triangles
                // Vertices start at index 1 (after color)
                // For N vertices, we have N-2 triangles
                int numVerts = count - 1;  // Subtract color
                int numTris = numVerts - 2;

                for (int i = 0; i < numTris; i++) {
                    int idx0 = 1 + i;      // Base vertex (after color)
                    int idx1 = 1 + i + 1;
                    int idx2 = 1 + i + 2;

                    const QVector3D &v0 = (*triList)[idx0];
                    const QVector3D &v1 = (*triList)[idx1];
                    const QVector3D &v2 = (*triList)[idx2];

                    // Alternate winding for triangle strip
                    if (i % 2 == 0) {
                        // Even triangle: (v0, v1, v2)
                        layerTriangles.append(CoverageTriangle(v0, v1, v2, color));
                    } else {
                        // Odd triangle: (v1, v0, v2) to maintain consistent winding
                        layerTriangles.append(CoverageTriangle(v1, v0, v2, color));
                    }
                }
            }

            // Add all triangles to the default layer
            if (!layerTriangles.isEmpty()) {
                layerService->addTrianglesToDefault(layerTriangles);
            }
        }
    }

    // Contour points ----------------------------------------------------------------------------

    filename = directoryName + "/" + caseInsensitiveFilename(directoryName, "Contour.txt");

    QFile contourFile(filename);
    if (!contourFile.open(QIODevice::ReadOnly))
    {
        TimedMessageBox(1500, tr("Field Error"), (tr("Couldn't open contour ") + filename + tr(" for reading!")));
    } else
    {
        // Phase 1.3: Parse without lock
        reader.setDevice(&contourFile);

        //read header
        line = reader.readLine();

        // Local temporary storage - no lock needed
        QVector<QSharedPointer<QVector<Vec3>>> localStripList;

        while (!reader.atEnd())
        {
            //read how many vertices in the following patch
            line = reader.readLine();
            int verts = line.toInt();

            Vec3 vecFix(0, 0, 0);

            QSharedPointer<QVector<Vec3>> ptList = QSharedPointer<QVector<Vec3>>(new QVector<Vec3>());
            ptList->reserve(verts);  // Phase 1.1: Pre-allocate memory
            localStripList.append(ptList);

            for (int v = 0; v < verts; v++)
            {
                line = reader.readLine();
                // Phase 1.2: Parse without QStringList allocation
                int comma1 = line.indexOf(',');
                int comma2 = line.indexOf(',', comma1 + 1);
                vecFix.easting = QStringView(line).left(comma1).toDouble();
                vecFix.northing = QStringView(line).mid(comma1 + 1, comma2 - comma1 - 1).toDouble();
                vecFix.heading = QStringView(line).mid(comma2 + 1).toDouble();
                ptList->append(vecFix);
            }
        }

        contourFile.close();

        // Phase 1.3: Lock only for final assignment (< 50ms)
        lock.lockForWrite();
        ct.ptList = localStripList.isEmpty() ? QSharedPointer<QVector<Vec3>>(new QVector<Vec3>()) : localStripList.last();
        ct.stripList = localStripList;
        lock.unlock();
    }

    // Flags -------------------------------------------------------------------------------------------------

    if (flags & LOAD_FLAGS) {

        //Either exit or update running save
        filename = directoryName + "/" + caseInsensitiveFilename(directoryName, "Flags.txt");

        QFile flagsFile(filename);
        if (!flagsFile.open(QIODevice::ReadOnly))
        {
            TimedMessageBox(1500, tr("Field Error"), (tr("Couldn't open flags ") + filename + tr(" for reading!")));
        } else
        {

            reader.setDevice(&flagsFile);

            FlagsInterface::instance()->flagModel()->clear();

            //read header
            line = reader.readLine();

            //number of flags
            line = reader.readLine();
            int points = line.toInt();

            if (points > 0)
            {
                double lat;
                double longi;
                double east;
                double nort;
                double head;
                int color, ID;
                QString notes;


                for (int v = 0; v < points; v++)
                {
                    line = reader.readLine();
                    // Phase 1.2: Parse without QStringList allocation
                    int comma1 = line.indexOf(',');
                    int comma2 = line.indexOf(',', comma1 + 1);
                    int comma3 = line.indexOf(',', comma2 + 1);
                    int comma4 = line.indexOf(',', comma3 + 1);
                    int comma5 = line.indexOf(',', comma4 + 1);
                    int comma6 = line.indexOf(',', comma5 + 1);
                    int comma7 = line.indexOf(',', comma6 + 1);

                    // Check format: 8 fields (with heading) or 6 fields (without heading)
                    if (comma7 != -1)  // 8 fields
                    {
                        lat = QStringView(line).left(comma1).toDouble();
                        longi = QStringView(line).mid(comma1 + 1, comma2 - comma1 - 1).toDouble();
                        east = QStringView(line).mid(comma2 + 1, comma3 - comma2 - 1).toDouble();
                        nort = QStringView(line).mid(comma3 + 1, comma4 - comma3 - 1).toDouble();
                        head = QStringView(line).mid(comma4 + 1, comma5 - comma4 - 1).toDouble();
                        color = QStringView(line).mid(comma5 + 1, comma6 - comma5 - 1).toInt();
                        ID = QStringView(line).mid(comma6 + 1, comma7 - comma6 - 1).toInt();
                        notes = QStringView(line).mid(comma7 + 1).trimmed().toString();
                    }
                    else  // 6 fields (old format)
                    {
                        lat = QStringView(line).left(comma1).toDouble();
                        longi = QStringView(line).mid(comma1 + 1, comma2 - comma1 - 1).toDouble();
                        east = QStringView(line).mid(comma2 + 1, comma3 - comma2 - 1).toDouble();
                        nort = QStringView(line).mid(comma3 + 1, comma4 - comma3 - 1).toDouble();
                        head = 0;
                        color = QStringView(line).mid(comma4 + 1, comma5 - comma4 - 1).toInt();
                        ID = QStringView(line).mid(comma5 + 1).toInt();
                        notes = "";
                    }

                    lock.lockForWrite();
                    FlagModel::Flag flagPt ( {ID, color, lat, longi, head, east, nort, notes} );
                    FlagsInterface::instance()->flagModel()->addFlag(flagPt);
                    lock.unlock();
                }
            }

            //a bit hackish but sync FlagsInterface count with
            //the model count, in case any properties are bound to it
            FlagsInterface::instance()->syncCount();
            flagsFile.close();

        }
    }

    //Boundaries
    //Either exit or update running save
    filename = directoryName + "/" + caseInsensitiveFilename(directoryName, "Boundary.txt");

    QFile boundariesFile(filename);
    if (!boundariesFile.open(QIODevice::ReadOnly))
    {
        TimedMessageBox(1500, tr("Field Error"), (tr("Couldn't open boundaries ") + filename + tr(" for reading!")));
    } else
    {
        // Phase 1.3: Parse without lock
        reader.setDevice(&boundariesFile);
        //read header
        line = reader.readLine();//Boundary

        // Local temporary storage - no lock needed
        QVector<CBoundaryList> localBndList;

        for (int k = 0; true; k++)
        {
            if (reader.atEnd()) break;
            CBoundaryList New;

            //True or False OR points from older boundary files
            line = reader.readLine();

            //Check for older boundary files, then above line string is num of points
            if (line == "True")
            {
                New.isDriveThru = true;
                line = reader.readLine();
            } else if (line == "False")
            {
                New.isDriveThru = false;
                line = reader.readLine(); //number of points
            }

            //Check for latest boundary files, then above line string is num of points
            if (line == "True" || line == "False")
            {
                line = reader.readLine(); //number of points
            }

            int numPoints = line.toInt();

            if (numPoints > 0)
            {
                New.fenceLine.reserve(numPoints);  // Phase 1.1: Pre-allocate memory

                //load the line
                for (int i = 0; i < numPoints; i++)
                {
                    line = reader.readLine();
                    // Phase 1.2: Parse without QStringList allocation (60,000+ iterations!)
                    int comma1 = line.indexOf(',');
                    int comma2 = line.indexOf(',', comma1 + 1);
                    Vec3 vecPt( QStringView(line).left(comma1).toDouble(),
                               QStringView(line).mid(comma1 + 1, comma2 - comma1 - 1).toDouble(),
                               QStringView(line).mid(comma2 + 1).toDouble() );

                    //if (turnheading)
                    //{
                    //    vecPt.heading = vecPt.heading + Math.PI;
                    //}
                    New.fenceLine.append(vecPt);
                }

                New.CalculateFenceArea(k);

                double delta = 0;
                New.fenceLineEar.clear();
                New.fenceLineEar.reserve(New.fenceLine.count());  // Phase 1.1: Pre-allocate worst-case size

                for (int i = 0; i < New.fenceLine.count(); i++)
                {
                    if (i == 0)
                    {
                        New.fenceLineEar.append(Vec2(New.fenceLine[i].easting, New.fenceLine[i].northing));
                        continue;
                    }
                    delta += (New.fenceLine[i - 1].heading - New.fenceLine[i].heading);
                    if (fabs(delta) > 0.005)
                    {
                        New.fenceLineEar.append(Vec2(New.fenceLine[i].easting, New.fenceLine[i].northing));
                        delta = 0;
                    }
                }
                localBndList.append(New);
            }
        }

        boundariesFile.close();

        // Phase 1.3: Lock only for final assignment (< 50ms)
        lock.lockForWrite();
        bnd.bndList = localBndList;
        calculateMinMax();
        bnd.BuildTurnLines();

        //let GUI know it can show btnABDraw
        BoundaryInterface::instance()->set_count(bnd.bndList.count());

        lock.unlock();
    }
    // Headland  -------------------------------------------------------------------------------------------------
    if (flags & LOAD_HEADLAND) {
        filename = directoryName + "/" + caseInsensitiveFilename(directoryName, "Headland.txt");

        QFile headlandFile(filename);
        if (!headlandFile.open(QIODevice::ReadOnly))
        {
            TimedMessageBox(1500, tr("Field Error"), (tr("Couldn't open headland ") + filename + tr(" for reading!")));
        } else {
            // Phase 1.3: Parse without lock
            reader.setDevice(&headlandFile);

            //read header
            line = reader.readLine();

            // Local temporary storage - no lock needed
            QVector<QVector<Vec3>> localHdLines;

            for (int k = 0; true; k++)
            {
                if (reader.atEnd()) break;

                //read the number of points
                line = reader.readLine();
                int numPoints = line.toInt();

                QVector<Vec3> hdLine;
                if (numPoints > 0)
                {
                    hdLine.reserve(numPoints);  // Phase 1.1: Pre-allocate memory

                    //load the line
                    for (int i = 0; i < numPoints; i++)
                    {
                        line = reader.readLine();
                        // Phase 1.2: Parse without QStringList allocation
                        int comma1 = line.indexOf(',');
                        int comma2 = line.indexOf(',', comma1 + 1);
                        Vec3 vecPt(QStringView(line).left(comma1).toDouble(),
                                   QStringView(line).mid(comma1 + 1, comma2 - comma1 - 1).toDouble(),
                                   QStringView(line).mid(comma2 + 1).toDouble());
                        hdLine.append(vecPt);
                    }
                }
                localHdLines.append(hdLine);
            }

            headlandFile.close();

            // Phase 1.3: Lock only for final assignment (< 50ms)
            lock.lockForWrite();
            for (int k = 0; k < localHdLines.count(); k++)
            {
                if (bnd.bndList.count() > k)
                {
                    bnd.bndList[k].hdLine = localHdLines[k];
                } else {
                    TimedMessageBox(4000, tr("Corrupt Headland File"), tr("Headland file is corrupt. Field still loaded."));
                    break;
                }
            }
            lock.unlock();
        }

        if (bnd.bndList.count() > 0 && bnd.bndList[0].hdLine.count() > 0)
        {
            MainWindowState::instance()->set_isHeadlandOn(true);
        }
        else
        {
            MainWindowState::instance()->set_isHeadlandOn(false);
        }
    }

    //trams ---------------------------------------------------------------------------------
    filename = directoryName + "/" + caseInsensitiveFilename(directoryName, "Tram.txt");

    lock.lockForWrite();
    tram.tramBndOuterArr.clear();
    tram.tramBndInnerArr.clear();
    tram.tramList.clear();
    tram.displayMode = 0;
    //btnTramDisplayMode.Visible = false;

    QFile tramFile(filename);
    if (!tramFile.open(QIODevice::ReadOnly))
    {
        TimedMessageBox(1500, tr("Field Error"), (tr("Couldn't open tram file ") + filename + tr(" for reading!")));
    } else {
        reader.setDevice(&tramFile);
            //read header
        line = reader.readLine();//$Tram

        //outer track of boundary tram
        line = reader.readLine();
        if (!line.isNull())
        {
            int numPoints = line.toInt();

            if (numPoints > 0)
            {
                tram.tramBndOuterArr.reserve(numPoints);  // Phase 1.1: Pre-allocate memory
                //load the line
                for (int i = 0; i < numPoints; i++)
                {
                    line = reader.readLine();
                    // Phase 1.2: Parse without QStringList allocation
                    int comma = line.indexOf(',');
                    Vec2 vecPt(
                        QStringView(line).left(comma).toDouble(),
                        QStringView(line).mid(comma + 1).toDouble());

                    tram.tramBndOuterArr.append(vecPt);
                }
                tram.displayMode = 1;
            }

            //inner track of boundary tram
            line = reader.readLine();
            numPoints = line.toInt();

            if (numPoints > 0)
            {
                tram.tramBndInnerArr.reserve(numPoints);  // Phase 1.1: Pre-allocate memory
                //load the line
                for (int i = 0; i < numPoints; i++)
                {
                    line = reader.readLine();
                    // Phase 1.2: Parse without QStringList allocation
                    int comma = line.indexOf(',');
                    Vec2 vecPt(
                        QStringView(line).left(comma).toDouble(),
                        QStringView(line).mid(comma + 1).toDouble());

                    tram.tramBndInnerArr.append(vecPt);
                }
            }

            if (!reader.atEnd())
            {
                line = reader.readLine();
                int numLines = line.toInt();

                for (int k = 0; k < numLines; k++)
                {
                    line = reader.readLine();
                    numPoints = line.toInt();

                    tram.tramArr = QSharedPointer<QVector<Vec2>>(new QVector<Vec2>);
                    tram.tramArr->reserve(numPoints);  // Phase 1.1: Pre-allocate memory
                    tram.tramList.append(tram.tramArr);

                    for (int i = 0; i < numPoints; i++)
                    {
                        line = reader.readLine();
                        // Phase 1.2: Parse without QStringList allocation
                        int comma = line.indexOf(',');
                        Vec2 vecPt(
                            QStringView(line).left(comma).toDouble(),
                            QStringView(line).mid(comma + 1).toDouble());

                        tram.tramArr->append(vecPt);
                    }
                }
            }
        }

        FixTramModeButton();
    }

    camera.SetZoom();
    lock.unlock();

    //Recorded Path
    filename = directoryName + "/" + caseInsensitiveFilename(directoryName, "RecPath.txt");

    QFile recpathFile(filename);
    if (!recpathFile.open(QIODevice::ReadOnly))
    {
        TimedMessageBox(1500, tr("Field Error"), (tr("Couldn't open Recorded Path ") + filename + tr(" for reading!")));
    } else
    {

        reader.setDevice(&recpathFile);
        //read header
        line = reader.readLine();
        line = reader.readLine();
        int numPoints = line.toInt();
        recPath.recList.clear();
        recPath.recList.reserve(numPoints);  // Phase 1.1: Pre-allocate memory

        lock.lockForWrite();

        while (!reader.atEnd())
        {
            for (int v = 0; v < numPoints; v++)
            {
                line = reader.readLine();
                // Phase 1.2: Parse without QStringList allocation
                int comma1 = line.indexOf(',');
                int comma2 = line.indexOf(',', comma1 + 1);
                int comma3 = line.indexOf(',', comma2 + 1);
                int comma4 = line.indexOf(',', comma3 + 1);
                CRecPathPt point(
                    QStringView(line).left(comma1).toDouble(),
                    QStringView(line).mid(comma1 + 1, comma2 - comma1 - 1).toDouble(),
                    QStringView(line).mid(comma2 + 1, comma3 - comma2 - 1).toDouble(),
                    QStringView(line).mid(comma3 + 1, comma4 - comma3 - 1).toDouble(),
                    (QStringView(line).mid(comma4 + 1) == u"True") );

                //add the point
                recPath.recList.append(point);
            }
        }

        if (recPath.recList.count() > 0)
        {
            //TODO: panelDrag.Visible = true;
        } else {
            //TODO: panelDrag.Visible = false;
        }
        lock.unlock();
    }

    filename = directoryName + "/" + caseInsensitiveFilename(directoryName, "BackPic.txt");

    QFile backPic(filename);
    if (backPic.open(QIODevice::ReadOnly))
    {
        lock.lockForWrite();

        reader.setDevice(&backPic);

        //read header
        line = reader.readLine();

        line = reader.readLine();
        worldGrid.set_isGeoMap ( (line == "True" ? true : false));

        line = reader.readLine();
        worldGrid.set_eastingMaxGeo(line.toDouble());
        line = reader.readLine();
        worldGrid.set_eastingMinGeo ( line.toDouble());
        line = reader.readLine();
        worldGrid.set_northingMaxGeo ( line.toDouble());
        line = reader.readLine();
        worldGrid.set_northingMinGeo ( line.toDouble());

        lock.unlock();
    }

    //update boundary list count in qml
    BoundaryInterface::instance()->set_count(bnd.bndList.count());
    return true;
}

void FormGPS::FileCreateField()
{
    //Saturday, February 11, 2017  -->  7:26:52 AM
    //$FieldDir
    //Bob_Feb11
    //$Offsets
    //533172,5927719,12 - offset easting, northing, zone

    CNMEA &pn = *Backend::instance()->pn();
    BACKEND_TRACK(track);


    if( ! Backend::instance()->isJobStarted())
    {
        qDebug() << "field not open";
        TimedMessageBox(3000, tr("Field Not Open"), tr("Create a new field."));
        return;
    }

    QString myFilename;

    //get the directory and make sure it exists, create if not

#ifdef __ANDROID__
    QString directoryName = androidDirectory + QCoreApplication::applicationName() + "/Fields/" + currentFieldDirectory;
#else
    QString directoryName = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
                            + "/" + QCoreApplication::applicationName() + "/Fields/" + currentFieldDirectory;
#endif

    QDir saveDir(directoryName);
    if (!saveDir.exists()) {
        bool ok = saveDir.mkpath(directoryName);
        if (!ok) {
            qWarning() << "Couldn't create path " << directoryName;
            return;
        }
    }

    myFilename = directoryName + "/" + caseInsensitiveFilename(directoryName, "Field.txt");
    QFile fieldFile(myFilename);
    if (!fieldFile.open(QIODevice::WriteOnly))
    {
        qWarning() << "Couldn't open " << myFilename << "for writing!";
        return;
    }

    QTextStream writer(&fieldFile);
    writer.setLocale(QLocale::C);
    writer.setRealNumberNotation(QTextStream::FixedNotation);

    QDateTime now = QDateTime::currentDateTime();

    //Write out the date
    writer << now.toString("yyyy-MMMM-dd hh:mm:ss tt") << Qt::endl;

    writer << "$FieldDir" << Qt::endl;
    writer << currentFieldDirectory << Qt::endl;

    //write out the easting and northing Offsets
    writer << "$Offsets" << Qt::endl;
    writer << "0,0" << Qt::endl;

    writer << "Convergence" << Qt::endl;
    writer << "0" << Qt::endl;

    writer << "StartFix" << Qt::endl;
    writer << pn.latitude << "," << pn.longitude << Qt::endl;
    // Phase 6.3.1: Use PropertyWrapper for safe QObject access
    pn.SetLocalMetersPerDegree();

    // Qt 6.8 TRACK RESTORATION: Save active track index for restoration when reopening field
    writer << "$ActiveTrackIndex" << Qt::endl;
    writer << track.idx() << Qt::endl;

    fieldFile.close();
}

void FormGPS::FileCreateElevation()
{
    //Saturday, February 11, 2017  -->  7:26:52 AM
    //$FieldDir
    //Bob_Feb11
    //$Offsets
    //533172,5927719,12 - offset easting, northing, zone

    CNMEA &pn = *Backend::instance()->pn();

    QString myFilename;

    //get the directory and make sure it exists, create if not

#ifdef __ANDROID__
    QString directoryName = androidDirectory + QCoreApplication::applicationName() + "/Fields/" + currentFieldDirectory;
#else
    QString directoryName = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
                            + "/" + QCoreApplication::applicationName() + "/Fields/" + currentFieldDirectory;
#endif

    QDir saveDir(directoryName);
    if (!saveDir.exists()) {
        bool ok = saveDir.mkpath(directoryName);
        if (!ok) {
            qWarning() << "Couldn't create path " << directoryName;
            return;
        }
    }

    myFilename = directoryName + "/" + caseInsensitiveFilename(directoryName, "Elevation.txt");
    QFile fieldFile(myFilename);
    if (!fieldFile.open(QIODevice::WriteOnly))
    {
        qWarning() << "Couldn't open " << myFilename << "for writing!";
        return;
    }

    QTextStream writer(&fieldFile);
    writer.setLocale(QLocale::C);
    writer.setRealNumberNotation(QTextStream::FixedNotation);

    QDateTime now = QDateTime::currentDateTime();

    //Write out the date
    writer << now.toString("yyyy-MMMM-dd hh:mm:ss tt") << Qt::endl;

    writer << "$FieldDir" << Qt::endl;
    writer << currentFieldDirectory << Qt::endl;

    //write out the easting and northing Offsets
    writer << "$Offsets" << Qt::endl;
    writer << "0,0" << Qt::endl;

    writer << "Convergence" << Qt::endl;
    writer << "0" << Qt::endl;

    writer << "StartFix" << Qt::endl;
    writer << pn.latitude << "," << pn.longitude << Qt::endl;
    writer << "Latitude,Longitude,Elevation,Quality,Easting,Northing,Heading,Roll";

    fieldFile.close();
}

//save field Patches
void FormGPS::FileSaveSections()
{
    if (tool.patchSaveList.count() == 0) return;

    QString myFilename;

    //get the directory and make sure it exists, create if not

#ifdef __ANDROID__
    QString directoryName = androidDirectory + QCoreApplication::applicationName() + "/Fields/" + currentFieldDirectory;
#else
    QString directoryName = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
                            + "/" + QCoreApplication::applicationName() + "/Fields/" + currentFieldDirectory;
#endif

    myFilename = directoryName + "/" + caseInsensitiveFilename(directoryName, "Sections.txt");

    // Phase 2.1: Build buffer in RAM, single disk write
    QString buffer;
    buffer.reserve(tool.patchSaveList.count() * 500);  // Pre-allocate ~500 bytes per patch

    //for each patch, write out the list of triangles to the buffer
    for(const QSharedPointer<QVector<QVector3D>> &triList: std::as_const(tool.patchSaveList))
    {
        int count2 = triList->count();
        buffer += QString::number(count2) + '\n';

        for (int i=0; i < count2; i++)
        {
            // Format: x,y,z with 3 decimal places
            buffer += QString::number((*triList)[i].x(), 'f', 3) + ','
                   + QString::number((*triList)[i].y(), 'f', 3) + ','
                   + QString::number((*triList)[i].z(), 'f', 3) + '\n';
        }
    }

    // Single disk write (no intermediate flushes)
    QFile sectionFile(myFilename);
    if (!sectionFile.open(QIODevice::Append))
    {
        qWarning() << "Couldn't open " << myFilename << "for appending!";
        return;
    }

    sectionFile.write(buffer.toUtf8());
    sectionFile.close();

    //clear out that patchList and begin adding new ones for next save
    tool.patchSaveList.clear();
}

void FormGPS::FileCreateSections()
{
    //FileSaveSections appends; we must create the file, overwriting any existing vesion
    QString myFilename;

    //get the directory and make sure it exists, create if not

#ifdef __ANDROID__
    QString directoryName = androidDirectory + QCoreApplication::applicationName() + "/Fields/" + currentFieldDirectory;
#else
    QString directoryName = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
                            + "/" + QCoreApplication::applicationName() + "/Fields/" + currentFieldDirectory;
#endif

    myFilename = directoryName + "/" + caseInsensitiveFilename(directoryName, "Sections.txt");
    QFile sectionFile(myFilename);
    if (!sectionFile.open(QIODevice::WriteOnly))
    {
        qWarning() << "Couldn't open " << myFilename << "for appending!";
        return;
    }
    //file should now exist; we can close it.
    sectionFile.close();

}

void FormGPS::FileCreateBoundary()
{
    //Create Boundary.txt, overwriting it if it exists.
#ifdef __ANDROID__
    QString directoryName = androidDirectory + QCoreApplication::applicationName() + "/Fields/" + currentFieldDirectory;
#else
    QString directoryName = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
                            + "/" + QCoreApplication::applicationName() + "/Fields/" + currentFieldDirectory;
#endif

    QDir saveDir(directoryName);
    if (!saveDir.exists()) {
        bool ok = saveDir.mkpath(directoryName);
        if (!ok) {
            qWarning() << "Couldn't create path " << directoryName;
            return;
        }
    }

    QString filename = directoryName + "/" + caseInsensitiveFilename(directoryName, "Boundary.txt");

    QFile boundfile(filename);
    if (!boundfile.open(QIODevice::WriteOnly))
    {
        qWarning() << "Couldn't open " << filename << "for writing!";
        return;
    }

    QTextStream writer(&boundfile);
    writer.setLocale(QLocale::C);
    writer.setRealNumberNotation(QTextStream::FixedNotation);
    writer << "$Boundary" << Qt::endl;
}

void FormGPS::FileCreateFlags()
{
    //create a new flags file, overwriting if it alraedy existis.
#ifdef __ANDROID__
    QString directoryName = androidDirectory + QCoreApplication::applicationName() + "/Fields/" + currentFieldDirectory;
#else
    QString directoryName = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
                            + "/" + QCoreApplication::applicationName() + "/Fields/" + currentFieldDirectory;
#endif

    QDir saveDir(directoryName);
    if (!saveDir.exists()) {
        bool ok = saveDir.mkpath(directoryName);
        if (!ok) {
            qWarning() << "Couldn't create path " << directoryName;
            return;
        }
    }

    QString filename = directoryName + "/" + caseInsensitiveFilename(directoryName, "Flags.txt");

    QFile flagsFile(filename);
    if (!flagsFile.open(QIODevice::WriteOnly))
    {
        qWarning() << "Couldn't open " << filename << "for writing!";
        return;
    }
}

void FormGPS::FileCreateContour()
{
    QString myFilename;

    //get the directory and make sure it exists, create if not

#ifdef __ANDROID__
    QString directoryName = androidDirectory + QCoreApplication::applicationName() + "/Fields/" + currentFieldDirectory;
#else
    QString directoryName = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
                            + "/" + QCoreApplication::applicationName() + "/Fields/" + currentFieldDirectory;
#endif

    myFilename = directoryName + "/" + caseInsensitiveFilename(directoryName, "Contour.txt");
    QFile contourFile(myFilename);
    if (!contourFile.open(QIODevice::WriteOnly))
    {
        qWarning() << "Couldn't open " << myFilename << "for appending!";
        return;
    }

    QTextStream writer(&contourFile);
    writer.setLocale(QLocale::C);
    writer.setRealNumberNotation(QTextStream::FixedNotation);

    writer << "$Contour" << Qt::endl;
}

void FormGPS::FileSaveContour()
{
    if (contourSaveList.count() == 0) return;

    QString myFilename;

    //get the directory and make sure it exists, create if not

#ifdef __ANDROID__
    QString directoryName = androidDirectory + QCoreApplication::applicationName() + "/Fields/" + currentFieldDirectory;
#else
    QString directoryName = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
                            + "/" + QCoreApplication::applicationName() + "/Fields/" + currentFieldDirectory;
#endif

    myFilename = directoryName + "/" + caseInsensitiveFilename(directoryName, "Contour.txt");

    // Phase 2.3: Build buffer in RAM, single disk write
    QString buffer;
    buffer.reserve(contourSaveList.count() * 300);  // Pre-allocate ~300 bytes per contour strip

    for (QSharedPointer<QVector<Vec3>> &triList: contourSaveList)
    {
        int count2 = triList->count();
        buffer += QString::number(count2) + '\n';

        for (int i = 0; i < count2; i++)
        {
            // Format: easting,northing,heading with 3 decimal places
            buffer += QString::number((*triList)[i].easting, 'f', 3) + ','
                   + QString::number((*triList)[i].northing, 'f', 3) + ','
                   + QString::number((*triList)[i].heading, 'f', 3) + '\n';
        }
    }

    // Single disk write (no intermediate flushes)
    QFile contourFile(myFilename);
    if (!contourFile.open(QIODevice::Append))
    {
        qWarning() << "Couldn't open " << myFilename << "for appending!";
        return;
    }

    contourFile.write(buffer.toUtf8());
    contourFile.close();

    contourSaveList.clear();
}

void FormGPS::FileSaveBoundary()
{
#ifdef __ANDROID__
    QString directoryName = androidDirectory + QCoreApplication::applicationName() + "/Fields/" + currentFieldDirectory;
#else
    QString directoryName = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
                            + "/" + QCoreApplication::applicationName() + "/Fields/" + currentFieldDirectory;
#endif

    QDir saveDir(directoryName);
    if (!saveDir.exists()) {
        bool ok = saveDir.mkpath(directoryName);
        if (!ok) {
            qWarning() << "Couldn't create path " << directoryName;
            return;
        }
    }

    QString filename = directoryName + "/" + caseInsensitiveFilename(directoryName, "Boundary.txt");

    // Phase 2.2: Build buffer in RAM, single disk write
    QString buffer;
    // Estimate buffer size: each boundary ~20KB for 1000 points
    int totalPoints = 0;
    for(int i = 0; i < bnd.bndList.count(); i++)
        totalPoints += bnd.bndList[i].fenceLine.count();
    buffer.reserve(totalPoints * 50 + 100);  // ~50 bytes per point + header

    buffer += "$Boundary\n";
    for(int i = 0; i < bnd.bndList.count(); i++)
    {
        buffer += (bnd.bndList[i].isDriveThru ? "True\n" : "False\n");
        buffer += QString::number(bnd.bndList[i].fenceLine.count()) + '\n';

        if (bnd.bndList[i].fenceLine.count() > 0)
        {
            for (int j = 0; j < bnd.bndList[i].fenceLine.count(); j++)
            {
                // Format: easting,northing,heading with 3 and 5 decimal places
                buffer += QString::number(bnd.bndList[i].fenceLine[j].easting, 'f', 3) + ','
                       + QString::number(bnd.bndList[i].fenceLine[j].northing, 'f', 3) + ','
                       + QString::number(bnd.bndList[i].fenceLine[j].heading, 'f', 5) + '\n';
            }
        }
    }

    // Single disk write (no intermediate flushes)
    QFile boundfile(filename);
    if (!boundfile.open(QIODevice::WriteOnly))
    {
        qWarning() << "Couldn't open " << filename << "for writing!";
        return;
    }

    boundfile.write(buffer.toUtf8());
    boundfile.close();

}

void FormGPS::FileSaveTram()
{
#ifdef __ANDROID__
    QString directoryName = androidDirectory + QCoreApplication::applicationName() + "/Fields/" + currentFieldDirectory;
#else
    QString directoryName = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
                            + "/" + QCoreApplication::applicationName() + "/Fields/" + currentFieldDirectory;
#endif

    QDir saveDir(directoryName);
    if (!saveDir.exists()) {
        bool ok = saveDir.mkpath(directoryName);
        if (!ok) {
            qWarning() << "Couldn't create path " << directoryName;
            return;
        }
    }

    QString filename = directoryName + "/" + caseInsensitiveFilename(directoryName, "Tram.txt");

    QFile tramFile(filename);
    if (!tramFile.open(QIODevice::WriteOnly))
    {
        qWarning() << "Couldn't open " << filename << "for writing!";
        return;
    }

    QTextStream writer(&tramFile);
    writer.setLocale(QLocale::C);
    writer.setRealNumberNotation(QTextStream::FixedNotation);

    writer << "$Tram" << Qt::endl;

    if (tram.tramBndOuterArr.count() > 0)
    {
        //outer track of outer boundary tram
        writer << tram.tramBndOuterArr.count() << Qt::endl;

        for (int i = 0; i < tram.tramBndOuterArr.count(); i++)
        {
            writer << qSetRealNumberPrecision(3)
            << tram.tramBndOuterArr[i].easting << ","
            << tram.tramBndOuterArr[i].northing << Qt::endl;
        }

        //inner track of outer boundary tram
        writer << tram.tramBndInnerArr.count();

        for (int i = 0; i < tram.tramBndInnerArr.count(); i++)
        {
            writer << qSetRealNumberPrecision(3)
            << tram.tramBndInnerArr[i].easting << ","
            << tram.tramBndInnerArr[i].northing << Qt::endl;
        }
    }

    //no outer bnd
    else
    {
        writer << "0" << Qt::endl;
        writer << "0" << Qt::endl;
    }

    if (tram.tramList.count() > 0)
    {
        writer << tram.tramList.count() << Qt::endl;
        for (int i = 0; i < tram.tramList.count(); i++)
        {
            writer << tram.tramList[i]->count() << Qt::endl;

            for (int h = 0; h < tram.tramList[i]->count(); h++)
            {
                writer << qSetRealNumberPrecision(3)
                << (*tram.tramList[i])[h].easting << ","
                << (*tram.tramList[i])[h].northing << Qt::endl;
            }
        }
    }
}

void FormGPS::FileSaveBackPic()
{
    WorldGrid &worldGrid = *WorldGrid::instance();

#ifdef __ANDROID__
    QString directoryName = androidDirectory + QCoreApplication::applicationName() + "/Fields/" + currentFieldDirectory;
#else
    QString directoryName = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
                            + "/" + QCoreApplication::applicationName() + "/Fields/" + currentFieldDirectory;
#endif

    QDir saveDir(directoryName);
    if (!saveDir.exists()) {
        bool ok = saveDir.mkpath(directoryName);
        if (!ok) {
            qWarning() << "Couldn't create path " << directoryName;
            return;
        }
    }

    QString filename = directoryName + "/" + caseInsensitiveFilename(directoryName, "BackPic.txt");

    QFile backFile(filename);
    if (!backFile.open(QIODevice::WriteOnly))
    {
        qWarning() << "Couldn't open " << filename << "for writing!";
        return;
    }

    QTextStream writer(&backFile);
    writer.setLocale(QLocale::C);
    writer.setRealNumberNotation(QTextStream::FixedNotation);

    writer << "$BackPic" << Qt::endl;

    if (worldGrid.isGeoMap())
    {
        writer << "True" << Qt::endl;
        writer << worldGrid.eastingMaxGeo() << Qt::endl;
        writer << worldGrid.eastingMinGeo() << Qt::endl;
        writer << worldGrid.northingMaxGeo() << Qt::endl;
        writer << worldGrid.northingMinGeo() << Qt::endl;
    }
    else
    {
        writer << "False" << Qt::endl;
        writer << 300 << Qt::endl;
        writer << -300 << Qt::endl;
        writer << 300 << Qt::endl;
        writer << -300 << Qt::endl;
    }
}

void FormGPS::FileSaveHeadland()
{
#ifdef __ANDROID__
    QString directoryName = androidDirectory + QCoreApplication::applicationName() + "/Fields/" + currentFieldDirectory;
#else
    QString directoryName = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
                            + "/" + QCoreApplication::applicationName() + "/Fields/" + currentFieldDirectory;
#endif

    QDir saveDir(directoryName);
    if (!saveDir.exists()) {
        bool ok = saveDir.mkpath(directoryName);
        if (!ok) {
            qWarning() << "Couldn't create path " << directoryName;
            return;
        }
    }

    QString filename = directoryName + "/" + caseInsensitiveFilename(directoryName, "Headland.txt");

    QFile headfile(filename);
    if (!headfile.open(QIODevice::WriteOnly))
    {
        qWarning() << "Couldn't open " << filename << "for writing!";
        return;
    }

    QTextStream writer(&headfile);
    writer.setLocale(QLocale::C);
    writer.setRealNumberNotation(QTextStream::FixedNotation);
    writer << "$Headland" << Qt::endl;
    if (bnd.bndList.count() > 0 && bnd.bndList[0].hdLine.count() > 0)
    {
        for(int i = 0; i < bnd.bndList.count(); i++)
        {
            writer << bnd.bndList[i].hdLine.count() << Qt::endl;
            if (bnd.bndList[i].hdLine.count() > 0)
            {
                for (int j = 0; j < bnd.bndList[i].hdLine.count(); j++)
                    writer << qSetRealNumberPrecision(3)
                           << bnd.bndList[i].hdLine[j].easting << ","
                           << bnd.bndList[i].hdLine[j].northing << ","
                           << bnd.bndList[i].hdLine[j].heading << Qt::endl;
            }
        }
    }

    headfile.close();

}

void FormGPS::FileCreateRecPath()
{
#ifdef __ANDROID__
    QString directoryName = androidDirectory + QCoreApplication::applicationName() + "/Fields/" + currentFieldDirectory;
#else
    QString directoryName = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
                            + "/" + QCoreApplication::applicationName() + "/Fields/" + currentFieldDirectory;
#endif

    QDir saveDir(directoryName);
    if (!saveDir.exists()) {
        bool ok = saveDir.mkpath(directoryName);
        if (!ok) {
            qWarning() << "Couldn't create path " << directoryName;
            return;
        }
    }

    QString filename = directoryName + "/" + caseInsensitiveFilename(directoryName, "RecPath.txt");

    QFile recpathfile(filename);
    if (!recpathfile.open(QIODevice::WriteOnly))
    {
        qWarning() << "Couldn't open " << filename << "for writing!";
        return;
    }

    QTextStream writer(&recpathfile);
    writer.setLocale(QLocale::C);
    writer.setRealNumberNotation(QTextStream::FixedNotation);

    writer << "$RecPath" << Qt::endl;
    writer << "0" << Qt::endl;

    recpathfile.close();

}

void FormGPS::FileSaveRecPath()
{
#ifdef __ANDROID__
    QString directoryName = androidDirectory + QCoreApplication::applicationName() + "/Fields/" + currentFieldDirectory;
#else
    QString directoryName = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
                            + "/" + QCoreApplication::applicationName() + "/Fields/" + currentFieldDirectory;
#endif

    QDir saveDir(directoryName);
    if (!saveDir.exists()) {
        bool ok = saveDir.mkpath(directoryName);
        if (!ok) {
            qWarning() << "Couldn't create path " << directoryName;
            return;
        }
    }

    QString filename = directoryName + "/" + caseInsensitiveFilename(directoryName, "RecPath.txt");

    QFile recpathfile(filename);
    if (!recpathfile.open(QIODevice::WriteOnly))
    {
        qWarning() << "Couldn't open " << filename << "for writing!";
        return;
    }

    QTextStream writer(&recpathfile);
    writer.setLocale(QLocale::C);
    writer.setRealNumberNotation(QTextStream::FixedNotation);

    writer << "$RecPath" << Qt::endl;
    writer << recPath.recList.count() << Qt::endl;

    if (recPath.recList.count() > 0)
    {
        for (int j = 0; j < recPath.recList.count(); j++)
            writer << qSetRealNumberPrecision(3)
                   << recPath.recList[j].easting << ","
                   << recPath.recList[j].northing << ","
                   << recPath.recList[j].heading << ","
                   << qSetRealNumberPrecision(1)
                   << recPath.recList[j].speed << ","
                   << recPath.recList[j].autoBtnState << Qt::endl;

    }

    recpathfile.close();

}

void FormGPS::FileLoadRecPath()
{
    //current field directory should already exist
#ifdef __ANDROID__
    QString directoryName = androidDirectory + QCoreApplication::applicationName() + "/Fields/" + currentFieldDirectory;
#else
    QString directoryName = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
                            + "/" + QCoreApplication::applicationName() + "/Fields/" + currentFieldDirectory;
#endif

    QDir loadDir(directoryName);
    if (!loadDir.exists()) {
        bool ok = loadDir.mkpath(directoryName);
        if (!ok) {
            qWarning() << "Couldn't create path " << directoryName;
            return;
        }
    }

    QString filename = directoryName + "/" + caseInsensitiveFilename(directoryName, "RecPath.txt");

    QFile recFile(filename);
    if (!recFile.open(QIODevice::ReadOnly))
    {
        TimedMessageBox(1500, tr("Field Error"), (tr("Couldn't open ") + filename + tr(" for reading!")));
        return;
    }

    QTextStream reader(&recFile);
    reader.setLocale(QLocale::C);

    //read header
    QString line = reader.readLine();
    line = reader.readLine();
    int numPoints = line.toInt();
    recPath.recList.clear();
    recPath.recList.reserve(numPoints);  // Phase 1.1: Pre-allocate

    while (!reader.atEnd())
    {
        for (int v = 0; v < numPoints; v++)
        {
            line = reader.readLine();
            // Phase 1.2: Parse without QStringList allocation
            int comma1 = line.indexOf(',');
            int comma2 = line.indexOf(',', comma1 + 1);
            int comma3 = line.indexOf(',', comma2 + 1);
            int comma4 = line.indexOf(',', comma3 + 1);
            if (comma1 == -1 || comma2 == -1 || comma3 == -1 || comma4 == -1) {
                recPath.recList.clear();
                qWarning() << "Ignoring " << filename << " because it is corrupt and cannot be read.";
                return;
            }

            CRecPathPt point(
                QStringView(line).left(comma1).toDouble(),
                QStringView(line).mid(comma1 + 1, comma2 - comma1 - 1).toDouble(),
                QStringView(line).mid(comma2 + 1, comma3 - comma2 - 1).toDouble(),
                QStringView(line).mid(comma3 + 1, comma4 - comma3 - 1).toDouble(),
                (QStringView(line).mid(comma4 + 1) == u"True"));

            //add the point
            recPath.recList.append(point);
        }
    }
}

void FormGPS::FileSaveFlags()
{
    //Saturday, February 11, 2017  -->  7:26:52 AM
    //$FlagsDir
    //Bob_Feb11
    //$Offsets
    //533172,5927719,12 - offset easting, northing, zone

#ifdef __ANDROID__
    QString directoryName = androidDirectory + QCoreApplication::applicationName() + "/Fields/" + currentFieldDirectory;
#else
    QString directoryName = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
                            + "/" + QCoreApplication::applicationName() + "/Fields/" + currentFieldDirectory;
#endif

    QDir saveDir(directoryName);
    if (!saveDir.exists()) {
        bool ok = saveDir.mkpath(directoryName);
        if (!ok) {
            qWarning() << "Couldn't create path " << directoryName;
            return;
        }
    }

    QString filename = directoryName + "/" + caseInsensitiveFilename(directoryName, "Flags.txt");

    QFile flagsfile(filename);
    if (!flagsfile.open(QIODevice::WriteOnly))
    {
        qWarning() << "Couldn't open " << filename << "for writing!";
        return;
    }

    QTextStream writer(&flagsfile);
    writer.setLocale(QLocale::C);
    writer.setRealNumberNotation(QTextStream::FixedNotation);

    writer << "$Flags" << Qt::endl;

    int count2 = FlagsInterface::instance()->flagModel()->count();
    writer << count2 << Qt::endl;

    FlagModel::Flag flag;
    for (int i = 0; i < count2; i++)
    {
        flag = FlagsInterface::instance()->flagModel()->flagAt(i+1);
        writer << flag.latitude << ","
               << flag.longitude << ","
               << flag.easting << ","
               << flag.northing << ","
               << flag.heading << ","
               << flag.color << ","
               << flag.id << ","
               << flag.notes << Qt::endl;
    }

    flagsfile.close();

}

void FormGPS::FileSaveNMEA()
{
    CNMEA &pn = *Backend::instance()->pn();

#ifdef __ANDROID__
    QString directoryName = androidDirectory + QCoreApplication::applicationName() + "/Fields/" + currentFieldDirectory;
#else
    QString directoryName = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
                            + "/" + QCoreApplication::applicationName() + "/Fields/" + currentFieldDirectory;
#endif

    QDir saveDir(directoryName);
    if (!saveDir.exists()) {
        bool ok = saveDir.mkpath(directoryName);
        if (!ok) {
            qWarning() << "Couldn't create path " << directoryName;
            return;
        }
    }

    QString filename = directoryName + "/" + caseInsensitiveFilename(directoryName, "NMEA_log.txt");

    QFile nmeafile(filename);
    if (!nmeafile.open(QIODevice::Append))
    {
        qWarning() << "Couldn't open " << filename << "for writing!";
        return;
    }

    QTextStream writer(&nmeafile);
    writer.setLocale(QLocale::C);

    writer << pn.logNMEASentence;

    pn.logNMEASentence.clear();

    nmeafile.close();
}

void FormGPS::FileSaveElevation()
{
    CNMEA &pn = *Backend::instance()->pn();

#ifdef __ANDROID__
    QString directoryName = androidDirectory + QCoreApplication::applicationName() + "/Fields/" + currentFieldDirectory;
#else
    QString directoryName = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
                            + "/" + QCoreApplication::applicationName() + "/Fields/" + currentFieldDirectory;
#endif

    QDir saveDir(directoryName);
    if (!saveDir.exists()) {
        bool ok = saveDir.mkpath(directoryName);
        if (!ok) {
            qWarning() << "Couldn't create path " << directoryName;
            return;
        }
    }

    QString filename = directoryName + "/" + caseInsensitiveFilename(directoryName, "Elevation.txt");

    QFile elevfile(filename);
    if (!elevfile.open(QIODevice::Append))
    {
        qWarning() << "Couldn't open " << filename << "for writing!";
        return;
    }

    QTextStream writer(&elevfile);
    writer.setLocale(QLocale::C);

    writer << sbGrid;

    sbGrid.clear();

    elevfile.close();
}

void FormGPS::FileSaveSingleFlagKML2(int flagNumber)
{
    CNMEA &pn = *Backend::instance()->pn();

#ifdef __ANDROID__
    QString directoryName = androidDirectory + QCoreApplication::applicationName() + "/Fields/" + currentFieldDirectory;
#else
    QString directoryName = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
                            + "/" + QCoreApplication::applicationName() + "/Fields/" + currentFieldDirectory;
#endif

    QDir saveDir(directoryName);
    if (!saveDir.exists()) {
        bool ok = saveDir.mkpath(directoryName);
        if (!ok) {
            qWarning() << "Couldn't create path " << directoryName;
            return;
        }
    }

    QString filename = directoryName + "/" + caseInsensitiveFilename(directoryName, QString("Flag%1.kml").arg(flagNumber));


    QFile kmlFile(filename);
    if (!kmlFile.open(QIODevice::WriteOnly))
    {
        qWarning() << "Couldn't open " << filename << "for writing!";
        return;
    }

    QTextStream writer(&kmlFile);
    writer.setLocale(QLocale::C);
    writer.setRealNumberNotation(QTextStream::FixedNotation);

    writer << "<?xml version=""1.0"" encoding=""UTF-8""?" << Qt::endl;
    writer << "<kml xmlns=""http://www.opengis.net/kml/2.2""> " << Qt::endl;

    double lat, lon;

    FlagModel::Flag flag;
    flag = FlagsInterface::instance()->flagModel()->flagAt(flagNumber);

    pn.ConvertLocalToWGS84(flag.northing, flag.easting, lat, lon);

    writer << "<Document>" << Qt::endl;

    writer << "<Placemark>"  << Qt::endl;;
    writer << "<Style><IconStyle>" << Qt::endl;
    if (flag.color == 0)  //red - xbgr
        writer << "<color>ff4400ff</color>" << Qt::endl;
    if (flag.color == 1)  //grn - xbgr
        writer << "<color>ff44ff00</color>" << Qt::endl;
    if (flag.color == 2)  //yel - xbgr
        writer << "<color>ff44ffff</color>" << Qt::endl;
    writer << "</IconStyle></Style>" << Qt::endl;
    writer << "<name>" << flagNumber << "</name>" << Qt::endl;
    writer << "<Point><coordinates>" << lon << "," << lat << ",0"
           << "</coordinates></Point>" << Qt::endl;
    writer << "</Placemark>" << Qt::endl;
    writer << "</Document>" << Qt::endl;
    writer << "</kml>" << Qt::endl;
}

void FormGPS::FileSaveSingleFlagKML(int flagNumber)
{
#ifdef __ANDROID__
    QString directoryName = androidDirectory + QCoreApplication::applicationName() + "/Fields/" + currentFieldDirectory;
#else
    QString directoryName = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
                            + "/" + QCoreApplication::applicationName() + "/Fields/" + currentFieldDirectory;
#endif

    QDir saveDir(directoryName);
    if (!saveDir.exists()) {
        bool ok = saveDir.mkpath(directoryName);
        if (!ok) {
            qWarning() << "Couldn't create path " << directoryName;
            return;
        }
    }

    QString filename = directoryName + "/" + caseInsensitiveFilename(directoryName, QString("Flag%1.kml").arg(flagNumber));


    QFile kmlFile(filename);
    if (!kmlFile.open(QIODevice::WriteOnly))
    {
        qWarning() << "Couldn't open " << filename << "for writing!";
        return;
    }

    QTextStream writer(&kmlFile);
    writer.setLocale(QLocale::C);
    writer.setRealNumberNotation(QTextStream::FixedNotation);

    writer << "<?xml version=""1.0"" encoding=""UTF-8""?" << Qt::endl;
    writer << "<kml xmlns=""http://www.opengis.net/kml/2.2""> " << Qt::endl;

    FlagModel::Flag flag;
    flag = FlagsInterface::instance()->flagModel()->flagAt(flagNumber);

    writer << "<Document>" << Qt::endl;

    writer << "<Placemark>"  << Qt::endl;;
    writer << "<Style><IconStyle>" << Qt::endl;
    if (flag.color == 0)  //red - xbgr
        writer << "<color>ff4400ff</color>" << Qt::endl;
    if (flag.color == 1)  //grn - xbgr
        writer << "<color>ff44ff00</color>" << Qt::endl;
    if (flag.color == 2)  //yel - xbgr
        writer << "<color>ff44ffff</color>" << Qt::endl;
    writer << "</IconStyle></Style>" << Qt::endl;
    writer << "<name>" << flagNumber << "</name>" << Qt::endl;
    writer << "<Point><coordinates>"
           << flag.longitude << ","
           << flag.latitude << ",0"
           << "</coordinates></Point>" << Qt::endl;
    writer << "</Placemark>" << Qt::endl;
    writer << "</Document>" << Qt::endl;
    writer << "</kml>" << Qt::endl;
}

void FormGPS::FileMakeKMLFromCurrentPosition(double lat, double lon)
{
#ifdef __ANDROID__
    QString directoryName = androidDirectory + QCoreApplication::applicationName() + "/Fields/" + currentFieldDirectory;
#else
    QString directoryName = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
                            + "/" + QCoreApplication::applicationName() + "/Fields/" + currentFieldDirectory;
#endif

    QDir saveDir(directoryName);
    if (!saveDir.exists()) {
        bool ok = saveDir.mkpath(directoryName);
        if (!ok) {
            qWarning() << "Couldn't create path " << directoryName;
            return;
        }
    }

    QString filename = directoryName + "/" + caseInsensitiveFilename(directoryName, "CurrentPosition.kml");


    QFile kmlFile(filename);
    if (!kmlFile.open(QIODevice::WriteOnly))
    {
        qWarning() << "Couldn't open " << filename << "for writing!";
        return;
    }

    QTextStream writer(&kmlFile);
    writer.setLocale(QLocale::C);
    writer.setRealNumberNotation(QTextStream::FixedNotation);

    writer << "<?xml version=""1.0"" encoding=""UTF-8""?>     " << Qt::endl;
    writer << "<kml xmlns=""http://www.opengis.net/kml/2.2""> " << Qt::endl;

    writer << "<Document>" << Qt::endl;
    writer << "<Placemark>" << Qt::endl;
    writer << "<Style> <IconStyle>" << Qt::endl;
    writer << "<color>ff4400ff</color>" << Qt::endl;
    writer << "</IconStyle></Style>" << Qt::endl;
    writer << "<name>Your Current Position</name>" << Qt::endl;
    writer << "<Point><coordinates> "
           << lon << "," << lat << ",0"
           << "</coordinates></Point>" << Qt::endl;
    writer << "</Placemark>" << Qt::endl;
    writer << "</Document>" << Qt::endl;
    writer << "</kml>" << Qt::endl;


}

void FormGPS::ExportFieldAs_KML()
{
    //TODO:  use XML library
}

QString FormGPS::GetBoundaryPointsLatLon(int bndNum)
{
    QString sb;
    QTextStream sb_writer(&sb);
    double lat = 0;
    double lon = 0;

    for (int i = 0; i < bnd.bndList[bndNum].fenceLine.count(); i++)
    {
        // Phase 6.3.1: Use PropertyWrapper for safe QObject access
    Backend::instance()->pn()->ConvertLocalToWGS84(bnd.bndList[bndNum].fenceLine[i].northing, bnd.bndList[bndNum].fenceLine[i].easting, lat, lon);
        sb_writer << qSetRealNumberPrecision(7)
                  << lon << ','
                  << lat << ",0 "
                  << Qt::endl; // TODO: should this be here?
    }

    return sb;
}

void FormGPS::FileUpdateAllFieldsKML()
{
    //Update or add the current field to the Field.kml file
    //TODO: use XML library
}

