#ifndef MODULECOMM_H
#define MODULECOMM_H

#include <QObject>
#include <QQmlEngine>
#include <QMutex>
#include <QPropertyBinding>
#include <QtCore>
#include <QString>
#include "cpgn.h"
#include "simpleproperty.h"

class CAHRS;

class ModuleComm: public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(ModuleComm)
    QML_SINGLETON

    Q_PROPERTY(CPGN_FE p_254 MEMBER p_254 NOTIFY p_254_changed)
    Q_PROPERTY(CPGN_FC p_252 MEMBER p_252 NOTIFY p_252_changed)
    Q_PROPERTY(CPGN_FB p_251 MEMBER p_251 NOTIFY p_251_changed)
    Q_PROPERTY(CPGN_EF p_239 MEMBER p_239 NOTIFY p_239_changed)
    Q_PROPERTY(CPGN_EE p_238 MEMBER p_238 NOTIFY p_238_changed)
    Q_PROPERTY(CPGN_EC p_236 MEMBER p_236 NOTIFY p_236_changed)
    Q_PROPERTY(CPGN_EB p_235 MEMBER p_235 NOTIFY p_235_changed)
    Q_PROPERTY(CPGN_E5 p_229 MEMBER p_229 NOTIFY p_229_changed)
    Q_PROPERTY(CPGN_F5 p_245 MEMBER p_245 NOTIFY p_245_changed)
    Q_PROPERTY(CPGN_F1 p_241 MEMBER p_241 NOTIFY p_241_changed)
    Q_PROPERTY(CPGN_F2 p_242 MEMBER p_242 NOTIFY p_242_changed)

private:
    explicit ModuleComm(QObject *parent = nullptr);
    ~ModuleComm() override = default;

    // Prevent copying
    ModuleComm(const ModuleComm &) = delete;
    ModuleComm &operator=(const ModuleComm &) = delete;

    static ModuleComm *s_instance;
    static QMutex s_mutex;
    static bool s_cpp_created;

public:
    static ModuleComm *instance();
    static ModuleComm *create(QQmlEngine *qmlEngine, QJSEngine *jsEngine);
    // ---- Section control switches to AOG  ---------------------------------------------------------
    //PGN - 32736 - 127.249 0x7FF9
    uchar ss[9];

    uchar ssP[9];

    enum ss_bytes {
        swHeader = 0,
        swMain = 1,
        swReserve = 2,
        swReserve2 = 3,
        swNumSections = 4,
        swOnGr0 = 5,
        swOffGr0 = 6,
        swOnGr1 = 7,
        swOffGr1 = 8
    };


    //for the workswitch
    bool isWorkSwitchEnabled,
        isWorkSwitchManualSections, isSteerWorkSwitchManualSections, isSteerWorkSwitchEnabled;

    bool oldWorkSwitchHigh, oldSteerSwitchHigh, oldSteerSwitchRemote;

    void CheckWorkAndSteerSwitch(bool isBtnAutoSteerOn);


    Q_INVOKABLE void modulesSend238();
    Q_INVOKABLE void modulesSend251();
    Q_INVOKABLE void modulesSend252();
    Q_INVOKABLE void modulesSend242();
    Q_INVOKABLE void modulesSend245();
    Q_INVOKABLE void setHydLiftPGN(int);

    /*
     * PGNs *
     */
    CPGN_FE p_254;
    CPGN_FC p_252;
    CPGN_FB p_251;
    CPGN_EF p_239;
    CPGN_EE p_238;
    CPGN_EC p_236;
    CPGN_EB p_235;
    CPGN_E5 p_229;
    CPGN_F5 p_245;
    CPGN_F1 p_241;
    CPGN_F2 p_242;

    SIMPLE_BINDABLE_PROPERTY(double,  actualSteerAngleDegrees)
    SIMPLE_BINDABLE_PROPERTY(bool, steerSwitchHigh)
    SIMPLE_BINDABLE_PROPERTY(bool, workSwitchHigh)
    SIMPLE_BINDABLE_PROPERTY(int, sensorData)
    SIMPLE_BINDABLE_PROPERTY(int, pwmDisplay)
    SIMPLE_BINDABLE_PROPERTY(int, steerModuleConnectedCounter)

signals:
    void p_254_changed();
    void p_252_changed();
    void p_251_changed();
    void p_239_changed();
    void p_238_changed();
    void p_236_changed();
    void p_235_changed();
    void p_229_changed();
    void p_241_changed();
    void p_242_changed();
    void p_245_changed();

public slots:
private:
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(ModuleComm, double, m_actualSteerAngleDegrees, 0, &ModuleComm::actualSteerAngleDegreesChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(ModuleComm, bool, m_steerSwitchHigh, 0, &ModuleComm::steerSwitchHighChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(ModuleComm, bool, m_workSwitchHigh, 0, &ModuleComm::workSwitchHighChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(ModuleComm, int, m_sensorData, -1, &ModuleComm::sensorDataChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(ModuleComm, int, m_pwmDisplay, 0, &ModuleComm::pwmDisplayChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(ModuleComm, int, m_steerModuleConnectedCounter, 0, &ModuleComm::steerModuleConnectedCounterChanged)
};

#endif // MODULECOMM_H
