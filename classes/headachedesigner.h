#ifndef HEADACHEDESIGNER_H
#define HEADACHEDESIGNER_H

#include <QObject>
#include <QPoint>
#include <QProperty>
#include <QBindable>
#include <QtQml/qqmlregistration.h>

/**
 * HeadacheDesigner - Qt 6.8 Rectangle Pattern class for headache design properties
 *
 * Phase 6.0.20 - Migration to Q_OBJECT_BINDABLE_PROPERTY for automatic change tracking.
 * Provides modern Qt 6.8 property bindings for headache design interface.
 */
class HeadacheDesigner : public QObject
{
    Q_OBJECT
    // QML registration handled manually in main.cpp

    // ===== QML PROPERTIES - Qt 6.8 Rectangle Pattern =====
    Q_PROPERTY(double zoom READ zoom WRITE setZoom NOTIFY zoomChanged BINDABLE bindableZoom)
    Q_PROPERTY(double sX READ sX WRITE setSX NOTIFY sXChanged BINDABLE bindableSX)
    Q_PROPERTY(double sY READ sY WRITE setSY NOTIFY sYChanged BINDABLE bindableSY)
    Q_PROPERTY(double lineDistance READ lineDistance WRITE setLineDistance NOTIFY lineDistanceChanged BINDABLE bindableLineDistance)
    Q_PROPERTY(bool curveLine READ curveLine WRITE setCurveLine NOTIFY curveLineChanged BINDABLE bindableCurveLine)

public:
    explicit HeadacheDesigner(QObject *parent = nullptr);

    // ===== Qt 6.8 Rectangle Pattern READ/WRITE/BINDABLE Methods =====
    double zoom() const;
    void setZoom(double value);
    QBindable<double> bindableZoom();

    double sX() const;
    void setSX(double value);
    QBindable<double> bindableSX();

    double sY() const;
    void setSY(double value);
    QBindable<double> bindableSY();

    double lineDistance() const;
    void setLineDistance(double value);
    QBindable<double> bindableLineDistance();

    bool curveLine() const;
    void setCurveLine(bool value);
    QBindable<bool> bindableCurveLine();

signals:
    // Qt 6.8 Rectangle Pattern NOTIFY signals
    void zoomChanged();
    void sXChanged();
    void sYChanged();
    void lineDistanceChanged();
    void curveLineChanged();

    // Optional grouped notifications
    void viewChanged();
    void designChanged();

private:
    // ===== Qt 6.8 Q_OBJECT_BINDABLE_PROPERTY Private Members =====
    Q_OBJECT_BINDABLE_PROPERTY(HeadacheDesigner, double, m_zoom, &HeadacheDesigner::zoomChanged)
    Q_OBJECT_BINDABLE_PROPERTY(HeadacheDesigner, double, m_sX, &HeadacheDesigner::sXChanged)
    Q_OBJECT_BINDABLE_PROPERTY(HeadacheDesigner, double, m_sY, &HeadacheDesigner::sYChanged)
    Q_OBJECT_BINDABLE_PROPERTY(HeadacheDesigner, double, m_lineDistance, &HeadacheDesigner::lineDistanceChanged)
    Q_OBJECT_BINDABLE_PROPERTY(HeadacheDesigner, bool, m_curveLine, &HeadacheDesigner::curveLineChanged)
};

#endif // HEADACHEDESIGNER_H