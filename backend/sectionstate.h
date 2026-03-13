// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
#ifndef SECTIONSTATE_H
#define SECTIONSTATE_H

#include <QObject>
#include <QtQml/qqmlregistration.h>

class SectionState : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    enum State {
        Off = 0,
        Auto = 1,
        On = 2
    };
    Q_ENUM(State)

    explicit SectionState(QObject *parent = nullptr) : QObject(parent) {}
};

#endif // SECTIONSTATE_H
