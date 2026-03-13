// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
//
// GUI to backend vehicle interface
#include "formgps.h"
#include "qmlutil.h"
#include "classes/settingsmanager.h"
#include <QTimer>

QString caseInsensitiveFilename(const QString &directory, const QString &filename);

void FormGPS::vehicle_saveas(QString vehicle_name) {
#ifdef __ANDROID__
    QString directoryName = androidDirectory + QCoreApplication::applicationName() + "/Vehicles";
#else
    QString directoryName = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
                            + "/" + QCoreApplication::applicationName() + "/Vehicles";
#endif

    QDir saveDir(directoryName);
    if (!saveDir.exists()) {
        bool ok = saveDir.mkpath(directoryName);
        if (!ok) {
            qWarning() << "Couldn't create path " << directoryName;
            return;
        }
    }

    QString filename = directoryName + "/" + caseInsensitiveFilename(directoryName, vehicle_name);

    // CRITICAL: Save current vehicle AND tool settings to SettingsManager BEFORE saving JSON
    qDebug() << "Before CVehicle::saveSettings()";
    CVehicle::instance()->saveSettings();
    qDebug() << "CVehicle::saveSettings() completed";

    qDebug() << "Before tool.saveSettings()";
    this->tool.saveSettings();
    qDebug() << "tool.saveSettings() completed";

    // RESTORED: JSON Vehicle Profile System (format compatible avec Documents/QtAgOpenGPS/Vehicles/)
    // ASYNC SOLUTION: Defer saveJson to avoid mutex deadlock (same as field_close fix)
    qDebug() << "Scheduling async saveJson:" << filename;
    QTimer::singleShot(50, this, [this, filename]() {
        qDebug() << "Executing async saveJson:" << filename;
        SettingsManager::instance()->saveJson(filename);

        // Set as active profile for future auto-saving
        SettingsManager::instance()->setActiveVehicleProfile(filename);
        qDebug() << "Async saveJson completed and set as active profile:" << filename;

        // Update vehicle list after save is complete for real-time UI refresh
        this->vehicle_update_list();
        qDebug() << "Vehicle list updated after save";
    });

}

void FormGPS::vehicle_load(QString vehicle_name) {
#ifdef __ANDROID__
    QString directoryName = androidDirectory + QCoreApplication::applicationName() + "/Vehicles";
#else
    QString directoryName = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
                            + "/" + QCoreApplication::applicationName() + "/Vehicles";
#endif

    QDir loadDir(directoryName);
    if (!loadDir.exists()) {
        bool ok = loadDir.mkpath(directoryName);
        if (!ok) {
            qWarning() << "Couldn't create path " << directoryName;
            return;
        }
    }

    if (!loadDir.exists(caseInsensitiveFilename(directoryName, vehicle_name)))
        qWarning() << vehicle_name << " may not exist but will try to load it anyway.";

    QString filename = directoryName + "/" + caseInsensitiveFilename(directoryName, vehicle_name);

    // CRITICAL: Set as active profile BEFORE loading to prevent vehicleName corruption
    // This ensures that loadJson() sets the correct active profile path before loading
    SettingsManager::instance()->setActiveVehicleProfile(filename);
    qDebug() << "Vehicle profile set as active before loading:" << filename;

    // RESTORED: JSON Vehicle Profile Loading System (format compatible avec Documents/QtAgOpenGPS/Vehicles/)
    qDebug() << "Vehicle load starting:" << filename;
    SettingsManager::instance()->loadJson(filename);
    qDebug() << "Vehicle JSON loaded successfully:" << filename;
}

void FormGPS::vehicle_delete(QString vehicle_name) {
#ifdef __ANDROID__
    QString directoryName = androidDirectory + QCoreApplication::applicationName() + "/Vehicles";
#else
    QString directoryName = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
                            + "/" + QCoreApplication::applicationName() + "/Vehicles";
#endif

    QDir vehicleDir(directoryName);
    if (vehicleDir.exists()) {
        if (! vehicleDir.remove(caseInsensitiveFilename(directoryName, vehicle_name)))
            qWarning() << "Could not delete vehicle " << vehicle_name;
    }
}

void FormGPS::vehicle_update_list() {
#ifdef __ANDROID__
    QString directoryName = androidDirectory + QCoreApplication::applicationName() + "/Vehicles";
#else
    QString directoryName = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
                            + "/" + QCoreApplication::applicationName() + "/Vehicles";
#endif

    QDir vehicleDirectory(directoryName);
    if(!vehicleDirectory.exists()) {
        vehicleDirectory.mkpath(directoryName);
    }

    vehicleDirectory.setFilter(QDir::Files);

    QFileInfoList filesList = vehicleDirectory.entryInfoList();

    QList<QVariant> vehicleList;
    QMap<QString, QVariant>vehicle;
    int index = 0;

    for (QFileInfo &file : filesList) {
        vehicle.clear();
        vehicle["index"] = index;
        vehicle["name"] = file.fileName();
        vehicleList.append(vehicle);
        index++;
    }

    CVehicle::instance()->setVehicleList(vehicleList);
    // Qt 6.8 QProperty + BINDABLE: vehicle_listChanged signal removed, automatic notification
}

