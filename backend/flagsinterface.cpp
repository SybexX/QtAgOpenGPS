// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
#include <QCoreApplication>
#include "flagsinterface.h"

FlagsInterface *FlagsInterface::s_instance = nullptr;
QMutex FlagsInterface::s_mutex;
bool FlagsInterface::s_cpp_created = false;

FlagsInterface::FlagsInterface(QObject *parent)
    : QObject{parent}
{
    m_flagModel = new FlagModel(this);
}

FlagsInterface *FlagsInterface::instance()
{
    QMutexLocker locker(&s_mutex);
    if (!s_instance) {
        s_instance = new FlagsInterface();
        s_cpp_created = true;
        // Ensure cleanup on app exit
        QObject::connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit,
                         s_instance, []() {
                             delete s_instance;
                             s_instance = nullptr;
                         });
    }
    return s_instance;
}

FlagsInterface *FlagsInterface::create(QQmlEngine *qmlEngine, QJSEngine *jsEngine)
{
    Q_UNUSED(jsEngine)

    QMutexLocker locker(&s_mutex);

    if (!s_instance) {
        s_instance = new FlagsInterface();
    } else if (s_cpp_created) {
        qmlEngine->setObjectOwnership(s_instance, QQmlEngine::CppOwnership);
    }

    return s_instance;
}

int FlagsInterface::count() const { return m_count.value(); }
void FlagsInterface::setCount(const int new_count) {
    m_count.setValue(new_count);
}
QBindable<int> FlagsInterface::bindableCount() {
    return &m_count;
}

int FlagsInterface::currentFlag() const { return m_currentFlag.value(); }
void FlagsInterface::setCurrentFlag(const int &index) {
    m_currentFlag.setValue(index);

    if (index >=0) {
        setCurrentColor(static_cast<FlagsInterface::Color>(m_flagModel->flags[index-1].color));
        set_currentLatitude(m_flagModel->flags[index-1].latitude);
        set_currentLongitude(m_flagModel->flags[index-1].longitude);
        set_currentHeading(m_flagModel->flags[index-1].heading);
        set_currentEasting(m_flagModel->flags[index-1].easting);
        set_currentNorthing(m_flagModel->flags[index-1].northing);
        setCurrentNotes(m_flagModel->flags[index-1].notes);
    } else {
        set_currentLatitude(0);
        set_currentLongitude(0);
        setCurrentNotes("");
    }
}

QBindable<int> FlagsInterface::bindableCurrentFlag() {
    return &m_currentFlag;
}

QString FlagsInterface::currentNotes() {
    return m_currentNotes.value();
}

void FlagsInterface::setCurrentNotes(const QString &value) {
    if (currentFlag() >= 0) {
        //update our own value but also update the model
        m_currentNotes.setValue(value);
        m_flagModel->setNotes(currentFlag(), value);
    }
}

QBindable<QString> FlagsInterface::bindableCurrentNotes() {
    return &m_currentNotes;
}


FlagsInterface::Color FlagsInterface::currentColor() {
    return m_currentColor.value();
}

void FlagsInterface::setCurrentColor(const FlagsInterface::Color &value) {
    if (currentFlag() >= 0) {
        //update our own value but also update the model
        m_currentColor.setValue(value);
        m_flagModel->setColor(currentFlag(), value);
    }
}

QBindable<FlagsInterface::Color> FlagsInterface::bindableCurrentColor() {
    return &m_currentColor;
}

void FlagsInterface::setNotes(int index, QString notes) {
    if (index >=0 && index < count()) {
        m_flagModel->setNotes(index, notes);
        if (index == currentFlag())
            setCurrentNotes(notes);
    }
}

void FlagsInterface::setColor(int index, FlagsInterface::Color color) {
    if (index >=0 && index < count()) {
        m_flagModel->setColor(index, color);
    }
}

int FlagsInterface::flag(double latitude, double longitude,
                          double easting, double northing,
                          double heading, Color color,
                          QString notes)
{
    //drop a flag at these coordinates
    int count = m_flagModel->count()+1;
    FlagModel::Flag flag( { count, color, latitude, longitude, heading, easting, northing, notes });
    m_flagModel->addFlag(flag);

    //we don't select the new flag here.  But we do let
    //the caller know what the index was and they can
    //select it if that's appropriate.
    setCount(flagModel()->count());
    return count; //return index of the added flag.
}

void FlagsInterface::nextFlag() {
    //select the next flag, and automatically
    //load properties so QML will update automatically
    if (currentFlag() >= 0) {
        setCurrentFlag((currentFlag()) % count() + 1);
    }
}

void FlagsInterface::prevFlag() {
    //select the previous flag, and automatically
    //load properties so QML will update automatically
    if (currentFlag() == 1) {
        setCurrentFlag(count()); //end
    } else {
        setCurrentFlag(currentFlag() - 1);
    }
}

void FlagsInterface::deleteCurrentFlag() {
    if (currentFlag() >= 0) {
        flagModel()->removeFlag(currentFlag());
        setCount(flagModel()->count());

        if (count() == 0) {
            setCurrentFlag(0); //deselect all flags
            return;
        }

        if (currentFlag() <= count()) {
            //load the next flag after the deleted one
            setCurrentFlag(currentFlag());
        } else {
            //load the flag right before the deleted one.
            setCurrentFlag(count());
        }
    }
}

void FlagsInterface::cancelCurrentFlag() {
    //any flag index below zero means there
    //is none.
    setCurrentFlag(-1);
    emit saveFlags(); //request that flags be saved
}

void FlagsInterface::syncCount() {
    setCount(flagModel()->count());
}

void FlagsInterface::clearFlags() {
    flagModel()->clear();
}
