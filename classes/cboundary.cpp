#include "cboundary.h"
#include <QCoreApplication>
#include <QDir>
#include <QString>
#include "classes/settingsmanager.h"
#include "boundaryinterface.h"
#include "backend.h"
#include "glm.h"
#include "cvehicle.h"
#include "siminterface.h"
#include "boundaryproperties.h"
#include "boundariesproperties.h"
#include "QLoggingCategory"

Q_LOGGING_CATEGORY (cboundary_log, "cboundary.qtagopengps")
#define QDEBUG qDebug(cboundary_log)


//this is defined in formgps_saveopen.cpp currently.
QString caseInsensitiveFilename(const QString &directory, const QString &filename);

CBoundary *CBoundary::s_instance = nullptr;

CBoundary *CBoundary::instance()
{
    if (!s_instance) {
        s_instance = new CBoundary();

        // Ensure cleanup on app exit
        QObject::connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit,
                         s_instance, []() {
                             delete s_instance;
                             s_instance = nullptr;
                         });
    }
    return s_instance;
}

CBoundary::CBoundary(QObject *parent) : QObject(parent)
{
    turnSelected = 0;

    //automatically connect to BoundaryInterface for QML interaction
    connect(BoundaryInterface::instance(), &BoundaryInterface::calculateArea, this, &CBoundary::calculateArea);
    connect(BoundaryInterface::instance(), &BoundaryInterface::updateList, this, &CBoundary::updateList);
    connect(BoundaryInterface::instance(), &BoundaryInterface::start, this, &CBoundary::start);
    connect(BoundaryInterface::instance(), &BoundaryInterface::stop, this, &CBoundary::stop);
    connect(BoundaryInterface::instance(), &BoundaryInterface::addPoint, this, &CBoundary::addPoint);
    connect(BoundaryInterface::instance(), &BoundaryInterface::deleteLastPoint, this, &CBoundary::deleteLastPoint);
    connect(BoundaryInterface::instance(), &BoundaryInterface::pause, this, &CBoundary::pause);
    connect(BoundaryInterface::instance(), &BoundaryInterface::record, this, &CBoundary::record);
    connect(BoundaryInterface::instance(), &BoundaryInterface::reset, this, &CBoundary::reset);
    connect(BoundaryInterface::instance(), &BoundaryInterface::deleteBoundary, this, &CBoundary::deleteBoundary);
    connect(BoundaryInterface::instance(), &BoundaryInterface::setDriveThrough, this, &CBoundary::setDriveThrough);
    connect(BoundaryInterface::instance(), &BoundaryInterface::deleteAll, this, &CBoundary::deleteAll);
    connect(BoundaryInterface::instance(), &BoundaryInterface::loadBoundaryFromKML, this, &CBoundary::loadBoundaryFromKML);
}

void CBoundary::loadSettings() {
    isSectionControlledByHeadland = SettingsManager::instance()->headland_isSectionControlled();
}


void CBoundary::AddCurrentPoint(double min_dist) {
    if (min_dist > 0 && glm::Distance(CVehicle::instance()->pivotAxlePos, prevBoundaryPos) < min_dist) {
        //we haven't traveled far enough to record another point
        return;
    }

    prevBoundaryPos.easting = CVehicle::instance()->pivotAxlePos.easting;
    prevBoundaryPos.northing = CVehicle::instance()->pivotAxlePos.northing;

    //build the boundary line
    if (isOkToAddPoints)
    {
        if (BoundaryInterface::instance()->isDrawRightSide())
        {
            //Right side
            Vec3 point(CVehicle::instance()->pivotAxlePos.easting + sin(CVehicle::instance()->pivotAxlePos.heading - glm::PIBy2) * -BoundaryInterface::instance()->createBndOffset(),
                       CVehicle::instance()->pivotAxlePos.northing + cos(CVehicle::instance()->pivotAxlePos.heading - glm::PIBy2) * -BoundaryInterface::instance()->createBndOffset(),
                       CVehicle::instance()->pivotAxlePos.heading);
            bndBeingMadePts.append(point);
        }

        //draw on left side
        else
        {
            //Right side
            Vec3 point(CVehicle::instance()->pivotAxlePos.easting + sin(CVehicle::instance()->pivotAxlePos.heading - glm::PIBy2) * BoundaryInterface::instance()->createBndOffset(),
                       CVehicle::instance()->pivotAxlePos.northing + cos(CVehicle::instance()->pivotAxlePos.heading - glm::PIBy2) * BoundaryInterface::instance()->createBndOffset(),
                       CVehicle::instance()->pivotAxlePos.heading);
            bndBeingMadePts.append(point);
        }
        calculateArea(); //update area for GUI
    }
}

void CBoundary::UpdateFieldBoundaryGUIAreas() {
    if (bndList.count() > 0)
    {
        Backend::instance()->m_currentField.areaOuterBoundary = bndList[0].area;
        Backend::instance()->m_currentField.areaBoundaryOuterLessInner = bndList[0].area;

        for (int i = 1; i < bndList.count(); i++)
        {
            Backend::instance()->m_currentField.areaBoundaryOuterLessInner -= bndList[i].area;
        }
    }
    else
    {
        Backend::instance()->m_currentField.areaOuterBoundary = 0;
        Backend::instance()->m_currentField.areaBoundaryOuterLessInner = 0;
    }

    emit Backend::instance()->currentFieldChanged();
}

bool CBoundary::CalculateMinMax() {
    Backend::instance()->m_currentField.minX = 9999999;
    Backend::instance()->m_currentField.minY = 9999999;
    Backend::instance()->m_currentField.maxX = -9999999;
    Backend::instance()->m_currentField.maxY = -9999999;

    if (bndList.count() == 0) return false;

    int bndCnt = bndList[0].fenceLine.count();
    for (int i = 0; i < bndCnt; i++)
    {
        double x = bndList[0].fenceLine[i].easting;
        double y = bndList[0].fenceLine[i].northing;

        //also tally the max/min of field x and z
        if (Backend::instance()->m_currentField.minX > x) Backend::instance()->m_currentField.minX = x;
        if (Backend::instance()->m_currentField.maxX < x) Backend::instance()->m_currentField.maxX = x;
        if (Backend::instance()->m_currentField.minY > y) Backend::instance()->m_currentField.minY = y;
        if (Backend::instance()->m_currentField.maxY < y) Backend::instance()->m_currentField.maxY = y;
    }

    //calculate center of field and max distance
    double maxFieldDistance;
    double fieldCenterX, fieldCenterY;

    //the largest distancew across field
    double dist = fabs(Backend::instance()->m_currentField.minX - Backend::instance()->m_currentField.maxX);
    double dist2 = fabs(Backend::instance()->m_currentField.minY - Backend::instance()->m_currentField.maxY);

    if (dist > dist2) maxFieldDistance = (dist);
    else maxFieldDistance = (dist2);

    if (maxFieldDistance < 100) maxFieldDistance = 100;
    if (maxFieldDistance > 19900) maxFieldDistance = 19900;

    Backend::instance()->m_currentField.calcCenter();

    Backend::instance()->m_currentField.maxDistance = maxFieldDistance;


    emit Backend::instance()->currentFieldChanged();
    return true;
}

//former methods from formgps_ui_boundary
void CBoundary::calculateArea() {
    int ptCount = bndBeingMadePts.count();
    double area = 0;

    if (ptCount > 0)
    {
        int j = ptCount - 1;  // The last vertex is the 'previous' one to the first

        for (int i = 0; i < ptCount; j = i++)
        {
            area += (bndBeingMadePts[j].easting + bndBeingMadePts[i].easting) * (bndBeingMadePts[j].northing - bndBeingMadePts[i].northing);
        }
        area = fabs(area / 2);
    }

    BoundaryInterface::instance()->set_area(area);

    // Update properties - automatic Qt 6.8 notification
    if (ptCount >= 3) {
        BoundaryInterface::instance()->set_pointCount(ptCount);
    }
}

bool CBoundary::loadBoundary(const QString &field_path) {
    QString filename = QDir(field_path).filePath(caseInsensitiveFilename(field_path, "Boundary.txt"));


    QFile boundariesFile(filename);
    QTextStream reader(&boundariesFile);
    QString line;

    if (!boundariesFile.open(QIODevice::ReadOnly))
    {
        Backend::instance()->timedMessage(1500, tr("Field Warning"), (tr("Couldn't open boundaries ") + filename + tr(" for reading!")));
        return false;
    } else
    {
        reader.setDevice(&boundariesFile);
        //read header
        line = reader.readLine();//Boundary

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

        bndList = localBndList;
        BoundaryInterface::instance()->set_count(bndList.count());
        updateList();
        //calculateMinMax();
        BuildTurnLines();
    }
    return true;
}

double CBoundary::getSavedFieldArea(const QString &boundarytxt_path){
    QFile boundariesFile(boundarytxt_path);
    QTextStream reader(&boundariesFile);
    QString line;

    if (boundariesFile.open(QIODevice::ReadOnly))
    {
        reader.setDevice(&boundariesFile);
        //read header
        line = reader.readLine();//Boundary

        //only look at first boundary
        if (!reader.atEnd()) {
            //True or False OR points from older boundary files
            line = reader.readLine();

            //Check for older boundary files, then above line string is num of points
            if (line == "True")
            {
                line = reader.readLine();
            } else if (line == "False")
            {
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
                QVector<Vec3> pointList;
                pointList.reserve(numPoints);  // Phase 1.1: Pre-allocate
                //load the line
                for (int i = 0; i < numPoints; i++)
                {
                    line = reader.readLine();
                    // Phase 1.2: Parse without QStringList allocation
                    int comma1 = line.indexOf(',');
                    int comma2 = line.indexOf(',', comma1 + 1);
                    Vec3 vecPt( QStringView(line).left(comma1).toDouble(),
                               QStringView(line).mid(comma1 + 1, comma2 - comma1 - 1).toDouble(),
                               QStringView(line).mid(comma2 + 1).toDouble() );
                    pointList.append(vecPt);
                }

                if (pointList.count() > 4) {
                    double area = 0;

                    //the last vertex is the 'previous' one to the first
                    int j = pointList.count() - 1;

                    for (int i = 0; i < pointList.count() ; j = i++) {
                        //pretend they are square; we'll divide by 2 later
                        area += (pointList[j].easting + pointList[i].easting) *
                                (pointList[j].northing - pointList[i].northing);
                    }

                    return fabs(area) / 2;
                }
            }
        }

    }

    return -10; //indicate no boundary present
}

void CBoundary::updateList() {
    QList<QVariant> boundaryList;
    QMap<QString, QVariant> bndMap;

    int index = 0;

    for (CBoundaryList &b: bndList) {
        bndMap["index"] = index++;
        bndMap["area"] = b.area;
        bndMap["drive_through"] = b.isDriveThru;

        boundaryList.append(bndMap);
    }

    BoundaryInterface::instance()->set_list(boundaryList);
}

void CBoundary::start() {
    double tool_width = SettingsManager::instance()->vehicle_toolWidth();
    BoundaryInterface::instance()->set_createBndOffset(tool_width * 0.5);
    BoundaryInterface::instance()->set_isBndBeingMade (true);
    bndBeingMadePts.clear();
    calculateArea();

    // Update properties - automatic Qt 6.8 notification
    BoundaryInterface::instance()->set_isRecording(true);
    BoundaryInterface::instance()->set_pointCount(0);
}

void CBoundary::stop() {
    if (bndBeingMadePts.count() > 2)
    {
        CBoundaryList New;

        for (int i = 0; i < bndBeingMadePts.count(); i++)
        {
            New.fenceLine.append(bndBeingMadePts[i]);
        }

        New.CalculateFenceArea(bndList.count());
        New.FixFenceLine(bndList.count());

        bndList.append(New);
        //this is really our business, but the question is where to store it.
        UpdateFieldBoundaryGUIAreas();

        //turn lines made from boundaries
        CalculateMinMax();
        emit saveBoundaryRequested();
        BuildTurnLines();
    }

    //stop it all for adding
    isOkToAddPoints = false;
    BoundaryInterface::instance()->set_isBndBeingMade (false);
    bndBeingMadePts.clear();
    updateList();
    BoundaryInterface::instance()->set_count(bndList.count());
    BoundaryInterface::instance()->set_isRecording(false);
    if (bndList.count() > 0) {
        BoundaryInterface::instance()->set_area(bndList[0].area);
    }
}

void CBoundary::addPoint() {
    //Add a manual point from button click

    isOkToAddPoints = true;
    AddCurrentPoint(0);
    isOkToAddPoints = false; //manual add stops automatic recording.

    // Update properties - automatic Qt 6.8 notification
    BoundaryInterface::instance()->set_pointCount(bndBeingMadePts.count());
}

void CBoundary::deleteLastPoint() {
    int ptCount = bndBeingMadePts.count();
    if (ptCount > 0)
        bndBeingMadePts.pop_back();
    calculateArea();

    // Update properties - automatic Qt 6.8 notification
    BoundaryInterface::instance()->set_pointCount(bndBeingMadePts.count());
}

void CBoundary::pause() {
    isOkToAddPoints = false;
}

void CBoundary::record() {
    isOkToAddPoints = true;
}

void CBoundary::reset() {
    bndBeingMadePts.clear();
    calculateArea();

    // Reset properties - automatic Qt 6.8 notification
    BoundaryInterface::instance()->set_isRecording(false);
    BoundaryInterface::instance()->set_pointCount(0);
    BoundaryInterface::instance()->set_area(0);
}

void CBoundary::deleteBoundary(int which_boundary) {
    //boundary 0 is special.  It's the outer boundary.
    if (which_boundary == 0 && bndList.count() > 1)
        return; //must remove other boundaries first.

    bndList.remove(which_boundary);
    BoundaryInterface::instance()->set_count(bndList.count());
    updateList();
}

void CBoundary::setDriveThrough(int which_boundary, bool drive_thru) {
    bndList[which_boundary].isDriveThru = drive_thru;
    updateList();
}

void CBoundary::deleteAll() {
    bndList.clear();
    emit saveBoundaryRequested();
    BuildTurnLines();
    BoundaryInterface::instance()->set_count(0);
    updateList();

}

void CBoundary::loadBoundaryFromKML(QString filename) {
    CNMEA &pn = *Backend::instance()->pn();

    qDebug(cboundary_log) << "Opening KML file:" << filename;
    QUrl fileUrl(filename);
    QString localPath = fileUrl.toLocalFile();

    double totalLon = 0;
    double totalLat = 0;

    QFile file(localPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Error opening file:" << file.errorString();
        return;
    }

    QTextStream stream(&file);
    QString line;
    QString coordinates;

    while (!stream.atEnd()) {
        line = stream.readLine().trimmed();

        int startIndex = line.indexOf(QLatin1String("<coordinates>"));
        if (startIndex != -1) {
            // Found opening tag
            while (true) {
                int endIndex = line.indexOf(QLatin1String("</coordinates>"));
                if (endIndex == -1) {
                    if (startIndex == -1) {
                        coordinates += line;
                    } else {
                        coordinates += QStringView(line).mid(startIndex + 13); // Skip "<coordinates>"
                    }
                } else {
                    if (startIndex == -1) {
                        coordinates += QStringView(line).left(endIndex);
                    } else {
                        coordinates += QStringView(line).mid(startIndex + 13, endIndex - (startIndex + 13));
                    }
                    break;
                }

                if (stream.atEnd()) break;
                line = stream.readLine().trimmed();
                startIndex = -1; // reset for subsequent lines
            }

            // Split coordinates by whitespace
            QStringList numberSets = coordinates.split(QRegularExpression(QStringLiteral("\\s+")),
                                                       Qt::SkipEmptyParts);

            if (numberSets.size() > 2) {
                double latK = 0.0, lonK = 0.0;

                CBoundaryList New;

                for (const QString& coord : std::as_const(numberSets)) {
                    if (coord.length() < 3) continue;

                    qDebug(cboundary_log) << coord;

                    int comma1 = coord.indexOf(QLatin1Char(','));
                    int comma2 = coord.indexOf(QLatin1Char(','), comma1 + 1);

                    if (comma1 == -1 || comma2 == -1) continue;

                    QString lonStr = coord.left(comma1);
                    QString latStr = coord.mid(comma1 + 1, comma2 - comma1 - 1);

                    bool ok1 = false, ok2 = false;
                    double lonVal = lonStr.toDouble(&ok1);
                    double latVal = latStr.toDouble(&ok2);

                    if (!ok1 || !ok2) continue;

                    latK = latVal;
                    lonK = lonVal;
                    totalLon += lonVal;
                    totalLat += latVal;


                    double easting = 0.0, northing = 0.0;
                    pn.ConvertWGS84ToLocal(latK, lonK, northing, easting);
                    Vec3 temp(easting, northing, 0);
                    New.fenceLine.append(temp);
                }

                totalLon /= numberSets.size();
                totalLat /= numberSets.size();

                // Build the boundary: clockwise for outer, counter-clockwise for inner
                New.CalculateFenceArea(bndList.count());
                New.FixFenceLine(bndList.count());
                bndList.append(New);

            } else {
                qWarning() << "Error reading KML: Too few coordinate points.";
                file.close();
                return;
            }

            break; // Process only the first <coordinates> block
        }
    }

    file.close();

    if (bndList.count() < 2) {
        //If we just added an outer boundary, reset latStart and lonStart,
        //and also the simulator to where this boundary is located

        pn.setLatStart(totalLat);
        pn.setLonStart(totalLon);

        if (SimInterface::instance()->isRunning())
        {
            pn.latitude = pn.latStart();
            pn.longitude = pn.lonStart();

            SettingsManager::instance()->setGps_simLatitude(pn.latStart());
            SettingsManager::instance()->setGps_simLongitude(pn.lonStart());
            SimInterface::instance()->reset();
        }
        // Phase 6.3.1: Use PropertyWrapper for safe QObject access
        pn.SetLocalMetersPerDegree();
        stop();
    }
}

void CBoundary::addBoundaryOSMPoint(double latitude, double longitude) {
    qDebug(cboundary_log)<<"point.easting";
    double northing;
    double easting;
    Backend::instance()->pn()->ConvertWGS84ToLocal(latitude, longitude, northing, easting);
    //save the north & east as previous
    Vec3 point(easting,northing,0);
    bndBeingMadePts.append(point);
    calculateArea();
}

quint64 CBoundary::calculateFingerprint() const {
    quint64 hash = bndList.count();
    for (const auto &bnd : bndList) {
        hash = hash * 31 + static_cast<quint64>(bnd.fenceLine.count());
        hash = hash * 31 + static_cast<quint64>(bnd.hdLine.count());
        hash = hash * 31 + static_cast<quint64>(bnd.turnLine.count());
    }
    hash = hash * 31 + static_cast<quint64>(bndBeingMadePts.count());
    return hash;
}

void CBoundary::updateInterface() {
    quint64 currentFingerprint = calculateFingerprint();

    if (currentFingerprint != m_lastFingerprint) {
        BoundariesProperties *props = BoundaryInterface::instance()->properties();

        props->clearAll();

        if (bndBeingMadePts.count() > 0) {
            QVector<QVector3D> beingMadePoints;
            beingMadePoints.reserve(bndBeingMadePts.count());
            for (const Vec3 &v : bndBeingMadePts) {
                beingMadePoints.append(QVector3D(v.easting, v.northing, 0));
            }
            props->set_beingMade(beingMadePoints);
        } else {
            props->set_beingMade(QVector<QVector3D>());
        }

        if (bndList.count() > 0) {
            // bndList[0].fenceLine -> outer boundary
            if (bndList[0].fenceLine.count() > 0) {
                auto *outer = new BoundaryProperties(props);
                QList<QVector3D> points;
                points.reserve(bndList[0].fenceLine.count());
                for (const Vec3 &v : bndList[0].fenceLine) {
                    points.append(QVector3D(v.easting, v.northing, 0));
                }
                outer->set_points(points);
                outer->set_visible(true);
                props->addOuter(outer);
            }

            // bndList[0].hdLine -> hdLine
            if (bndList[0].hdLine.count() > 0) {
                auto *hd = new BoundaryProperties(props);
                QList<QVector3D> hdPoints;
                hdPoints.reserve(bndList[0].hdLine.count());
                for (const Vec3 &v : bndList[0].hdLine) {
                    hdPoints.append(QVector3D(v.easting, v.northing, 0));
                }
                hd->set_points(hdPoints);
                if (MainWindowState::instance()->isHeadlandOn()) {
                    hd->set_visible(true);
                } else {
                    hd->set_visible(false);
                }
                props->set_hdLine(hd);
            }

            // bndList[i].fenceLine (i > 0) -> inner boundaries
            for (int i = 1; i < bndList.count(); i++) {
                if (bndList[i].fenceLine.count() > 0) {
                    auto *inner = new BoundaryProperties(props);
                    QList<QVector3D> points;
                    points.reserve(bndList[i].fenceLine.count());
                    for (const Vec3 &v : bndList[i].fenceLine) {
                        points.append(QVector3D(v.easting, v.northing, 0));
                    }
                    inner->set_points(points);
                    inner->set_visible(true);
                    props->addInner(inner);
                }
            }
        }

        m_lastFingerprint = currentFingerprint;
    }
}
