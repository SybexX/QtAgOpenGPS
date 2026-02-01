// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
//
// Boundaries properties for FieldViewItem - grouped property for QML
// Contains outer and inner boundary lists

#ifndef BOUNDARIESPROPERTIES_H
#define BOUNDARIESPROPERTIES_H

#include <QObject>
#include <QProperty>
#include <QBindable>
#include <QQmlListProperty>
#include <QList>
#include <QColor>
#include <QtQml/qqmlregistration.h>

#include "boundaryproperties.h"
#include "simpleproperty.h"

class BoundariesProperties : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(Boundaries)

    Q_PROPERTY(QQmlListProperty<BoundaryProperties> outer READ getOuter NOTIFY outerChanged)
    Q_PROPERTY(QQmlListProperty<BoundaryProperties> inner READ getInner NOTIFY innerChanged)

public:
    explicit BoundariesProperties(QObject *parent = nullptr);

    QQmlListProperty<BoundaryProperties> getOuter();
    QQmlListProperty<BoundaryProperties> getInner();

    // Direct access to the lists from C++
    QList<BoundaryProperties*>& outerList() { return m_outer; }
    const QList<BoundaryProperties*>& outer() const { return m_outer; }
    QList<BoundaryProperties*>& innerList() { return m_inner; }
    const QList<BoundaryProperties*>& inner() const { return m_inner; }

    // C++ wrapper methods for outer list manipulation
    void addOuter(BoundaryProperties *boundary);
    void removeOuter(BoundaryProperties *boundary);
    void removeOuterAt(int index);
    void clearAllOuter();
    int outerCount() const { return m_outer.count(); }
    BoundaryProperties* outerAt(int index) const;

    // C++ wrapper methods for inner list manipulation
    void addInner(BoundaryProperties *boundary);
    void removeInner(BoundaryProperties *boundary);
    void removeInnerAt(int index);
    void clearAllInner();
    int innerCount() const { return m_inner.count(); }
    BoundaryProperties* innerAt(int index) const;

    SIMPLE_BINDABLE_PROPERTY(QColor, colorInner)
    SIMPLE_BINDABLE_PROPERTY(QColor, colorOuter)
    SIMPLE_BINDABLE_PROPERTY(QList<QVector3D>, beingMade)
    SIMPLE_BINDABLE_PROPERTY(float, markBoundary)

signals:
    void outerChanged();
    void innerChanged();

private:
    QList<BoundaryProperties*> m_outer;
    QList<BoundaryProperties*> m_inner;

    // QQmlListProperty callbacks for outer
    static void appendOuter(QQmlListProperty<BoundaryProperties> *list, BoundaryProperties *boundary);
    static qsizetype outerCount(QQmlListProperty<BoundaryProperties> *list);
    static BoundaryProperties *outerAt(QQmlListProperty<BoundaryProperties> *list, qsizetype index);
    static void clearOuter(QQmlListProperty<BoundaryProperties> *list);

    // QQmlListProperty callbacks for inner
    static void appendInner(QQmlListProperty<BoundaryProperties> *list, BoundaryProperties *boundary);
    static qsizetype innerCount(QQmlListProperty<BoundaryProperties> *list);
    static BoundaryProperties *innerAt(QQmlListProperty<BoundaryProperties> *list, qsizetype index);
    static void clearInner(QQmlListProperty<BoundaryProperties> *list);


    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(BoundariesProperties, QColor, m_colorInner, QColor(1,1,0), &BoundariesProperties::colorInnerChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(BoundariesProperties, QColor, m_colorOuter, QColor(1,1,0), &BoundariesProperties::colorOuterChanged)
    Q_OBJECT_BINDABLE_PROPERTY(BoundariesProperties, QVector<QVector3D>, m_beingMade, &BoundariesProperties::beingMadeChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(BoundariesProperties, double, m_markBoundary, 0, &BoundariesProperties::markBoundaryChanged)
};

#endif // BOUNDARIESPROPERTIES_H
