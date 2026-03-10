#include <QQuickView>
#include <QQuickWidget>
#include <QQmlContext>
#include <QScreen>
#include "formgps.h"
#include "qmlutil.h"
#include <QTimer>
#include <QThread>
#include "cvehicle.h"
#include "ccontour.h"
#include "cabline.h"
#include "classes/settingsmanager.h"
#include <QGuiApplication>
#include <QQmlEngine>
#include <QCoreApplication>
#include <functional>
#include <assert.h>
#include "aogrenderer.h"
//include "qmlsectionbuttons.h"
#include "cboundarylist.h"
#include <cmath>
#include <cstring>
#include <QTranslator>
// FormLoop removed - Phase 4.4: AgIOService standalone
#include <algorithm>
#include "rendering.h"
#include "backend.h"
#include "backendaccess.h"
#include "boundaryinterface.h"
#include "fieldinterface.h"
#include "mainwindowstate.h"
#include "flagsinterface.h"
#include "recordedpath.h"
#include "siminterface.h"
#include "modulecomm.h"
#include "camera.h"
#include "vehicleproperties.h"
#include "layerservice.h"

Q_LOGGING_CATEGORY (formgps_ui, "formgps_ui.qtagopengps")
#define QDEBUG qDebug(formgps_ui)

#define STRINGISE_IMPL(x) #x
#define STRINGISE(x) STRINGISE_IMPL(x)

#if defined(_MSC_VER)
// MSVC format: file(line): warning CXXXX: message
#define FILE_LINE_LINK __FILE__ "(" STRINGISE(__LINE__) ") : "
#define COMPILER_WARNING(msg) __pragma(message(FILE_LINE_LINK "warning: " msg))
#elif defined(__GNUC__) || defined(__clang__)
// GCC/Clang use _Pragma to embed #pragma GCC warning inside a macro
#define COMPILER_WARNING(msg) _Pragma(STRINGISE(GCC warning msg))
#else
#define COMPILER_WARNING(msg)
#endif

QString caseInsensitiveFilename(QString directory, QString filename);

// ⚡ PHASE 6.0.4: Q_PROPERTY setters moved to inline implementation in formgps.h

void FormGPS::setupGui()
{
    BACKEND_TRACK(track);

    // Phase 4.5: AgIOService will be created by QML factory automatically
    QDEBUG << "🚀 AgIOService will be initialized by QML factory on first access";


    QDEBUG << "AgIO: All context properties set, loading QML...";

    // ⚡ PHASE 6.3.0 CRITICAL FIX: Expose FormGPS Q_PROPERTY to QML context
    // This makes C++ Q_PROPERTY accessible to both InterfaceProperty and QML
    rootContext()->setContextProperty("formGPS", this);
    QDEBUG << "✅ FormGPS exposed to QML context - Q_PROPERTY now accessible";

    addImportPath("qrc:/qt/qml/");


    /* Load the QML UI and display it in the main area of the GUI */
    setProperty("title","QtAgOpenGPS");
    addImportPath(":/");

    // Translation will be loaded automatically in constructor via on_language_changed()
    // No manual loading needed here

    connect(this, &QQmlApplicationEngine::objectCreated,
            this, &FormGPS::on_qml_created, Qt::QueuedConnection);

//tell the QML what OS we are using
#ifdef __ANDROID__
    rootContext()->setContextProperty("OS", "ANDROID");
#elif defined(__WIN32)
    rootContext()->setContextProperty("OS", "WINDOWS");
#else
    rootContext()->setContextProperty("OS", "LINUX");
#endif

    //Load the QML into a view
    rootContext()->setContextProperty("screenPixelDensity",QGuiApplication::primaryScreen()->physicalDotsPerInch() * QGuiApplication::primaryScreen()->devicePixelRatio());
    rootContext()->setContextProperty("aog", this);        // New unified interface
    //rootContext()->setContextProperty("mainForm", this);  // Legacy compatibility

    //populate vehicle_list property in vehicleInterface
    vehicle_update_list();
    
    // ===== PHASE 6.0.20 Task 22: CTrack member registration restored =====
    // Original architecture: qmlRegisterSingletonInstance in formgps_ui.cpp (not main.cpp)
  //  rootContext()->setContextProperty("TracksInterface", &track);
    qmlRegisterSingletonInstance("AOG", 1, 0, "TracksInterface", &track);

    // Only tram still uses setContextProperty (not yet modernized)
    rootContext()->setContextProperty("tram", &tram);

#ifdef LOCAL_QML
    // Look for QML files relative to our current directory
    QStringList search_pathes = { "..",
                                 "../../",
                                 "../../../",
                                 "../qtaog",
                                 "../QtAgOpenGPS",
                                 "."
    };

    qWarning() << "Looking for QML.";
    for(const QString &search_path : search_pathes) {
        //look relative to current working directory
        QDir d = QDir(QDir::currentPath() + "/" + search_path + "/qml/");
        if (d.exists("MainWindow.qml")) {
            QDir::addSearchPath("local",QDir::currentPath() + "/" + search_path);
            addImportPath(QDir::currentPath() + "/" + search_path + "/qml/");
            qWarning() << "QML path is " << search_path;
            qWarning() << "added" << (QDir::currentPath() + "/" + search_path + "/qml/") << "to qml engine path.";
            break;
        }

        //look relative to the executable's directory
        d = QDir(QCoreApplication::applicationDirPath() + "/" + search_path + "/qml/");
        if (d.exists("MainWindow.qml")) {
            QDir::addSearchPath("local",QCoreApplication::applicationDirPath() + "/" + search_path);
            addImportPath( QCoreApplication::applicationDirPath() + "/" + search_path + "/qml/");
            qWarning() << "QML path is " << search_path;
            qWarning() << "added" << (QCoreApplication::applicationDirPath() + "/" + search_path + "/qml/") << "to qml engine path.";
            break;
        }
    }

    /*
    QObject::connect(this, &QQmlApplicationEngine::warnings, [=] (const QList<QQmlError> &warnings) {
        foreach (const QQmlError &error, warnings) {
            qWarning() << "warning: " << error.toString();
        }
    });
    */

    rootContext()->setContextProperty("prefix","local:");
    load("local:/qml/MainWindow.qml");
#else
    rootContext()->setContextProperty("prefix","qrc:/AOG");
    //load(QUrl("qrc:/qml/MainWindow.qml"));
    addImportPath(QString("%1/modules").arg(QGuiApplication::applicationDirPath()));
    loadFromModule("AOG", "MainWindow");

    if (rootObjects().isEmpty()) {
        QDEBUG << "Error: Failed to load QML!";
        return;
    } else {
        QDEBUG << "QML loaded successfully.";

        // ⚡ PHASE 6.3.0 TIMING: QML interfaces initialization moved to on_qml_created()
        // mainWindow must be set before calling initializeQMLInterfaces()
        QDEBUG << "🔄 QML loaded - interface initialization will happen in on_qml_created()";

    }

    QDEBUG << "AgOpenGPS started successfully";
#endif
    // Connect to the AgIOService instance
    connectToAgIOFactoryInstance();
}

void FormGPS::on_qml_created(QObject *object, const QUrl &url)
{
    BACKEND_TRACK(track);
    BACKEND_YT(yt);

    QDEBUG << "object is now created. " << url.toString();
    //get pointer to root QML object, which is the OpenGLControl,
    //store in a member variable for future use.
    QList<QObject*> root_context = rootObjects();

    if (root_context.length() == 0) {
        qWarning() << "MainWindow.qml did not load.  Aborting.";
        assert(root_context.length() > 0);
    }

    QObject *mainWindow = root_context.first();

    mainWindow->setProperty("visible",true);

    // ⚡ Qt 6.8 Modern Pattern: objectCreated signal has fired, QML root is ready
    QDEBUG << "🎯 Qt 6.8: QML root object created, scheduling interface initialization...";

    // Defer initialization to let QML complete its Component.onCompleted
    QTimer::singleShot(100, this, [this]() {
        QDEBUG << "⏰ Timer fired - initializing QML interfaces now...";
        initializeQMLInterfaces();
    });

    // ⚡ MOVED: Interface initialization moved to initializeQMLInterfaces() for proper timing

    // QMLSectionButtons completely removed - using direct btnStates[] array instead

    //initialize interface properties (MOVED to initializeQMLInterfaces() after PropertyWrapper init)
    MainWindowState::instance()->set_isBtnAutoSteerOn(false);
    Backend::instance()->pn()->setLatStart(0.0);
    Backend::instance()->pn()->setLonStart(0.0);

    connect(SettingsManager::instance(), &SettingsManager::menu_languageChanged, this, &FormGPS::on_language_changed);

    //vehicle saving and loading
    connect(CVehicle::instance(), &CVehicle::vehicle_update_list, this, &FormGPS::vehicle_update_list, Qt::QueuedConnection);
    connect(CVehicle::instance(), &CVehicle::vehicle_load, this, &FormGPS::vehicle_load, Qt::QueuedConnection);
    connect(CVehicle::instance(), &CVehicle::vehicle_delete, this, &FormGPS::vehicle_delete, Qt::QueuedConnection);
    connect(CVehicle::instance(), &CVehicle::vehicle_saveas, this, &FormGPS::vehicle_saveas, Qt::QueuedConnection);

    headland_form.bnd = &bnd;
    headland_form.hdl = &hdl;

    connect(&headland_form, &FormHeadland::saveHeadland, this, &FormGPS::headland_save);

    headache_form.bnd = &bnd;
    headache_form.hdl = &hdl;

    connect(&headache_form, SIGNAL(saveHeadland()),this,SLOT(headland_save()));
    connect(&headache_form, SIGNAL(saveHeadlines()), this,SLOT(headlines_save()));
    connect(&headache_form, SIGNAL(loadHeadlines()), this,SLOT(headlines_load()));

    BoundaryInterface::instance()->set_isOutOfBounds(false);

    connect(FieldInterface::instance(), &FieldInterface::updateList, this, &FormGPS::field_update_list);
    connect(FieldInterface::instance(), &FieldInterface::newField, this, &FormGPS::field_new);
    connect(FieldInterface::instance(), &FieldInterface::openField, this, &FormGPS::field_open);
    connect(FieldInterface::instance(), &FieldInterface::newFieldFrom, this, &FormGPS::field_new_from);
    connect(FieldInterface::instance(), &FieldInterface::newFieldFromKML, this, &FormGPS::field_new_from_KML);
    connect(FieldInterface::instance(), &FieldInterface::closeField, this, &FormGPS::field_close);
    connect(FieldInterface::instance(), &FieldInterface::deleteField, this, &FormGPS::field_delete);
    //connect(FieldInterface::instance(), &FieldInterface::exportFieldZip, this, &FormGPS::field_export_zip);
    //connect(FieldInterface::instance(), &FieldInterface::importFieldZip, this, &FormGPS::field_import_zip);

    QDEBUG << "🎯 Connected FieldInterface signals.";

    connect(FlagsInterface::instance(), &FlagsInterface::saveFlags, this, &FormGPS::FileSaveFlags);


    //connect qml button signals to callbacks (it's not automatic with qml)

    tmrWatchdog = new QTimer(this);
    connect (tmrWatchdog, SIGNAL(timeout()),this,SLOT(tmrWatchdog_timeout()));
    tmrWatchdog->start(250); //fire every 50ms.

    //SIM
    connect(SimInterface::instance(), &SimInterface::newPosition,
            this, &FormGPS::onSimNewPosition, Qt::UniqueConnection);

    // Phase 6.0.33: GPS timer for real GPS mode (50 Hz fixed rate)
    // 50 Hz = 20ms interval for smooth rendering and PGN 254 AutoSteer commands
    connect(&timerGPS, &QTimer::timeout, this, &FormGPS::onGPSTimerTimeout, Qt::UniqueConnection);
    // Timer will be started when GPS data starts arriving (not in simulation mode)
    timerGPS.start(100);  // 100ms = 10 Hz (synchronized with NMEA data rate)

    connect(Backend::instance(), &Backend::resetDirection, this, &FormGPS::resetDirection);
    connect(Backend::instance(), &Backend::deleteAppliedArea, this, &FormGPS::deleteAppliedArea);
    connect(Backend::instance(), &Backend::centerOgl, this, &FormGPS::centerOgl);

    connect(Backend::instance(), &Backend::contourPriority, this, &FormGPS::contourPriority);

    connect(Backend::instance(), &Backend::snapToPivot, this, &FormGPS::snapToPivot);
    connect(Backend::instance(), &Backend::snapSideways, this, &FormGPS::snapSideways);

    connect(Camera::instance(), &Camera::updateView, [this]() {
        if (openGLControl) {
            QMetaObject::invokeMethod(openGLControl, "update", Qt::QueuedConnection);
        }
    });

    connect(RecordedPath::instance(), &RecordedPath::stoppedDriving, this, &FormGPS::onStoppedDriving, Qt::QueuedConnection);

    connect(&bnd, &CBoundary::saveBoundaryRequested, this, &FormGPS::FileSaveBoundary, Qt::DirectConnection);

    connect(&track, &CTrack::saveTracks, this, &FormGPS::FileSaveTracks, Qt::QueuedConnection);

    loadSettings(); //load settings and properties

    Backend::instance()->set_isJobStarted(false);

    // StartLoopbackServer(); // ❌ REMOVED - Phase 4.6: UDP FormGPS completely eliminated
    if (SettingsManager::instance()->menu_isSimulatorOn() == false) {
        QDEBUG << "Stopping simulator because it's off in settings.";
        SimInterface::instance()->stop();
    }

    //star Sim
    swFrame.start();

    stopwatch.start();

    vehicle_update_list();

}

void FormGPS::onGLControl_dragged(int pressX, int pressY, int mouseX, int mouseY)
{
    Camera &camera = *Camera::instance();
    QVector3D from,to,offset;

    from = mouseClickToPan(pressX, pressY);
    to = mouseClickToPan(mouseX, mouseY);
    offset = to - from;

    camera.set_panX(camera.panX() + offset.x());
    camera.set_panY(camera.panY() + offset.y());
    if (openGLControl) {
        QMetaObject::invokeMethod(openGLControl, "update", Qt::QueuedConnection);
    }
}

void FormGPS::centerOgl() {
    Camera &camera = *Camera::instance();
    QDEBUG<<"center ogl";

    camera.set_panX(0);
    camera.set_panY(0);
    if (openGLControl) {
        QMetaObject::invokeMethod(openGLControl, "update", Qt::QueuedConnection);
    }
}

void FormGPS::onGLControl_clicked(const QVariant &event)
{
    QObject *m = event.value<QObject *>();

    //Pass the click on to the rendering routine.
    //make the bottom left be 0,0
    mouseX = m->property("x").toInt();
    mouseY = m->property("y").toInt();

    QVector3D field = mouseClickToField(mouseX, mouseY);
    mouseEasting = field.x();
    mouseNorthing = field.y();

    leftMouseDownOnOpenGL = true;
    // CRITICAL: Force OpenGL update in GUI thread to prevent threading violation
    if (openGLControl) {
        QMetaObject::invokeMethod(openGLControl, "update", Qt::QueuedConnection);
    }
}

void FormGPS::settingsReload() {
    on_settings_reload();
}

void FormGPS::settingsSave() {
    on_settings_save();
}

// ===== BATCH 9 - 2 ACTIONS Snap Track - Qt 6.8 Q_INVOKABLE Implementation =====
// ===== BATCH 13 - 7 ACTIONS Field Management - Qt 6.8 Q_INVOKABLE Implementation =====
void FormGPS::fieldUpdateList() {
    // Modern implementation - same logic as field_update_list()
    field_update_list();
}

void FormGPS::fieldClose() {
    // Modern implementation - same logic as field_close()
    field_close();
}

void FormGPS::fieldOpen(const QString& fieldName) {
    // Modern implementation - same logic as field_open(QString)
    field_open(fieldName);
}

void FormGPS::fieldNew(const QString& fieldName) {
    // Modern implementation - same logic as field_new(QString)
    field_new(fieldName);
}

void FormGPS::fieldNewFrom(const QString& fieldName, const QString& sourceField, int fieldType) {
    // Modern implementation - same logic as field_new_from(QString,QString,int)
    field_new_from(fieldName, sourceField, fieldType);
}

void FormGPS::fieldNewFromKML(const QString& fieldName, const QString& kmlPath) {
    // Modern implementation - same logic as field_new_from_KML(QString,QString)
    QDEBUG << fieldName << " " << kmlPath;
    field_new_from_KML(fieldName, kmlPath);
}

void FormGPS::fieldDelete(const QString& fieldName) {
    // Modern implementation - same logic as field_delete(QString)
    field_delete(fieldName);
}

void FormGPS::loadBoundaryFromKML(QString filename) {
    // Modern implementation
    boundary_new_from_KML(filename);
}

/*
 * put as methods in CRecPath
void FormGPS::recordedPathDelete(const QString& pathName) {
    // TODO: Backend implementation needed in RecordedPath
    // This would delete a recorded path file from disk

    // Reset if this was the active path
    if (recordedPathName() == pathName) {
        setRecordedPathName("");
        setIsDrivingRecordedPath(false);
    }
}

void FormGPS::recordedPathStartDriving() {
    // Call existing backend method with required parameters
    bool success = RecordedPath::instance()->StartDrivingRecordedPath(*vehicle, yt);

    // Update property only if successfully started - automatic QML notification
    if (success) {
        setIsDrivingRecordedPath(true);
    }
}

void FormGPS::recordedPathStopDriving() {
    // Call existing backend method
    RecordedPath::instance()->StopDrivingRecordedPath();

    // Automatic notification via Qt 6.8 binding
    setIsDrivingRecordedPath(false);
}

void FormGPS::recordedPathClear() {
    // Clear the recorded path data
    RecordedPath::instance()->recList.clear();
    RecordedPath::instance()->recListCount = 0;
    RecordedPath::instance()->isFollowingRecPath = false;

    // Reset properties
    setRecordedPathName("");
    setIsDrivingRecordedPath(false);
}
*/

void FormGPS::onBtnTramlines_clicked(){
    QDEBUG<<"tramline";
}

void FormGPS::resetDirection(){
    QDEBUG<<"reset Direction";
    // c#Array.Clear(stepFixPts, 0, stepFixPts.Length);

    std::memset(stepFixPts, 0, sizeof(stepFixPts));
    CVehicle::instance()->vehicleProperties()->set_firstHeadingSet(false);
    //isFirstHeadingSet = false;

    CVehicle::instance()->setIsReverse(false);
    TimedMessageBox(2000, "Reset Direction", "Drive Forward > 1.5 kmh");
}

void FormGPS::contourPriority(bool isRight) {
    COMPILER_WARNING ("ct.isRightPriority is never used anywhere.  bug?")
    BACKEND_TRACK(track);
    CContour &ct = track.contour;
    ct.set_isRightPriority (isRight);
    QDEBUG << "Contour isRight: " << isRight;
}

void FormGPS::TimedMessageBox(int timeout, QString s1, QString s2)
{
    QDEBUG << "Timed message " << timeout << s1 << ", " << s2 << Qt::endl;
    //Use the pointer stoerd in Backend to access this QML item.
    Backend::instance()->timedMessage(timeout, s1, s2);
}

void FormGPS::turnOffBoundAlarm()
{
    QDEBUG << "Bound alarm should be off" << Qt::endl;
    //TODO implement sounds
}

void FormGPS::FixTramModeButton()
{
    //TODO QML
    //unhide button if it should be seen
    if (tram.tramList.count() > 0 || tram.tramBndOuterArr.count() > 0)
    {
        //btnTramDisplayMode.Visible = true;
        tram.displayMode = 1;
    }

    //make sure tram has right icon.  DO this through javascript

}
void FormGPS::on_settings_reload() {
    loadSettings();
    //TODO: if vehicle name is set, write settings out to that
    //vehicle json file
}

void FormGPS::on_settings_save() {
    // Qt6 Pure Architecture: All property setters auto-sync to INI, no manual sync needed
    loadSettings();
}

void FormGPS::on_language_changed() {
    QString lang = SettingsManager::instance()->menu_language();
    QDEBUG << "Changing language to:" << lang;

#ifdef Q_OS_ANDROID
    // Для Android - упрощенный подход
    QStringList paths;
    paths << QString("assets:/i18n/qml_%1.qm").arg(lang);
    paths << QString(":/i18n/qml_%1.qm").arg(lang);

    bool translationLoaded = false;

    for (const QString &path : paths) {
        if (m_translator->load(path)) {
            translationLoaded = true;
            //qDebug() << "Translation loaded from:" << path;
            break;
        }
    }

    if (!translationLoaded) {
        // qDebug() << "Failed to load translation for:" << lang;

        if (lang != "en") {
            for (const QString &path : paths) {
                if (m_translator->load(path.arg("en"))) {
                    translationLoaded = true;
                    //qDebug() << "Loaded English fallback from:" << path.arg("en");
                    break;
                }
            }
        }
    }

    if (translationLoaded) {
        QCoreApplication::installTranslator(m_translator);
        this->retranslate();
        //qDebug() << "Language switched to:" << lang;
    }

#endif

    // Load translation file (note: CMake generates resources with i18n/ prefix)
    if (m_translator->load(QString(":/qt/qml/AOG/i18n/i18n/qml_%1.qm").arg(lang))) {
        QCoreApplication::installTranslator(m_translator);
        QDEBUG << "Translation loaded and installed successfully";

        // Force QML retranslation - this updates ALL qsTr() texts automatically
        this->retranslate();
        QDEBUG << "QML retranslation completed";
    } else {
        QDEBUG << "Failed to load translation for language:" << lang;
    }
}

void FormGPS::headland_save() {
    //TODO make FileHeadland() a slot so we don't have to have this
    //wrapper.
    FileSaveHeadland();
}

void FormGPS::headlines_load() {
    //TODO make FileLoadHeadLines a slot, skip this wrapper
    FileLoadHeadLines();
}

void FormGPS::headlines_save() {
    //TODO make FileSaveHeadLines a slot, skip this wrapper
    FileSaveHeadLines();
}

//Track Snap buttons
void FormGPS::snapToPivot() {
    COMPILER_WARNING("snapToPivot not yet implemented")
    //TODO
    QDEBUG<<"snap to pivot";
}

void FormGPS::snapSideways(double distance) {
    COMPILER_WARNING("snapSideways not yet implemented")
    //TODO
}

void FormGPS::deleteAppliedArea() {
    BACKEND_TRACK(track);
    CContour &ct = track.contour;

    if (Backend::instance()->isJobStarted())
    {
        LayerService::instance()->clearAllLayers();
        //clear out the contour Lists
        ct.StopContourLine(contourSaveList);
        ct.ResetContour();

        Backend::instance()->currentField_setWorkedAreaTotal(0);

        //clear the section lists
        for (int j = 0; j < tool.triStrip.count(); j++)
        {
            //clean out the lists
            tool.triStrip[j].patchList.clear();
            tool.triStrip[j].triangleList.clear();
        }

        tool.patchesBufferDirty=true;
        //patchSaveList.clear();
        tool.patchSaveList.clear();

        FileCreateContour();
        FileCreateSections();
    }
}

// OLD loadTranslation function removed - replaced by on_language_changed()

// TracksInterface and VehicleInterface now use QML_SINGLETON + QML_ELEMENT (same approach as Settings)

// ===== QML INTERFACE INITIALIZATION - DELAYED TIMING FIX =====
void FormGPS::initializeQMLInterfaces()
{
    QDEBUG << "🔄 Starting QML interface initialization...";

    //no need to emit Backend::fixFrameChanged() here really.  it defaults
    //to 0 anyway
    Backend::instance()->m_fixFrame.sentenceCounter = 0;
    Backend::instance()->set_isPatchesChangingColor( false);


    auto setup_gl_callbacks = [this]() {
        openGLControl = qobject_cast<QQuickItem *>(Backend::instance()->aogRenderer);
        QDEBUG << "🎯 Setting up OpenGL callbacks - InterfaceProperty verified safe";
        openGLControl->setProperty("callbackObject",QVariant::fromValue((void *) this));
        openGLControl->setProperty("initCallback",QVariant::fromValue<std::function<void (void)>>(std::bind(&FormGPS::openGLControl_Initialized, this)));
#ifdef USE_INDIRECT_RENDERING
        //do indirect rendering for now.
        openGLControl->setProperty("paintCallback",QVariant::fromValue<std::function<void (void)>>(std::bind(&FormGPS::render_main_fbo,this)));
#else
        //direct rendering in the QML render thread.  Will need locking to be safe.
        openGLControl->setProperty("paintCallback",QVariant::fromValue<std::function<void (void)>>(std::bind(&FormGPS::oglMain_Paint,this)));
#endif
#ifdef USE_QSGRENDERNODE
        openGLControl->setProperty("cleanupCallback",QVariant::fromValue<std::function<void (void)>>(std::bind(&FormGPS::openGLControl_Shutdown,this)));
#else

        openGLControl->setProperty("samples",SettingsManager::instance()->display_antiAliasSamples());
        static_cast<AOGRendererInSG *>(openGLControl)->setMirrorVertically(true);
#endif
        connect(openGLControl,SIGNAL(clicked(QVariant)),this,SLOT(onGLControl_clicked(QVariant)));
        connect(openGLControl,SIGNAL(dragged(int,int,int,int)),this,SLOT(onGLControl_dragged(int,int,int,int)));
        QDEBUG << "✅ OpenGL callbacks configured - rendering can now safely access InterfaceProperty";
    };

    if (!Backend::instance()->aogRenderer) {
        // If QML not ready yet; defer setting up of rendering callbacks
        connect(Backend::instance(), &Backend::aogRendererChanged, this, setup_gl_callbacks);
    } else {
        // set everything up.
        setup_gl_callbacks();
    }

    if (SettingsManager::instance()->menu_isSimulatorOn()) {
        if (!SimInterface::instance()->isRunning()) {
            SimInterface::instance()->startUp();
            QDEBUG << "✅ Simulator timer started (10Hz)";
        }
    }
}

// ===== AB LINES MANAGEMENT STUB IMPLEMENTATIONS =====
// TODO: Implement these methods properly when AB Lines functionality is ready
void FormGPS::updateABLines() {
    qWarning() << "updateABLines() NOT IMPLEMENTED YET";
    // TODO: Future implementation will update AB lines list
}

void FormGPS::updateCurves() {
    qWarning() << "updateCurves() NOT IMPLEMENTED YET";
    // TODO: Future implementation will update curves list
}

void FormGPS::setCurrentABCurve(int index) {
    qWarning() << "setCurrentABCurve() NOT IMPLEMENTED YET - index:" << index;
    // TODO: Future implementation will set current AB curve
}

// ===== AB Lines Methods - Phase 6.0.20 =====

void FormGPS::swapABLineHeading(int index) {
    BACKEND_TRACK(track);

    if (index >= 0 && index < track.count()) {
        track.swapAB(index);
        updateABLines();
    }
}

void FormGPS::deleteABLine(int index) {
    BACKEND_TRACK(track);

    if (index >= 0 && index < track.count()) {
        track.delete_track(index);
        updateABLines();
    }
}

void FormGPS::addABLine(const QString& name) {
    // Note: AB Line creation is handled by TrackNewSet.qml interface
    // which uses TracksInterface.start_new() + mark_start() + finish_new()
    // This method is kept for API consistency but delegates to existing workflow
    qWarning() << "addABLine() called - Use TrackNewSet.qml interface for full creation workflow";
    updateABLines();
}

void FormGPS::changeABLineName(int index, const QString& newName) {
    BACKEND_TRACK(track);

    if (index >= 0 && index < track.count()) {
        track.changeName(index, newName);
        updateABLines();
    }
}

