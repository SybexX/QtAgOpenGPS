// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
//
// Section properties implementation

#include "sectionproperties.h"

SectionProperties::SectionProperties(QObject *parent)
    : QObject{parent}
{
    connect(this, &SectionProperties::leftPositionChanged, this, &SectionProperties::sectionChanged);
    connect(this, &SectionProperties::rightPositionChanged, this,  &SectionProperties::sectionChanged);
    connect(this, &SectionProperties::colorChanged, this,  &SectionProperties::sectionChanged);
}
