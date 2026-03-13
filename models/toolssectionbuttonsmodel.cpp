// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
#include "toolssectionbuttonsmodel.h"

ToolsSectionsButtonsModel::ToolsSectionsButtonsModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int ToolsSectionsButtonsModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return toolsSectionsButtons.count();
}

QVariant ToolsSectionsButtonsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= toolsSectionsButtons.count())
        return QVariant();

    const ToolSectionsButtons &row = toolsSectionsButtons[index.row()];

    switch (role) {
    case IndexRole:
        return row.index;
    case SectionButtonsModelRole:
        // Return the QObject pointer for QML
        return QVariant::fromValue(row.sectionButtonsModel.data());
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> ToolsSectionsButtonsModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[IndexRole] = "toolIndex";
    roles[SectionButtonsModelRole] = "sectionButtonsModel";
    return roles;
}

void ToolsSectionsButtonsModel::setToolsSections(const QVector<ToolSectionsButtons> &new_toolsSections)
{
    beginResetModel();

    toolsSectionsButtons = new_toolsSections;
    // Force index to align with real index number
    for (int i = 0; i < toolsSectionsButtons.count(); i++) {
        toolsSectionsButtons[i].index = i;
    }
    endResetModel();
}

void ToolsSectionsButtonsModel::addToolSections(const ToolSectionsButtons &toolSections)
{
    beginInsertRows(QModelIndex(), toolsSectionsButtons.count(), toolsSectionsButtons.count());

    // Make the index the same as the actual index number
    ToolSectionsButtons new_tool = toolSections;
    new_tool.index = toolsSectionsButtons.count();
    toolsSectionsButtons.append(new_tool);

    endInsertRows();
}

void ToolsSectionsButtonsModel::addSectionsModel(SectionButtonsModel *model)
{
    ToolSectionsButtons tool;
    tool.index = toolsSectionsButtons.count();
    tool.sectionButtonsModel = model;
    addToolSections(tool);
}

void ToolsSectionsButtonsModel::removeRowAt(int at_index)
{
    if (at_index < 0 || at_index >= toolsSectionsButtons.count())
        return;

    beginRemoveRows(QModelIndex(), at_index, at_index);
    toolsSectionsButtons.removeAt(at_index);
    endRemoveRows();

    // Adjust indices to match real index number
    for (int i = 0; i < toolsSectionsButtons.count(); i++) {
        if (toolsSectionsButtons[i].index != i) {
            toolsSectionsButtons[i].index = i;
            QModelIndex idx = index(i);
            emit dataChanged(idx, idx, {IndexRole});
        }
    }
}

void ToolsSectionsButtonsModel::clear()
{
    beginResetModel();
    toolsSectionsButtons.clear();
    endResetModel();
}

ToolsSectionsButtonsModel::ToolSectionsButtons ToolsSectionsButtonsModel::toolAt(int at_index) const
{
    if (at_index >= 0 && at_index < toolsSectionsButtons.count())
        return toolsSectionsButtons[at_index];
    return ToolSectionsButtons();
}

SectionButtonsModel* ToolsSectionsButtonsModel::sectionsModelAt(int at_index) const
{
    if (at_index >= 0 && at_index < toolsSectionsButtons.count())
        return toolsSectionsButtons[at_index].sectionButtonsModel.data();
    return nullptr;
}
