// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
//
// CTraffic - Traffic Monitoring Implementation
// Extracted from udpworker.cpp in Phase 6.0.24

#include "ctraffic.h"

// Setters - Moved from inline to external to fix LNK2019 MOC linker errors

void CTraffic::setHelloFromMachine(quint32 value) {
    m_helloFromMachine = value;
}

void CTraffic::setHelloFromBlockage(quint32 value) {
    m_helloFromBlockage = value;
}

void CTraffic::setHelloFromRateControl(quint32 value) {
    m_helloFromRateControl = value;
}

void CTraffic::setHelloFromAutoSteer(quint32 value) {
    m_helloFromAutoSteer = value;
}

void CTraffic::setHelloFromIMU(quint32 value) {
    m_helloFromIMU = value;
}

void CTraffic::setCntrUDPOut(quint32 value) {
    m_cntrUDPOut = value;
}

void CTraffic::setCntrUDPIn(quint32 value) {
    m_cntrUDPIn = value;
}

void CTraffic::setCntrUDPInBytes(quint32 value) {
    m_cntrUDPInBytes = value;
}

void CTraffic::setUdpOutRate(double value) {
    m_udpOutRate = value;
}

void CTraffic::setUdpInRate(double value) {
    m_udpInRate = value;
}

void CTraffic::setUdpOutFreq(double value) {
    m_udpOutFreq = value;
}

void CTraffic::setUdpInFreq(double value) {
    m_udpInFreq = value;
}

// Getters - Qt 6.8 Rectangle Pattern

quint32 CTraffic::helloFromMachine() const {
    return m_helloFromMachine;
}

quint32 CTraffic::helloFromBlockage() const {
    return m_helloFromBlockage;
}

quint32 CTraffic::helloFromRateControl() const {
    return m_helloFromRateControl;
}

quint32 CTraffic::helloFromAutoSteer() const {
    return m_helloFromAutoSteer;
}

quint32 CTraffic::helloFromIMU() const {
    return m_helloFromIMU;
}

quint32 CTraffic::cntrUDPOut() const {
    return m_cntrUDPOut;
}

quint32 CTraffic::cntrUDPIn() const {
    return m_cntrUDPIn;
}

quint32 CTraffic::cntrUDPInBytes() const {
    return m_cntrUDPInBytes;
}

double CTraffic::udpOutRate() const {
    return m_udpOutRate;
}

double CTraffic::udpInRate() const {
    return m_udpInRate;
}

double CTraffic::udpOutFreq() const {
    return m_udpOutFreq;
}

double CTraffic::udpInFreq() const {
    return m_udpInFreq;
}

// Bindables - Qt 6.8 Rectangle Pattern

QBindable<quint32> CTraffic::bindableHelloFromMachine() {
    return QBindable<quint32>(&m_helloFromMachine);
}

QBindable<quint32> CTraffic::bindableHelloFromBlockage() {
    return QBindable<quint32>(&m_helloFromBlockage);
}

QBindable<quint32> CTraffic::bindableHelloFromRateControl() {
    return QBindable<quint32>(&m_helloFromRateControl);
}

QBindable<quint32> CTraffic::bindableHelloFromAutoSteer() {
    return QBindable<quint32>(&m_helloFromAutoSteer);
}

QBindable<quint32> CTraffic::bindableHelloFromIMU() {
    return QBindable<quint32>(&m_helloFromIMU);
}

QBindable<quint32> CTraffic::bindableCntrUDPOut() {
    return QBindable<quint32>(&m_cntrUDPOut);
}

QBindable<quint32> CTraffic::bindableCntrUDPIn() {
    return QBindable<quint32>(&m_cntrUDPIn);
}

QBindable<quint32> CTraffic::bindableCntrUDPInBytes() {
    return QBindable<quint32>(&m_cntrUDPInBytes);
}

QBindable<double> CTraffic::bindableUdpOutRate() {
    return QBindable<double>(&m_udpOutRate);
}

QBindable<double> CTraffic::bindableUdpInRate() {
    return QBindable<double>(&m_udpInRate);
}

QBindable<double> CTraffic::bindableUdpOutFreq() {
    return QBindable<double>(&m_udpOutFreq);
}

QBindable<double> CTraffic::bindableUdpInFreq() {
    return QBindable<double>(&m_udpInFreq);
}
