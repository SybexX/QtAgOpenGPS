// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
//
// main
#include "formgps.h"
#include "backend.h"
#include <QApplication>
#include <QCoreApplication>
#include <QLabel>
#include "classes/settingsmanager.h"  // MASSIVE MIGRATION: New SettingsManager
#include "aogrenderer.h"
#include "classes/agioservice.h"      // For auto-registration + C++ usage
#include "classes/ctrack.h"           // For auto-registration + C++ usage
#include "classes/cvehicle.h"         // For auto-registration + C++ usage
#include "classes/pgnparser.h"        // Phase 6.0.21: For ParsedData metatype registration
#include <QProcess>
#include <QSysInfo>
#include <QTranslator> //for translations
#include <QtQml/QQmlEngine>
#include <QtQml/QJSEngine>
#include <QtQml/qqmlregistration.h>
#include <QLoggingCategory>
#include <QIcon>
#include <QPermissions>
#include <QTimer>
#ifdef Q_OS_ANDROID
#include <QJniObject>
#include <QJniEnvironment>
#endif

Q_LOGGING_CATEGORY (mainLog, "main.qtagopengps")

QLabel *grnPixelsWindow;
QLabel *overlapPixelsWindow;

QString findIconPath() {
    QString appDir = QCoreApplication::applicationDirPath();

#ifdef Q_OS_WIN
    return appDir + "/icons/icon.ico";
#else
    return appDir + "/icons/64x64/icon.png";
#endif
}

int main(int argc, char *argv[])
{
    qputenv("QSG_RENDER_LOOP", "threaded");

#ifdef  Q_OS_ANDROID
    QNativeInterface::QAndroidApplication::runOnAndroidMainThread([]() {
        QJniObject activity = QNativeInterface::QAndroidApplication::context();
        if (activity.isValid()) {

            //Keep screen on while app is running
            QJniObject window = activity.callObjectMethod("getWindow", "()Landroid/view/Window;");
            if (window.isValid()) {
                const int FLAG_KEEP_SCREEN_ON = 128;
                window.callMethod<void>("addFlags", "(I)V", FLAG_KEEP_SCREEN_ON);
            }

            // Acquire wake lock to keep CPU running in background
            QJniObject serviceName = QJniObject::getStaticObjectField(
                "android/content/Context",
                "POWER_SERVICE",
                "Ljava/lang/String;");

            QJniObject powerManager = activity.callObjectMethod(
                "getSystemService",
                "(Ljava/lang/String;)Ljava/lang/Object;",
                serviceName.object());

            if (powerManager.isValid()) {
                // Check if battery optimization is disabled
                QJniObject packageName = activity.callObjectMethod(
                    "getPackageName",
                    "()Ljava/lang/String;");

                bool isIgnoringBatteryOptimizations = powerManager.callMethod<jboolean>(
                    "isIgnoringBatteryOptimizations",
                    "(Ljava/lang/String;)Z",
                    packageName.object<jstring>());

                if (!isIgnoringBatteryOptimizations) {
                    qDebug(mainLog) << "Battery optimization is enabled - opening settings for user to disable it";

                    // Open battery optimization settings
                    QJniObject intent("android/content/Intent", "(Ljava/lang/String;)V",
                        QJniObject::fromString("android.settings.REQUEST_IGNORE_BATTERY_OPTIMIZATIONS").object());

                    QJniObject uri = QJniObject::callStaticObjectMethod(
                        "android/net/Uri",
                        "parse",
                        "(Ljava/lang/String;)Landroid/net/Uri;",
                        QJniObject::fromString("package:org.qtagopengps.qtagopengps").object());

                    intent.callObjectMethod("setData", "(Landroid/net/Uri;)Landroid/content/Intent;", uri.object());

                    activity.callMethod<void>("startActivity", "(Landroid/content/Intent;)V", intent.object());
                    qDebug(mainLog) << "Battery optimization settings opened";
                } else {
                    qDebug(mainLog) << "Battery optimization already disabled - app can run unrestricted";
                }

                // PARTIAL_WAKE_LOCK = 1 (keeps CPU running but allows screen to turn off)
                QJniObject tag = QJniObject::fromString("QtAgOpenGPS:WakeLock");
                QJniObject wakeLock = powerManager.callObjectMethod(
                    "newWakeLock",
                    "(ILjava/lang/String;)Landroid/os/PowerManager$WakeLock;",
                    1, // PARTIAL_WAKE_LOCK
                    tag.object<jstring>());

                if (wakeLock.isValid()) {
                    wakeLock.callMethod<void>("acquire", "()V");
                    qDebug(mainLog) << "Wake lock acquired - CPU will stay active in background";
                }
            }

            // Create and show a persistent notification to indicate the app is running
            // This helps prevent Android from killing the app
            QJniObject notificationService = QJniObject::getStaticObjectField(
                "android/content/Context",
                "NOTIFICATION_SERVICE",
                "Ljava/lang/String;");

            QJniObject notificationManager = activity.callObjectMethod(
                "getSystemService",
                "(Ljava/lang/String;)Ljava/lang/Object;",
                notificationService.object());

            if (notificationManager.isValid()) {
                // Check Android version
                jint sdkInt = QJniObject::getStaticField<jint>("android/os/Build$VERSION", "SDK_INT");

                // Create notification channel for Android 8.0+ (API 26+)
                if (sdkInt >= 26) {
                    QJniObject channelId = QJniObject::fromString("qtagopengps_service");
                    QJniObject channelName = QJniObject::fromString("QtAgOpenGPS Service");
                    jint importance = 2; // IMPORTANCE_LOW - no sound

                    QJniObject channel("android/app/NotificationChannel",
                        "(Ljava/lang/String;Ljava/lang/CharSequence;I)V",
                        channelId.object<jstring>(),
                        channelName.object<jstring>(),
                        importance);

                    QJniObject description = QJniObject::fromString("Shows when QtAgOpenGPS is running");
                    channel.callMethod<void>("setDescription", "(Ljava/lang/String;)V",
                        description.object<jstring>());

                    notificationManager.callMethod<void>("createNotificationChannel",
                        "(Landroid/app/NotificationChannel;)V",
                        channel.object());
                }

                // Create notification builder
                QJniObject builder;
                if (sdkInt >= 26) {
                    QJniObject channelId = QJniObject::fromString("qtagopengps_service");
                    builder = QJniObject("android/app/Notification$Builder",
                        "(Landroid/content/Context;Ljava/lang/String;)V",
                        activity.object(),
                        channelId.object<jstring>());
                } else {
                    builder = QJniObject("android/app/Notification$Builder",
                        "(Landroid/content/Context;)V",
                        activity.object());
                }

                // Set notification content
                QJniObject title = QJniObject::fromString("QtAgOpenGPS Running");
                QJniObject text = QJniObject::fromString("GPS guidance active");

                builder.callObjectMethod("setContentTitle",
                    "(Ljava/lang/CharSequence;)Landroid/app/Notification$Builder;",
                    title.object());
                builder.callObjectMethod("setContentText",
                    "(Ljava/lang/CharSequence;)Landroid/app/Notification$Builder;",
                    text.object());

                // Get the application icon from the manifest
                QJniObject appInfo = activity.callObjectMethod("getApplicationInfo",
                    "()Landroid/content/pm/ApplicationInfo;");
                jint iconResId = appInfo.getField<jint>("icon");

                qDebug(mainLog) << "Using application icon resource ID:" << iconResId;

                builder.callObjectMethod("setSmallIcon",
                    "(I)Landroid/app/Notification$Builder;",
                    iconResId);

                // Set ongoing = true to make it persistent
                builder.callObjectMethod("setOngoing",
                    "(Z)Landroid/app/Notification$Builder;",
                    true);

                // Build and show notification
                QJniObject notification = builder.callObjectMethod("build",
                    "()Landroid/app/Notification;");

                notificationManager.callMethod<void>("notify",
                    "(ILandroid/app/Notification;)V",
                    1, // notification ID
                    notification.object());

                qDebug(mainLog) << "Persistent notification created - app marked as running in background";
            }
        }
    });
#endif

    // PHASE 6.0.23.1: Disable debug logs to prevent performance issues (40Hz PGN spam)
    // Phase 6.0.24: Allow selective debug logging for AgIOService (change agioservice.debug=false to true)
    QLoggingCategory::setFilterRules(QStringLiteral(
        "*.debug=false\n"
        "agioservice.debug=false\n"  // Change to true to enable AgIOService debug logs
        "*.qtagopengps.debug=true\n"
        "formgps_position.qtagopengps=false\n"
        "ctool.qtagopengps=false\n"
        "formgps_opengl.qtagopengps=false\n"
        "qt.scenegraph.general=true\n"
        "*.warning=true\n"
        "*.critical=true\n"
        "*.fatal=true"
    ));

    qSetMessagePattern("%{time hh:mm:ss.zzz} [%{type}] %{function}:%{line} - %{message}");
    QApplication a(argc, argv);

    QString iconPath = findIconPath();
    if (QFile::exists(iconPath)) {
        a.setWindowIcon(QIcon(iconPath));
    }

    QFont f = a.font();
    f.setPointSize(16);
    a.setFont(f);
    QCoreApplication::setOrganizationName("QtAgOpenGPS");
    QCoreApplication::setOrganizationDomain("qtagopengps");
    QCoreApplication::setApplicationName("QtAgOpenGPS");
    //QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts, true);
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat,
                       QSettings::UserScope,
                       QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));

    //We're supposed to be compatible with the saved data
    //from this version of AOG:
    QCoreApplication::setApplicationVersion("4.1.0");
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#error Requires Qt 6
#endif

    // Phase 6.0.21: Register PGNParser::ParsedData for Qt::QueuedConnection signals
    qRegisterMetaType<PGNParser::ParsedData>("PGNParser::ParsedData");

    //QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);

    // Explicit QML module registration (ensures static initializer runs)
    // qt_add_qml_module generates this function in qtagopengps_qmltyperegistrations.cpp
    extern void qml_register_types_AOG();
    qml_register_types_AOG();

#ifdef Q_OS_ANDROID
    // Request storage permissions for accessing Documents folder on Android
    // Use QTimer to defer the request slightly to ensure proper context
    QTimer::singleShot(100, []() {
        qDebug(mainLog) << "Checking storage permissions...";

        // Check Android version
        jint sdkInt = QJniObject::getStaticField<jint>("android/os/Build$VERSION", "SDK_INT");
        qDebug(mainLog) << "Android SDK version:" << sdkInt;

        bool hasPermission = false;

        if (sdkInt >= 30) { // Android 11+
            // Check for MANAGE_EXTERNAL_STORAGE permission
            hasPermission = QJniObject::callStaticMethod<jboolean>(
                "android/os/Environment",
                "isExternalStorageManager",
                "()Z");
            qDebug(mainLog) << "MANAGE_EXTERNAL_STORAGE permission:" << hasPermission;

            if (!hasPermission) {
                qDebug(mainLog) << "Opening Settings to request MANAGE_EXTERNAL_STORAGE...";

                // For Android 11+, need to open Settings for MANAGE_EXTERNAL_STORAGE
                QJniObject activity = QJniObject::callStaticObjectMethod(
                    "org/qtproject/qt/android/QtNative",
                    "activity",
                    "()Landroid/app/Activity;");

                if (activity.isValid()) {
                    QJniObject intent("android/content/Intent", "(Ljava/lang/String;)V",
                        QJniObject::fromString("android.settings.MANAGE_APP_ALL_FILES_ACCESS_PERMISSION").object());

                    QJniObject uri = QJniObject::callStaticObjectMethod(
                        "android/net/Uri",
                        "parse",
                        "(Ljava/lang/String;)Landroid/net/Uri;",
                        QJniObject::fromString("package:org.qtagopengps.qtagopengps").object());

                    intent.callObjectMethod("setData", "(Landroid/net/Uri;)Landroid/content/Intent;", uri.object());

                    activity.callMethod<void>("startActivity", "(Landroid/content/Intent;)V", intent.object());
                    qDebug(mainLog) << "Settings activity started";
                }
            }
        } else { // Android 10 and below
            QJniObject activity = QJniObject::callStaticObjectMethod(
                "org/qtproject/qt/android/QtNative",
                "activity",
                "()Landroid/app/Activity;");

            if (activity.isValid()) {
                QJniObject permissionString = QJniObject::fromString("android.permission.READ_EXTERNAL_STORAGE");
                jint result = activity.callMethod<jint>("checkSelfPermission", "(Ljava/lang/String;)I",
                                                        permissionString.object<jstring>());

                qDebug(mainLog) << "READ_EXTERNAL_STORAGE permission check result:" << result << "(0 = granted)";
                hasPermission = (result == 0);

                if (!hasPermission) {
                    qDebug(mainLog) << "Storage permissions not granted for Android <11";
                    qDebug(mainLog) << "Note: Standard permission request may not work with Qt Activity";
                    qDebug(mainLog) << "Consider granting permissions manually in Android Settings";
                }
            }
        }

        if (hasPermission) {
            qDebug(mainLog) << "Storage permissions already granted";
        }
    });
#endif

    // Request location permissions for GPS functionality
    // Use Always availability to get background location access
    QLocationPermission locationPermission;
    locationPermission.setAccuracy(QLocationPermission::Precise);
    locationPermission.setAvailability(QLocationPermission::Always);

    switch (a.checkPermission(locationPermission)) {
    case Qt::PermissionStatus::Undetermined:
        qDebug(mainLog) << "Location permission undetermined, requesting...";
        a.requestPermission(locationPermission, [](const QPermission &permission) {
            if (qApp->checkPermission(permission) == Qt::PermissionStatus::Granted) {
                qDebug(mainLog) << "Location permission granted";
            } else {
                qWarning() << "Location permission denied - GPS functionality will not work";
            }
        });
        break;
    case Qt::PermissionStatus::Denied:
        qWarning() << "Location permission denied - GPS functionality will not work";
        break;
    case Qt::PermissionStatus::Granted:
        qDebug(mainLog) << "Location permission already granted";
        break;
    }

    FormGPS *w = new FormGPS();
    if (!w) {
        qFatal(mainLog) << "Could not allocate FormGPS on the stack!";
    }

#ifdef Q_OS_ANDROID
    // Monitor application state to log when going to background
    QObject::connect(&a, &QGuiApplication::applicationStateChanged,
        [](Qt::ApplicationState state) {
            switch (state) {
            case Qt::ApplicationActive:
                qDebug(mainLog) << "App became ACTIVE (foreground)";
                break;
            case Qt::ApplicationInactive:
                qDebug(mainLog) << "App became INACTIVE (transitioning)";
                break;
            case Qt::ApplicationHidden:
                qDebug(mainLog) << "App became HIDDEN";
                break;
            case Qt::ApplicationSuspended:
                qDebug(mainLog) << "App became SUSPENDED (background)";
                qDebug(mainLog) << "WARNING: Qt Activity is paused - OpenGL context may be destroyed";
                qDebug(mainLog) << "Critical processing should continue due to wake lock";
                break;
            }
        });
#endif

    // MASSIVE MIGRATION: Validate SettingsManager accessible
    qDebug(mainLog) << "SettingsManager instance:" << SettingsManager::instance();
    qDebug(mainLog) << "SettingsManager initialization: completed";

    if (SettingsManager::instance()->display_showBack()) {
        grnPixelsWindow = new QLabel("Back Buffer");
        grnPixelsWindow->setFixedWidth(500);
        grnPixelsWindow->setFixedHeight(500);
        grnPixelsWindow->show();
        overlapPixelsWindow = new QLabel("overlap buffer");
        //overlapPixelsWindow->setFixedWidth(1300);
        //overlapPixelsWindow->setFixedHeight(900);
        overlapPixelsWindow->show();
    }

    int result = a.exec();
    delete w;
    return result;
}
