#ifndef RateControl_H
#define RateControl_H

#include <QObject>
#include <QVariant>
#include <QVariantList>
#include <QTimer>
#include <QElapsedTimer>
#include "settingsmanager.h"
#include "pgnparser.h"
#include "rcmodel.h"
#include "simpleproperty.h"

class RateControl : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    QMutex mutex;

protected:
    explicit RateControl(QObject *parent = nullptr);
    ~RateControl() override=default;

    //prevent copying
    RateControl(const RateControl &) = delete;
    RateControl &operator=(const RateControl &) = delete;

    static RateControl *s_instance;
    static QMutex s_mutex;
    static bool s_cpp_created;

public:
    static RateControl *instance();
    static RateControl *create (QQmlEngine *qmlEngine, QJSEngine *jsEngine);
    Q_PROPERTY(RCModel *rcModel READ rcModel CONSTANT)
    RCModel *rcModel() const { return m_rcModel; }
    RCModel *m_rcModel;

    // Заменяем currentProductId на currentProductIndex
    Q_PROPERTY(int currentProductIndex READ currentProductIndex WRITE setCurrentProductIndex NOTIFY currentProductIndexChanged)

    Q_INVOKABLE void rate_bump(bool up, int ID);
    Q_INVOKABLE void setCurrentProductIndex(int index);
    Q_INVOKABLE QVariantMap getProductDataByIndex(int index) const;
    Q_INVOKABLE void increaseSetRate(int index, double step = 10);
    Q_INVOKABLE void decreaseSetRate(int index, double step = 10);
    Q_INVOKABLE void refreshProduct(int index);
    Q_INVOKABLE void updateAllProducts();
    Q_INVOKABLE  void rate_pwm_auto(int ID);

    int currentProductIndex() const { return m_currentProductIndex; }

    // Публичные поля для доступа извне
    int ModID;
    int ManualPWM[4];
    double cUPM;
    double Quantity[4];
    double MeterCal[4];
    double RateSet;
    double actualRate;
    int ControlType[4];
    double TargetUPM(int ID);
    double RateApplied(int ID);
    double SmoothRate(int ID);
    double CurrentRate(int ID);
    int Command(int ID);
    double MinUPMSpeed(int ID);
    double MinUPM(int ID);
    void loadSettings(int ID);
    void modulesSend241(int ID);
    void updateModel(int ID);
    int PWMsetting[4];
    bool SensorReceiving[4];
    bool aBtnState;
    bool mBtnState;
    double cRateApplied[4];
    double cSmoothRate[4];
    double cCurrentRate[4];
    double cTargetUPM[4];
    double cMinUPMSpeed[4];
    double cMinUPM[4];
    double speed; //For now filled in by formGPS.
    double width; // tool width section control
    double swidth; // tool width fot constant upm

private:
    // Вспомогательные методы
    void initializeProducts();
    QString getProductNameFromSettings(int index);

    bool ProductOn(int ID);
    int OnScreen[4];
    int kp;
    int ki;
    int kd;
    int minpwm;
    int maxpwm;
    int pidscale[4];
    int rateSensor;
    double TargetRate[4] {100,200,200,240};
    double HectaresPerMinute;
    int CoverageUnits[4];
    int AppMode[4];
    double appRate[4];
    double minSpeed[4];
    double minUPM[4];
    double ProdDensity[4];
    bool cEnableProdDensity = false;
    int m_currentProductIndex = 0; // Заменяем на индекс

signals:
    void currentProductIndexChanged(int index);

public slots:
    void onRateControlDataReady(const PGNParser::ParsedData& data);
};

#endif // RateControl_H
