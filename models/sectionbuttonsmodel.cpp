// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
#include "sectionbuttonsmodel.h"

SectionButtonsModel::SectionButtonsModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int SectionButtonsModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return buttonStates.count();
}

QVariant SectionButtonsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= buttonStates.count())
        return QVariant();

    const ButtonState &section = buttonStates[index.row()];

    switch (role) {
    case IndexRole:
        return section.index;
    case StateRole:
        return section.state;
    default:
        return QVariant();
    }
}

bool SectionButtonsModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid() || index.row() >= buttonStates.count())
        return false;

    ButtonState &row = buttonStates[index.row()];

    switch (role) {
    case StateRole:
        if (row.state != value.toInt()) {
            row.state = static_cast<State>(value.toInt());
            emit dataChanged(index, index, {StateRole});
            emit stateChanged(index.row(), row.state);
            return true;
        }
        break;
    case IndexRole:
        // Index is read-only, managed internally
        return false;
    }

    return false;
}

Qt::ItemFlags SectionButtonsModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    return QAbstractListModel::flags(index) | Qt::ItemIsEditable;
}

QHash<int, QByteArray> SectionButtonsModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[IndexRole] = "buttonNumber";
    roles[StateRole] = "state";
    return roles;
}

void SectionButtonsModel::setButtonStates(const QVector<ButtonState> &new_sectionstates)
{
    beginResetModel();

    buttonStates = new_sectionstates;
    //force secNumber to align with real index number
    for (int i = 0 ; i < buttonStates.count(); i++) {
        buttonStates[i].index = i;
    }
    endResetModel();
}

void SectionButtonsModel::addSectionState(const ButtonState &sectionState)
{
    beginInsertRows(QModelIndex(), buttonStates.count(), buttonStates.count());

    //make the secNumber the same as the actual index number
    ButtonState new_state = sectionState;
    new_state.index = buttonStates.count();
    buttonStates.append(new_state);

    endInsertRows();
}

void SectionButtonsModel::removeRowAt(int at_index)
{
    beginRemoveRows(QModelIndex(), at_index, at_index);
    buttonStates.removeAt(at_index);
    endRemoveRows();

    //adjust secNumbers to match real index number
    for (int i = 0; i < buttonStates.count(); i++) {
        if (buttonStates[i].index != i) {
            buttonStates[i].index = i;
            QModelIndex idx = index(at_index);
            emit dataChanged(idx, idx, {StateRole});
        }
    }
}

void SectionButtonsModel::setAllState(State state) {
    for (int i=0; i < buttonStates.count(); i++) {
        setState(i,state);
    }
}

void SectionButtonsModel::clear()
{
    beginResetModel();
    buttonStates.clear();
    endResetModel();
}

void SectionButtonsModel::setState(int at_index, State state)
{
    if (at_index < 0 || at_index >= buttonStates.count())
        return;

    buttonStates[at_index].index = at_index;
    buttonStates[at_index].state = state;
    QModelIndex idx = index(at_index);
    emit dataChanged(idx, idx, {StateRole});
    emit stateChanged(at_index, state);
}

SectionButtonsModel::ButtonState SectionButtonsModel::rowAt(int at_index) const
{
    if (buttonStates.count() > at_index)
        return buttonStates[at_index];
    return ButtonState();
}
