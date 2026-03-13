#include "steerconfig.h"
#include <QCoreApplication>
#include <QLoggingCategory>
#include "glm.h"
#include "cvehicle.h"
#include "backend.h"
#include "mainwindowstate.h"
#include "settingsmanager.h"

Q_LOGGING_CATEGORY (steerconfig_log, "steerconfig.qtagopengps")
#define QDEBUG qDebug(steerconfig_log)

SteerConfig *SteerConfig::s_instance = nullptr;
QMutex SteerConfig::s_mutex;
bool SteerConfig::s_cpp_created = false;

SteerConfig::SteerConfig(QObject *parent)
    : QObject{parent}
{
    connect(&timer, &QTimer::timeout, this, &SteerConfig::on_timer);
    timer.setSingleShot(false);
    timer.start(250);
}

SteerConfig *SteerConfig::instance() {
    QMutexLocker locker(&s_mutex);
    if (!s_instance) {
        s_instance = new SteerConfig();
        qDebug(steerconfig_log) << "SteerConfig singleton created by C++ code.";
        s_cpp_created = true;
        // ensure cleanup on app exit
        QObject::connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit,
                         s_instance, []() {
                             delete s_instance; s_instance = nullptr;
                         });
    }
    return s_instance;
}

SteerConfig *SteerConfig::create(QQmlEngine *qmlEngine, QJSEngine *jsEngine) {
    Q_UNUSED(jsEngine)

    QMutexLocker locker(&s_mutex);

    if(!s_instance) {
        s_instance = new SteerConfig();
        qDebug(steerconfig_log) << "SteerConfig singleton created by QML engine.";
    } else if (s_cpp_created) {
        qmlEngine->setObjectOwnership(s_instance, QQmlEngine::CppOwnership);
    }

    return s_instance;
}

void SteerConfig::startSA() {
    qDebug(steerconfig_log) << "Starting SA";

    m_isSA = true;
    startFix = CVehicle::instance()->pivotAxlePos;
    dist = 0;
    m_diameter = 0;
    cntr = 0;
    m_haveSteerAngle = false; //probably not needed

    /*
    //lblDiameter.Text = "0";
    setLblCalcSteerAngleInner("Drive Steady");
    // DEAD CODE from C# original - lblCalcSteerAngleOuter never displayed (FormSteer.cs:848 commented)
    lblCalcSteerAngleOuter = "Consistent Steering Angle!!";
    */
}

void SteerConfig::stopSA(){
        m_isSA = false;
}

void SteerConfig::on_timer() {
    if (m_isSA)
    {
        dist = glm::Distance(startFix, CVehicle::instance()->pivotAxlePos);
        cntr++;
        if (dist > m_diameter)
        {
            m_diameter = dist;
            cntr = 0;
        }
        if (cntr > 9)
        {
            double steerAngleRight = atan(CVehicle::instance()->wheelbase / ((m_diameter - CVehicle::instance()->trackWidth * 0.5) / 2));
            m_calcSteerAngleInner = glm::toDegrees(steerAngleRight);

            /*
            //lblCalcSteerAngleInner = steerAngleRight.ToString("N1") + "°";
            setLblCalcSteerAngleInner(locale.toString(steerAngleRight, 'g', 3) + tr("°"));
            //lblDiameter.Text = diameter.ToString("N2") + " m";
            setLblDiameter(locale.toString(_diameter, 'g', 3) + tr(" m"));
            */
            m_isSA = false;
        }
    }
}

// Начало сбора данных
void SteerConfig::startDataCollection()
{
    isCollectingData = true;
    lastCollectionTime = QDateTime::currentDateTime();
    QDEBUG<< "StartDataCollection";
}

// Завершение сбора данных
void SteerConfig::stopDataCollection()
{
    isCollectingData = false;
    QDEBUG<<"StopDataCollection";
}

// Полностью сбрасываем историю и аналитику
void SteerConfig::resetData()
{
    steerAngleHistory.clear();
    m_sampleCount = 0;
    recommendedWASZero = 0;
    m_confidenceLevel = 0;
    m_hasValidRecommendation = false;
    mean = 0;
    standardDeviation = 0;
    median = 0;
}

// Применяем смещение к историческим данным
void SteerConfig::applyOffsetToCollectedData(double appliedOffsetDegrees)
{
    if (steerAngleHistory.empty()) return;

    for (size_t i = 0; i < steerAngleHistory.size(); ++i)
    {
        steerAngleHistory[i] += appliedOffsetDegrees;
    }

    if (m_sampleCount >= MIN_SAMPLES_FOR_ANALYSIS)
    {
        PerformStatisticalAnalysis();
    }

    QDEBUG << "Smart WAS: Applied " << appliedOffsetDegrees << "° offset to "
             << steerAngleHistory.size() << " collected samples.";
}

// Добавляем новую запись угла направления
void SteerConfig::AddSteerAngleSample(double guidanceSteerAngle, double currentSpeed)
{   //QDEBUG<<"AddSteerAngleSample";
    if (!isCollectingData || !ShouldCollectSample(guidanceSteerAngle, currentSpeed))
        return;

    steerAngleHistory.push_back(guidanceSteerAngle);
    lastCollectionTime = QDateTime::currentDateTime();

    if (steerAngleHistory.size() > MAX_SAMPLES)
    {
        steerAngleHistory.pop_front();  // удаляем самый старый элемент
    }

    m_sampleCount = steerAngleHistory.size();

    if (m_sampleCount >= MIN_SAMPLES_FOR_ANALYSIS)
    {
        PerformStatisticalAnalysis();
    }

}

// Возвращаем поправочный коэффициент на основе текущих данных
int SteerConfig::getRecommendedWASOffsetAdjustment(int currentCPD)
{
    if (!m_hasValidRecommendation) return 0;

    return static_cast<int>(std::round(recommendedWASZero * currentCPD));
}

// Проверяем подходит ли данный образец для сбора
bool SteerConfig::ShouldCollectSample(double steerAngle, double speed)
{
    if (speed < MIN_SPEED_THRESHOLD) return false;
    if (std::abs(steerAngle) > MAX_ANGLE_THRESHOLD) return false;
    if (!MainWindowState::instance()->isBtnAutoSteerOn()) return false;
    if (std::abs(CVehicle::instance()->guidanceLineDistanceOff()) > 15000) return false;

    return true;
}

// Основная процедура статистического анализа
void SteerConfig::PerformStatisticalAnalysis()
{
    if (steerAngleHistory.size() < MIN_SAMPLES_FOR_ANALYSIS) return;

    auto sortedData = steerAngleHistory;
    std::sort(sortedData.begin(), sortedData.end()); // сортируем массив

    mean = std::accumulate(steerAngleHistory.begin(), steerAngleHistory.end(), 0.0) /
           steerAngleHistory.size();

    median = CalculateMedian(sortedData);
    standardDeviation = CalculateStandardDeviation(steerAngleHistory, mean);

    recommendedWASZero = -median; // отрицательная коррекция приближает к центру

    CalculateConfidenceLevel(sortedData);

    m_hasValidRecommendation = m_confidenceLevel > 50.0 &&
                             m_sampleCount >= MIN_SAMPLES_FOR_ANALYSIS;
    //QDEBUG<<"m_hasValidRecommendation"<<m_hasValidRecommendation;
}

// Функция для нахождения медианы
double SteerConfig::CalculateMedian(QVector<double> sortedData)
{
    int count = sortedData.size();
    if (count == 0) return 0;

    if (count % 2 == 0)
    {
        return (sortedData[count / 2 - 1] + sortedData[count / 2]) / 2.0;
    }
    else
    {
        return sortedData[count / 2];
    }
}

// Расчет стандартного отклонения
double SteerConfig::CalculateStandardDeviation(QVector<double> data, double mean)
{
    if (data.size() < 2) return 0;

    double sumOfSquares = 0.0;
    for (double d : data)
    {
        sumOfSquares += std::pow(d - mean, 2);
    }

    return std::sqrt(sumOfSquares / (data.size() - 1));
}

// Подсчет коэффициента уверенности
void SteerConfig::CalculateConfidenceLevel(QVector<double> sortedData)
{
    if (sortedData.size() < MIN_SAMPLES_FOR_ANALYSIS)
    {
        m_confidenceLevel = 0;
        return;
    }

    double oneStdDevRange = standardDeviation;
    double twoStdDevRange = 2 * standardDeviation;

    int withinOneStdDev = 0;
    int withinTwoStdDev = 0;

    for (double angle : sortedData)
    {
        double deviationFromMedian = std::abs(angle - median);
        if (deviationFromMedian <= oneStdDevRange) withinOneStdDev++;
        if (deviationFromMedian <= twoStdDevRange) withinTwoStdDev++;
    }

    double oneStdDevPercentage = static_cast<double>(withinOneStdDev) / sortedData.size();
    double twoStdDevPercentage = static_cast<double>(withinTwoStdDev) / sortedData.size();

    // ожидаемое нормальное распределение данных
    double expectedOneStdDev = 0.68;
    double expectedTwoStdDev = 0.95;

    // считаем баллы для каждой метрики
    double oneStdDevScore = std::max(0.0, 1 - std::abs(oneStdDevPercentage - expectedOneStdDev) / expectedOneStdDev);
    double twoStdDevScore = std::max(0.0, 1 - std::abs(twoStdDevPercentage - expectedTwoStdDev) / expectedTwoStdDev);
    double magnitudeScore = std::max(0.0, 1 - std::abs(recommendedWASZero) / 10.0); // штрафуем большие поправки
    double sampleSizeFactor = std::min(1.0, static_cast<double>(sortedData.size()) / (MIN_SAMPLES_FOR_ANALYSIS * 3)); // размер выборки влияет положительно

    // объединяем факторы
    m_confidenceLevel = ((oneStdDevScore * 0.3 + twoStdDevScore * 0.3 + magnitudeScore * 0.2 + sampleSizeFactor * 0.2) * 100);
    m_confidenceLevel = std::clamp(confidenceLevel(), 0.0, 100.0);
}

void SteerConfig::smartCalLabelClick()
{
    // Сброс калибровки Smart WAS при клике на любую статусную метку
    if (isCollectingData)
    {
        resetData();

        // Покажите короткое подтверждение сброса
        Backend::instance()->timedMessage(1500, tr("Reset To Default"), tr("CalibrationDataReset"));
    }
    QDEBUG<<"SmartCalLabelClick";
}

void SteerConfig::zeroWAS()
{
    if (!isCollectingData)
    {   Backend::instance()->timedMessage(2000, "SmartCalibrationErro", "gsSmartWASNotAvailable");
        return;
    }

    if (!m_hasValidRecommendation)
    {
        if (m_sampleCount < 200)
        {
            Backend::instance()->timedMessage(2000, tr("Need at least 200 samples for calibration. Drive on guidance lines to collect more data."), QString(tr("Insufficient Data")) + " " +
                                                                         QString::number(m_sampleCount, 'f', 1));
        }
        else
        {
            Backend::instance()->timedMessage(2000, tr("Calibration confidence is low. Need at least 70% confidence. Drive more consistently on guidance lines."), QString(tr("Low Confidence")) + " " +
                                                                                                                                      QString::number(confidenceLevel(), 'f', 1));
        }
        return;
    }

    // Получаем рекомендацию по смещению
    int recommendedOffsetAdjustment = getRecommendedWASOffsetAdjustment(SettingsManager::instance()->as_countsPerDegree());
    int newOffset = SettingsManager::instance()->as_wasOffset() + recommendedOffsetAdjustment;

    // Проверяем новое значение смещения на допустимый диапазон
    if (std::abs(newOffset) > 3900)
    {
        Backend::instance()->timedMessage(2000, tr("Recommended adjustment {0} exceeds safe range (±50). Please check WAS sensor alignment"), QString(tr("Exceeded Range")) + " " +
                                                                                                                                  QString::number(newOffset, 'f', 1));
        QDEBUG << "Smart Zero превысил диапазон:" << newOffset;
        return;
    }

    // Применяем смещение нуля WAS
    SettingsManager::instance()->setAs_wasOffset(newOffset);

    // Критически важно: применяем смещение к ранее собранным данным
    applyOffsetToCollectedData(recommendedWASZero);

    // Сообщаем об успешной настройке
    Backend::instance()->timedMessage(2000, tr("%1 образцов, %2% уверенности, коррекция %3°")
                                  .arg(sampleCount())
                                                .arg(QString::number(confidenceLevel(), 'f', 1))
                                  .arg(QString::number(recommendedWASZero, 'f', 2)),
    QString(tr("Смещение успешно применено")));


    QDEBUG << "Настройка Smart WAS выполнена -"
             << "Образцы:" << m_sampleCount
             << ", Уверенность:" << QString::number(confidenceLevel(), 'f', 1) << "%,"
             << "Корректировка:" << QString::number(recommendedWASZero, 'f', 2) << "°";
}

