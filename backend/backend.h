#ifndef BACKEND_H
#define BACKEND_H

#include <QObject>
#include <QBindable>
#include <QQmlEngine>
#include <QMutex>

#include "fieldinfo.h"
#include "fixframe.h"
#include "guidance.h"
#include "vec2.h"
#include "simpleproperty.h"
#include "cnmea.h"

class Backend : public QObject
{
    Q_OBJECT
    QML_SINGLETON
    QML_ELEMENT

    Q_PROPERTY(FieldInfo currentField READ currentField NOTIFY currentFieldChanged)
    Q_PROPERTY(FixFrame fixFrame READ fixFrame NOTIFY fixFrameChanged)
    Q_PROPERTY(QObject* aogRenderer MEMBER aogRenderer NOTIFY aogRendererChanged) //only ever written to by QML

    Q_PROPERTY(CNMEA *pn READ pn CONSTANT)

    // Guidance data - exposed as Q_GADGET for QML access
    Q_PROPERTY(Guidance gyd READ gyd)

    //experimental host some of the core backend objects that FormGPS held
    //use QObject * for faster compiling and less dependencies
    Q_PROPERTY(QObject *track READ track CONSTANT)
    Q_PROPERTY(QObject *yt READ yt CONSTANT)

private:
    explicit Backend(QObject *parent = nullptr);
    ~Backend() override=default;

    //prevent copying
    Backend(const Backend &) = delete;
    Backend &operator=(const Backend &) = delete;

    static Backend *s_instance;
    static QMutex s_mutex;
    static bool s_cpp_created;

public:
    //allow direct access from C++
    FieldInfo m_currentField;
    FixFrame m_fixFrame;
    CNMEA *m_pn;
    QObject *m_track;
    QObject *m_yt;
    Guidance m_gyd;

    QObject *aogRenderer = nullptr;

    static Backend *instance();
    static Backend *create (QQmlEngine *qmlEngine, QJSEngine *jsEngine);

    //const getter for QML
    FieldInfo currentField() const { return m_currentField; }
    FixFrame fixFrame() const { return m_fixFrame; }
    CNMEA *pn() const { return m_pn; }
    QObject *track() const { return m_track; }
    QObject *yt() const { return m_yt; }
    Guidance &gyd() { return m_gyd; }

    //mutation methods for currentField
    Q_INVOKABLE void currentField_setDistanceUser(double newdist) {
        m_currentField.distanceUser = newdist;
        emit currentFieldChanged();
    }

    Q_INVOKABLE void currentField_addWorkedAreaTotal(double netarea) {
        m_currentField.workedAreaTotal += netarea;
        emit currentFieldChanged();
    }

    Q_INVOKABLE void currentField_setWorkedAreaTotal(double area) {
        m_currentField.workedAreaTotal = area;
        emit currentFieldChanged();
    }

    Q_INVOKABLE void currentField_addWorkedAreaTotalUser(double netarea) {
        m_currentField.workedAreaTotalUser += netarea;
        emit currentFieldChanged();
    }

    Q_INVOKABLE void currentField_setWorkedAreaTotalUser(double area) {
        m_currentField.workedAreaTotalUser = area;
        emit currentFieldChanged();
    }

    Q_INVOKABLE void currentField_setActualAreaCovered(double area) {
        m_currentField.actualAreaCovered = area;
        emit currentFieldChanged();
    }

    Q_INVOKABLE void toggleHeadlandOn();
    Q_INVOKABLE void toggleHydLift();

    Q_INVOKABLE void toggleContour();

    SIMPLE_BINDABLE_PROPERTY(bool, isJobStarted)
    SIMPLE_BINDABLE_PROPERTY(bool, applicationClosing)
    SIMPLE_BINDABLE_PROPERTY(double, distancePivotToTurnLine)
    SIMPLE_BINDABLE_PROPERTY(bool, isYouTurnRight)
    SIMPLE_BINDABLE_PROPERTY(bool, isYouTurnTriggered)
    SIMPLE_BINDABLE_PROPERTY(double, guidanceLookAheadTime)

    //These don't seem to be used outside of FormGPS. When FormGPS
    //becomes CoreGPS singleton, consider moving them to CoreGPS
    SIMPLE_BINDABLE_PROPERTY(bool, imuCorrected)
    SIMPLE_BINDABLE_PROPERTY(bool, isReverseWithIMU)
    //Consider moving this to Tool
    SIMPLE_BINDABLE_PROPERTY(bool, isPatchesChangingColor)


signals:
    //signals implicitly created by BINDABLE_PROPERTY() macro
    void currentFieldChanged();
    void aogRendererChanged();
    void fixFrameChanged();

    //These are essentially commands coming from QML
    void timedMessage(int timeout, QString s1, QString s2);

    void resetTool();
    void resetDirection();

    //these can be accessed directly via Backend.yt.manualUTurn() etc
    //or we can use them and make a connection through to yt
    void manualUTurn(bool right);
    void lateral(bool right);
    void swapAutoYouTurnDirection();
    void resetCreatedYouTurn();
    void toggleAutoYouTurn();
    void toggleYouSkip();

    void deleteAppliedArea();

    void centerOgl();

    void contourLock();
    void contourPriority(bool isRight);

    void snapToPivot();
    void snapSideways(double distance);

    void zoomIn();
    void zoomOut();
    void tiltDown();
    void tiltUp();
    void view2D();
    void view3D();
    void normal2D();
    void normal3D();

private:

    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(Backend, bool, m_isJobStarted, false, &Backend::isJobStartedChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(Backend, bool, m_applicationClosing, false, &Backend::applicationClosingChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(Backend, double, m_distancePivotToTurnLine, 0, &Backend::distancePivotToTurnLineChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(Backend, bool, m_isYouTurnRight, false, &Backend::isYouTurnRightChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(Backend, bool, m_isYouTurnTriggered, false, &Backend::isYouTurnTriggeredChanged)

    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(Backend, double, m_guidanceLookAheadTime, 2, &Backend::guidanceLookAheadTimeChanged)

    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(Backend, bool, m_imuCorrected, false, &Backend::imuCorrectedChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(Backend, bool, m_isReverseWithIMU, false, &Backend::isReverseWithIMUChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(Backend, bool, m_isPatchesChangingColor, false, &Backend::isPatchesChangingColorChanged)
};

#endif // BACKEND_H
