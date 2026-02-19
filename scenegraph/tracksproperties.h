#ifndef TRACKSPROPERTIES_H
#define TRACKSPROPERTIES_H

#include <QObject>
#include <QQmlEngine>
#include <QBindable>
#include <QProperty>
#include <QVector>
#include <QVector3D>

#include "simpleproperty.h"

class TracksProperties : public QObject
{
    Q_OBJECT
    QML_ELEMENT
public:
    explicit TracksProperties(QObject *parent = nullptr);

    //lines:
    //newTrack, refLine, shadowLines (use to create shadow area), currentLine, sideLines, lookahead point, stanley line

    //cabcurve uses desList. curvePts, smooList, curList,

    SIMPLE_BINDABLE_PROPERTY(QVector<QVector3D>, newTrack)
    SIMPLE_BINDABLE_PROPERTY(QVector<QVector3D>, refLine)
    SIMPLE_BINDABLE_PROPERTY(QVector<QVector3D>, currentLine)
    SIMPLE_BINDABLE_PROPERTY(bool, showRefFlags)
    SIMPLE_BINDABLE_PROPERTY(QVector3D, aRefFlag)
    SIMPLE_BINDABLE_PROPERTY(QVector3D, bRefFlag)
signals:
    void tracksPropertiesChanged();

private:
    Q_OBJECT_BINDABLE_PROPERTY(TracksProperties, QVector<QVector3D>, m_newTrack, &TracksProperties::newTrackChanged)
    Q_OBJECT_BINDABLE_PROPERTY(TracksProperties, QVector<QVector3D>, m_refLine, &TracksProperties::refLineChanged)
    Q_OBJECT_BINDABLE_PROPERTY(TracksProperties, QVector<QVector3D>, m_currentLine, &TracksProperties::currentLineChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(TracksProperties, bool, m_showRefFlags, &TracksProperties::showRefFlagsChanged)
    Q_OBJECT_BINDABLE_PROPERTY(TracksProperties, QVector3D, m_aRefFlag, &TracksProperties::aRefFlagChanged)
    Q_OBJECT_BINDABLE_PROPERTY(TracksProperties, QVector3D, m_bRefFlag, &TracksProperties::bRefFlagChanged)

};

#endif // TRACKSPROPERTIES_H
