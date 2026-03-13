// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
#ifndef FLAGSINTERFACE_H
#define FLAGSINTERFACE_H

#include <QObject>
#include <QProperty>
#include <QQmlEngine>
#include <QMutex>

#include "simpleproperty.h"
#include "flagmodel.h"



class FlagsInterface : public QObject
{
    Q_OBJECT
    QML_SINGLETON
    QML_ELEMENT
public:
    enum Color {
        Red = 0,
        Green = 1,
        Yellow = 2
    };
    Q_ENUM(Color)

private:
    explicit FlagsInterface(QObject *parent = nullptr);
    ~FlagsInterface() override = default;

    // Prevent copying
    FlagsInterface(const FlagsInterface &) = delete;
    FlagsInterface &operator=(const FlagsInterface &) = delete;

    static FlagsInterface *s_instance;
    static QMutex s_mutex;
    static bool s_cpp_created;

public:
    static FlagsInterface *instance();
    static FlagsInterface *create(QQmlEngine *qmlEngine, QJSEngine *jsEngine);

    Q_PROPERTY(FlagModel* flagModel READ flagModel CONSTANT)
    FlagModel *flagModel() const { return m_flagModel; }
    FlagModel *m_flagModel;

    Q_PROPERTY(int count READ count WRITE setCount
                   NOTIFY countChanged BINDABLE bindableCount)
    int count() const;
    void setCount( const int new_count);
    QBindable<int> bindableCount();

    Q_PROPERTY(int currentFlag READ currentFlag WRITE setCurrentFlag
                   NOTIFY currentFlagChanged BINDABLE bindableCurrentFlag)
    int currentFlag() const;
    void setCurrentFlag(const int &index);
    QBindable<int> bindableCurrentFlag();

    Q_PROPERTY(QString currentNotes READ currentNotes WRITE setCurrentNotes
                   NOTIFY currentNotesChanged BINDABLE bindableCurrentNotes)
    QString currentNotes();
    void setCurrentNotes(const QString &value);
    QBindable<QString> bindableCurrentNotes();

    Q_PROPERTY(FlagsInterface::Color currentColor READ currentColor WRITE setCurrentColor
                   NOTIFY currentColorChanged BINDABLE bindableCurrentColor)
    FlagsInterface::Color currentColor();
    void setCurrentColor(const Color &value);
    QBindable<FlagsInterface::Color> bindableCurrentColor();

    SIMPLE_BINDABLE_PROPERTY(double, currentLatitude)
    SIMPLE_BINDABLE_PROPERTY(double, currentLongitude)
    SIMPLE_BINDABLE_PROPERTY(double, currentHeading)
    SIMPLE_BINDABLE_PROPERTY(double, currentEasting)
    SIMPLE_BINDABLE_PROPERTY(double, currentNorthing)

    //methods for QML to use
    Q_INVOKABLE void setNotes(int index, QString notes);
    Q_INVOKABLE void setColor(int index, FlagsInterface::Color color);

    Q_INVOKABLE int flag(double latitude, double longitude, double easting, double northing, double heading, FlagsInterface::Color color, QString notes);
    Q_INVOKABLE void nextFlag();
    Q_INVOKABLE void prevFlag();
    Q_INVOKABLE void deleteCurrentFlag();
    Q_INVOKABLE void cancelCurrentFlag();

    Q_INVOKABLE void syncCount();
    Q_INVOKABLE void clearFlags();

signals:
    void countChanged();
    void currentFlagChanged();
    void currentNotesChanged();
    void currentColorChanged();

    void saveFlags();

private:
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(FlagsInterface, int, m_count, 0, &FlagsInterface::countChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(FlagsInterface, int, m_currentFlag, -1, &FlagsInterface::currentFlagChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(FlagsInterface, double, m_currentLatitude, 255, &FlagsInterface::currentLatitudeChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(FlagsInterface, double, m_currentLongitude, 255, &FlagsInterface::currentLongitudeChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(FlagsInterface, double, m_currentEasting, 99999999, &FlagsInterface::currentEastingChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(FlagsInterface, double, m_currentNorthing, 99999999, &FlagsInterface::currentNorthingChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(FlagsInterface, double, m_currentHeading, 999, &FlagsInterface::currentHeadingChanged)
    Q_OBJECT_BINDABLE_PROPERTY(FlagsInterface, QString, m_currentNotes, &FlagsInterface::currentNotesChanged)
    Q_OBJECT_BINDABLE_PROPERTY(FlagsInterface, FlagsInterface::Color, m_currentColor, &FlagsInterface::currentColorChanged)
};

#endif // FLAGSINTERFACE_H
