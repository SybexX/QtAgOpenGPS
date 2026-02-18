// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
//
// GUI to backend field interface
#include "formgps.h"
#include "qmlutil.h"
#include "classes/settingsmanager.h"
#include <QUrl>
#include <QTimer>

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <iomanip>

#include "cboundarylist.h"
#include "fieldinterface.h"
#include "siminterface.h"
#include "backend.h"


void FormGPS::field_update_list() {

#ifdef __ANDROID__
    QString directoryName = androidDirectory + QCoreApplication::applicationName() + "/Fields";
#else
    QString directoryName = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
                            + "/" + QCoreApplication::applicationName() + "/Fields";
#endif

    // fieldInterface is now a class member, initialized in formgps_ui.cpp
    QList<QVariant> fieldList;
    QMap<QString, QVariant> field;
    int index = 0;

    QDirIterator it(directoryName, QStringList() << "Field.txt", QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()){
        field = FileFieldInfo(it.next());
        if(field.contains("latitude")) {
            field["index"] = index;
            fieldList.append(field);
            index++;
        }
    }

    FieldInterface::instance()->set_field_list(fieldList);
}

void FormGPS::field_close() {
    qDebug() << "field_close";

    // Get current field name and set active field profile for saving
    QString currentField = SettingsManager::instance()->f_currentDir();
    if (!currentField.isEmpty() && currentField != "Default") {
        QString jsonFilename;
#ifdef __ANDROID__
        jsonFilename = androidDirectory + QCoreApplication::applicationName() + "/Fields/" + currentField + "_settings.json";
#else
        jsonFilename = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
                      + "/" + QCoreApplication::applicationName() + "/Fields/" + currentField + "_settings.json";
#endif

        // Set active field profile so FileSaveEverythingBeforeClosingField() saves to field JSON
        SettingsManager::instance()->setActiveFieldProfile(jsonFilename);
        qDebug() << "field_close: Set active field profile for saving:" << jsonFilename;
    }

    FileSaveEverythingBeforeClosingField(false);  // Don't save vehicle when closing field

    // Clear field profile after saving
    SettingsManager::instance()->clearActiveFieldProfile();
    qDebug() << "field_close: Cleared field profile after saving";
}

void FormGPS::field_open(QString field_name) {
    qDebug() << "field_open";

    // Phase 6.0.4: PropertyWrapper completely removed - using Qt 6.8 Q_PROPERTY native architecture

    qDebug() << "✅ field_open: AOGInterface ready, proceeding with field operations";

    FileSaveEverythingBeforeClosingField(false);  // Don't save vehicle when opening field
    if (! FileOpenField(field_name)) {
        TimedMessageBox(8000, tr("Saved field does not exist."), QString(tr("Cannot find the requested saved field.")) + " " +
                                                                field_name);

        SettingsManager::instance()->setF_currentDir("Default");
    } else {
        // Field opened successfully, try to load JSON profile if it exists
        field_load_json(field_name);
    }
}

void FormGPS::field_new(QString field_name) {
    CNMEA &pn = *Backend::instance()->pn();

    FileSaveEverythingBeforeClosingField(false);  // Don't save vehicle to avoid async deadlock

    currentFieldDirectory = field_name.trimmed();
    SettingsManager::instance()->setF_currentDir(currentFieldDirectory);
    JobNew();
    lock.lockForWrite();

    // Phase 6.3.1: Use PropertyWrapper for safe property access
    pn.setLatStart(pn.latitude);
    // Phase 6.3.1: Use PropertyWrapper for safe property access
    pn.setLonStart(pn.longitude);
    // Phase 6.3.1: Use PropertyWrapper for safe QObject access
    pn.SetLocalMetersPerDegree();

    FileCreateField();
    FileCreateSections();
    FileCreateRecPath();
    FileCreateContour();
    FileCreateElevation();
    FileSaveFlags();
    FileCreateBoundary();
    FileSaveTram();
    lock.unlock();
}

void FormGPS::field_new_from(QString existing, QString field_name, int flags) {
    qDebug() << "field_new_from - REFACTORED: save first, then create with single lock";

    // STEP 1: Save current field WITHOUT any lock (cleaner approach)
    qDebug() << "Saving current field before creating new one";
    FileSaveEverythingBeforeClosingField(false);  // Don't save vehicle to avoid async operations
    qDebug() << "Current field saved, proceeding to create new field";

    // STEP 2: Load existing field BEFORE acquiring lock (FileOpenField has its own locks)
    qDebug() << "Before FileOpenField(" << existing << ")";
    if (! FileOpenField(existing,flags)) { //load whatever is requested from existing field
        TimedMessageBox(8000, tr("Existing field cannot be found"), QString(tr("Cannot find the existing saved field.")) + " " +
                                                                existing);
    }
    qDebug() << "After FileOpenField, acquiring lock for field creation operations";

    // STEP 3: Create new field with lock
    lock.lockForWrite();
    qDebug() << "Lock acquired, changing to new name:" << field_name;

    //change to new name
    currentFieldDirectory = field_name;
    qDebug() << "Before SettingsManager setValue";
    SettingsManager::instance()->setF_currentDir(currentFieldDirectory);
    qDebug() << "After SettingsManager setValue";

    FileCreateField();
    FileCreateSections();
    FileCreateElevation();
    FileSaveFlags();
    FileSaveABLines();
    FileSaveCurveLines();

    contourSaveList.clear();
    contourSaveList.append(ct.ptList);
    FileSaveContour();

    FileSaveRecPath();
    FileSaveTram();

    //some how we have to write the existing patches to the disk.
    //FileSaveSections only write pending triangles

    for(QSharedPointer<PatchTriangleList> &l: tool.triStrip[0].patchList) {
        tool.patchSaveList.append(l);
    }
    FileSaveSections();
    lock.unlock();
    qDebug() << "field_new_from completed successfully";
}

bool parseDouble(const std::string& input, double& output) {
    std::string cleaned = input;

    // Replace comma with dot for decimal point compatibility
    std::replace(cleaned.begin(), cleaned.end(), ',', '.');

    std::istringstream iss(cleaned);
    iss.imbue(std::locale::classic()); // Ensures '.' is the decimal point

    iss >> output;

    // Check for parsing success and no extra characters
    return !iss.fail() && iss.eof();
}

void FormGPS::FindLatLon(QString filename)
{
    qDebug() << "Finding average Lat/Lon from KML file:" << filename;

    QFile file(filename);
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
            // Found <coordinates> tag — extract content
            while (true) {
                int endIndex = line.indexOf(QLatin1String("</coordinates>"));
                if (endIndex == -1) {
                    // No closing tag in this line
                    if (startIndex == -1) {
                        coordinates += line;
                    } else {
                        coordinates += QStringView(line).mid(startIndex + 13); // Skip "<coordinates>"
                    }
                } else {
                    // Closing tag found
                    if (startIndex == -1) {
                        coordinates += QStringView(line).left(endIndex);
                    } else {
                        coordinates += QStringView(line).mid(startIndex + 13, endIndex - (startIndex + 13));
                    }
                    break;
                }

                if (stream.atEnd()) break;
                line = stream.readLine().trimmed();
                startIndex = -1; // reset for continuation lines
            }

            // Split coordinates by whitespace (spaces, tabs, newlines)
            QStringList coordList = coordinates.split(QRegularExpression(QStringLiteral("\\s+")),
                                                      Qt::SkipEmptyParts);

            if (coordList.size() <= 2) {
                qWarning() << "Error reading KML: Too few coordinate points.";
                file.close();
                return;
            }

            double totalLat = 0.0;
            double totalLon = 0.0;
            int validCount = 0;

            for (const QString& coord : std::as_const(coordList)) {
                if (coord.length() < 3) continue;

                int comma1 = coord.indexOf(QLatin1Char(','));
                int comma2 = coord.indexOf(QLatin1Char(','), comma1 + 1);

                if (comma1 == -1 || comma2 == -1) continue;

                QString lonStr = coord.left(comma1);
                QString latStr = coord.mid(comma1 + 1, comma2 - comma1 - 1);

                bool okLon = false, okLat = false;
                double lon = lonStr.toDouble(&okLon);
                double lat = latStr.toDouble(&okLat);

                if (okLon && okLat) {
                    totalLon += lon;
                    totalLat += lat;
                    ++validCount;
                }
            }

            if (validCount > 0) {
                lonK = totalLon / validCount;
                latK = totalLat / validCount;
                qDebug() << "Average Lat:" << latK << "Lon:" << lonK;
            } else {
                qWarning() << "No valid coordinates found in KML.";
            }

            break; // Process only the first <coordinates> block
        }
    }

    file.close();
}

void FormGPS::LoadKMLBoundary(QString filename) {
    CNMEA &pn = *Backend::instance()->pn();

    QFile file(filename);
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

                    qDebug() << coord;

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

                    double easting = 0.0, northing = 0.0;
                    pn.ConvertWGS84ToLocal(latK, lonK, northing, easting);
                    Vec3 temp(easting, northing, 0);
                    New.fenceLine.append(temp);
                }

                // Build the boundary: clockwise for outer, counter-clockwise for inner
                New.CalculateFenceArea(bnd.bndList.count());
                New.FixFenceLine(bnd.bndList.count());
                bnd.bndList.append(New);

            } else {
                qWarning() << "Error reading KML: Too few coordinate points.";
                file.close();
                return;
            }

            break; // Process only the first <coordinates> block
        }
    }

    file.close();
}

void FormGPS::field_new_from_KML(QString field_name, QString file_name) {
    CNMEA &pn = *Backend::instance()->pn();

    qDebug() << field_name << " " << file_name;

    //assume the GUI will vet the name a little bit
    field_close();
    lock.lockForWrite();
    FileSaveEverythingBeforeClosingField(false);  // Don't save vehicle when creating field from KML
    currentFieldDirectory = field_name.trimmed();
    SettingsManager::instance()->setF_currentDir(currentFieldDirectory);
    JobNew();
    // Convert QML file URL to local path using QUrl for robustness
    QUrl fileUrl(file_name);
    QString localPath = fileUrl.toLocalFile();
    if (localPath.isEmpty()) {
        // Fallback for manual parsing if QUrl fails
        file_name.remove("file:///");
        if (file_name.startsWith("/") && file_name.length() > 3 && file_name[2] == ':') {
            file_name.remove(0, 1);
        }
        localPath = file_name;
    }
    file_name = localPath;
    FindLatLon(file_name);

    // Phase 6.3.1: Use PropertyWrapper for safe property access
    pn.setLatStart(latK);
    // Phase 6.3.1: Use PropertyWrapper for safe property access
    pn.setLonStart(lonK);
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


    FileCreateField();
    FileCreateSections();
    FileCreateRecPath();
    FileCreateContour();
    FileCreateElevation();
    FileSaveFlags();
    FileSaveABLines();     // Create empty AB lines files
    FileSaveCurveLines();  // Create empty curve lines files
    FileCreateBoundary();
    FileSaveTram();
    FileSaveHeadland();    // Create empty Headland.txt to prevent load errors

    LoadKMLBoundary(file_name);
    lock.unlock();
}

void FormGPS::field_delete(QString field_name) {
#ifdef __ANDROID__
    QString directoryName = androidDirectory + QCoreApplication::applicationName() + "/Fields/" + field_name;
#else
    QString directoryName = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
                            + "/" + QCoreApplication::applicationName() + "/Fields/" + field_name;
#endif

    QDir fieldDir(directoryName);

    if(! fieldDir.exists()) {
        TimedMessageBox(8000,tr("Cannot find saved field"),QString(tr("Cannot find saved field to delete.")) + " " + field_name);
        return;
    }
    if(!QFile::moveToTrash(directoryName)){
        fieldDir.removeRecursively();
    }
    field_update_list();
}

void FormGPS::field_saveas(QString field_name) {
#ifdef __ANDROID__
    QString directoryName = androidDirectory + QCoreApplication::applicationName() + "/Fields";
#else
    QString directoryName = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
                            + "/" + QCoreApplication::applicationName() + "/Fields";
#endif

    QDir saveDir(directoryName);
    if (!saveDir.exists()) {
        bool ok = saveDir.mkpath(directoryName);
        if (!ok) {
            qWarning() << "Couldn't create path " << directoryName;
            return;
        }
    }

    QString jsonFilename = directoryName + "/" + field_name + "_settings.json";

    // Save current field settings to JSON for auto-sync
    qDebug() << "Field saveas: Scheduling async saveJson:" << jsonFilename;
    QTimer::singleShot(50, this, [this, jsonFilename, field_name]() {
        qDebug() << "Field saveas: Executing async saveJson:" << jsonFilename;
        SettingsManager::instance()->saveJson(jsonFilename);

        // Set as active field profile for future auto-saving
        SettingsManager::instance()->setActiveFieldProfile(jsonFilename);
        qDebug() << "Field saveas: JSON saved and set as active profile:" << jsonFilename;

        // Also save traditional .txt files (existing system)
        // This keeps compatibility with existing Field.txt, Boundary.txt etc.
        this->field_update_list();
        qDebug() << "Field saveas: Field list updated";
    });
}

void FormGPS::field_load_json(QString field_name) {
#ifdef __ANDROID__
    QString directoryName = androidDirectory + QCoreApplication::applicationName() + "/Fields";
#else
    QString directoryName = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
                            + "/" + QCoreApplication::applicationName() + "/Fields";
#endif

    QString jsonFilename = directoryName + "/" + field_name + "_settings.json";

    // Check if JSON profile exists
    if (QFile::exists(jsonFilename)) {
        qDebug() << "Field load JSON starting:" << jsonFilename;
        SettingsManager::instance()->loadJson(jsonFilename);

        // Set as active field profile for auto-saving
        SettingsManager::instance()->setActiveFieldProfile(jsonFilename);
        qDebug() << "Field JSON loaded and set as active profile:" << jsonFilename;
    } else {
        qDebug() << "Field JSON profile not found, using traditional .txt system:" << jsonFilename;
    }
}
