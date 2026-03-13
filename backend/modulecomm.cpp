#include "modulecomm.h"
#include "settingsmanager.h"
#include "mainwindowstate.h"
#include "agioservice.h"

Q_LOGGING_CATEGORY (cmodulecomm_log, "cmodulecomm.qtagopengps")
#define QDEBUG qDebug(cmodulecomm_log)

ModuleComm *ModuleComm::s_instance = nullptr;
QMutex ModuleComm::s_mutex;
bool ModuleComm::s_cpp_created = false;

ModuleComm::ModuleComm(QObject *parent) : QObject(parent)
{
}

ModuleComm *ModuleComm::instance()
{
    QMutexLocker locker(&s_mutex);
    if (!s_instance) {
        s_instance = new ModuleComm();
        s_cpp_created = true;
        // Ensure cleanup on app exit
        QObject::connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit,
                         s_instance, []() {
                             delete s_instance;
                             s_instance = nullptr;
                         });
    }
    return s_instance;
}

ModuleComm *ModuleComm::create(QQmlEngine *qmlEngine, QJSEngine *jsEngine)
{
    Q_UNUSED(jsEngine)

    QMutexLocker locker(&s_mutex);

    if (!s_instance) {
        s_instance = new ModuleComm();
    } else if (s_cpp_created) {
        qmlEngine->setObjectOwnership(s_instance, QQmlEngine::CppOwnership);
    }

    return s_instance;
}


void ModuleComm::CheckWorkAndSteerSwitch(bool isBtnAutoSteerOn)
{
    //This really doesn't want to be in this class.  It's only used in FormGPS

    //AutoSteerAuto button enable - Ray Bear inspired code - Thx Ray!
    if (SettingsManager::instance()->f_isRemoteWorkSystemOn() && m_steerSwitchHigh != oldSteerSwitchRemote)
    {
        oldSteerSwitchRemote = m_steerSwitchHigh;
        //steerSwith is active low
        set_steerSwitchHigh(isBtnAutoSteerOn);
        if (m_steerSwitchHigh)
            MainWindowState::instance()->set_isBtnAutoSteerOn(false);
    }

    if (SettingsManager::instance()->f_isRemoteWorkSystemOn())
    {
        if (isWorkSwitchEnabled && (oldWorkSwitchHigh != workSwitchHigh()))
        {
            oldWorkSwitchHigh = workSwitchHigh();

            if (workSwitchHigh() != SettingsManager::instance()->f_isWorkSwitchActiveLow())
            {
                if (isWorkSwitchManualSections)
                {
                    MainWindowState::instance()->set_manualBtnState(SectionState::Off);
                }
                else
                {
                    MainWindowState::instance()->set_autoBtnState(SectionState::Off);
                }
            }

            else//Checks both on-screen buttons, performs click if button is not off
            {
                MainWindowState::instance()->set_autoBtnState(SectionState::Off);
                MainWindowState::instance()->set_manualBtnState(SectionState::Off);
            }
        }

        if (isSteerWorkSwitchEnabled && (oldSteerSwitchHigh != m_steerSwitchHigh))
        {
            oldSteerSwitchHigh = m_steerSwitchHigh;

            if ((isBtnAutoSteerOn && SettingsManager::instance()->as_isAutoSteerAutoOn())
                || (!SettingsManager::instance()->as_isAutoSteerAutoOn() && !m_steerSwitchHigh))
            {
                if (isSteerWorkSwitchManualSections)
                {
                    MainWindowState::instance()->set_manualBtnState(SectionState::Off);
                }
                else
                {
                    MainWindowState::instance()->set_autoBtnState(SectionState::Off);
                }
            }

            else//Checks both on-screen buttons, performs click if button is not off
            {
                MainWindowState::instance()->set_autoBtnState(SectionState::Off);
                MainWindowState::instance()->set_manualBtnState(SectionState::Off);
            }
        }
    }
}

void ModuleComm::modulesSend238() {
    QDEBUG << "Sending 238 message to AgIO";
    p_238.pgn[p_238.set0] = SettingsManager::instance()->ardMac_setting0();
    p_238.pgn[p_238.raiseTime] = SettingsManager::instance()->ardMac_hydRaiseTime();
    p_238.pgn[p_238.lowerTime] = SettingsManager::instance()->ardMac_hydLowerTime();

    p_238.pgn[p_238.user1] = SettingsManager::instance()->ardMac_user1();
    p_238.pgn[p_238.user2] = SettingsManager::instance()->ardMac_user2();
    p_238.pgn[p_238.user3] = SettingsManager::instance()->ardMac_user3();
    p_238.pgn[p_238.user4] = SettingsManager::instance()->ardMac_user4();

    QDEBUG << SettingsManager::instance()->ardMac_user1();

    AgIOService::instance()->sendPgn(p_238.pgn);
}

void ModuleComm::modulesSend251() {
    //QDEBUG << "Sending 251 message to AgIO";
    p_251.pgn[p_251.set0] = SettingsManager::instance()->ardSteer_setting0();
    p_251.pgn[p_251.set1] = SettingsManager::instance()->ardSteer_setting1();
    p_251.pgn[p_251.maxPulse] = SettingsManager::instance()->ardSteer_maxPulseCounts();
    p_251.pgn[p_251.minSpeed] = 5; //0.5 kmh THIS IS CHANGED IN AOG FIXES

    if (SettingsManager::instance()->as_isConstantContourOn())
        p_251.pgn[p_251.angVel] = 1;
    else p_251.pgn[p_251.angVel] = 0;

    QDEBUG << p_251.pgn;

    AgIOService::instance()->sendPgn(p_251.pgn);
}

void ModuleComm::modulesSend252() {
    //QDEBUG << "Sending 252 message to AgIO";
    p_252.pgn[p_252.gainProportional] = SettingsManager::instance()->as_Kp();
    p_252.pgn[p_252.highPWM] = SettingsManager::instance()->as_highSteerPWM();
    p_252.pgn[p_252.lowPWM] = SettingsManager::instance()->as_lowSteerPWM();
    p_252.pgn[p_252.minPWM] = SettingsManager::instance()->as_minSteerPWM();
    p_252.pgn[p_252.countsPerDegree] = SettingsManager::instance()->as_countsPerDegree();
    int wasOffset = (int)SettingsManager::instance()->as_wasOffset();
    p_252.pgn[p_252.wasOffsetHi] = (char)(wasOffset >> 8);
    p_252.pgn[p_252.wasOffsetLo] = (char)wasOffset;
    p_252.pgn[p_252.ackerman] = SettingsManager::instance()->as_ackerman();


    QDEBUG << p_252.pgn;

    AgIOService::instance()->sendPgn(p_252.pgn);
}

void ModuleComm::modulesSend245() {
       //qDebug() << "Sending 245 message to AgIO";

    // Получаем все значения строк
    // get all the row values
    int row1 = SettingsManager::instance()->seed_blockRow1();
    int row2 = SettingsManager::instance()->seed_blockRow2();
    int row3 = SettingsManager::instance()->seed_blockRow3();
    int row4 = SettingsManager::instance()->seed_blockRow4();
    uint16_t min_count = SettingsManager::instance()->seed_blockCountMin();
    uint16_t max_count = SettingsManager::instance()->seed_blockCountMax();

    // Массив значений для удобства обработки
    int rows[] = {row1, row2, row3, row4};

    // Отправляем данные для каждого модуля
    // send the data for each module
    for (int module_index = 0; module_index < 4; module_index++) {
        // Получаем значения для текущего модуля

        p_245.pgn[p_245.module_id] = module_index;
        p_245.pgn[p_245.module_rows] = rows[module_index];
        p_245.pgn[p_245.min_countLO] = min_count & 0xFF;
        p_245.pgn[p_245.min_countHI] = (min_count >> 8) & 0xFF;
        p_245.pgn[p_245.max_countLO] = max_count & 0xFF;
        p_245.pgn[p_245.max_countHI] = (max_count >> 8) & 0xFF;

        //qDebug() << "Module " << module_index << ": " << p_245.pgn;

        AgIOService::instance()->sendPgn(p_245.pgn);
    }
}

void ModuleComm::modulesSend242() {
    //qDebug() << "Sending 242 message to AgIO";
    // PID from RC to module

    // Получаем векторы настроек
    QVector<QVector<int>> module_settings;
    module_settings.append(SettingsManager::instance()->rate_confProduct0());
    module_settings.append(SettingsManager::instance()->rate_confProduct1());
    module_settings.append(SettingsManager::instance()->rate_confProduct2());
    module_settings.append(SettingsManager::instance()->rate_confProduct3());

    for (int module_index = 0; module_index < 4; module_index++) {
        const QVector<int>& module_data = module_settings[module_index];

        if (module_data.size() < 16) {
            qWarning() << "Module" << module_index << "data vector too small:" << module_data.size();
            continue;
        }

        p_242.pgn[p_242.ID] = module_data[0];
        p_242.pgn[p_242.KP] = module_data[3];
        p_242.pgn[p_242.KI] = module_data[4];
        p_242.pgn[p_242.KD] = module_data[5];
        p_242.pgn[p_242.MinPWM] = module_data[6];
        p_242.pgn[p_242.MaxPWM] = module_data[7];
        p_242.pgn[p_242.PIDScale] = module_data[8];

        //qDebug() << "RC Module " << module_index << ": " << p_242.pgn;

        AgIOService::instance()->sendPgn(p_242.pgn);
    }
}

void ModuleComm::setHydLiftPGN(int value) {
    p_239.pgn[CPGN_EF::hydLift] = value;
    emit p_239_changed();
}
