// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
#ifndef SECTIONBUTTONSMODEL_H
#define SECTIONBUTTONSMODEL_H

#include <QAbstractListModel>
#include <QVector>
#include "sectionstate.h"

class SectionButtonsModel : public QAbstractListModel
{
    Q_OBJECT

public:
    // Use SectionState::State for the state enum
    using State = SectionState::State;

    enum Roles {
        IndexRole = Qt::UserRole + 1,
        StateRole
    };
    Q_ENUM(Roles)

    struct ButtonState {
        int index;
        State state;
    };

    explicit SectionButtonsModel(QObject *parent = nullptr);

    // QAbstractListModel interface
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QHash<int, QByteArray> roleNames() const override;

    // Data management
    void setButtonStates(const QVector<ButtonState> &new_sectionstates);
    void addSectionState(const ButtonState &sectionState);
    void removeRowAt(int at_index);
    void clear();

    void setState(int at_index, State state);
    void setAllState(State state);

    // Utility
    int count() const { return buttonStates.count(); }
    ButtonState rowAt(int at_index) const;

    QVector<ButtonState> buttonStates;

signals:
    void stateChanged(int index, int state);

private:
};

#endif // SECTIONBUTTONSMODEL_H
