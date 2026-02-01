#ifndef MAINWINDOWSTATE_H
#define MAINWINDOWSTATE_H

#include <QObject>
#include <QPropertyBinding>
#include <QQmlEngine>
#include <QMutex>
#include "simpleproperty.h"
#include "sectionstate.h"

//convenience macro
#define MAINWINDOW MainWindowState::instance()

class MainWindowState : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(MainWindowState)
    QML_SINGLETON
private:
    explicit MainWindowState(QObject *parent = nullptr);
    ~MainWindowState() override=default;

    //prevent copying
    MainWindowState(const MainWindowState &) = delete;
    MainWindowState &operator=(const MainWindowState &) = delete;

    static MainWindowState *s_instance;
    static QMutex s_mutex;
    static bool s_cpp_created;

public:
    static MainWindowState *instance();
    static MainWindowState *create (QQmlEngine *qmlEngine, QJSEngine *jsEngine);

    SIMPLE_BINDABLE_PROPERTY(bool, isBtnAutoSteerOn)
    SIMPLE_BINDABLE_PROPERTY(bool, isHeadlandOn)
    SIMPLE_BINDABLE_PROPERTY(bool, isContourBtnOn)
    SIMPLE_BINDABLE_PROPERTY(bool, isYouTurnBtnOn)
    SIMPLE_BINDABLE_PROPERTY(bool, btnIsContourLocked)
    SIMPLE_BINDABLE_PROPERTY(SectionState::State, autoBtnState)
    SIMPLE_BINDABLE_PROPERTY(SectionState::State, manualBtnState)

    //should move this into CSim
    //SIMPLE_BINDABLE_PROPERTY(double, simSteerAngle)

private:
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(MainWindowState, bool, m_isBtnAutoSteerOn, false, &MainWindowState::isBtnAutoSteerOnChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(MainWindowState, bool, m_isHeadlandOn, false, &MainWindowState::isHeadlandOnChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(MainWindowState, bool, m_isContourBtnOn, false, &MainWindowState::isContourBtnOnChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(MainWindowState, bool, m_isYouTurnBtnOn, false, &MainWindowState::isYouTurnBtnOnChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(MainWindowState, bool, m_btnIsContourLocked, false, &MainWindowState::btnIsContourLockedChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(MainWindowState, SectionState::State, m_autoBtnState, SectionState::Off, &MainWindowState::autoBtnStateChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(MainWindowState, SectionState::State, m_manualBtnState, SectionState::Off, &MainWindowState::manualBtnStateChanged)

    //Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(MainWindowState, double, m_simSteerAngle, 0, &MainWindowState::simSteerAngleChanged)

signals:

};

#endif // MAINWINDOWSTATE_H
