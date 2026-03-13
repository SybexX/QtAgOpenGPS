#ifndef STEERCONFIG_H
#define STEERCONFIG_H

#include <QObject>
#include <QBindable>
#include <QQmlEngine>
#include <QMutex>
#include <QTimer>
#include <QElapsedTimer>

#include "simpleproperty.h"
#include "vec3.h"


class SteerConfig : public QObject
{
    Q_OBJECT
    QML_SINGLETON
    QML_ELEMENT
private:
    explicit SteerConfig(QObject *parent = nullptr);
    ~SteerConfig() override=default;

    //prevent copying
    SteerConfig(const SteerConfig &) = delete;
    SteerConfig &operator=(const SteerConfig &) = delete;

    static SteerConfig *s_instance;
    static QMutex s_mutex;
    static bool s_cpp_created;

public:
    static SteerConfig *instance();
    static SteerConfig *create (QQmlEngine *qmlEngine, QJSEngine *jsEngine);

    SIMPLE_BINDABLE_PROPERTY(bool, isSA)
    SIMPLE_BINDABLE_PROPERTY(double, calcSteerAngleInner)
    SIMPLE_BINDABLE_PROPERTY(double, calcSteerAngleOuter)
    SIMPLE_BINDABLE_PROPERTY(double, diameter)
    SIMPLE_BINDABLE_PROPERTY(bool, haveSteerAngle)
    SIMPLE_BINDABLE_PROPERTY(bool, hasValidRecommendation)
    SIMPLE_BINDABLE_PROPERTY(int, sampleCount)
    SIMPLE_BINDABLE_PROPERTY(double, confidenceLevel)

    Q_INVOKABLE void startSA();
    Q_INVOKABLE void stopSA();

     // Методы для работы с данными
    Q_INVOKABLE int getRecommendedWASOffsetAdjustment(int currentCPD);
    Q_INVOKABLE void startDataCollection();
    Q_INVOKABLE void stopDataCollection();
    Q_INVOKABLE void resetData();
    Q_INVOKABLE void applyOffsetToCollectedData(double appliedOffsetDegrees);
    Q_INVOKABLE void smartCalLabelClick();
    Q_INVOKABLE void zeroWAS();
    void AddSteerAngleSample(double guidanceSteerAngle, double currentSpeed);

    bool isCollectingData = false;

public slots:
    void on_timer();

signals:

private:
    QTimer timer;
    QElapsedTimer elapsed;

    Vec3 startFix;
    double dist = 0.0;
    int counter = 0, secondCntr = 0, cntr = 0;  // Phase 6.0.24 Problem 18

    // Публичные свойства was wizard
    double recommendedWASZero = 0;
    QDateTime lastCollectionTime;

    // Средние показатели распределения
    double mean;
    double standardDeviation;
    double median;

    // Вектор для хранения истории углов
    QVector<double> steerAngleHistory;

    // Критерии проверки собираемых данных
    static constexpr int MAX_SAMPLES = 2000;          // Максимальное число хранимых образцов
    static constexpr int MIN_SAMPLES_FOR_ANALYSIS = 200; // Минимальное число образцов для анализа
    static constexpr double MIN_SPEED_THRESHOLD = 2.0;  // км/ч — минимальная скорость для начала записи
    static constexpr double MAX_ANGLE_THRESHOLD = 25.0; // Градусы — максимальный угол для включения записи

    // Личные вспомогательные методы
    bool ShouldCollectSample(double steerAngle, double speed);
    void PerformStatisticalAnalysis();
    double CalculateMedian(QVector<double> sortedData);
    double CalculateStandardDeviation(QVector<double> data, double mean);
    void CalculateConfidenceLevel(QVector<double> sortedData);

    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(SteerConfig, bool, m_isSA, false, &SteerConfig::isSAChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(SteerConfig, double, m_calcSteerAngleInner, 0, &SteerConfig::calcSteerAngleInnerChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(SteerConfig, double, m_calcSteerAngleOuter, 0, &SteerConfig::calcSteerAngleOuterChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(SteerConfig, double, m_diameter, 0, &SteerConfig::diameterChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(SteerConfig, bool, m_haveSteerAngle, false, &SteerConfig::haveSteerAngleChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(SteerConfig, bool, m_hasValidRecommendation, false, &SteerConfig::hasValidRecommendationChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(SteerConfig, int, m_sampleCount, false, &SteerConfig::sampleCountChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(SteerConfig, double, m_confidenceLevel, 0, &SteerConfig::confidenceLevelChanged)
};

#endif // STEERCONFIG_H
