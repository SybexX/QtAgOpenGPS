#include "backend.h"
#include <QCoreApplication>
#include <QLoggingCategory>
#include "mainwindowstate.h"
#include "cvehicle.h"
#include "modulecomm.h"
#include "settingsmanager.h"
#include "cyouturn.h"
#include "ctrack.h"

Q_LOGGING_CATEGORY (backend_log, "backend.qtagopengps")

Backend *Backend::s_instance = nullptr;
QMutex Backend::s_mutex;
bool Backend::s_cpp_created = false;

Backend::Backend(QObject *parent)
    : QObject{parent}{
    m_pn = new CNMEA(this);

    m_track = new CTrack(this);
    m_yt = new CYouTurn(this);

    //connect signals through to yt, although this is kind of redundant
    //since qml can access Backend.yt.slot directly
    connect(this, &Backend::manualUTurn, qobject_cast<CYouTurn *>(m_yt), &CYouTurn::manualUTurn);
    connect(this, &Backend::lateral, qobject_cast<CYouTurn *>(m_yt), &CYouTurn::lateral);
    connect(this, &Backend::swapAutoYouTurnDirection, qobject_cast<CYouTurn *>(m_yt), &CYouTurn::swapAutoYouTurnDirection);
    connect(this, &Backend::resetCreatedYouTurn, qobject_cast<CYouTurn *>(m_yt), &CYouTurn::ResetCreatedYouTurn);
    connect(this, &Backend::toggleYouSkip, qobject_cast<CYouTurn *>(m_yt), &CYouTurn::toggleYouSkip);

    //TODO when CTrack is its own singleton, move this to CTrack's constructor
    connect(this, &Backend::contourLock, &(qobject_cast<CTrack *>(m_track)->contour), &CContour::setLockToLine);

    connect(qobject_cast<CTrack *>(m_track), &CTrack::resetCreatedYouTurn, qobject_cast<CYouTurn *>(m_yt), &CYouTurn::ResetCreatedYouTurn, Qt::QueuedConnection);
}

Backend *Backend::instance() {
    QMutexLocker locker(&s_mutex);
    if (!s_instance) {
        s_instance = new Backend();
        qDebug(backend_log) << "Backend singleton created by C++ code.";
        s_cpp_created = true;
        // ensure cleanup on app exit
        QObject::connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit,
                         s_instance, []() {
                             delete s_instance; s_instance = nullptr;
                         });
    }
    return s_instance;
}

Backend *Backend::create(QQmlEngine *qmlEngine, QJSEngine *jsEngine) {
    Q_UNUSED(jsEngine)

    QMutexLocker locker(&s_mutex);

    if(!s_instance) {
        s_instance = new Backend();
        qDebug(backend_log) << "Backend singleton created by QML engine.";
    } else if (s_cpp_created) {
        qmlEngine->setObjectOwnership(s_instance, QQmlEngine::CppOwnership);
    }

    return s_instance;
}
// ===== USER DATA MANAGEMENT IMPLEMENTATIONS =====
// note: not currently used in QML
/*
void Backend::setDistanceUser(const QString& value) {
    bool ok;
    double distance = value.toDouble(&ok);
    if (ok) {
        set_distanceUser(distance);
    }
}

void Backend::setWorkedAreaTotalUser(const QString& value) {
    bool ok;
    double area = value.toDouble(&ok);
    if (ok) {
        set_workedAreaTotalUser(area);
    }
}
*/

void Backend::toggleHeadlandOn() {
    //This can all be done in Javascript. Should it be there or here?

    //toggle the property
    MainWindowState::instance()->set_isHeadlandOn(! MainWindowState::instance()->isHeadlandOn());


    if (CVehicle::instance()->isHydLiftOn() && !MainWindowState::instance()->isHeadlandOn())
        CVehicle::instance()->setIsHydLiftOn(false);

    if (!MainWindowState::instance()->isHeadlandOn())
    {
        //shut off the hyd lift pgn
        ModuleComm::instance()->setHydLiftPGN(0);
    }
}

void Backend::toggleHydLift() {
    if (MainWindowState::instance()->isHeadlandOn())
    {
        CVehicle::instance()->setIsHydLiftOn(!CVehicle::instance()->isHydLiftOn());
        if (CVehicle::instance()->isHydLiftOn())
        {
        }
        else
        {
            ModuleComm::instance()->setHydLiftPGN(0);
        }
    }
    else
    {
        ModuleComm::instance()->setHydLiftPGN(0);
        CVehicle::instance()->setIsHydLiftOn(false);
    }
}

void Backend::toggleContour() {
     MainWindowState::instance()->set_isContourBtnOn(
        ! MainWindowState::instance()->isContourBtnOn());

    if (MainWindowState::instance()->isContourBtnOn()) {
        m_guidanceLookAheadTime = 0.5;
    }else{
        m_guidanceLookAheadTime = SettingsManager::instance()->as_guidanceLookAheadTime();
    }
}
