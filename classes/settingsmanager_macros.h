#pragma once

#include <QObject>
#include <QProperty>
#include <QBindable>
#include <QSettings>
#include <QVariant>
#include <QDebug>


// Qt6 Pure SettingsManager Property Macros - Phase 6.0.19
// Generates DECLARATIONS ONLY in .h files (Rectangle pattern compliance)
// Implementations must be generated separately in .cpp files
// CRITICAL: Strict .h/.cpp separation for MOC compatibility

// Qt6 Pure property macro - generates DECLARATIONS ONLY in .h
// Usage: SETTINGS_PROPERTY_DECLARATIONS(double, vehicle_toolWidth, setVehicle_toolWidth)
// Result: Q_PROPERTY + method declarations (implementations in .cpp)
#define SETTINGS_PROPERTY_DECLARATIONS(type, name, setterName) \
    Q_PROPERTY(type name READ name WRITE setterName NOTIFY name##Changed BINDABLE bindable##name) \
    type name() const; \
    void setterName(const type& value); \
    QBindable<type> bindable##name();

// Specialized macros for different types - DECLARATIONS ONLY
#define SETTINGS_PROPERTY_STRING_DECLARATIONS(name, setterName) \
    SETTINGS_PROPERTY_DECLARATIONS(QString, name, setterName)

#define SETTINGS_PROPERTY_COLOR_DECLARATIONS(name, setterName) \
    SETTINGS_PROPERTY_DECLARATIONS(QColor, name, setterName)

#define SETTINGS_PROPERTY_POINT_DECLARATIONS(name, setterName) \
    SETTINGS_PROPERTY_DECLARATIONS(QPoint, name, setterName)

#define SETTINGS_PROPERTY_RECT_DECLARATIONS(name, setterName) \
    SETTINGS_PROPERTY_DECLARATIONS(QRect, name, setterName)

#define SETTINGS_PROPERTY_VECTOR_INT_DECLARATIONS(name, setterName) \
    SETTINGS_PROPERTY_DECLARATIONS(QVector<int>, name, setterName)

// ===== IMPLEMENTATIONS MACROS FOR .CPP GENERATION =====
// These generate the actual method implementations with QSettings persistence

#define SETTINGS_PROPERTY_IMPL(type, name, iniGroup, iniKey, defaultVal, setterName) \
    type SettingsManager::name() const { \
        return m_##name.value(); \
    } \
    void SettingsManager::setterName(const type& value) { \
        m_##name.setValue(value); \
        m_qsettings->setValue(iniKey, QVariant::fromValue(value)); \
        m_qsettings->sync(); \
        emit iniGroup##GroupChanged(); \
        qDebug() << "SettingsManager: save" << iniKey << value; \
        saveToActiveProfile(); \
    } \
    QBindable<type> SettingsManager::bindable##name() { \
        return &m_##name; \
    }

// Specialized implementation macros
#define SETTINGS_PROPERTY_STRING_IMPL(name, iniGroup, iniKey, defaultVal, setterName) \
    SETTINGS_PROPERTY_IMPL(QString, name, iniGroup, iniKey, QString(defaultVal), setterName)

#define SETTINGS_PROPERTY_COLOR_IMPL(name, iniGroup, iniKey, defaultVal, setterName) \
    SETTINGS_PROPERTY_IMPL(QColor, name, iniGroup, iniKey, defaultVal, setterName)

#define SETTINGS_PROPERTY_POINT_IMPL(name, iniGroup, iniKey, defaultVal, setterName) \
    SETTINGS_PROPERTY_IMPL(QPoint, name, iniGroup, iniKey, defaultVal, setterName)

#define SETTINGS_PROPERTY_RECT_IMPL(name, iniGroup, iniKey, defaultVal, setterName) \
    SETTINGS_PROPERTY_IMPL(QRect, name, iniGroup, iniKey, defaultVal, setterName)

#define SETTINGS_PROPERTY_VECTOR_INT_IMPL(name, iniGroup, iniKey, defaultVal, setterName) \
    SETTINGS_PROPERTY_IMPL(QVector<int>, name, iniGroup, iniKey, defaultVal, setterName)

