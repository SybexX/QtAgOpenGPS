// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
//
// Recorded path properties for FieldViewItem scene graph rendering

#include "recordedpathproperties.h"

RecordedPathProperties::RecordedPathProperties(QObject *parent) : QObject(parent)
{
    connect(this, &RecordedPathProperties::recordedLineChanged, this, &RecordedPathProperties::recordedPathPropertiesChanged);
    connect(this, &RecordedPathProperties::dubinsPathChanged, this, &RecordedPathProperties::recordedPathPropertiesChanged);
    connect(this, &RecordedPathProperties::lookaheadPointChanged, this, &RecordedPathProperties::recordedPathPropertiesChanged);
    connect(this, &RecordedPathProperties::showLookaheadChanged, this, &RecordedPathProperties::recordedPathPropertiesChanged);
}
