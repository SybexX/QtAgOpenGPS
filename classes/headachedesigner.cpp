#include "headachedesigner.h"

HeadacheDesigner::HeadacheDesigner(QObject *parent)
    : QObject(parent)
{
    // Initialize Qt 6.8 Q_OBJECT_BINDABLE_PROPERTY members
    m_zoom = 1.0;
    m_sX = 0.0;
    m_sY = 0.0;
    m_lineDistance = 0.0;
    m_curveLine = false;
}

// ===== Qt 6.8 Rectangle Pattern Implementation =====
double HeadacheDesigner::zoom() const {
    return m_zoom;
}

void HeadacheDesigner::setZoom(double value) {
    m_zoom = value;
}

QBindable<double> HeadacheDesigner::bindableZoom() {
    return QBindable<double>(&m_zoom);
}

double HeadacheDesigner::sX() const {
    return m_sX;
}

void HeadacheDesigner::setSX(double value) {
    m_sX = value;
}

QBindable<double> HeadacheDesigner::bindableSX() {
    return QBindable<double>(&m_sX);
}

double HeadacheDesigner::sY() const {
    return m_sY;
}

void HeadacheDesigner::setSY(double value) {
    m_sY = value;
}

QBindable<double> HeadacheDesigner::bindableSY() {
    return QBindable<double>(&m_sY);
}

double HeadacheDesigner::lineDistance() const {
    return m_lineDistance;
}

void HeadacheDesigner::setLineDistance(double value) {
    m_lineDistance = value;
}

QBindable<double> HeadacheDesigner::bindableLineDistance() {
    return QBindable<double>(&m_lineDistance);
}

bool HeadacheDesigner::curveLine() const {
    return m_curveLine;
}

void HeadacheDesigner::setCurveLine(bool value) {
    m_curveLine = value;
}

QBindable<bool> HeadacheDesigner::bindableCurveLine() {
    return QBindable<bool>(&m_curveLine);
}