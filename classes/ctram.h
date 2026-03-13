#ifndef CTRAM_H
#define CTRAM_H

#include <QVector>
#include <QMatrix4x4>
#include <QObject>
#include <QProperty>
#include <QBindable>
#include <QtQml/qqmlregistration.h>
#include "vec2.h"
#include "setter.h"

class CBoundary;
class QOpenGLFunctions;
class CCamera;

/**
 * CTram - Qt 6.8 Rectangle Pattern class for tramline control
 *
 * Phase 6.0.20 - Migration to Q_OBJECT_BINDABLE_PROPERTY for automatic change tracking.
 * Provides modern Qt 6.8 property bindings for tramline manual controls.
 */
class CTram : public QObject
{
    Q_OBJECT
    // QML registration handled manually in main.cpp

    // ===== QML PROPERTIES - Qt 6.8 Rectangle Pattern =====
    Q_PROPERTY(bool isLeftManualOn READ isLeftManualOn WRITE setIsLeftManualOn NOTIFY isLeftManualOnChanged BINDABLE bindableIsLeftManualOn)
    Q_PROPERTY(bool isRightManualOn READ isRightManualOn WRITE setIsRightManualOn NOTIFY isRightManualOnChanged BINDABLE bindableIsRightManualOn)



public:

    //the list of constants and multiples of the boundary
    QVector<Vec2> calcList;

    //the triangle strip of the outer tram highlight
    QVector<Vec2> tramBndOuterArr;

    QVector<Vec2> tramBndInnerArr;

    //tram settings
    double tramWidth;
    double halfWheelTrack;
    int passes;

    bool isOuter;


    //tramlines
    QSharedPointer<QVector<Vec2>> tramArr;
    QVector<QSharedPointer<QVector<Vec2>>> tramList;

    // 0 off, 1 All, 2, Lines, 3 Outer
    int displayMode, generateMode = 0;

    int controlByte;

    explicit CTram(QObject *parent = 0);
    void loadSettings();
    void IsTramOuterOrInner();
    void DrawTram(QOpenGLFunctions *gl, const QMatrix4x4 &mvp, double camSetDistance);
    void BuildTramBnd(const CBoundary &bnd);
    void CreateBndInnerTramTrack(const CBoundary &bnd);
    void CreateBndOuterTramTrack(const CBoundary &bnd);

    // ===== Qt 6.8 Rectangle Pattern READ/WRITE/BINDABLE Methods =====
    bool isLeftManualOn() const;
    void setIsLeftManualOn(bool value);
    QBindable<bool> bindableIsLeftManualOn();

    bool isRightManualOn() const;
    void setIsRightManualOn(bool value);
    QBindable<bool> bindableIsRightManualOn();

signals:
    // Qt 6.8 Rectangle Pattern NOTIFY signals
    void isLeftManualOnChanged();
    void isRightManualOnChanged();

private:
    // ===== Qt 6.8 Q_OBJECT_BINDABLE_PROPERTY Private Members =====
    Q_OBJECT_BINDABLE_PROPERTY(CTram, bool, m_isLeftManualOn, &CTram::isLeftManualOnChanged)
    Q_OBJECT_BINDABLE_PROPERTY(CTram, bool, m_isRightManualOn, &CTram::isRightManualOnChanged)
};

#endif // CTRAM_H
