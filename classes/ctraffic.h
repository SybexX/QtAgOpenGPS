// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
//
// CTraffic - Traffic Monitoring Class
// Extracted from udpworker.h in Phase 6.0.24

#ifndef CTRAFFIC_H
#define CTRAFFIC_H

#include <QObject>
#include <QProperty>
#include <QBindable>
#include <QtQml/qqmlregistration.h>

/**
 * @brief Traffic monitoring class - Qt 6.8 QObject + QProperty + BINDABLE
 *
 * Monitors hello messages from modules to determine connection status.
 * Logic: if helloFromXXX < 3, module is considered connected.
 *
 * Qt 6.8 Migration: Q_GADGET → QObject + QProperty + BINDABLE
 * Compatible with AgIO C# CTraffic via QML_ELEMENT
 */
class CTraffic : public QObject {
    Q_OBJECT
    QML_ELEMENT

    // Qt 6.8 QProperty + BINDABLE + NOTIFY Pattern
    // Phase 6.0.21.7: Renamed GPS → UDP (these properties track ALL UDP traffic, not just GPS)
    Q_PROPERTY(quint32 helloFromMachine READ helloFromMachine WRITE setHelloFromMachine NOTIFY helloFromMachineChanged BINDABLE bindableHelloFromMachine)
    Q_PROPERTY(quint32 helloFromBlockage READ helloFromBlockage WRITE setHelloFromBlockage NOTIFY helloFromBlockageChanged BINDABLE bindableHelloFromBlockage)
    Q_PROPERTY(quint32 helloFromRateControl READ helloFromRateControl WRITE setHelloFromRateControl NOTIFY helloFromRateControlChanged BINDABLE bindableHelloFromRateControl)
    Q_PROPERTY(quint32 helloFromAutoSteer READ helloFromAutoSteer WRITE setHelloFromAutoSteer NOTIFY helloFromAutoSteerChanged BINDABLE bindableHelloFromAutoSteer)
    Q_PROPERTY(quint32 helloFromIMU READ helloFromIMU WRITE setHelloFromIMU NOTIFY helloFromIMUChanged BINDABLE bindableHelloFromIMU)
    Q_PROPERTY(quint32 cntrUDPOut READ cntrUDPOut WRITE setCntrUDPOut NOTIFY cntrUDPOutChanged BINDABLE bindableCntrUDPOut)
    Q_PROPERTY(quint32 cntrUDPIn READ cntrUDPIn WRITE setCntrUDPIn NOTIFY cntrUDPInChanged BINDABLE bindableCntrUDPIn)
    Q_PROPERTY(quint32 cntrUDPInBytes READ cntrUDPInBytes WRITE setCntrUDPInBytes NOTIFY cntrUDPInBytesChanged BINDABLE bindableCntrUDPInBytes)
    Q_PROPERTY(double udpOutRate READ udpOutRate WRITE setUdpOutRate NOTIFY udpOutRateChanged BINDABLE bindableUdpOutRate)
    Q_PROPERTY(double udpInRate READ udpInRate WRITE setUdpInRate NOTIFY udpInRateChanged BINDABLE bindableUdpInRate)
    Q_PROPERTY(double udpOutFreq READ udpOutFreq WRITE setUdpOutFreq NOTIFY udpOutFreqChanged BINDABLE bindableUdpOutFreq)
    Q_PROPERTY(double udpInFreq READ udpInFreq WRITE setUdpInFreq NOTIFY udpInFreqChanged BINDABLE bindableUdpInFreq)


public:
    // Constructor - Phase 6.0.21.2: Initialize Q_OBJECT_BINDABLE_PROPERTY in initialization list
    explicit CTraffic(QObject *parent = nullptr)
        : QObject(parent)
        , m_helloFromMachine(99)
        , m_helloFromBlockage(99)
        , m_helloFromRateControl(99)
        , m_helloFromAutoSteer(99)
        , m_helloFromIMU(99)
        , m_cntrUDPOut(0)
        , m_cntrUDPIn(0)
        , m_cntrUDPInBytes(0)
        , m_udpOutRate(0.0)
        , m_udpInRate(0.0)
        , m_udpOutFreq(0.0)
        , m_udpInFreq(0.0)
    {
        // Q_OBJECT_BINDABLE_PROPERTY members initialized in list above
    }

    // Qt 6.8 READ accessors
    quint32 helloFromMachine() const;
    quint32 helloFromBlockage() const;
    quint32 helloFromRateControl() const;
    quint32 helloFromAutoSteer() const;
    quint32 helloFromIMU() const;
    quint32 cntrUDPOut() const;
    quint32 cntrUDPIn() const;
    quint32 cntrUDPInBytes() const;
    double udpOutRate() const;
    double udpInRate() const;
    double udpOutFreq() const;
    double udpInFreq() const;

    // Qt 6.8 WRITE mutators
    void setHelloFromMachine(quint32 value);
    void setHelloFromBlockage(quint32 value);
    void setHelloFromRateControl(quint32 value);
    void setHelloFromAutoSteer(quint32 value);
    void setHelloFromIMU(quint32 value);
    void setCntrUDPOut(quint32 value);
    void setCntrUDPIn(quint32 value);
    void setCntrUDPInBytes(quint32 value);
    void setUdpOutRate(double value);
    void setUdpInRate(double value);
    void setUdpOutFreq(double value);
    void setUdpInFreq(double value);

    // Qt 6.8 BINDABLE functions
    QBindable<quint32> bindableHelloFromMachine();
    QBindable<quint32> bindableHelloFromBlockage();
    QBindable<quint32> bindableHelloFromRateControl();
    QBindable<quint32> bindableHelloFromAutoSteer();
    QBindable<quint32> bindableHelloFromIMU();
    QBindable<quint32> bindableCntrUDPOut();
    QBindable<quint32> bindableCntrUDPIn();
    QBindable<quint32> bindableCntrUDPInBytes();
    QBindable<double> bindableUdpOutRate();
    QBindable<double> bindableUdpInRate();
    QBindable<double> bindableUdpOutFreq();
    QBindable<double> bindableUdpInFreq();

signals:
    // Qt 6.8 QProperty NOTIFY signals for Q_OBJECT_BINDABLE_PROPERTY migration
    void helloFromMachineChanged();
    void helloFromBlockageChanged();
    void helloFromRateControlChanged();
    void helloFromAutoSteerChanged();
    void helloFromIMUChanged();
    void cntrUDPOutChanged();
    void cntrUDPInChanged();
    void cntrUDPInBytesChanged();
    void udpOutRateChanged();
    void udpInRateChanged();
    void udpOutFreqChanged();
    void udpInFreqChanged();

private:
    // Qt 6.8 Q_OBJECT_BINDABLE_PROPERTY Private Members
    Q_OBJECT_BINDABLE_PROPERTY(CTraffic, quint32, m_helloFromMachine, &CTraffic::helloFromMachineChanged)
    Q_OBJECT_BINDABLE_PROPERTY(CTraffic, quint32, m_helloFromBlockage, &CTraffic::helloFromBlockageChanged)
    Q_OBJECT_BINDABLE_PROPERTY(CTraffic, quint32, m_helloFromRateControl, &CTraffic::helloFromRateControlChanged)
    Q_OBJECT_BINDABLE_PROPERTY(CTraffic, quint32, m_helloFromAutoSteer, &CTraffic::helloFromAutoSteerChanged)
    Q_OBJECT_BINDABLE_PROPERTY(CTraffic, quint32, m_helloFromIMU, &CTraffic::helloFromIMUChanged)
    Q_OBJECT_BINDABLE_PROPERTY(CTraffic, quint32, m_cntrUDPOut, &CTraffic::cntrUDPOutChanged)
    Q_OBJECT_BINDABLE_PROPERTY(CTraffic, quint32, m_cntrUDPIn, &CTraffic::cntrUDPInChanged)
    Q_OBJECT_BINDABLE_PROPERTY(CTraffic, quint32, m_cntrUDPInBytes, &CTraffic::cntrUDPInBytesChanged)
    Q_OBJECT_BINDABLE_PROPERTY(CTraffic, double, m_udpOutRate, &CTraffic::udpOutRateChanged)
    Q_OBJECT_BINDABLE_PROPERTY(CTraffic, double, m_udpInRate, &CTraffic::udpInRateChanged)
    Q_OBJECT_BINDABLE_PROPERTY(CTraffic, double, m_udpOutFreq, &CTraffic::udpOutFreqChanged)
    Q_OBJECT_BINDABLE_PROPERTY(CTraffic, double, m_udpInFreq, &CTraffic::udpInFreqChanged)

};

Q_DECLARE_METATYPE(CTraffic*)

#endif // CTRAFFIC_H
