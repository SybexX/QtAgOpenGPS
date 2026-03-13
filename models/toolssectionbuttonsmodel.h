// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
#ifndef TOOLSSECTIONBUTTONSMODEL_H
#define TOOLSSECTIONBUTTONSMODEL_H

#include <QAbstractListModel>
#include <QVector>
#include <QPointer>
#include "sectionbuttonsmodel.h"

class ToolsSectionsButtonsModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Roles {
        IndexRole = Qt::UserRole + 1,
        SectionButtonsModelRole
    };
    Q_ENUM(Roles)

    struct ToolSectionsButtons {
        int index;
        QPointer<SectionButtonsModel> sectionButtonsModel;
    };

    explicit ToolsSectionsButtonsModel(QObject *parent = nullptr);

    // QAbstractListModel interface
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    // Data management
    void setToolsSections(const QVector<ToolSectionsButtons> &new_toolsSections);
    void addToolSections(const ToolSectionsButtons &tool);
    void addSectionsModel(SectionButtonsModel *model);
    void removeRowAt(int at_index);
    void clear();

    // Utility
    int count() const { return toolsSectionsButtons.count(); }
    ToolSectionsButtons toolAt(int at_index) const;
    SectionButtonsModel* sectionsModelAt(int at_index) const;

    QVector<ToolSectionsButtons> toolsSectionsButtons;

private:
};

#endif // TOOLSSECTIONBUTTONSMODEL_H
