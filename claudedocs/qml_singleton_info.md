# C++ QML singleton with static instance access

**human** (Dec 5, 2025, 10:28 AM)

I need to create a c++ class that is a QML singleton.  It will be registered as a singleton with cmake's qml_add_mdule macro.  I also want to be able to access this same singleton instance from c++ using the MyClass::instance() pattern.  If possible I need the static instance to be created either from C++ code, or from QML, depending on which comes first.

**assistant** (Dec 5, 2025, 10:29 AM)

 This is a great question about QML/C++ integration. The challenge is creating a singleton that works seamlessly from both sides while allowing either to initialize it first.

Here's a clean implementation:

```cpp
// myclass.h
#ifndef MYCLASS_H
#define MYCLASS_H

#include <QObject>
#include <QQmlEngine>
#include <QMutex>

class MyClass : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    // C++ access point
    static MyClass* instance();

    // QML singleton factory - called by QML engine
    static MyClass* create(QQmlEngine* qmlEngine, QJSEngine* jsEngine);

    // Your properties and methods here
    Q_PROPERTY(QString someProperty READ someProperty WRITE setSomeProperty NOTIFY somePropertyChanged)
    
    QString someProperty() const;
    void setSomeProperty(const QString& value);

signals:
    void somePropertyChanged();

private:
    explicit MyClass(QObject* parent = nullptr);
    ~MyClass() override = default;

    // Prevent copying
    MyClass(const MyClass&) = delete;
    MyClass& operator=(const MyClass&) = delete;

    static MyClass* s_instance;
    static QMutex s_mutex;

    QString m_someProperty;
};

#endif // MYCLASS_H
```

```cpp
// myclass.cpp
#include "myclass.h"

MyClass* MyClass::s_instance = nullptr;
QMutex MyClass::s_mutex;

MyClass::MyClass(QObject* parent)
    : QObject(parent)
{
}

MyClass* MyClass::instance()
{
    QMutexLocker locker(&s_mutex);
    if (!s_instance) {
        s_instance = new MyClass();
    }
    return s_instance;
}

MyClass* MyClass::create(QQmlEngine* qmlEngine, QJSEngine* jsEngine)
{
    Q_UNUSED(jsEngine)

    QMutexLocker locker(&s_mutex);
    if (!s_instance) {
        s_instance = new MyClass();
    }

    // Important: tell QML engine not to take ownership
    QQmlEngine::setObjectOwnership(s_instance, QQmlEngine::CppOwnership);
    
    return s_instance;
}

QString MyClass::someProperty() const
{
    return m_someProperty;
}

void MyClass::setSomeProperty(const QString& value)
{
    if (m_someProperty != value) {
        m_someProperty = value;
        emit somePropertyChanged();
    }
}
```

For your CMakeLists.txt, you'd have something like:

```cmake
qt_add_qml_module(myapp
    URI MyApp
    VERSION 1.0
    QML_FILES
        Main.qml
    SOURCES
        myclass.h myclass.cpp
)
```

**Key points:**

1. **`QML_SINGLETON` + `create()` factory**: When you use `QML_SINGLETON`, the QML engine looks for a static `create(QQmlEngine*, QJSEngine*)` function to instantiate the singleton.

2. **Thread safety**: The mutex ensures that whether C++ or QML accesses first, only one instance is ever created.

3. **`CppOwnership`**: This is crucial—it prevents the QML engine from deleting your singleton when it shuts down, which would cause issues if C++ code still holds references.

4. **Usage from QML**:
   ```qml
   import MyApp
   
   Text {
       text: MyClass.someProperty
   }
   ```

5. **Usage from C++**:
   ```cpp
   MyClass::instance()->setSomeProperty("Hello");
   ```

**One caveat**: If you need the singleton to be parented to the QML engine for proper cleanup ordering, you can modify the `create()` function to set the parent when the engine is available, but then you need to be careful about the C++ side accessing it after engine destruction.

**human** (Dec 5, 2025, 10:30 AM)

Looks good.   in the create() method, can you make it so that if QML caused the creation, go ahead and take ownership of the instance, otherwise, tell the qml engine to not take ownership

**assistant** (Dec 5, 2025, 10:31 AM)

 Good thinking—that gives you more flexible lifetime management. Here's the updated implementation:

```cpp
// myclass.h
#ifndef MYCLASS_H
#define MYCLASS_H

#include <QObject>
#include <QQmlEngine>
#include <QMutex>

class MyClass : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    // C++ access point
    static MyClass* instance();

    // QML singleton factory - called by QML engine
    static MyClass* create(QQmlEngine* qmlEngine, QJSEngine* jsEngine);

    // Your properties and methods here
    Q_PROPERTY(QString someProperty READ someProperty WRITE setSomeProperty NOTIFY somePropertyChanged)
    
    QString someProperty() const;
    void setSomeProperty(const QString& value);

signals:
    void somePropertyChanged();

private:
    explicit MyClass(QObject* parent = nullptr);
    ~MyClass() override = default;

    // Prevent copying
    MyClass(const MyClass&) = delete;
    MyClass& operator=(const MyClass&) = delete;

    static MyClass* s_instance;
    static QMutex s_mutex;
    static bool s_createdByCpp;

    QString m_someProperty;
};

#endif // MYCLASS_H
```

```cpp
// myclass.cpp
#include "myclass.h"

MyClass* MyClass::s_instance = nullptr;
QMutex MyClass::s_mutex;
bool MyClass::s_createdByCpp = false;

MyClass::MyClass(QObject* parent)
    : QObject(parent)
{
}

MyClass* MyClass::instance()
{
    QMutexLocker locker(&s_mutex);
    if (!s_instance) {
        s_instance = new MyClass();
        s_createdByCpp = true;
    }
    return s_instance;
}

MyClass* MyClass::create(QQmlEngine* qmlEngine, QJSEngine* jsEngine)
{
    Q_UNUSED(jsEngine)

    QMutexLocker locker(&s_mutex);
    
    if (!s_instance) {
        // QML is creating it first - let QML engine manage lifetime
        s_instance = new MyClass();
        // s_createdByCpp remains false
    } else if (s_createdByCpp) {
        // C++ created it first - don't let QML take ownership
        QQmlEngine::setObjectOwnership(s_instance, QQmlEngine::CppOwnership);
    }
    
    return s_instance;
}

QString MyClass::someProperty() const
{
    return m_someProperty;
}

void MyClass::setSomeProperty(const QString& value)
{
    if (m_someProperty != value) {
        m_someProperty = value;
        emit somePropertyChanged();
    }
}
```

**What changed:**

1. Added `s_createdByCpp` static flag to track who created the instance first.

2. In `instance()`: sets the flag to `true` when C++ creates it.

3. In `create()`: 
   - If QML creates first, the flag stays `false` and QML engine takes default ownership (JavaScriptOwnership).
   - If C++ created first, we explicitly set `CppOwnership` so QML won't delete it.

**Lifetime implications:**

| Created by | Ownership | Destroyed when |
|------------|-----------|----------------|
| QML first | QML Engine | QML engine shuts down |
| C++ first | C++ | You must delete it manually (or never, if it lives for app lifetime) |

If C++ creates it first and you need cleanup, you could add a static `destroy()` method or tie it to `QCoreApplication` lifetime:

```cpp
MyClass* MyClass::instance()
{
    QMutexLocker locker(&s_mutex);
    if (!s_instance) {
        s_instance = new MyClass();
        s_createdByCpp = true;
        
        // Optional: ensure cleanup on app exit
        QObject::connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit,
                         s_instance, []() { delete s_instance; s_instance = nullptr; });
    }
    return s_instance;
}
```

**human** (Dec 5, 2025, 10:31 AM)

Good idea.  Why is the qmutex required?

**assistant** (Dec 5, 2025, 10:32 AM)

 The mutex is there to handle the case where multiple threads might call `instance()` or `create()` simultaneously, which could result in two instances being created (a race condition).

**However**, in practice for most Qt/QML applications, you likely don't need it because:

1. **QML engine operations happen on the main thread.** The `create()` function is called by the QML engine during component loading, which is always on the main/GUI thread.

2. **You typically call `instance()` from the main thread too.** Most Qt application logic runs on the main thread, especially anything interacting with QML.

3. **If you do access from other threads**, you'd already need to be careful about thread safety for the object's *properties and methods* anyway—the mutex here only protects *creation*, not subsequent usage.

**So you can simplify to:**

```cpp
// myclass.cpp
#include "myclass.h"

MyClass* MyClass::s_instance = nullptr;
bool MyClass::s_createdByCpp = false;

MyClass::MyClass(QObject* parent)
    : QObject(parent)
{
}

MyClass* MyClass::instance()
{
    if (!s_instance) {
        s_instance = new MyClass();
        s_createdByCpp = true;
        
        QObject::connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit,
                         s_instance, []() { delete s_instance; s_instance = nullptr; });
    }
    return s_instance;
}

MyClass* MyClass::create(QQmlEngine* qmlEngine, QJSEngine* jsEngine)
{
    Q_UNUSED(qmlEngine)
    Q_UNUSED(jsEngine)
    
    if (!s_instance) {
        s_instance = new MyClass();
    } else if (s_createdByCpp) {
        QQmlEngine::setObjectOwnership(s_instance, QQmlEngine::CppOwnership);
    }
    
    return s_instance;
}
```

And remove the mutex from the header.

**When you would want the mutex:**

- If you have worker threads that might access `MyClass::instance()` before the main thread does
- If you're being defensive about future code changes
- If this is library code where you can't control how consumers use it

For a typical application where you control the code, the mutex is unnecessary overhead.

**human** (Dec 5, 2025, 10:32 AM)

What is the purpose of the  Q_UNUSED macro?

**assistant** (Dec 5, 2025, 10:32 AM)

 `Q_UNUSED` is a Qt macro that suppresses compiler warnings about unused parameters.

Without it, if you compile with warnings enabled (like `-Wunused-parameter`), you'd get warnings like:

```
warning: unused parameter 'qmlEngine' [-Wunused-parameter]
warning: unused parameter 'jsEngine' [-Wunused-parameter]
```

**Alternatives:**

1. **Omit the parameter names entirely** (modern C++ way):

```cpp
MyClass* MyClass::create(QQmlEngine*, QJSEngine*)
{
    // ...
}
```

2. **Use `[[maybe_unused]]`** (C++17):

```cpp
MyClass* MyClass::create([[maybe_unused]] QQmlEngine* qmlEngine, 
                         [[maybe_unused]] QJSEngine* jsEngine)
{
    // ...
}
```

3. **Cast to void** (old C trick that `Q_UNUSED` essentially wraps):

```cpp
(void)qmlEngine;
(void)jsEngine;
```

**My recommendation:** Just omit the parameter names since you're not using them. It's cleaner and doesn't require any macros:

```cpp
MyClass* MyClass::create(QQmlEngine*, QJSEngine*)
{
    if (!s_instance) {
        s_instance = new MyClass();
    } else if (s_createdByCpp) {
        QQmlEngine::setObjectOwnership(s_instance, QQmlEngine::CppOwnership);
    }
    
    return s_instance;
}
```

`Q_UNUSED` is most useful when you *sometimes* use the parameter (e.g., only in debug builds or conditionally) and want to keep the name visible for documentation purposes.
