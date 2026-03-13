# QML Module Organization and Backend Plugins

## Problem: QML Recompilation on Backend Changes

When backend.h changes in a monolithic QML module, all QML files get recompiled because:

1. `backend.h` → `qmltyperegistrations.cpp` (includes all QML-exposed headers)
2. Compiled QML cache files have dependencies on C++ types
3. CMake sees dependency chain and rebuilds everything

**Symptom:** Changing a single line in backend.h triggers 30-60 second rebuilds of 150+ QML files.

## Solution: Separate Backend QML Module

Organize the project into separate QML modules:
- **Backend module** (C++ singletons) - `AOG.Backend`
- **UI module** (QML files) - `AOG`

This creates a clean separation and enables faster iteration during development.

---

## Project Structure Options

### Option 1: Static Backend (Current - Single Binary)

```
QtAgOpenGPS/
├── CMakeLists.txt (main)
├── backend/
│   ├── CMakeLists.txt
│   ├── backend.h/cpp
│   ├── settingsmanager.h/cpp
│   ├── cvehicle.h/cpp
│   └── agioservice.h/cpp
├── qml/
│   ├── CMakeLists.txt
│   ├── MainWindow.qml
│   └── config/...
└── src/
    ├── CMakeLists.txt
    ├── main.cpp
    └── formgps.cpp
```

**Backend as STATIC library:**
```cmake
# backend/CMakeLists.txt
qt_add_library(QtAgOpenGPSBackend STATIC)

qt_add_qml_module(QtAgOpenGPSBackend
    URI AOG.Backend
    VERSION 1.0

    SOURCES
        backend.h backend.cpp
        settingsmanager.h settingsmanager.cpp
        cvehicle.h cvehicle.cpp
)
```

**Result:**
- ✅ Single binary deployment
- ✅ Faster startup (no plugin loading)
- ✅ Simpler build
- ❌ Changing backend.h rebuilds QML files (if QML compiled)
- ❌ Slower development iteration

### Option 2: Shared Backend Plugin (Development-Friendly)

**Backend as SHARED library:**
```cmake
# backend/CMakeLists.txt
qt_add_library(QtAgOpenGPSBackend SHARED)

qt_add_qml_module(QtAgOpenGPSBackend
    URI AOG.Backend
    VERSION 1.0
    OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/AOG/Backend

    SOURCES
        backend.h backend.cpp
        settingsmanager.h settingsmanager.cpp
        cvehicle.h cvehicle.cpp
)
```

**Build output:**
```
build/
├── QtAgOpenGPS                              # Main executable
└── AOG/Backend/
    ├── libQtAgOpenGPSBackendplugin.so       # Backend plugin
    └── qmldir
```

**Result:**
- ✅ **Changing backend.h only rebuilds backend.so** (3-5 seconds)
- ✅ QML files NOT recompiled
- ✅ Fast development iteration
- ❌ Must deploy .so file with executable
- ❌ Slightly slower startup

---

## Build Configuration Comparison

| Configuration | Backend | QML | Rebuild on backend.h | Build Time | Use Case |
|--------------|---------|-----|---------------------|------------|----------|
| **Development** | SHARED | LOCAL_QML=ON | Backend .so only | ~3-5s | Daily development |
| **Testing** | SHARED | Compiled | Backend .so only | ~10-15s | Test compiled QML |
| **Production** | STATIC | Compiled | Everything | ~30-60s | Release builds |

---

## Complete CMake Setup

### Root CMakeLists.txt

```cmake
# CMakeLists.txt (root)
cmake_minimum_required(VERSION 3.22)

project(QtAgOpenGPS VERSION 0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS
    Core Gui Quick Network Multimedia SerialPort
)

qt_policy(SET QTP0001 NEW)
qt_policy(SET QTP0004 NEW)

qt_standard_project_setup(REQUIRES 6.8)

# QML import paths for Qt Creator
set(QML_IMPORT_PATH
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}
    CACHE STRING "Import paths for Qt Creator"
    FORCE
)

# Build options
option(LOCAL_QML "Load QML from disk (dev mode)" OFF)
option(BACKEND_SHARED "Build backend as shared library plugin" OFF)

# Add subdirectories
add_subdirectory(backend)
add_subdirectory(qml)
add_subdirectory(src)
```

### Backend Module CMakeLists.txt

```cmake
# backend/CMakeLists.txt

# Choose library type based on option
if(BACKEND_SHARED)
    qt_add_library(QtAgOpenGPSBackend SHARED)
    message(STATUS "Backend: SHARED plugin (fast dev iteration)")
else()
    qt_add_library(QtAgOpenGPSBackend STATIC)
    message(STATUS "Backend: STATIC library (single binary)")
endif()

qt_add_qml_module(QtAgOpenGPSBackend
    URI AOG.Backend
    VERSION 1.0
    OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/AOG/Backend

    SOURCES
        # Core backend
        backend.h backend.cpp

        # Settings
        settingsmanager.h settingsmanager.cpp
        settingsmanager_macros.h
        settingsmanager_properties.h
        settingsmanager_members.h

        # Singletons
        cvehicle.h cvehicle.cpp
        agioservice.h agioservice.cpp
        ctrack.h ctrack.cpp

        # Other backend classes
        aogrenderer.h aogrenderer.cpp
        ctraffic.h ctraffic.cpp
        pgnparser.h pgnparser.cpp
        ntripworker.h ntripworker.cpp
        serialworker.h serialworker.cpp
)

target_include_directories(QtAgOpenGPSBackend PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(QtAgOpenGPSBackend PUBLIC
    Qt6::Core
    Qt6::Quick
    Qt6::Network
    Qt6::SerialPort
    Qt6::Multimedia
)

set_target_properties(QtAgOpenGPSBackend PROPERTIES
    AUTOMOC ON
)
```

### QML Module CMakeLists.txt

```cmake
# qml/CMakeLists.txt

if(NOT LOCAL_QML)
    # Production: Compile QML into binary
    qt_add_qml_module(QtAgOpenGPS_QmlModule
        URI AOG
        VERSION 1.0
        OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/AOG

        # Import backend module
        IMPORTS AOG.Backend

        QML_FILES
            MainWindow.qml
            App.qml
            config/Config.qml
            field/FieldData.qml
            # ... all your QML files

        RESOURCES
            ../images/icon.png
            ../shaders/shader.vert
            ../sounds/beep.wav
    )
else()
    # Development: Load QML from disk
    message(STATUS "LOCAL_QML: QML files loaded from disk at runtime")
    add_custom_target(QmlFiles SOURCES
        MainWindow.qml
        App.qml
        # ... (for IDE project tree)
    )
endif()
```

### Main Executable CMakeLists.txt

```cmake
# src/CMakeLists.txt

qt_add_executable(QtAgOpenGPS
    main.cpp
    formgps.h formgps.cpp
    formgps_opengl.cpp
    formgps_position.cpp
    formgps_sections.cpp

    # Core classes (not QML-exposed)
    classes/cabline.h classes/cabline.cpp
    classes/cabcurve.h classes/cabcurve.cpp
    classes/cboundary.h classes/cboundary.cpp
    # ... etc
)

target_link_libraries(QtAgOpenGPS PRIVATE
    QtAgOpenGPSBackend              # Backend module
    Qt6::Core
    Qt6::Quick
    Qt6::Widgets
    Qt6::Network
)

if(NOT LOCAL_QML)
    target_link_libraries(QtAgOpenGPS PRIVATE
        QtAgOpenGPS_QmlModuleplugin # QML module (if compiled)
    )
endif()

set_target_properties(QtAgOpenGPS PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
```

### Main Application Code

```cpp
// src/main.cpp
#include <QApplication>
#include <QQmlApplicationEngine>
#include <QCoreApplication>

// Auto-registration function from backend module
extern void qml_register_types_AOG_Backend();

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // For SHARED backend: Register types explicitly
    // For STATIC backend: Also works (types compiled in)
    qml_register_types_AOG_Backend();

    QQmlApplicationEngine engine;

    // Add import path for plugin discovery
    engine.addImportPath(QCoreApplication::applicationDirPath());

#ifdef LOCAL_QML
    // Development: Load QML from source directory
    const QUrl url(QStringLiteral("qml/MainWindow.qml"));
    engine.load(url);
#else
    // Production: Load from compiled module
    engine.loadFromModule("AOG", "MainWindow");
#endif

    return app.exec();
}
```

### QML Usage

```qml
// qml/MainWindow.qml
import QtQuick
import QtQuick.Controls
import AOG.Backend 1.0  // Import the backend module

Window {
    visible: true

    Label {
        text: Backend.someProperty  // Access singleton from backend module
    }

    Button {
        onClicked: SettingsManager.someSetting = 42
    }

    Component.onCompleted: {
        console.log("Vehicle:", CVehicle.someProperty)
    }
}
```

---

## Development Workflows

### Fast Development Iteration

```bash
# Initial setup
cmake -B build -S . -DLOCAL_QML=ON -DBACKEND_SHARED=ON
cmake --build build

# Daily workflow
# 1. Edit backend.h
cmake --build build  # Rebuilds backend.so only (~3-5 seconds)

# 2. Edit QML files
# Just reload app - no rebuild needed!

# 3. Edit main C++ code (formgps.cpp)
cmake --build build  # Rebuilds changed files only
```

### Testing Build

```bash
# Test with compiled QML but still plugin backend
cmake -B build -S . -DLOCAL_QML=OFF -DBACKEND_SHARED=ON
cmake --build build
./build/QtAgOpenGPS
```

### Production Build

```bash
# Single binary release
cmake -B build -S . -DLOCAL_QML=OFF -DBACKEND_SHARED=OFF
cmake --build build

# Result: Single QtAgOpenGPS executable with everything compiled in
```

---

## Deployment

### Development (SHARED backend)

```
Deployment/
├── QtAgOpenGPS                    # Main executable
└── AOG/
    └── Backend/
        ├── libQtAgOpenGPSBackendplugin.so
        └── qmldir
```

### Production (STATIC backend)

```
Deployment/
└── QtAgOpenGPS                    # Single executable (all-in-one)
```

---

## Compile Time Analysis

### Scenario: Change backend.h

**STATIC backend + Compiled QML (Current):**
```
Files rebuilt:
- backend.cpp
- qmltyperegistrations.cpp
- 150+ QML cache files
- Relink executable

Time: 30-60 seconds
```

**SHARED backend + Compiled QML:**
```
Files rebuilt:
- backend.cpp
- Relink backend.so

QML files: NOT rebuilt
Executable: NOT relinked

Time: 3-5 seconds
```

**SHARED backend + LOCAL_QML:**
```
Files rebuilt:
- backend.cpp
- Relink backend.so

QML files: NOT compiled (loaded from disk)
Executable: NOT relinked

Time: 3-5 seconds
```

### Scenario: Change QML file

**Compiled QML:**
```
Files rebuilt:
- Changed QML cache file
- Relink executable

Time: 5-10 seconds
```

**LOCAL_QML:**
```
Files rebuilt: NONE
Action: Reload application

Time: 0 seconds (instant)
```

---

## Recommended Setup

### For QtAgOpenGPS Project

**Development (recommended):**
```cmake
-DLOCAL_QML=ON -DBACKEND_SHARED=ON
```
- Edit backend.h → 3-5 second rebuild
- Edit QML → instant (just reload)
- Best iteration speed

**Production:**
```cmake
-DLOCAL_QML=OFF -DBACKEND_SHARED=OFF
```
- Single binary
- Everything compiled in
- Optimal runtime performance

---

## LOCAL_QML Option (You Already Have This!)

Your project already supports LOCAL_QML mode (CMakeLists.txt lines 177-183).

**Enable it:**
```bash
cmake -B build -S . -DLOCAL_QML=ON
cmake --build build
```

**Benefits:**
- ✅ QML files NOT compiled during build
- ✅ Edit QML and reload - no rebuild
- ✅ Changing backend.h doesn't trigger QML recompilation
- ✅ Fastest development iteration

**Trade-off:**
- QML errors only detected at runtime (not compile-time)

---

## Migration Steps

### Step 1: Create Backend Module

```bash
mkdir backend
mv backend.h backend.cpp backend/
mv classes/settingsmanager.* backend/
mv classes/cvehicle.* backend/
mv classes/agioservice.* backend/
# ... move other backend files
```

### Step 2: Create backend/CMakeLists.txt

(Use template from above)

### Step 3: Update Root CMakeLists.txt

```cmake
# Add option
option(BACKEND_SHARED "Build backend as shared library" OFF)

# Add subdirectory
add_subdirectory(backend)
```

### Step 4: Update QML Imports

```qml
// Was:
import AOG 1.0

// Now:
import AOG.Backend 1.0
```

### Step 5: Test

```bash
# Development mode
cmake -B build -S . -DBACKEND_SHARED=ON
cmake --build build
./build/QtAgOpenGPS

# Production mode
cmake -B build -S . -DBACKEND_SHARED=OFF
cmake --build build
./build/QtAgOpenGPS
```

---

## Advanced: Multiple Backend Modules

For larger projects, you can further organize into domain modules:

```
backend/
├── core/           # AOG.Core - Backend, SettingsManager
│   └── CMakeLists.txt (URI AOG.Core)
├── gps/            # AOG.GPS - GPS, NTRIP, NMEA
│   └── CMakeLists.txt (URI AOG.GPS)
├── vehicle/        # AOG.Vehicle - CVehicle, CTool
│   └── CMakeLists.txt (URI AOG.Vehicle)
└── guidance/       # AOG.Guidance - CABLine, CCurve
    └── CMakeLists.txt (URI AOG.Guidance)
```

Each module can be SHARED or STATIC independently.

**QML usage:**
```qml
import AOG.Core 1.0
import AOG.GPS 1.0
import AOG.Vehicle 1.0

Window {
    Label { text: Backend.status }
    Label { text: GPSService.latitude.toString() }
    Label { text: VehicleInterface.speed.toString() }
}
```

---

## Summary

**Problem:** Changing backend.h causes all QML files to rebuild (30-60 seconds)

**Solutions:**

1. **Quick fix** (already available): Use `-DLOCAL_QML=ON`
   - QML not compiled
   - Changing backend.h doesn't trigger QML rebuild

2. **Best fix**: Separate backend module as SHARED library
   - Changing backend.h only rebuilds backend.so (3-5 seconds)
   - QML files independent

3. **Recommended**: Combine both for development
   - `-DLOCAL_QML=ON -DBACKEND_SHARED=ON`
   - Edit backend.h → 3-5 second rebuild
   - Edit QML → instant reload
   - **Fastest iteration possible**

**For production**: Use STATIC backend + compiled QML for single-binary deployment.

---

## Forcing Early Singleton Instantiation

When using Qt 6's automatic registration via `QML_ELEMENT` and `QML_SINGLETON`, singletons are instantiated lazily (when first accessed). To force early instantiation at application startup:

### Option 1: Component.onCompleted

Access singletons in your root QML component's `Component.onCompleted`:

```qml
// In App.qml or MainWindow.qml
import QtQuick
import AOG.Backend 1.0

Item {
    Component.onCompleted: {
        // Simply reference them to trigger instantiation
        AgIOService.toString()
        VehicleInterface.toString()
        SettingsManager.toString()
        TracksInterface.toString()

        // Or call actual initialization methods if needed
        // AgIOService.startListening()
    }

    // ... rest of your QML
}
```

### Option 2: Property Bindings (Even Earlier)

Force instantiation during component creation using readonly property bindings:

```qml
import QtQuick
import AOG.Backend 1.0

Item {
    // These get evaluated immediately when the component is created
    readonly property var _forceAgIO: AgIOService
    readonly property var _forceVehicle: VehicleInterface
    readonly property var _forceSettings: SettingsManager
    readonly property var _forceTracks: TracksInterface

    // The _ prefix is a convention indicating "private/unused"
    // These properties exist solely to force instantiation
}
```

**When to use:**
- Services that need to start listening (like AgIOService UDP sockets) immediately
- Singletons that perform initialization tasks
- Classes that other components depend on

**Benefits:**
- Clean and declarative
- Works perfectly with automatic QML registration
- No manual C++ registration code needed
- Singletons created at predictable times during startup
