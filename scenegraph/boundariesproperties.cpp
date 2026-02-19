// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later

#include "boundariesproperties.h"

BoundariesProperties::BoundariesProperties(QObject *parent)
    : QObject{parent}
{}

// ============================================================================
// QQmlListProperty getters
// ============================================================================

QQmlListProperty<BoundaryProperties> BoundariesProperties::getOuter()
{
    return QQmlListProperty<BoundaryProperties>(this, nullptr,
                                                &BoundariesProperties::appendOuter,
                                                &BoundariesProperties::outerCount,
                                                &BoundariesProperties::outerAt,
                                                &BoundariesProperties::clearOuter);
}

QQmlListProperty<BoundaryProperties> BoundariesProperties::getInner()
{
    return QQmlListProperty<BoundaryProperties>(this, nullptr,
                                                &BoundariesProperties::appendInner,
                                                &BoundariesProperties::innerCount,
                                                &BoundariesProperties::innerAt,
                                                &BoundariesProperties::clearInner);
}

// ============================================================================
// Outer list QQmlListProperty callbacks
// ============================================================================

void BoundariesProperties::appendOuter(QQmlListProperty<BoundaryProperties> *list, BoundaryProperties *boundary)
{
    auto *self = static_cast<BoundariesProperties*>(list->object);
    boundary->setParent(self);  // Take ownership
    self->connect(boundary, &BoundaryProperties::boundaryChanged, self, &BoundariesProperties::outerChanged);
    self->m_outer.append(boundary);
    emit self->outerChanged();
}

qsizetype BoundariesProperties::outerCount(QQmlListProperty<BoundaryProperties> *list)
{
    auto *self = static_cast<BoundariesProperties*>(list->object);
    return self->m_outer.count();
}

BoundaryProperties *BoundariesProperties::outerAt(QQmlListProperty<BoundaryProperties> *list, qsizetype index)
{
    auto *self = static_cast<BoundariesProperties*>(list->object);
    return self->m_outer.at(index);
}

void BoundariesProperties::clearOuter(QQmlListProperty<BoundaryProperties> *list)
{
    auto *self = static_cast<BoundariesProperties*>(list->object);
    self->m_outer.clear();
    emit self->outerChanged();
}

// ============================================================================
// Inner list QQmlListProperty callbacks
// ============================================================================

void BoundariesProperties::appendInner(QQmlListProperty<BoundaryProperties> *list, BoundaryProperties *boundary)
{
    auto *self = static_cast<BoundariesProperties*>(list->object);
    boundary->setParent(self);  // Take ownership
    self->connect(boundary, &BoundaryProperties::boundaryChanged, self, &BoundariesProperties::innerChanged);
    self->m_inner.append(boundary);
    emit self->innerChanged();
}

qsizetype BoundariesProperties::innerCount(QQmlListProperty<BoundaryProperties> *list)
{
    auto *self = static_cast<BoundariesProperties*>(list->object);
    return self->m_inner.count();
}

BoundaryProperties *BoundariesProperties::innerAt(QQmlListProperty<BoundaryProperties> *list, qsizetype index)
{
    auto *self = static_cast<BoundariesProperties*>(list->object);
    return self->m_inner.at(index);
}

void BoundariesProperties::clearInner(QQmlListProperty<BoundaryProperties> *list)
{
    auto *self = static_cast<BoundariesProperties*>(list->object);
    self->m_inner.clear();
    emit self->innerChanged();
}

// ============================================================================
// Outer list C++ wrapper methods
// ============================================================================

void BoundariesProperties::addOuter(BoundaryProperties *boundary)
{
    if (!boundary)
        return;

    boundary->setParent(this);
    connect(boundary, &BoundaryProperties::boundaryChanged, this, &BoundariesProperties::outerChanged);
    m_outer.append(boundary);
    emit outerChanged();
}

void BoundariesProperties::removeOuter(BoundaryProperties *boundary)
{
    if (!boundary)
        return;

    int index = m_outer.indexOf(boundary);
    if (index >= 0) {
        m_outer.removeAt(index);
        boundary->disconnect(this);
        emit outerChanged();
    }
}

void BoundariesProperties::removeOuterAt(int index)
{
    if (index < 0 || index >= m_outer.count())
        return;

    BoundaryProperties *boundary = m_outer.at(index);
    m_outer.removeAt(index);
    if (boundary) {
        boundary->disconnect(this);
    }
    emit outerChanged();
}

void BoundariesProperties::clearAllOuter()
{
    for (BoundaryProperties *boundary : m_outer) {
        if (boundary) {
            boundary->disconnect(this);
        }
    }
    m_outer.clear();
    emit outerChanged();
}

BoundaryProperties* BoundariesProperties::outerAt(int index) const
{
    if (index < 0 || index >= m_outer.count())
        return nullptr;
    return m_outer.at(index);
}

// ============================================================================
// Inner list C++ wrapper methods
// ============================================================================

void BoundariesProperties::addInner(BoundaryProperties *boundary)
{
    if (!boundary)
        return;

    boundary->setParent(this);
    connect(boundary, &BoundaryProperties::boundaryChanged, this, &BoundariesProperties::innerChanged);
    m_inner.append(boundary);
    emit innerChanged();
}

void BoundariesProperties::removeInner(BoundaryProperties *boundary)
{
    if (!boundary)
        return;

    int index = m_inner.indexOf(boundary);
    if (index >= 0) {
        m_inner.removeAt(index);
        boundary->disconnect(this);
        emit innerChanged();
    }
}

void BoundariesProperties::removeInnerAt(int index)
{
    if (index < 0 || index >= m_inner.count())
        return;

    BoundaryProperties *boundary = m_inner.at(index);
    m_inner.removeAt(index);
    if (boundary) {
        boundary->disconnect(this);
    }
    emit innerChanged();
}

void BoundariesProperties::clearAllInner()
{
    for (BoundaryProperties *boundary : m_inner) {
        if (boundary) {
            boundary->disconnect(this);
        }
    }
    m_inner.clear();
    emit innerChanged();
}

BoundaryProperties* BoundariesProperties::innerAt(int index) const
{
    if (index < 0 || index >= m_inner.count())
        return nullptr;
    return m_inner.at(index);
}

void BoundariesProperties::clearAll() {
    clearAllInner();
    clearAllOuter();
    auto *hd = new BoundaryProperties(this);
    hd->set_visible(false);
    set_hdLine(hd);
}
