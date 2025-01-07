#include "bluetoothdevicelist.h"
#include "formloop.h"
#include "agioproperty.h"

BluetoothDeviceList::BluetoothDeviceList(QObject *parent)
    : QAbstractListModel(parent)
{

}

BluetoothDeviceList::~BluetoothDeviceList()
{

}

int BluetoothDeviceList::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);

    return devices_list.count();
}

// Get device name at specific row
QString BluetoothDeviceList::get(int row) const
{
    if (row < 0 || row >= devices_list.count())
        return QString();
    return devices_list.at(row);
}

QVariant BluetoothDeviceList::data(const QModelIndex &index, int role) const
{
    // the index returns the requested row and column information.
    // we ignore the column and only use the row information
    int row = index.row();

    // boundary check for the row
    if(row < 0 || row >= devices_list.count()) {
        return QVariant();
    }

    switch(role) {
    case Qt::DisplayRole:
        return devices_list.value(row);
    }

    // The view asked for other data, just return an empty QVariant
    return QVariant();
}

void BluetoothDeviceList::addDevice(const QString &deviceName){
    beginInsertRows(QModelIndex(), devices_list.count(), devices_list.count());
    devices_list.append(deviceName);
    endInsertRows();

    emit modelChanged();
}

void BluetoothDeviceList::clear(){
    devices_list.clear();
    emit modelChanged();
}

