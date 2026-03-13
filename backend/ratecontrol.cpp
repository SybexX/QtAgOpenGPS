// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
//
// Rate control source code used from https://github.com/SK21/AOG_RC
#include "ratecontrol.h"
#include <QDebug>
#include "agioservice.h"
#include "pgnparser.h"
#include "cpgn.h"
#include "modulecomm.h"

Q_LOGGING_CATEGORY (rc_log, "backend.qtagopengps")

RateControl *RateControl::s_instance = nullptr;
QMutex RateControl::s_mutex;
bool RateControl::s_cpp_created = false;

RateControl::RateControl(QObject *parent)
    : QObject{parent}
{
    m_currentProductIndex = 0;
    ModID = 0;
    cUPM = 0;
    RateSet = 0;
    actualRate = 0;
    aBtnState = 0;
    mBtnState = 0;
    HectaresPerMinute = 0;
    width = 0;
    swidth = 0;
    speed = 0;
    kp = 0;
    ki = 0;
    kd = 0;
    minpwm = 0;
    maxpwm = 0;
    rateSensor = 0;
    speed = 0;

    // Сначала создаем модель
    m_rcModel = new RCModel(this);

    // Инициализация всех массивов
    for (int i = 0; i < 4; i++) {
        ManualPWM[i] = 0;
        Quantity[i] = 0;
        MeterCal[i] = 0;
        ControlType[i] = 0;
        PWMsetting[i] = 0;
        SensorReceiving[i] = false;
        cRateApplied[i] = 0;
        cSmoothRate[i] = 0;
        cCurrentRate[i] = 0;
        cTargetUPM[i] = 0;
        cMinUPMSpeed[i] = 0;
        cMinUPM[i] = 0;
        OnScreen[i] = 0;
        pidscale[i] = 0;
        CoverageUnits[i] = 0;
        AppMode[i] = 0;
        appRate[i] = 0;
        minSpeed[i] = 0;
        minUPM[i] = 0;
        ProdDensity[i] = 0;
        TargetRate[i] = 0;
    }

    // Создаем начальные продукты в модели
    initializeProducts();

    // Подключаем к AgIOService
    connect(AgIOService::instance(), &AgIOService::rateControlDataReady,
            this, &RateControl::onRateControlDataReady, Qt::DirectConnection);
}

// Метод для инициализации продуктов
void RateControl::initializeProducts()
{
    if (!m_rcModel) return;

    // Очищаем текущие продукты
    m_rcModel->clear();

    // Создаем 4 продукта
    for (int i = 0; i < 4; ++i) {
        loadSettings(i);
        RCModel::Product product;
        product.name = getProductNameFromSettings(i); // Получаем имя из настроек
        product.setRate = TargetRate[i];
        product.smoothRate = cSmoothRate[i];
        product.actualRate = cCurrentRate[i];
        product.quantity = Quantity[i];
        product.pwm = ManualPWM[i];
        product.isActive = ProductOn(i);

        m_rcModel->addProduct(product);
    }
}

// Вспомогательный метод для получения имени из настроек
QString RateControl::getProductNameFromSettings(int index)
{
    if (index < 0 || index >= 4) return QString("Product %1").arg(index + 1);

    switch (index) {
    case 0: return SettingsManager::instance()->rate_productName0();
    case 1: return SettingsManager::instance()->rate_productName1();
    case 2: return SettingsManager::instance()->rate_productName2();
    case 3: return SettingsManager::instance()->rate_productName3();
    default: return QString("Product %1").arg(index + 1);
    }
}

RateControl *RateControl::instance() {
    QMutexLocker locker(&s_mutex);
    if (!s_instance) {
        s_instance = new RateControl();
        qDebug(rc_log) << "RateControl singleton created by C++ code.";
        s_cpp_created = true;
        // ensure cleanup on app exit
        QObject::connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit,
                         s_instance, []() {
                             delete s_instance; s_instance = nullptr;
                         });
    }
    return s_instance;
}

RateControl *RateControl::create(QQmlEngine *qmlEngine, QJSEngine *jsEngine) {
    Q_UNUSED(jsEngine)

    QMutexLocker locker(&s_mutex);

    if(!s_instance) {
        s_instance = new RateControl();
        qDebug(rc_log) << "RateControl singleton created by QML engine.";
    } else if (s_cpp_created) {
        qmlEngine->setObjectOwnership(s_instance, QQmlEngine::CppOwnership);
    }

    return s_instance;
}

void RateControl::rate_bump(bool up, int index)
{
    // Проверка границ индекса
    if (index < 0 || index >= 4) return;

    if (up) {
        if (ManualPWM[index] < 250) ManualPWM[index] += 10;
        else ManualPWM[index] = 255;
    } else {
        if (ManualPWM[index] > -250) ManualPWM[index] -= 10;
        else ManualPWM[index] = -255;
    }

    // qDebug(rc_log) << "RateControl::rate_bump - Index:" << index
    //            << "ManualPWM:" << ManualPWM[index];

    // ОБНОВЛЯЕМ МОДЕЛЬ!
    updateModel(index);

    // Отправляем команду модулю
    modulesSend241(index);
}

void RateControl::rate_pwm_auto(int index)
{
    // Проверка границ индекса
    if (index < 0 || index >= 4) return;

    ManualPWM[index] = 0;

    // qDebug(rc_log) << "RateControl::rate_pwm_auto - Index:" << index
    //            << "ManualPWM reset to 0";

    // ОБНОВЛЯЕМ МОДЕЛЬ!
    updateModel(index);

    // Отправляем команду модулю
    modulesSend241(index);
}

int RateControl::Command(int index)
{
    // Проверка границ индекса
    if (index < 0 || index >= 4) return 0;

    int Result = 0;

    // Безопасная битовая маска
    if (ControlType[index] >= 0 && ControlType[index] < 32) {
        Result |= (1 << (ControlType[index] + 1));
    }

    if (aBtnState) Result |= (1 << 6);
    if ((mBtnState) || (ManualPWM[index] == 0)) Result |= (1 << 4);

    return Result;
}

bool RateControl::ProductOn(int index)
{
    // Проверка границ индекса
    if (index < 0 || index >= 4) return false;

    bool Result = false;
    if (ControlType[index] == 4) {
        Result = SensorReceiving[index];
    } else {
        Result = (SensorReceiving[index] && HectaresPerMinute > 0);
    }
    return Result;
}

double RateControl::SmoothRate(int index)
{
    // Проверка границ индекса
    if (index < 0 || index >= 4) return 0;

    double Result = 0;
    if (ProductOn(index)) {
        double Ra = RateApplied(index);
        if (ProdDensity[index] > 0) Ra *= ProdDensity[index];

        if (TargetRate[index] > 0) {
            double Rt = Ra / TargetRate[index];

            if (Rt >= 0.95 && Rt <= 1.05 && aBtnState) {
                Result = TargetRate[index];
            } else {
                Result = Ra;
            }
        } else {
            Result = Ra;
        }
    }
    return Result;
}

double RateControl::CurrentRate(int index)
{
    // Проверка границ индекса
    if (index < 0 || index >= 4) return 0;

    if (ProductOn(index)) {
        double V = RateApplied(index);
        if (ProdDensity[index] > 0) V *= ProdDensity[index];
        return V;
    } else {
        return 0;
    }
}

double RateControl::TargetUPM(int index)
{
    // Проверка границ индекса
    if (index < 0 || index >= 4) return 0;

    double Result = 0;
    switch (CoverageUnits[index]) {
    case 0: // acres
        if (AppMode[index] == 1) {
            // Constant UPM
            double HPM = swidth * cMinUPMSpeed[index] / 600.0;
            // Проверка деления на ноль
            if (HPM > 0.0001) {
                Result = TargetRate[index] * HPM * 2.47;
            }
        } else {
            // section controlled UPM
            Result = TargetRate[index] * HectaresPerMinute * 2.47;
        }
        break;

    case 1: // hectares
        if (AppMode[index] == 1) {
            // Constant UPM
            double HPM = swidth * cMinUPMSpeed[index] / 600.0;
            if (HPM > 0.0001) {
                Result = TargetRate[index] * HPM;
            }
        } else {
            // section controlled UPM
            Result = TargetRate[index] * HectaresPerMinute;
        }
        break;

    case 2: // minutes
        Result = TargetRate[index];
        break;

    default: // hours
        Result = TargetRate[index] / 60;
        break;
    }

    if (ProdDensity[index] > 0) {
        Result /= ProdDensity[index];
    }
    return Result;
}

double RateControl::RateApplied(int index)
{
    // Проверка границ индекса
    if (index < 0 || index >= 4) return 0;

    HectaresPerMinute = width * cMinUPMSpeed[index] / 600.0;
    double Result = 0;

    switch (CoverageUnits[index]) {
    case 0: // acres
        if (AppMode[index] == 1) {
            // Constant UPM
            double HPM = swidth * cMinUPMSpeed[index] / 600.0;
            // Правильное условие и проверка деления на ноль
            if (HPM > 0.0001) {
                Result = appRate[index] / (HPM * 2.47);
            }
        } else if (AppMode[index] == 0 || AppMode[index] == 2) {
            // Правильное условие
            if (HectaresPerMinute > 0.0001) {
                Result = appRate[index] / (HectaresPerMinute * 2.47);
            }
        } else {
            // Document target rate
            Result = TargetRate[index];
        }
        break;

    case 1: // hectares
        if (AppMode[index] == 1) {
            double HPM = swidth * cMinUPMSpeed[index] / 600.0;
            if (HPM > 0.0001) {
                Result = appRate[index] / HPM;
            }
        } else if (AppMode[index] == 0 || AppMode[index] == 2) {
            if (HectaresPerMinute > 0.0001) {
                Result = appRate[index] / HectaresPerMinute;
            }
        } else {
            Result = TargetRate[index];
        }
        break;

    case 2: // minutes
        if (AppMode[index] == 3) {
            Result = TargetRate[index];
        } else {
            Result = appRate[index];
        }
        break;

    default: // hours
        if (AppMode[index] == 3) {

            Result = TargetRate[index];
        } else {
            Result = appRate[index] * 60;
        }
        break;
    }
    return Result;
}

double RateControl::MinUPMSpeed(int index)
{
    // Проверка границ индекса
    if (index < 0 || index >= 4) return 0;

    if (speed < minSpeed[index])
        return 0;
    else
        return speed;
}

double RateControl::MinUPM(int index)
{
    // Проверка границ индекса
    if (index < 0 || index >= 4) return 0;

    if (cTargetUPM[index] != 0 && cTargetUPM[index] < minUPM[index])
        return minUPM[index];
    else
        return cTargetUPM[index];
}

void RateControl::loadSettings(int index)
{
    if (index < 0 || index >= 4) return;

    QVector<int> rateSettings;

    switch (index) {
    case 0:
        rateSettings = SettingsManager::instance()->rate_confProduct0();
        break;
    case 1:
        rateSettings = SettingsManager::instance()->rate_confProduct1();
        break;
    case 2:
        rateSettings = SettingsManager::instance()->rate_confProduct2();
        break;
    case 3:
        rateSettings = SettingsManager::instance()->rate_confProduct3();
        break;
    }

    if (rateSettings.size() >= 15) {
        ProdDensity[index] = rateSettings[1];
        OnScreen[index] = rateSettings[2];
        pidscale[index] = rateSettings[8];
        MeterCal[index] = rateSettings[9];
        TargetRate[index] = rateSettings[10];
        AppMode[index] = rateSettings[11];
        ControlType[index] = rateSettings[12];
        CoverageUnits[index] = rateSettings[13];
        minSpeed[index] = rateSettings[14];
        minUPM[index] = rateSettings[15];
    }

    // Обновляем модель после загрузки настроек
    updateModel(index);
}

void RateControl::onRateControlDataReady(const PGNParser::ParsedData &data)
{
    // Update data from RC modules

    if (!data.isValid) return;

    // PGN 240: RC Data
    if (data.pgnNumber == 240) {

        int moduleIndex = data.rateControlInData[0];

        if (moduleIndex >= 0 && moduleIndex < 4) {
            appRate[moduleIndex] = data.rateControlInData[1];
            Quantity[moduleIndex] = data.rateControlInData[2];
            PWMsetting[moduleIndex] = data.rateControlInData[3];
            SensorReceiving[moduleIndex] = data.rateControlInData[4];

            loadSettings(moduleIndex);

            cTargetUPM[moduleIndex] = TargetUPM(moduleIndex) * 10;
            cRateApplied[moduleIndex] = RateApplied(moduleIndex);
            cSmoothRate[moduleIndex] = SmoothRate(moduleIndex);
            cCurrentRate[moduleIndex] = CurrentRate(moduleIndex);
            cMinUPMSpeed[moduleIndex] = MinUPMSpeed(moduleIndex);
            cMinUPM[moduleIndex] = MinUPM(moduleIndex);

            // Обновляем модель
            if (m_rcModel) {
                updateModel(moduleIndex);
            }

            modulesSend241(moduleIndex);
        }
    }
}

void RateControl::modulesSend241(int index)
{
    if (index < 0 || index >= 4) return;

    CPGN_F1 &p_241 = ModuleComm::instance()->p_241;
    p_241.pgn[CPGN_F1::ID] = index; // Используем индекс
    p_241.pgn[CPGN_F1::RateSetLo] = (char)((int)cMinUPM[index]); // target rate
    p_241.pgn[CPGN_F1::RateSetHI] = (char)((int)cMinUPM[index] >> 8);
    p_241.pgn[CPGN_F1::FlowCalLO] = (char)((int)MeterCal[index]);
    p_241.pgn[CPGN_F1::FlowCalHI] = (char)((int)MeterCal[index] >> 8);
    p_241.pgn[CPGN_F1::Command] = Command(index);
    p_241.pgn[CPGN_F1::ManualPWM] = (char)(ManualPWM[index]/2);

    AgIOService::instance()->sendPgn(p_241.pgn);


}

void RateControl::updateModel(int index)
{
    if (index < 0 || index >= 4) return;

    // Проверяем валидность индекса в модели
    if (!m_rcModel || index >= m_rcModel->count()) return;

    // Получаем данные из Settings для имени
    QString productName = getProductNameFromSettings(index);
    bool isActive = SensorReceiving[index];

    // ОБНОВЛЯЕМ ВСЕ ДАННЫЕ В МОДЕЛИ
    m_rcModel->updateName(index, productName);
    m_rcModel->updateSetRate(index, TargetRate[index]);
    m_rcModel->updateSmoothRate(index, cSmoothRate[index]);
    m_rcModel->updateActualRate(index, cCurrentRate[index]);
    m_rcModel->updateQuantity(index, Quantity[index]);
    m_rcModel->updatePWM(index, ManualPWM[index]);  // Используем ManualPWM
    m_rcModel->updateIsActive(index, isActive);

    // qDebug(rc_log) << "RateControl::updateModel - Index:" << index
    //            << "Name:" << productName
    //            << "SetRate:" << TargetRate[index]
    //            << "PWM:" << ManualPWM[index]
    //            << "Active:" << isActive;
}

void RateControl::setCurrentProductIndex(int index)
{
    if (index >= 0 && index < 4 && m_currentProductIndex != index) {
        m_currentProductIndex = index;
        emit currentProductIndexChanged(index);

        // Обновляем данные для выбранного продукта
        updateModel(index);
    }
}

QVariantMap RateControl::getProductDataByIndex(int index) const
{
    if (!m_rcModel || index < 0 || index >= 4)
        return QVariantMap();

    return m_rcModel->get(index);
}

// Q_INVOKABLE методы для QML
void RateControl::increaseSetRate(int index, double step)
{
    if (index < 0 || index >= 4 || !m_rcModel) return;

    TargetRate[index] += step;

    SettingsManager* manager = SettingsManager::instance();

    // Получаем текущий вектор настроек
    QVector<int> settings;

    switch (index) {
    case 0: settings = manager->rate_confProduct0(); break;
    case 1: settings = manager->rate_confProduct1(); break;
    case 2: settings = manager->rate_confProduct2(); break;
    case 3: settings = manager->rate_confProduct3(); break;
    }

    if (settings.size() <= 10) {
        settings.resize(11); // Увеличиваем размер до 11 элементов
        // Заполняем нулями новые элементы, если нужно
        for (int i = settings.size(); i < 10; i++) {
            settings.append(0.0);
        }
    }

    // Изменяем только 10-й элемент (индекс 10)
    settings[10] = TargetRate[index];

    // Сохраняем измененный вектор обратно
    switch (index) {
    case 0: manager->setRate_confProduct0(settings); break;
    case 1: manager->setRate_confProduct1(settings); break;
    case 2: manager->setRate_confProduct2(settings); break;
    case 3: manager->setRate_confProduct3(settings); break;
    }

    // Обновляем модель
    m_rcModel->updateSetRate(index, TargetRate[index]);
    updateModel(index);

    qDebug(rc_log) << "Increased set rate for product" << index
                   << "by" << step << "New rate:" << TargetRate[index];
}

void RateControl::decreaseSetRate(int index, double step)
{
    if (index < 0 || index >= 4 || !m_rcModel) return;

    TargetRate[index] -= step;

    SettingsManager* manager = SettingsManager::instance();

    // Получаем текущий вектор настроек
    QVector<int> settings;

    switch (index) {
    case 0: settings = manager->rate_confProduct0(); break;
    case 1: settings = manager->rate_confProduct1(); break;
    case 2: settings = manager->rate_confProduct2(); break;
    case 3: settings = manager->rate_confProduct3(); break;
    }

    if (settings.size() <= 10) {
        settings.resize(11); // Увеличиваем размер до 11 элементов
        // Заполняем нулями новые элементы, если нужно
        for (int i = settings.size(); i < 10; i++) {
            settings.append(0.0);
        }
    }

    // Изменяем только 10-й элемент (индекс 10)
    settings[10] = TargetRate[index];

    // Сохраняем измененный вектор обратно
    switch (index) {
    case 0: manager->setRate_confProduct0(settings); break;
    case 1: manager->setRate_confProduct1(settings); break;
    case 2: manager->setRate_confProduct2(settings); break;
    case 3: manager->setRate_confProduct3(settings); break;
    }

    // Обновляем модель
    m_rcModel->updateSetRate(index, TargetRate[index]);
    updateModel(index);

    qDebug(rc_log) << "Increased set rate for product" << index
                   << "by" << step << "New rate:" << TargetRate[index];
}

// Метод для обновления всех продуктов
void RateControl::updateAllProducts()
{
    if (!m_rcModel) return;

    for (int i = 0; i < 4 && i < m_rcModel->count(); ++i) {
        updateModel(i);
    }
}

// Метод для принудительного обновления продукта
void RateControl::refreshProduct(int index)
{
    if (index < 0 || index >= 4) return;

    loadSettings(index);
    updateModel(index);
}
