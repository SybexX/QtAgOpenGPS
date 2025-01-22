#include "bluetoothmanager.h"
#include "properties.h"
#include "bluetoothdevicelist.h"

BluetoothManager::BluetoothManager(FormLoop* loop, QObject* parent)
    : QObject(parent), formLoop(loop) {
}
BluetoothManager::~BluetoothManager(){

}
void BluetoothManager::startBluetoothDiscovery(){

    if(consoleDebug) qDebug() << "starting bluetooth discovery";
    //empty device list
    formLoop->btDevicesList->clear();

    // Create a discovery agent and connect to its signals
    discoveryAgent = new QBluetoothDeviceDiscoveryAgent(this);

    connect(discoveryAgent, SIGNAL(deviceDiscovered(QBluetoothDeviceInfo)),
            this, SLOT(deviceDiscovered(QBluetoothDeviceInfo)));
    connect(discoveryAgent, SIGNAL(finished()), this, SLOT(discoveryFinished()));

    // Start a discovery
    discoveryAgent->start();
}
// In your local slot, read information about the found devices
void BluetoothManager::deviceDiscovered(const QBluetoothDeviceInfo &device)
{
    if(devicesNotAvailable.contains(device.name())) return;

    formLoop->btDevicesList->addDevice(device.name());

    //check if we are already connected or connecting
    if(deviceConnected || deviceConnecting) return;

    deviceList = property_setBluetooth_deviceList.value().toStringList();

    if(devicesUserWants.contains(device.name())){
        if(consoleDebug) qDebug() << "Bluetooth Device usr found!";
        connectToDevice(device);
        return;
    }

    if(deviceList.contains(device.name())){
        if(consoleDebug) qDebug() << "Bluetooth Device found!";
        connectToDevice(device);
        return;
    }

}

void BluetoothManager::connectToDevice(const QBluetoothDeviceInfo &device){
    QBluetoothAddress address;
    QBluetoothUuid uuid(QStringLiteral("00001101-0000-1000-8000-00805f9b34fb"));

    address = device.address();

    //set here as it is used for display purposes. If it actually doesn't connect,
    // it's cleared
    connectedDeviceName = device.name();

    if(consoleDebug){
        qDebug() << "Attempting to connect to ";
        qDebug() << connectedDeviceName;
    }

    deviceConnecting = true;

    socket = new QBluetoothSocket(QBluetoothServiceInfo::RfcommProtocol, this);

    connect(socket, SIGNAL(errorOccurred(QBluetoothSocket::SocketError)),
            this, SLOT(onSocketErrorOccurred(QBluetoothSocket::SocketError)));
    connect(socket, SIGNAL(connected()), this, SLOT(connected()));
    connect(socket, SIGNAL(disconnected()), this, SLOT(disconnected()));
    connect(socket, SIGNAL(readyRead()), this, SLOT(readData()));

    socket->connectToService(address, uuid, QIODeviceBase::ReadOnly);
}

void BluetoothManager::connected(){
    if(!deviceList.contains(connectedDeviceName)){
        deviceList.append(connectedDeviceName);
        property_setBluetooth_deviceList = deviceList;
    }
    deviceConnecting = false;
    deviceConnected = true;

    devicesNotAvailable.clear(); // reset so we start over when connection is lost

    formLoop->agio->setProperty("connectedBTDevices", connectedDeviceName);

    if(consoleDebug) qDebug() << "Waiting for incoming data...";
}
void BluetoothManager::disconnected(){

    deviceConnecting = false;
    connectedDeviceName.clear();
    formLoop->agio->setProperty("connectedBTDevices", ""); // tell the frontend nothing is connected
    //restart the discoveryAgent

    discoveryAgent->stop();

    //empty device list
    formLoop->btDevicesList->clear();

    discoveryAgent->start();
    if(deviceConnected) {
        startBluetoothDiscovery();
        formLoop->TimedMessageBox(2000, "Bluetooth Device Disconnected!", "Disconnected from device " + connectedDeviceName);
    }
    deviceConnected = false;
}

void BluetoothManager::readData(){
    QByteArray data = socket->readAll();
    rawBuffer += QString::fromLatin1(data);
    formLoop->ParseNMEA(rawBuffer);
}

void BluetoothManager::discoveryFinished(){
    devicesNotAvailable.clear(); //completely start over
    //make sure we are not connected, and that the user didn't shut off bluetooth
    if(!deviceConnected && property_setBluetooth_isOn) startBluetoothDiscovery();
}
void BluetoothManager::onSocketErrorOccurred(QBluetoothSocket::SocketError error) {
    // Handle different error cases
    switch (error) {
    case QBluetoothSocket::SocketError::HostNotFoundError:
        if(consoleDebug) qDebug() << "Error: Host not found for device" << connectedDeviceName;
        //this is because a paired device that is not connected will show up as an available device
        //when "searching" for devices
        devicesNotAvailable.append(connectedDeviceName);
        socket->disconnectFromService();
        break;
    case QBluetoothSocket::SocketError::ServiceNotFoundError:
        if(consoleDebug) qDebug() << "Error: Service not found for device" << connectedDeviceName;
        break;
    case QBluetoothSocket::SocketError::NetworkError:
        if(consoleDebug) qDebug() << "Error: Network error occurred while communicating with" << connectedDeviceName;
        break;
    case QBluetoothSocket::SocketError::OperationError:
        if(consoleDebug) qDebug() << "Error: Operation error occurred for device" << connectedDeviceName;
        break;
    default:
        if(consoleDebug) qDebug() << "Error: Unknown error occurred for device" << connectedDeviceName << "Error code:" << error;
    }
}

void BluetoothManager::userConnectBluetooth(const QString &device){
    devicesUserWants.append(device);
    if(consoleDebug) qDebug() << devicesUserWants;
    startBluetoothDiscovery();
}

void BluetoothManager::userRemoveDevice(const QString &device){
    qDebug() << "userRemoveDevice: " << device;
    if (connectedDeviceName == device) socket->disconnectFromService();

    if(deviceList.contains(device)){
        deviceList.removeAll(device);
        property_setBluetooth_deviceList = deviceList;
    }
}

void BluetoothManager::kill(){
    socket->disconnectFromService();
    property_setBluetooth_isOn = false;
}

void BluetoothManager::bluetooth_console_debug(bool doWeDebug){
    consoleDebug = doWeDebug;
}
