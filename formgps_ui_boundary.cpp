// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later

//GUI to backend boundary interface
#include "formgps.h"
#include "qmlutil.h"
#include "classes/settingsmanager.h"
#include "backend.h"
#include "siminterface.h"

void FormGPS::boundary_new_from_KML(QString filename) {
    CNMEA &pn = *Backend::instance()->pn();

    qDebug() << "Opening KML file:" << filename;
    QUrl fileUrl(filename);
    QString localPath = fileUrl.toLocalFile();
    FindLatLon(localPath);

    pn.setLatStart(latK);
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
    LoadKMLBoundary(localPath);
    bnd.stop();
}
