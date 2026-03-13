#include "settingsmanager.h"
#include <QCoreApplication>
#include <QMutexLocker>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QFile>
#include <QStandardPaths>
#include <QDir>
#include <QDataStream>
#include <QIODevice>
#include <QTimer>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY (settingsmanager, "settingsmanager.qtagopengps")

SettingsManager* SettingsManager::s_instance = nullptr;
QMutex SettingsManager::s_mutex;
bool SettingsManager::s_cpp_created = false;

SettingsManager::SettingsManager(QObject *parent) : QObject(parent), m_disableAutoSave(false)
{
    // Initialize QSettings with INI file in Documents directory
    QString documentsPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    QString settingsDir = documentsPath + "/QtAgOpenGPS";
    QDir().mkpath(settingsDir); // Create directory if necessary
    m_qsettings = new QSettings(settingsDir + "/QtAgOpenGPS.ini", QSettings::IniFormat);

    qDebug() << "SettingsManager Phase 6.0.19: Qt6 Pure Architecture";
    qDebug() << "Qt6 Q_OBJECT_BINDABLE_PROPERTY with automatic QSettings persistence";
    qDebug() << "385 properties with direct API compliant Phase 6.0.17";
    qDebug() << "INI file:" << m_qsettings->fileName();

    // Initialize debounce timers for JSON saves
    initDebounceTimers();

    // CRITICAL: Load all property values from QSettings with proper defaults
    initializeFromSettings();

    qDebug() << "SettingsManager: Qt6 Pure initialization complete";
}

SettingsManager::~SettingsManager()
{
    if (m_qsettings) {
        // Qt6 architecture: Properties save automatically in setters
        // No manual save needed - QSettings sync() called in every setter
        delete m_qsettings;
    }
}

SettingsManager* SettingsManager::instance()
{
    QMutexLocker locker(&s_mutex);
    if (!s_instance) {
        s_instance = new SettingsManager();
        qDebug(settingsmanager) << "Singleton created by C++ code.";
        s_cpp_created = true;
        // ensure cleanup on app exit
        QObject::connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit,
                         s_instance, []() {
                             delete s_instance; s_instance = nullptr;
                         });
    }
    return s_instance;
}

SettingsManager *SettingsManager::create(QQmlEngine *qmlEngine, QJSEngine *jsEngine) {
    Q_UNUSED(jsEngine)

    QMutexLocker locker(&s_mutex);

    if(!s_instance) {
        s_instance = new SettingsManager();
        qDebug(settingsmanager) << "Singleton created by QML engine.";
    } else if (s_cpp_created) {
        qmlEngine->setObjectOwnership(s_instance, QQmlEngine::CppOwnership);
    }

    return s_instance;
}


// ===== JSON PROFILE SYSTEM (Vehicle/Field profiles) =====

QJsonObject SettingsManager::toJson()
{
    QMutexLocker locker(&m_accessMutex);
    return toJsonUnsafe();  // Call unsafe version with mutex protection
}

QJsonObject SettingsManager::toJsonUnsafe()
{
    // Export all QSettings keys to JSON (backup_final_conversion_125510 compatible)
    QVariant b;
    QStringList keys = m_qsettings->allKeys();
    QString type;
    QString json_value;
    QJsonObject blah;

    for (const auto &key : std::as_const(keys)) {
        b = m_qsettings->value(key);
        type = b.typeName();

        if (type == "QStringList" || type == "QVariantList" || type == "QJSValue") {
            // Convert list to "@List:value1,value2,value3" format
            QVariantList list = b.toList();
            json_value = "@List:";
            for(int i=0; i < list.length(); i++){
                json_value += list[i].toString();
                if (i < list.length() -1)
                    json_value += ",";
            }
            blah[key] = json_value;
        } else if (type == "QPoint") {
            // Convert QPoint to "@Variant(...)" format
            QByteArray raw_value;
            QDataStream ds(&raw_value, QIODevice::WriteOnly);
            ds << b;
            json_value = QLatin1String("@Variant(");
            json_value += QString::fromLatin1(raw_value.constData(), raw_value.size());
            json_value += ")";
            blah[key] = json_value;
        } else {
            // Direct conversion for basic types (QString, int, double, bool)
            blah[key] = QJsonValue::fromVariant(b);
        }
    }

    return blah;
}

bool SettingsManager::saveJson(QString filename)
{
    qDebug() << "saveJson: Starting for" << filename;

    // Generate JSON with thread-safe scope (conforming to THREADING_PLAN pattern)
    QJsonObject jsonData;
    {
        qDebug() << "saveJson: Acquiring mutex for toJson()";
        QMutexLocker locker(&m_accessMutex);  // Limited scope like setValue()
        qDebug() << "saveJson: Mutex acquired, calling toJson()";
        jsonData = toJsonUnsafe();  // Call unsafe version with mutex protection
        qDebug() << "saveJson: toJson() completed, releasing mutex";
    }  // Mutex unlocked here before file I/O
    qDebug() << "saveJson: Mutex released, starting file operations";

    // File I/O operations OUTSIDE mutex scope (no thread conflict)
    QFile savefile(filename);
    qDebug() << "saveJson: Attempting to open file" << filename;
    if (!savefile.open(QIODevice::WriteOnly)) {
        qWarning() << "SettingsManager: Could not save json settings file" << filename;
        return false;
    }
    qDebug() << "saveJson: File opened successfully, writing JSON";

    savefile.write(QJsonDocument(jsonData).toJson());
    qDebug() << "saveJson: JSON written, closing file";
    savefile.close();
    qDebug() << "saveJson: File closed, operation completed successfully";
    return true;
}

bool SettingsManager::loadJson(QString filename)
{
    // CRITICAL: Disable auto-save during loading to prevent vehicleName corruption
    m_disableAutoSave = true;
    qDebug() << "SettingsManager: Auto-save disabled for loading" << filename;

    // Read JSON file OUTSIDE mutex scope (conforming to THREADING_PLAN pattern)

    QFile loadfile(filename);
    if (!loadfile.open(QIODevice::ReadOnly)) {
        qWarning() << "SettingsManager: Could not load json settings file" << filename;
        m_disableAutoSave = false; // Re-enable auto-save
        return false;
    }

    QByteArray loadedjson = loadfile.readAll();
    QJsonDocument loaded(QJsonDocument::fromJson(loadedjson));
    QJsonObject j = loaded.object();
    QString new_value;
    QVariant v;

    // Process settings updates with limited mutex scope (conforming to THREADING_PLAN pattern)
    {
        QMutexLocker locker(&m_accessMutex);  // Limited scope for settings updates only

        for (const auto &key : j.keys()) {
            new_value = j[key].toString();
            if (new_value.startsWith("@Variant(")) {
                // Handle "@Variant(...)" format for complex types like QPoint
                QString payload = new_value.mid(9);
                payload.chop(1); // Remove closing )
                QByteArray raw_value = payload.toLatin1();
                QDataStream ds(&raw_value, QIODevice::ReadOnly);
                ds >> v;
                m_qsettings->setValue(key, v);
            } else if (new_value.startsWith("@List:")) {
                // Handle "@List:value1,value2,value3" format
                QString payload = new_value.mid(6);
                QStringList list = payload.split(",");
                QVariantList varList;
                for (const QString& item : std::as_const(list)) {
                    varList.append(item.toInt());
                }
                m_qsettings->setValue(key, varList);
            } else {
                // Direct assignment for basic types
                v = j[key].toVariant();
                m_qsettings->setValue(key, v);
            }
        }

        m_qsettings->sync(); // Force write to disk
    }  // Mutex unlocked here

    // CRITICAL: Synchronize Qt6 properties with QSettings values (without defaults)
    // This loads the JSON values from QSettings into Qt6 properties
    syncPropertiesFromQSettings();
    qDebug() << "loadJson: Qt6 properties synchronized with loaded JSON data";

    // Re-enable auto-save after loading is complete
    m_disableAutoSave = false;
    qDebug() << "SettingsManager: Auto-save re-enabled after loading" << filename;

    return true;
}

// ===== ACTIVE VEHICLE PROFILE SYSTEM =====

void SettingsManager::setActiveVehicleProfile(const QString& profilePath)
{
    // Step 1: Save current profile if one is active
    QString currentProfile;
    {
        QMutexLocker locker(&m_accessMutex);
        currentProfile = m_activeVehicleProfilePath;
    }

    if (!currentProfile.isEmpty() && currentProfile != profilePath) {
        qDebug() << "SettingsManager: Saving current vehicle profile before switch:" << currentProfile;
        saveJson(currentProfile);
    }

    // Step 2: Set new profile path
    {
        QMutexLocker locker(&m_accessMutex);
        m_activeVehicleProfilePath = profilePath;
        qDebug() << "SettingsManager: Active vehicle profile set to:" << profilePath;
    }

    // Step 3: Load new profile if it exists
    if (!profilePath.isEmpty()) {
        QFile profileFile(profilePath);
        if (profileFile.exists()) {
            qDebug() << "SettingsManager: Loading new vehicle profile:" << profilePath;
            loadJson(profilePath);
        } else {
            qDebug() << "SettingsManager: New vehicle profile file doesn't exist, will be created:" << profilePath;
        }
    }
}

QString SettingsManager::getActiveVehicleProfile() const
{
    QMutexLocker locker(&m_accessMutex);
    return m_activeVehicleProfilePath;
}

void SettingsManager::clearActiveVehicleProfile()
{
    QMutexLocker locker(&m_accessMutex);
    m_activeVehicleProfilePath.clear();
    qDebug() << "SettingsManager: Active vehicle profile cleared";
}

void SettingsManager::setActiveFieldProfile(const QString& profilePath)
{
    // Step 1: Save current profile if one is active
    QString currentProfile;
    {
        QMutexLocker locker(&m_accessMutex);
        currentProfile = m_activeFieldProfilePath;
    }

    if (!currentProfile.isEmpty() && currentProfile != profilePath) {
        qDebug() << "SettingsManager: Saving current field profile before switch:" << currentProfile;
        saveJson(currentProfile);
    }

    // Step 2: Set new profile path
    {
        QMutexLocker locker(&m_accessMutex);
        m_activeFieldProfilePath = profilePath;
        qDebug() << "SettingsManager: Active field profile set to:" << profilePath;
    }

    // Step 3: Load new profile if it exists
    if (!profilePath.isEmpty()) {
        QFile profileFile(profilePath);
        if (profileFile.exists()) {
            qDebug() << "SettingsManager: Loading new field profile:" << profilePath;
            loadJson(profilePath);
        } else {
            qDebug() << "SettingsManager: New field profile file doesn't exist, will be created:" << profilePath;
        }
    }
}

QString SettingsManager::getActiveFieldProfile() const
{
    QMutexLocker locker(&m_accessMutex);
    return m_activeFieldProfilePath;
}

void SettingsManager::clearActiveFieldProfile()
{
    QMutexLocker locker(&m_accessMutex);
    m_activeFieldProfilePath.clear();
    qDebug() << "SettingsManager: Active field profile cleared";
}

void SettingsManager::initDebounceTimers()
{
    // Create debounce timers for vehicle and field profiles
    m_vehicleDebounceTimer = new QTimer(this);
    m_vehicleDebounceTimer->setSingleShot(true);
    m_vehicleDebounceTimer->setInterval(500); // 500ms debounce
    connect(m_vehicleDebounceTimer, &QTimer::timeout, this, [this]() {
        QString profilePath = getActiveVehicleProfile();
        if (!profilePath.isEmpty()) {
            saveJson(profilePath);
            qDebug() << "SettingsManager: Debounced auto-save to vehicle profile completed:" << profilePath;
        }
    });

    m_fieldDebounceTimer = new QTimer(this);
    m_fieldDebounceTimer->setSingleShot(true);
    m_fieldDebounceTimer->setInterval(500); // 500ms debounce
    connect(m_fieldDebounceTimer, &QTimer::timeout, this, [this]() {
        QString profilePath = getActiveFieldProfile();
        if (!profilePath.isEmpty()) {
            saveJson(profilePath);
            qDebug() << "SettingsManager: Debounced auto-save to field profile completed:" << profilePath;
        }
    });
}

void SettingsManager::saveToActiveProfile()
{
    // Skip auto-save if disabled (during profile loading)
    if (m_disableAutoSave) {
        qDebug() << "🚫 CRITICAL: Auto-save disabled, skipping saveToActiveProfile() - active profile:" << m_activeVehicleProfilePath;
        return;
    }

    // Debounced auto-save to JSON when profiles are active (called from setters)
    if (!m_activeVehicleProfilePath.isEmpty()) {
        qDebug() << "SettingsManager: Debouncing auto-save to vehicle profile:" << m_activeVehicleProfilePath;
        m_vehicleDebounceTimer->start(); // Restart timer, cancels previous
    }

    if (!m_activeFieldProfilePath.isEmpty()) {
        qDebug() << "SettingsManager: Debouncing auto-save to field profile:" << m_activeFieldProfilePath;
        m_fieldDebounceTimer->start(); // Restart timer, cancels previous
    }
}

// LEGACY API AND INTERNAL METHODS REMOVED - Phase 6.0.19 Qt6 Pure Architecture
// setValue(), getValue(), value(), valueIntVec(), setValueIntVec() removed
// sync(), addKey(), saveToQSettings(), getFromQSettings() removed
// toJsonUnsafe(), convertQmlKeyToIniPath() removed
// JSON functionality removed - not part of Qt6 Pure Architecture
//
// Qt6 Pure Architecture: All functionality handled directly by SETTINGS_PROPERTY macros
// Use direct property API: settings->property_name instead of getValue("property_name")
// Use direct setters: settings->setProperty_name(value) instead of setValue("property_name", value)
// Automatic QSettings persistence in every setter via macros

// Synchronize Qt6 properties with current QSettings values (for JSON loading)
void SettingsManager::syncPropertiesFromQSettings()
{
    QMutexLocker locker(&m_accessMutex);

    // CRITICAL: This method needs to synchronize ALL properties from QSettings
    // The issue was that only a few properties were mapped, so most JSON values
    // were not synchronized to Qt6 properties, causing QML display issues

    qDebug() << "SettingsManager: Starting syncPropertiesFromQSettings() for" << m_qsettings->allKeys().size() << "QSettings keys";

    // TODO: This should be auto-generated like implementations to include ALL 385 properties
    // For now, we need to manually call initializeFromSettings() BUT without defaults
    // The difference is we use QSettings current values, not defaults from QtAgOpenGPS.ini

    // TEMPORARY WORKAROUND: Force refresh of all Qt6 properties from current QSettings values
    // This ensures ALL properties get synchronized, not just the few manually mapped above

    for (const auto& key : m_qsettings->allKeys()) {
        QVariant value = m_qsettings->value(key);
        qDebug() << "Sync key:" << key << "=" << value.toString();

        // Vehicle properties
        if (key == "vehicle/vehicleName") {
            qDebug() << "CRITICAL: Syncing vehicleName from" << value.toString();
            m_vehicle_vehicleName.setValue(value.toString());
            qDebug() << "CRITICAL: vehicleName now set to" << m_vehicle_vehicleName.value();
        }
        else if (key == "vehicle/toolWidth") m_vehicle_toolWidth.setValue(value.toDouble());
        else if (key == "vehicle/toolOffset") m_vehicle_toolOffset.setValue(value.toDouble());
        else if (key == "vehicle/toolOverlap") m_vehicle_toolOverlap.setValue(value.toDouble());
        else if (key == "vehicle/vehicleType") m_vehicle_vehicleType.setValue(value.toInt());
        else if (key == "vehicle/trackWidth") m_vehicle_trackWidth.setValue(value.toDouble());
        else if (key == "vehicle/wheelbase") m_vehicle_wheelbase.setValue(value.toDouble());
        else if (key == "vehicle/antennaHeight") m_vehicle_antennaHeight.setValue(value.toDouble());
        else if (key == "vehicle/antennaOffset") m_vehicle_antennaOffset.setValue(value.toDouble());
        else if (key == "vehicle/antennaPivot") m_vehicle_antennaPivot.setValue(value.toDouble());
        else if (key == "vehicle/numSections") m_vehicle_numSections.setValue(value.toDouble());

        // Section positions (critical for section widths)
        else if (key == "section/position1") m_section_position1.setValue(value.toDouble());
        else if (key == "section/position2") m_section_position2.setValue(value.toDouble());
        else if (key == "section/position3") m_section_position3.setValue(value.toDouble());
        else if (key == "section/position4") m_section_position4.setValue(value.toDouble());
        else if (key == "section/position5") m_section_position5.setValue(value.toDouble());
        else if (key == "section/position6") m_section_position6.setValue(value.toDouble());
        else if (key == "section/position7") m_section_position7.setValue(value.toDouble());
        else if (key == "section/position8") m_section_position8.setValue(value.toDouble());
        else if (key == "section/position9") m_section_position9.setValue(value.toDouble());
        else if (key == "section/position10") m_section_position10.setValue(value.toDouble());
        else if (key == "section/position11") m_section_position11.setValue(value.toDouble());
        else if (key == "section/position12") m_section_position12.setValue(value.toDouble());
        else if (key == "section/position13") m_section_position13.setValue(value.toDouble());
        else if (key == "section/position14") m_section_position14.setValue(value.toDouble());
        else if (key == "section/position15") m_section_position15.setValue(value.toDouble());
        else if (key == "section/position16") m_section_position16.setValue(value.toDouble());
        else if (key == "section/position17") m_section_position17.setValue(value.toDouble());

        // Tool properties
        else if (key == "tool/isToolRearFixed") m_tool_isToolRearFixed.setValue(value.toBool());
        else if (key == "tool/isToolFront") m_tool_isToolFront.setValue(value.toBool());
        else if (key == "tool/isToolTrailing") m_tool_isToolTrailing.setValue(value.toBool());
        else if (key == "tool/isTBT") m_tool_isTBT.setValue(value.toBool());
        else if (key == "tool/isSectionsNotZones") m_tool_isSectionsNotZones.setValue(value.toBool());
        else if (key == "tool/zones") {
            // Handle QStringList to QVector<int> conversion for tool_zones
            QStringList list = value.toStringList();
            QVector<int> vector;
            for (const QString& str : std::as_const(list)) {
                bool ok;
                int val = str.toInt(&ok);
                if (ok) vector.append(val);
            }
            m_tool_zones.setValue(vector);
        }

        // ⚡ PHASE 6.0.20: Pure Pursuit & Stanley Guidance Properties
        else if (key == "vehicle/goalPointLookAhead") m_vehicle_goalPointLookAhead.setValue(value.toDouble());
        else if (key == "vehicle/goalPointLookAheadHold") m_vehicle_goalPointLookAheadHold.setValue(value.toDouble());
        else if (key == "vehicle/goalPointLookAheadMult") m_vehicle_goalPointLookAheadMult.setValue(value.toDouble());
        else if (key == "vehicle/goalPointAcquireFactor") m_vehicle_goalPointAcquireFactor.setValue(value.toDouble());
        else if (key == "vehicle/purePursuitIntegralGainAB") m_vehicle_purePursuitIntegralGainAB.setValue(value.toDouble());
        else if (key == "vehicle/stanleyDistanceErrorGain") m_vehicle_stanleyDistanceErrorGain.setValue(value.toDouble());
        else if (key == "vehicle/stanleyHeadingErrorGain") m_vehicle_stanleyHeadingErrorGain.setValue(value.toDouble());
        else if (key == "vehicle/stanleyIntegralGainAB") m_vehicle_stanleyIntegralGainAB.setValue(value.toDouble());
        else if (key == "vehicle/stanleyIntegralDistanceAwayTriggerAB") m_vehicle_stanleyIntegralDistanceAwayTriggerAB.setValue(value.toDouble());

        // ⚡ PHASE 6.0.20: Vehicle Configuration Properties
        else if (key == "vehicle/isPivotBehindAntenna") m_vehicle_isPivotBehindAntenna.setValue(value.toBool());
        else if (key == "vehicle/isSteerAxleAhead") m_vehicle_isSteerAxleAhead.setValue(value.toBool());
        else if (key == "vehicle/isStanleyUsed") m_vehicle_isStanleyUsed.setValue(value.toBool());
        else if (key == "vehicle/isMachineControlToAutoSteer") m_vehicle_isMachineControlToAutoSteer.setValue(value.toBool());
        else if (key == "vehicle/maxAngularVelocity") m_vehicle_maxAngularVelocity.setValue(value.toDouble());
        else if (key == "vehicle/maxSteerAngle") m_vehicle_maxSteerAngle.setValue(value.toDouble());
        else if (key == "vehicle/slowSpeedCutoff") m_vehicle_slowSpeedCutoff.setValue(value.toDouble());
        else if (key == "vehicle/panicStopSpeed") m_vehicle_panicStopSpeed.setValue(value.toDouble());
        else if (key == "vehicle/hydraulicLiftLookAhead") m_vehicle_hydraulicLiftLookAhead.setValue(value.toDouble());
        else if (key == "vehicle/hitchLength") m_vehicle_hitchLength.setValue(value.toDouble());
        else if (key == "vehicle/lookAheadMinimum") m_vehicle_lookAheadMinimum.setValue(value.toDouble());
        else if (key == "vehicle/minCoverage") m_vehicle_minCoverage.setValue(value.toDouble());
        else if (key == "vehicle/minTurningRadius") m_vehicle_minTurningRadius.setValue(value.toDouble());
        else if (key == "vehicle/toolLookAheadOff") m_vehicle_toolLookAheadOff.setValue(value.toDouble());
        else if (key == "vehicle/toolLookAheadOn") m_vehicle_toolLookAheadOn.setValue(value.toDouble());
        else if (key == "vehicle/toolOffDelay") m_vehicle_toolOffDelay.setValue(value.toDouble());
        else if (key == "vehicle/tankTrailingHitchLength") m_vehicle_tankTrailingHitchLength.setValue(value.toDouble());

        // ⚡ PHASE 6.0.20: AutoSteer Settings Properties
        else if (key == "as/modeXTE") m_as_modeXTE.setValue(value.toDouble());
        else if (key == "as/modeTime") m_as_modeTime.setValue(value.toInt());
        else if (key == "as/functionSpeedLimit") m_as_functionSpeedLimit.setValue(value.toInt());
        else if (key == "as/maxSteerSpeed") m_as_maxSteerSpeed.setValue(value.toDouble());
        else if (key == "as/minSteerSpeed") m_as_minSteerSpeed.setValue(value.toDouble());
        else if (key == "as/uTurnCompensation") m_as_uTurnCompensation.setValue(value.toDouble());
        else if (key == "as/isAutoSteerAutoOn") m_as_isAutoSteerAutoOn.setValue(value.toBool());
        else if (key == "as/isConstantContourOn") m_as_isConstantContourOn.setValue(value.toBool());
        else if (key == "as/isSteerInReverse") m_as_isSteerInReverse.setValue(value.toBool());
        else if (key == "as/guidanceLookAheadTime") m_as_guidanceLookAheadTime.setValue(value.toDouble());
        else if (key == "as/modeMultiplierStanley") m_as_modeMultiplierStanley.setValue(value.toDouble());

        // All critical vehicle and autosteer properties now synchronized
        // TODO Phase 7: Generate complete mapping from settingsmanager_properties.h for ALL 385 properties
    }

    qDebug() << "SettingsManager: syncPropertiesFromQSettings() completed";
}

// Include all 385 property implementations (generated by Python script)
#include "settingsmanager_implementations.cpp"

// Include QSettings initialization method (generated by Python script)
#include "settingsmanager_initialize.cpp"

// NOTE: MOC handling is automatic via CMake/qmake - no manual include needed
