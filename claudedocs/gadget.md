# Q_GADGET in Qt 6 - Complete Guide

## What is Q_GADGET?

`Q_GADGET` is a Qt macro for **lightweight value types** that need Qt meta-object features (like `Q_PROPERTY`, `Q_ENUM`) but **don't need QObject features** (signals, slots, parent/child ownership).

## Q_GADGET vs Q_OBJECT

| Feature | Q_OBJECT | Q_GADGET |
|---------|----------|----------|
| Signals/Slots | ✅ Yes | ❌ No |
| Q_PROPERTY | ✅ Yes | ✅ Yes |
| Q_ENUM | ✅ Yes | ✅ Yes |
| Parent/Child | ✅ Yes | ❌ No |
| Copyable | ❌ No (QObject not copyable) | ✅ Yes |
| Value semantics | ❌ No (pointers) | ✅ Yes |
| QML registration | ✅ Yes | ✅ Yes (with Q_DECLARE_METATYPE) |
| Overhead | Heavy | Lightweight |
| Inheritance | Must inherit QObject | Plain struct/class |

## When to Use Q_GADGET

### ✅ Use Q_GADGET for:
1. **Data Transfer Objects (DTOs)** - Passing structured data between C++ and QML
2. **Value types** - Things you want to copy/assign like regular structs
3. **Configuration data** - Settings, options, parameters
4. **Read-only data** - Information bundles without behavior

### ❌ Use Q_OBJECT for:
1. **Singletons/Services** - Backend, managers, controllers
2. **Objects with signals** - Need to notify changes
3. **Objects with lifecycle** - Need parent/child ownership
4. **Active objects** - Have timers, threads, state machines

## Basic Q_GADGET Example

```cpp
// fieldinfo.h
#include <QObject>
#include <QString>
#include <QDateTime>

// A simple data container - no signals, no slots, just data
struct FieldInfo {
    Q_GADGET  // ← Makes it work with Qt's meta-object system

    // Properties accessible from QML
    Q_PROPERTY(QString name MEMBER name)
    Q_PROPERTY(double area MEMBER area)
    Q_PROPERTY(QDateTime created MEMBER created)
    Q_PROPERTY(bool isActive MEMBER isActive)

public:
    QString name;
    double area = 0.0;
    QDateTime created;
    bool isActive = false;

    // Can have methods (but no slots)
    bool isValid() const { return !name.isEmpty() && area > 0; }
};

// Required for QVariant/QML
Q_DECLARE_METATYPE(FieldInfo)
```

## Using Q_GADGET with Q_OBJECT

```cpp
// backend.h
class Backend : public QObject {
    Q_OBJECT

    Q_PROPERTY(FieldInfo currentField READ currentField NOTIFY currentFieldChanged)

public:
    // Return by value - Q_GADGET supports this!
    FieldInfo currentField() const { return m_currentField; }

    Q_INVOKABLE FieldInfo getFieldInfo(const QString& name) const {
        // Create and return a value
        FieldInfo info;
        info.name = name;
        info.area = 125.5;
        info.created = QDateTime::currentDateTime();
        info.isActive = true;
        return info;  // Copyable!
    }

signals:
    void currentFieldChanged();

private:
    FieldInfo m_currentField;
};
```

## Using in QML

```qml
Item {
    Label {
        // Access Q_GADGET properties from QML
        text: "Field: " + Backend.currentField.name
    }

    Label {
        text: "Area: " + Backend.currentField.area.toFixed(2) + " acres"
    }

    Label {
        text: Backend.currentField.isActive ? "Active" : "Inactive"
    }

    Button {
        onClicked: {
            // Method returns Q_GADGET by value
            let info = Backend.getFieldInfo("North40")
            console.log("Field:", info.name, "Area:", info.area)
        }
    }
}
```

## Q_GADGET with Q_ENUM

```cpp
struct VehicleSettings {
    Q_GADGET

public:
    enum SteerMode { Manual, LineAB, Curve, Contour };
    Q_ENUM(SteerMode)

    enum DriveMode { Forward, Reverse };
    Q_ENUM(DriveMode)

    Q_PROPERTY(SteerMode steerMode MEMBER steerMode)
    Q_PROPERTY(DriveMode driveMode MEMBER driveMode)
    Q_PROPERTY(double speed MEMBER speed)

    SteerMode steerMode = Manual;
    DriveMode driveMode = Forward;
    double speed = 0.0;
};
Q_DECLARE_METATYPE(VehicleSettings)
```

```qml
// Use enums in QML
ComboBox {
    model: [
        VehicleSettings.Manual,
        VehicleSettings.LineAB,
        VehicleSettings.Curve,
        VehicleSettings.Contour
    ]
    currentIndex: Backend.vehicleSettings.steerMode
    onCurrentIndexChanged: {
        Backend.setSteerMode(currentIndex)
    }
}
```

## Q_GADGET Binding and Updates

### Important: Q_GADGET has no signals

- ❌ Q_GADGET **cannot** use `Q_OBJECT_BINDABLE_PROPERTY` or `BINDABLE` (no QObject infrastructure)
- ✅ QML **can** bind to Q_GADGET properties
- ⚠️ But updates happen at the **whole-gadget level**, not individual properties

### Q_GADGET properties are just data

```cpp
struct FieldInfo {
    Q_GADGET
    // ❌ Cannot use BINDABLE - Q_GADGET doesn't support it
    // ❌ Cannot use NOTIFY - Q_GADGET has no signals
    Q_PROPERTY(QString name MEMBER name)  // Just MEMBER or READ/WRITE
    Q_PROPERTY(double area MEMBER area)

public:
    QString name;
    double area = 0.0;
};
```

### The containing Q_OBJECT manages updates

```cpp
class Backend : public QObject {
    Q_OBJECT

    // The NOTIFY is on the gadget property itself, not individual fields
    Q_PROPERTY(FieldInfo currentField READ currentField NOTIFY currentFieldChanged)

public:
    FieldInfo currentField() const { return m_currentField; }

    void updateField(const QString& name, double area) {
        // Change the gadget
        m_currentField.name = name;
        m_currentField.area = area;

        // Emit signal - tells QML the WHOLE gadget changed
        emit currentFieldChanged();  // ← This triggers QML updates
    }

signals:
    void currentFieldChanged();  // ← Whole-gadget notification

private:
    FieldInfo m_currentField;
};
```

### QML binds to the gadget properties

```qml
Item {
    // QML binding to gadget properties
    Label {
        // When currentFieldChanged() fires, QML re-reads name
        text: Backend.currentField.name  // ✅ Works!
    }

    Label {
        // When currentFieldChanged() fires, QML re-reads area
        text: Backend.currentField.area.toString()  // ✅ Works!
    }
}
```

## Comparison: Granular vs Whole-Object Updates

### Q_OBJECT - Granular Updates (Fine-grained)

```cpp
class Vehicle : public QObject {
    Q_OBJECT

    // Each property has its own NOTIFY
    Q_PROPERTY(double speed READ speed NOTIFY speedChanged BINDABLE bindableSpeed)
    Q_PROPERTY(double heading READ heading NOTIFY headingChanged BINDABLE bindableHeading)

signals:
    void speedChanged();    // ← Only speed bindings update
    void headingChanged();  // ← Only heading bindings update

private:
    Q_OBJECT_BINDABLE_PROPERTY(Vehicle, double, m_speed, &Vehicle::speedChanged)
    Q_OBJECT_BINDABLE_PROPERTY(Vehicle, double, m_heading, &Vehicle::headingChanged)
};
```

```qml
Label {
    text: vehicle.speed  // ← Updates ONLY when speed changes
}
Label {
    text: vehicle.heading  // ← Updates ONLY when heading changes
}
```

### Q_GADGET - Whole-Object Updates (Coarse-grained)

```cpp
struct VehicleData {
    Q_GADGET
    // No individual NOTIFY - properties are just data
    Q_PROPERTY(double speed MEMBER speed)
    Q_PROPERTY(double heading MEMBER heading)

    double speed = 0.0;
    double heading = 0.0;
};

class Backend : public QObject {
    Q_OBJECT
    Q_PROPERTY(VehicleData vehicle READ vehicle NOTIFY vehicleChanged)

signals:
    void vehicleChanged();  // ← BOTH properties update together

private:
    VehicleData m_vehicle;
};
```

```qml
Label {
    text: Backend.vehicle.speed  // ← Updates when vehicleChanged() fires
}
Label {
    text: Backend.vehicle.heading  // ← Also updates when vehicleChanged() fires
}
// Both update even if only speed changed!
```

## When to Use Each Approach

### ✅ Q_GADGET is fine when:
- Properties logically change together (e.g., latitude/longitude)
- Update frequency is low
- The gadget is small (few properties)
- Data is read-mostly

```cpp
struct Position {
    Q_GADGET
    Q_PROPERTY(double latitude MEMBER latitude)
    Q_PROPERTY(double longitude MEMBER longitude)

    double latitude = 0.0;
    double longitude = 0.0;
};
// ✅ Good: lat/lon always change together
```

### ❌ Use Q_OBJECT when:
- Properties change independently and frequently
- Need fine-grained updates for performance
- Have expensive bindings/computations

```cpp
// ❌ Bad as Q_GADGET - these change independently at high frequency
struct TelemetryData {
    Q_GADGET
    Q_PROPERTY(double speed MEMBER speed)      // Updates 10 Hz
    Q_PROPERTY(double heading MEMBER heading)  // Updates 10 Hz
    Q_PROPERTY(double roll MEMBER roll)        // Updates 10 Hz
    Q_PROPERTY(int satellites MEMBER satellites)  // Updates 1 Hz
    Q_PROPERTY(QString status MEMBER status)   // Updates rarely

    // Problem: changing speed triggers updates of ALL 5 properties!
};

// ✅ Better as Q_OBJECT with individual signals
class Telemetry : public QObject {
    Q_OBJECT
    Q_PROPERTY(double speed ... NOTIFY speedChanged)      // Independent
    Q_PROPERTY(double heading ... NOTIFY headingChanged)  // Independent
    // Each updates only when needed
};
```

## Workaround: Multiple Q_GADGETs with Separate Signals

If you need granular updates with value semantics:

```cpp
struct GPSData {
    Q_GADGET
    Q_PROPERTY(double latitude MEMBER latitude)
    Q_PROPERTY(double longitude MEMBER longitude)
    double latitude = 0.0;
    double longitude = 0.0;
};

struct VehicleData {
    Q_GADGET
    Q_PROPERTY(double speed MEMBER speed)
    Q_PROPERTY(double heading MEMBER heading)
    double speed = 0.0;
    double heading = 0.0;
};

class Backend : public QObject {
    Q_OBJECT

    // Separate properties with separate signals
    Q_PROPERTY(GPSData gpsData READ gpsData NOTIFY gpsDataChanged)
    Q_PROPERTY(VehicleData vehicleData READ vehicleData NOTIFY vehicleDataChanged)

    void updateGPS(double lat, double lon) {
        m_gpsData.latitude = lat;
        m_gpsData.longitude = lon;
        emit gpsDataChanged();  // ← Only GPS bindings update
    }

    void updateSpeed(double speed) {
        m_vehicleData.speed = speed;
        emit vehicleDataChanged();  // ← Only vehicle bindings update
    }

signals:
    void gpsDataChanged();
    void vehicleDataChanged();

private:
    GPSData m_gpsData;
    VehicleData m_vehicleData;
};
```

## Real-World AgOpenGPS Examples

### Boundary Point Data

```cpp
struct BoundaryPoint {
    Q_GADGET
    Q_PROPERTY(double latitude MEMBER latitude)
    Q_PROPERTY(double longitude MEMBER longitude)
    Q_PROPERTY(double heading MEMBER heading)

public:
    double latitude = 0.0;
    double longitude = 0.0;
    double heading = 0.0;
};
Q_DECLARE_METATYPE(BoundaryPoint)
```

### Vehicle Status

```cpp
struct VehicleStatus {
    Q_GADGET
    Q_PROPERTY(double speed MEMBER speed)
    Q_PROPERTY(double heading MEMBER heading)
    Q_PROPERTY(bool isReverse MEMBER isReverse)
    Q_PROPERTY(QPointF position MEMBER position)

public:
    double speed = 0.0;
    double heading = 0.0;
    bool isReverse = false;
    QPointF position;
};
Q_DECLARE_METATYPE(VehicleStatus)
```

### Using in Backend

```cpp
class Backend : public QObject {
    Q_OBJECT

    Q_PROPERTY(VehicleStatus vehicle READ vehicle NOTIFY vehicleChanged)

public:
    Q_INVOKABLE QList<BoundaryPoint> getBoundaryPoints() const {
        // Return list of gadgets by value - efficient!
        return m_boundaryPoints;
    }

signals:
    void vehicleChanged();

private:
    VehicleStatus m_vehicleStatus;
    QList<BoundaryPoint> m_boundaryPoints;
};
```

## Summary

| Feature | Q_OBJECT Properties | Q_GADGET Properties |
|---------|-------------------|-------------------|
| QML binding | ✅ Yes | ✅ Yes |
| BINDABLE macro | ✅ Yes | ❌ No (not supported) |
| NOTIFY signal | ✅ Per-property | ❌ Whole-gadget only |
| Update granularity | Fine (individual) | Coarse (all properties) |
| Overhead | Higher | Lower |
| Best for | Frequently changing, independent data | Infrequent, related data |

## Key Takeaways

1. **Q_GADGET** = Lightweight data containers with Qt meta-object features
2. **Q_OBJECT** = Full-featured objects with signals/slots/ownership
3. Use **Q_GADGET** for DTOs, settings, read-only data
4. Use **Q_OBJECT** for services, controllers, active objects
5. **Q_GADGET** types are **copyable and return-by-value friendly**
6. Perfect for **C++ ↔ QML data transfer** without overhead
7. QML can bind to Q_GADGET properties, but updates happen at the **whole-gadget level** via the containing Q_OBJECT's NOTIFY signal
8. No BINDABLE macros needed (or possible) in Q_GADGET itself!

Q_GADGET lets you have the best of both worlds: lightweight C++ value types that still work seamlessly with QML's property system!

---

## Accessing Q_GADGET Members in C++

Since Q_GADGET is just a regular struct with value semantics, you access members **exactly like any normal C++ struct** - there's nothing special about it.

### Basic Access Patterns

#### 1. Direct Member Access (Read)

```cpp
struct Position {
    Q_GADGET
    Q_PROPERTY(double latitude MEMBER latitude)
    Q_PROPERTY(double longitude MEMBER longitude)

    double latitude = 0.0;
    double longitude = 0.0;
};

class Backend : public QObject {
    Q_OBJECT
    Q_PROPERTY(Position position READ position NOTIFY positionChanged)

public:
    Position position() const { return m_position; }

    void printPosition() const {
        // Direct access - just like any struct
        qDebug() << "Lat:" << m_position.latitude;
        qDebug() << "Lon:" << m_position.longitude;
    }

    double getLatitude() const {
        return m_position.latitude;  // Simple member access
    }

signals:
    void positionChanged();

private:
    Position m_position;
};
```

#### 2. Modifying Members (Must Emit NOTIFY!)

The key is **you must emit the NOTIFY signal** after modifying gadget members:

```cpp
class Backend : public QObject {
    Q_OBJECT
    Q_PROPERTY(Position position READ position NOTIFY positionChanged)

public:
    void setPosition(double lat, double lon) {
        // Direct member access to modify
        m_position.latitude = lat;
        m_position.longitude = lon;

        // CRITICAL: Emit signal so QML knows it changed!
        emit positionChanged();
    }

    void updateLatitude(double lat) {
        m_position.latitude = lat;  // Modify just one field
        emit positionChanged();      // Still emit for whole gadget
    }

private:
    Position m_position;
};
```

### Pattern Options for Modification

#### Pattern 1: Direct Modification (Simple)

```cpp
struct VehicleData {
    Q_GADGET
    Q_PROPERTY(double speed MEMBER speed)
    Q_PROPERTY(double heading MEMBER heading)
    Q_PROPERTY(bool isReverse MEMBER isReverse)

    double speed = 0.0;
    double heading = 0.0;
    bool isReverse = false;
};

class Backend : public QObject {
    Q_OBJECT
    Q_PROPERTY(VehicleData vehicle READ vehicle NOTIFY vehicleChanged)

public:
    VehicleData vehicle() const { return m_vehicle; }

    void updateSpeed(double speed) {
        m_vehicle.speed = speed;      // ✅ Direct member access
        emit vehicleChanged();
    }

    void updateHeading(double heading) {
        m_vehicle.heading = heading;  // ✅ Direct member access
        emit vehicleChanged();
    }

    void setReverse(bool reverse) {
        m_vehicle.isReverse = reverse;  // ✅ Direct member access
        emit vehicleChanged();
    }

    // Update multiple fields at once
    void updateVehicle(double speed, double heading, bool reverse) {
        m_vehicle.speed = speed;        // ✅ Direct access
        m_vehicle.heading = heading;    // ✅ Direct access
        m_vehicle.isReverse = reverse;  // ✅ Direct access
        emit vehicleChanged();          // Single signal for all changes
    }

signals:
    void vehicleChanged();

private:
    VehicleData m_vehicle;
};
```

#### Pattern 2: Copy-Modify-Replace (Safer with const getter)

```cpp
class Backend : public QObject {
    Q_OBJECT
    Q_PROPERTY(VehicleData vehicle READ vehicle NOTIFY vehicleChanged)

public:
    // Const getter returns copy
    VehicleData vehicle() const { return m_vehicle; }

    void updateSpeed(double speed) {
        // Copy-modify-replace pattern
        VehicleData updated = m_vehicle;  // Copy
        updated.speed = speed;            // Modify copy
        m_vehicle = updated;              // Replace
        emit vehicleChanged();
    }

    // Or using a temporary:
    void updateHeading(double heading) {
        auto v = m_vehicle;
        v.heading = heading;
        m_vehicle = v;
        emit vehicleChanged();
    }

private:
    VehicleData m_vehicle;
};
```

#### Pattern 3: Setter Method (Encapsulation)

```cpp
class Backend : public QObject {
    Q_OBJECT
    Q_PROPERTY(VehicleData vehicle READ vehicle WRITE setVehicle NOTIFY vehicleChanged)

public:
    VehicleData vehicle() const { return m_vehicle; }

    void setVehicle(const VehicleData& data) {
        if (m_vehicle.speed != data.speed ||
            m_vehicle.heading != data.heading ||
            m_vehicle.isReverse != data.isReverse) {
            m_vehicle = data;
            emit vehicleChanged();
        }
    }

    // Convenience methods that modify specific fields
    void updateSpeed(double speed) {
        VehicleData updated = m_vehicle;
        updated.speed = speed;
        setVehicle(updated);  // Use setter (checks for changes)
    }

signals:
    void vehicleChanged();

private:
    VehicleData m_vehicle;
};
```

### Real-World AgOpenGPS Example

```cpp
struct FieldData {
    Q_GADGET
    Q_PROPERTY(QString name MEMBER name)
    Q_PROPERTY(double area MEMBER area)
    Q_PROPERTY(double workedArea MEMBER workedArea)
    Q_PROPERTY(bool isActive MEMBER isActive)

    QString name;
    double area = 0.0;
    double workedArea = 0.0;
    bool isActive = false;
};
Q_DECLARE_METATYPE(FieldData)

struct GPSStatus {
    Q_GADGET
    Q_PROPERTY(double latitude MEMBER latitude)
    Q_PROPERTY(double longitude MEMBER longitude)
    Q_PROPERTY(double altitude MEMBER altitude)
    Q_PROPERTY(int satellites MEMBER satellites)
    Q_PROPERTY(double hdop MEMBER hdop)

    double latitude = 0.0;
    double longitude = 0.0;
    double altitude = 0.0;
    int satellites = 0;
    double hdop = 99.9;
};
Q_DECLARE_METATYPE(GPSStatus)

class Backend : public QObject {
    Q_OBJECT

    Q_PROPERTY(FieldData currentField READ currentField NOTIFY currentFieldChanged)
    Q_PROPERTY(GPSStatus gpsStatus READ gpsStatus NOTIFY gpsStatusChanged)

public:
    FieldData currentField() const { return m_currentField; }
    GPSStatus gpsStatus() const { return m_gpsStatus; }

    // === Field Management ===

    Q_INVOKABLE void openField(const QString& name, double area) {
        // Direct access to modify members
        m_currentField.name = name;
        m_currentField.area = area;
        m_currentField.workedArea = 0.0;
        m_currentField.isActive = true;
        emit currentFieldChanged();
    }

    void addWorkedArea(double additionalArea) {
        // Read current value, modify, signal
        m_currentField.workedArea += additionalArea;
        emit currentFieldChanged();
    }

    void closeField() {
        m_currentField.isActive = false;
        emit currentFieldChanged();
    }

    // === GPS Updates ===

    void updateGPSPosition(double lat, double lon, double alt) {
        // Update multiple fields at once
        m_gpsStatus.latitude = lat;
        m_gpsStatus.longitude = lon;
        m_gpsStatus.altitude = alt;
        // Single signal for all changes
        emit gpsStatusChanged();
    }

    void updateGPSQuality(int sats, double hdop) {
        m_gpsStatus.satellites = sats;
        m_gpsStatus.hdop = hdop;
        emit gpsStatusChanged();
    }

    // === Getters for specific values ===

    Q_INVOKABLE double getCurrentLatitude() const {
        return m_gpsStatus.latitude;  // Direct member access
    }

    Q_INVOKABLE bool isFieldActive() const {
        return m_currentField.isActive;  // Direct member access
    }

    Q_INVOKABLE double getFieldProgress() const {
        if (m_currentField.area <= 0.0) return 0.0;
        return (m_currentField.workedArea / m_currentField.area) * 100.0;
    }

signals:
    void currentFieldChanged();
    void gpsStatusChanged();

private:
    FieldData m_currentField;
    GPSStatus m_gpsStatus;

    // Helper method for GPS updates from NMEA
    void processNMEASentence(const QString& sentence) {
        // ... parse NMEA ...

        // Update gadget members directly
        m_gpsStatus.latitude = parsedLat;
        m_gpsStatus.longitude = parsedLon;
        m_gpsStatus.satellites = parsedSats;
        m_gpsStatus.hdop = parsedHdop;

        emit gpsStatusChanged();
    }
};
```

### Accessing Nested Gadgets

```cpp
struct Position {
    Q_GADGET
    Q_PROPERTY(double latitude MEMBER latitude)
    Q_PROPERTY(double longitude MEMBER longitude)

    double latitude = 0.0;
    double longitude = 0.0;
};

struct VehicleState {
    Q_GADGET
    Q_PROPERTY(Position position MEMBER position)
    Q_PROPERTY(double speed MEMBER speed)

    Position position;  // Nested gadget
    double speed = 0.0;
};

class Backend : public QObject {
    Q_OBJECT
    Q_PROPERTY(VehicleState vehicle READ vehicle NOTIFY vehicleChanged)

public:
    VehicleState vehicle() const { return m_vehicle; }

    void updatePosition(double lat, double lon) {
        // Access nested gadget members
        m_vehicle.position.latitude = lat;   // ✅ Nested access
        m_vehicle.position.longitude = lon;  // ✅ Nested access
        emit vehicleChanged();
    }

    void updateSpeed(double speed) {
        m_vehicle.speed = speed;
        emit vehicleChanged();
    }

    double getLatitude() const {
        return m_vehicle.position.latitude;  // ✅ Nested read
    }

signals:
    void vehicleChanged();

private:
    VehicleState m_vehicle;
};
```

### Common Pitfalls

#### ❌ Forgetting to Emit Signal

```cpp
void updateSpeed(double speed) {
    m_vehicle.speed = speed;
    // ❌ FORGOT: emit vehicleChanged();
    // QML won't update!
}
```

#### ❌ Returning Non-Const Reference (Dangerous)

```cpp
// ❌ BAD: Direct reference breaks encapsulation
VehicleData& vehicle() { return m_vehicle; }

// Problem: External code can modify without signaling
backend.vehicle().speed = 999;  // No signal emitted!
```

#### ✅ Proper Const Correctness

```cpp
// ✅ Good: Return by value or const reference
VehicleData vehicle() const { return m_vehicle; }  // Copy
const VehicleData& vehicle() const { return m_vehicle; }  // Const ref
```

### Summary: Accessing Q_GADGET Members in C++

**Accessing Q_GADGET members in C++:**

1. **Read access:** Just use `.member` - it's a normal struct
   ```cpp
   double lat = m_position.latitude;
   ```

2. **Write access:** Use `.member = value` then emit signal
   ```cpp
   m_position.latitude = 42.0;
   emit positionChanged();
   ```

3. **Nested access:** Chain member access
   ```cpp
   m_vehicle.position.latitude = 42.0;
   ```

4. **Multiple changes:** Modify all, then single emit
   ```cpp
   m_data.field1 = val1;
   m_data.field2 = val2;
   emit dataChanged();  // Once for all changes
   ```

5. **Always emit after modification** - QML needs the signal to update bindings!

**Q_GADGET is just a struct** - no special access needed! The only Qt-specific part is remembering to emit the containing Q_OBJECT's NOTIFY signal after changes.

---

## Computed Properties with Custom Getters

Q_GADGET properties can use `READ` (and `WRITE`) with custom getter/setter functions. The property doesn't need to map to an actual member variable - it can be completely synthesized/computed on demand.

### Computed Properties with READ

```cpp
struct FieldStats {
    Q_GADGET

    // === Properties with custom getters (synthesized data) ===
    Q_PROPERTY(double area READ area)                    // Getter computes from internal data
    Q_PROPERTY(QString areaDisplay READ areaDisplay)     // Formatted for display
    Q_PROPERTY(int progressPercent READ progressPercent) // Computed percentage
    Q_PROPERTY(bool isComplete READ isComplete)          // Derived boolean
    Q_PROPERTY(QString status READ status)               // Synthesized status text

public:
    // === Custom getters - synthesize data ===

    double area() const {
        // Convert internal square meters to user's preferred unit
        if (m_useMetric) {
            return m_areaSquareMeters;  // Return as hectares
        } else {
            return m_areaSquareMeters * 0.000247105;  // Convert to acres
        }
    }

    QString areaDisplay() const {
        // Synthesize formatted string
        if (m_useMetric) {
            return QString("%1 ha").arg(area(), 0, 'f', 2);
        } else {
            return QString("%1 acres").arg(area(), 0, 'f', 2);
        }
    }

    int progressPercent() const {
        // Computed from two internal values
        if (m_areaSquareMeters <= 0.0) return 0;
        return qRound((m_workedAreaSquareMeters / m_areaSquareMeters) * 100.0);
    }

    bool isComplete() const {
        // Derived boolean logic
        return progressPercent() >= 95;  // Consider 95%+ as complete
    }

    QString status() const {
        // Synthesized status based on internal state
        if (!m_isActive) return "Inactive";
        if (isComplete()) return "Complete";
        if (progressPercent() > 0) return "In Progress";
        return "Not Started";
    }

    // === Internal data (not exposed) ===
    double m_areaSquareMeters = 0.0;
    double m_workedAreaSquareMeters = 0.0;
    bool m_isActive = false;
    bool m_useMetric = false;

    // === Setters for internal data ===
    void setArea(double sqm) { m_areaSquareMeters = sqm; }
    void setWorkedArea(double sqm) { m_workedAreaSquareMeters = sqm; }
    void setActive(bool active) { m_isActive = active; }
    void setUseMetric(bool metric) { m_useMetric = metric; }
};
Q_DECLARE_METATYPE(FieldStats)
```

**In QML:**
```qml
Label {
    text: Backend.fieldStats.areaDisplay  // "125.50 acres" or "50.75 ha"
}

ProgressBar {
    value: Backend.fieldStats.progressPercent / 100.0  // 0.0 to 1.0
}

Label {
    text: Backend.fieldStats.status  // "In Progress"
    color: Backend.fieldStats.isComplete ? "green" : "orange"
}
```

### Computed Properties with READ/WRITE

```cpp
struct GPSPosition {
    Q_GADGET

    // === Property with READ/WRITE - no backing member! ===
    Q_PROPERTY(double latitudeDegrees READ latitudeDegrees WRITE setLatitudeDegrees)
    Q_PROPERTY(double longitudeDegrees READ longitudeDegrees WRITE setLongitudeDegrees)

    // === Formatted properties (read-only computed) ===
    Q_PROPERTY(QString latitudeFormatted READ latitudeFormatted)
    Q_PROPERTY(QString longitudeFormatted READ longitudeFormatted)
    Q_PROPERTY(QString coordinatesFormatted READ coordinatesFormatted)

public:
    // === Getters - convert from internal radians to degrees ===
    double latitudeDegrees() const {
        return qRadiansToDegrees(m_latitudeRadians);
    }

    double longitudeDegrees() const {
        return qRadiansToDegrees(m_longitudeRadians);
    }

    // === Setters - convert from degrees to internal radians ===
    void setLatitudeDegrees(double degrees) {
        m_latitudeRadians = qDegreesToRadians(degrees);
    }

    void setLongitudeDegrees(double degrees) {
        m_longitudeRadians = qDegreesToRadians(degrees);
    }

    // === Formatted getters - synthesize display strings ===
    QString latitudeFormatted() const {
        double deg = latitudeDegrees();
        char dir = deg >= 0 ? 'N' : 'S';
        return QString("%1° %2").arg(std::abs(deg), 0, 'f', 6).arg(dir);
    }

    QString longitudeFormatted() const {
        double deg = longitudeDegrees();
        char dir = deg >= 0 ? 'E' : 'W';
        return QString("%1° %2").arg(std::abs(deg), 0, 'f', 6).arg(dir);
    }

    QString coordinatesFormatted() const {
        return QString("%1, %2").arg(latitudeFormatted(), longitudeFormatted());
    }

private:
    // Internal storage in radians (better for calculations)
    double m_latitudeRadians = 0.0;
    double m_longitudeRadians = 0.0;
};
Q_DECLARE_METATYPE(GPSPosition)
```

**Usage:**
```cpp
GPSPosition pos;
pos.setLatitudeDegrees(42.3601);   // QML or C++ sets in degrees
pos.setLongitudeDegrees(-71.0589);

// Internal storage is radians, getter converts back
qDebug() << pos.latitudeDegrees();  // 42.3601

// Formatted output synthesized on demand
qDebug() << pos.coordinatesFormatted();  // "42.360100° N, -71.058900° W"
```

```qml
TextField {
    text: gpsPos.latitudeDegrees.toString()  // Read as degrees
    onEditingFinished: {
        gpsPos.latitudeDegrees = parseFloat(text)  // Write as degrees
    }
}

Label {
    text: gpsPos.coordinatesFormatted  // Formatted display
}
```

### Complex Synthesis: Aggregating Multiple Sources

```cpp
struct VehicleStatus {
    Q_GADGET

    // === Synthesized properties (no direct storage) ===
    Q_PROPERTY(QString speedDisplay READ speedDisplay)
    Q_PROPERTY(QString statusSummary READ statusSummary)
    Q_PROPERTY(QColor statusColor READ statusColor)
    Q_PROPERTY(bool hasWarnings READ hasWarnings)
    Q_PROPERTY(QStringList warnings READ warnings)

public:
    // === Getters synthesize from multiple internal values ===

    QString speedDisplay() const {
        // Synthesize with unit conversion and formatting
        if (m_useMetric) {
            return QString("%1 km/h").arg(m_speedMPS * 3.6, 0, 'f', 1);
        } else {
            return QString("%1 mph").arg(m_speedMPS * 2.23694, 0, 'f', 1);
        }
    }

    QString statusSummary() const {
        // Synthesize complex status from multiple conditions
        QStringList parts;

        if (m_isMoving) {
            parts << "Moving";
        } else {
            parts << "Stopped";
        }

        if (m_gpsFixQuality >= 4) {
            parts << "RTK";
        } else if (m_gpsFixQuality >= 2) {
            parts << "DGPS";
        } else if (m_gpsFixQuality >= 1) {
            parts << "GPS";
        } else {
            parts << "No Fix";
        }

        if (m_sectionsOn > 0) {
            parts << QString("%1 sections on").arg(m_sectionsOn);
        }

        return parts.join(" • ");
    }

    QColor statusColor() const {
        // Synthesize color based on conditions
        if (!hasWarnings()) return QColor("green");
        if (m_speedMPS > m_maxSpeedMPS) return QColor("red");
        if (m_gpsFixQuality < 2) return QColor("orange");
        return QColor("yellow");
    }

    bool hasWarnings() const {
        // Computed from multiple conditions
        return m_speedMPS > m_maxSpeedMPS ||
               m_gpsFixQuality < 2 ||
               m_gpsSatellites < 6 ||
               m_batteryPercent < 20;
    }

    QStringList warnings() const {
        // Synthesize list of warning messages
        QStringList list;

        if (m_speedMPS > m_maxSpeedMPS) {
            list << "Speed exceeds limit";
        }
        if (m_gpsFixQuality < 2) {
            list << "Poor GPS quality";
        }
        if (m_gpsSatellites < 6) {
            list << QString("Low satellites: %1").arg(m_gpsSatellites);
        }
        if (m_batteryPercent < 20) {
            list << QString("Low battery: %1%").arg(m_batteryPercent);
        }

        return list;
    }

    // === Internal data ===
    double m_speedMPS = 0.0;
    double m_maxSpeedMPS = 5.0;
    int m_gpsFixQuality = 0;
    int m_gpsSatellites = 0;
    int m_batteryPercent = 100;
    int m_sectionsOn = 0;
    bool m_isMoving = false;
    bool m_useMetric = true;

    // === Setters ===
    void updateSpeed(double mps) { m_speedMPS = mps; m_isMoving = mps > 0.1; }
    void updateGPS(int quality, int sats) { m_gpsFixQuality = quality; m_gpsSatellites = sats; }
    void updateSections(int on) { m_sectionsOn = on; }
    void setBattery(int percent) { m_batteryPercent = percent; }
};
Q_DECLARE_METATYPE(VehicleStatus)
```

**In QML:**
```qml
Rectangle {
    color: Backend.vehicleStatus.statusColor  // Synthesized color

    Label {
        text: Backend.vehicleStatus.speedDisplay  // "12.5 mph"
    }

    Label {
        text: Backend.vehicleStatus.statusSummary
        // "Moving • RTK • 3 sections on"
    }

    Column {
        visible: Backend.vehicleStatus.hasWarnings
        Repeater {
            model: Backend.vehicleStatus.warnings
            Label {
                text: "⚠ " + modelData
                color: "red"
            }
        }
    }
}
```

### Pattern: Different Representations of Same Data

```cpp
struct TimeData {
    Q_GADGET

    // === Multiple computed views of the same internal timestamp ===
    Q_PROPERTY(qint64 unixTimestamp READ unixTimestamp WRITE setUnixTimestamp)
    Q_PROPERTY(QString isoString READ isoString)
    Q_PROPERTY(QString displayString READ displayString)
    Q_PROPERTY(QString relativeString READ relativeString)
    Q_PROPERTY(int secondsAgo READ secondsAgo)
    Q_PROPERTY(bool isRecent READ isRecent)

public:
    qint64 unixTimestamp() const {
        return m_dateTime.toSecsSinceEpoch();
    }

    void setUnixTimestamp(qint64 secs) {
        m_dateTime = QDateTime::fromSecsSinceEpoch(secs);
    }

    QString isoString() const {
        return m_dateTime.toString(Qt::ISODate);
    }

    QString displayString() const {
        return m_dateTime.toString("yyyy-MM-dd hh:mm:ss");
    }

    QString relativeString() const {
        int secs = secondsAgo();
        if (secs < 60) return QString("%1 seconds ago").arg(secs);
        if (secs < 3600) return QString("%1 minutes ago").arg(secs / 60);
        if (secs < 86400) return QString("%1 hours ago").arg(secs / 3600);
        return QString("%1 days ago").arg(secs / 86400);
    }

    int secondsAgo() const {
        return m_dateTime.secsTo(QDateTime::currentDateTime());
    }

    bool isRecent() const {
        return secondsAgo() < 300;  // Within 5 minutes
    }

private:
    QDateTime m_dateTime;
};
Q_DECLARE_METATYPE(TimeData)
```

### Summary: Computed Properties in Q_GADGET

**Q_PROPERTY with READ getter can:**

1. ✅ **Convert units** - Internal meters → Display acres
2. ✅ **Format data** - Double → "125.50 acres"
3. ✅ **Compute from multiple sources** - Progress from area/worked
4. ✅ **Derive booleans** - isComplete from percent
5. ✅ **Synthesize strings** - Status from multiple flags
6. ✅ **Aggregate data** - Summary from multiple properties
7. ✅ **Transform representation** - Radians ↔ Degrees
8. ✅ **Conditional logic** - Different output based on state

**Best practices:**

- Use `READ` for computed/synthesized data that QML needs
- Store data internally in the most efficient format
- Convert/format in getters for QML consumption
- Keep internal members private/C++-only
- Getters should be `const` and side-effect-free

**The getter is called every time QML accesses the property**, so make sure it's efficient. For expensive computations, consider caching the result in a C++-only member and updating it only when source data changes.
