#ifndef FORMGPS_H
#define FORMGPS_H

#include <QMainWindow>
#include <QScopedPointer>
#include <memory> // C++17 smart pointers
#include <QProperty> // Qt 6.8 QProperty + BINDABLE
#include <QBindable> // Qt 6.8 QBindable for automatic change tracking
#include <QtQuick/QQuickItem>
#include <QOpenGLFunctions>
#include <QOpenGLTexture>
#include <QUdpSocket>
#include <QElapsedTimer>
#include <QOffscreenSurface>
#include <QOpenGLContext>
#include <QSurfaceFormat>
#include <QOpenGLFramebufferObject>
#include <QOpenGLBuffer>
#include <QQmlApplicationEngine>
#include <QTranslator>
//#include <QSerialPort>
#include "common.h"
#include "vecfix2fix.h"
#include "vec2.h"
#include "vec3.h"
#include "btnenum.h"

#include "cvehicle.h"
#include "ctool.h"
#include "agioservice.h"
#include "pgnparser.h"  // Phase 6.0.21: For ParsedData struct
#include "cboundary.h"
#include "cabline.h"
#include "ctram.h"
#include "cabcurve.h"
#include "cyouturn.h"
#include "cfielddata.h"
#include "cahrs.h"
// CRecordedPath merged into RecordedPath singleton - use RecordedPath::instance()
#include "cheadline.h"
#include "ctrack.h"
#include "formheadland.h"
#include "formheadache.h"
#include "formtrackdrawer.h"

#include <QVector>
#include <QDateTime>
#include <QDebug>

//forward declare classes referred to below, to break circular
//references in the code
class QOpenGLShaderProgram;
class AOGRendererInSG;
class QQuickCloseEvent;
class QVector3D;

struct PatchBuffer {
    QOpenGLBuffer patchBuffer;
    int length;
};

struct PatchInBuffer {
    int which;
    int offset;
    int length;
};


class FormGPS : public QQmlApplicationEngine
{
    Q_OBJECT

public:
    explicit FormGPS(QWidget *parent = 0);
    ~FormGPS();

    // ===== Q_PROPERTY GETTERS, SETTERS AND BINDABLES =====
    // Manual declarations for all Rectangle Pattern properties

    // Misc Status
    void setSensorData(int value);
    QBindable<int> bindableSensorData();

    // GPS/IMU Heading - Phase 6.0.20 Task 24 Step 2
    bool isReverseWithIMU() const;
    void setIsReverseWithIMU(bool value);
    QBindable<bool> bindableIsReverseWithIMU();

    // Job Control
    bool isPatchesChangingColor() const;
    void setIsPatchesChangingColor(bool value);
    QBindable<bool> bindableIsPatchesChangingColor();

     /***********************************************
     * Qt-specific things we need to keep track of *
     ***********************************************/
    QLocale locale;
    QSignalMapper *sectionButtonsSignalMapper;
    QTimer *tmrWatchdog;
    QTimer timerGPS;  // Phase 6.0.24: Fixed 40 Hz timer for real GPS mode (like timerSim for simulation)

    /***************************
     * Qt and QML GUI elements *
     ***************************/
    //QQuickView *qmlview;
    QWidget *qmlcontainer;
    QQuickItem *openGLControl;

    //flag context menu and buttons
    QObject *recordedPathInterface;

    //section buttons
    QObject *sectionButton[MAXSECTIONS-1]; //zero based array

    QObject *txtDistanceOffABLine;

    //offscreen GL objects:
    QSurfaceFormat backSurfaceFormat;
    QOpenGLContext backOpenGLContext;
    QOffscreenSurface backSurface;
    std::unique_ptr<QOpenGLFramebufferObject> backFBO; // C++17 RAII - automatic cleanup

    QSurfaceFormat zoomSurfaceFormat;
    QOpenGLContext zoomOpenGLContext;
    QOffscreenSurface zoomSurface;
    std::unique_ptr<QOpenGLFramebufferObject> zoomFBO; // C++17 RAII - automatic cleanup

    QSurfaceFormat mainSurfaceFormat;
    QOpenGLContext mainOpenGLContext;
    QOffscreenSurface mainSurface;

    std::unique_ptr<QOpenGLFramebufferObject> mainFBO[2]; // C++17 RAII - automatic cleanup
    int active_fbo=-1;

    /*******************
     * from FormGPS.cs *
     *******************/
    //The base directory where AgOpenGPS will be stored and fields and vehicles branch from
    QString baseDirectory;

    //current directory of vehicle
    QString vehiclesDirectory, vehicleFileName = "";

    //current directory of tools
    QString toolsDirectory, toolFileName = "";

    //current directory of Environments
    QString envDirectory, envFileName = "";

    //current fields and field directory
    QString fieldsDirectory, currentFieldDirectory, displayFieldName;

    // android directory
    QString androidDirectory = "/storage/emulated/0/Documents/";


    bool leftMouseDownOnOpenGL; //mousedown event in opengl window
    int flagNumberPicked = 0;

    //bool for whether or not a job is active
    bool /*setIsJobStarted(false),*/ isAreaOnRight = true /*, setIsBtnAutoSteerOn(false)*/;

    //if we are saving a file
    bool isSavingFile = false, isLogElevation = false;

    //the currentversion of software
    QString currentVersionStr, inoVersionStr;

    int inoVersionInt = 0;  // Phase 6.0.24 Problem 18: Initialize to prevent garbage

    //create instance of a stopwatch for timing of frames and NMEA hz determination
    QElapsedTimer swFrame;

    double secondsSinceStart = 0.0;  // Phase 6.0.24 Problem 18: Initialize to prevent garbage

    //Time to do fix position update and draw routine
    double gpsHz = 10;

    bool isStanleyUsed = false;

    // Phase 6.0.24 Problem 18: Initialize progress bar values
    int pbarSteer = 0;
    int pbarRelay = 0;
    int pbarUDP = 0;

    double nudNumber = 0;

private:
    // AgIO Service (main thread for zero-latency OpenGL access)
    AgIOService* m_agioService;

    // ⚡ QML Interface Initialization - Delayed to avoid timing issues
    void initializeQMLInterfaces();

    // Geodetic Conversion - Phase 6.0.20 Task 24 Step 3.5
    void updateMPerDegreeLat();

    // ⚡ PHASE 6.0.3.2: Constructor completion protection
    // Qt 6.8: Simplified - no complex state tracking needed
    
    //For field saving in background
    int fileSaveCounter = 1;
    int minuteCounter = 1;

    int tenMinuteCounter = 1;

    //used to update the screen status bar etc
    int displayUpdateHalfSecondCounter = 0, displayUpdateOneSecondCounter = 0, displayUpdateOneFifthCounter = 0, displayUpdateThreeSecondCounter = 0;
    int tenSecondCounter = 0, tenSeconds = 0;
    int threeSecondCounter = 0, threeSeconds = 0;
    int oneSecondCounter = 0, oneSecond = 0;
    int oneHalfSecondCounter = 0, oneHalfSecond = 0;
    int oneFifthSecondCounter = 0, oneFifthSecond = 0;

    //moved to CYouTurn
    //int makeUTurnCounter = 0;


     /*******************
     * GUI.Designer.cs *
     *******************/
public:
    //ABLines directory
    QString ablinesdirectory;

    //colors for sections and field background
    int flagColor = 0;

    //how many cm off line per big pixel
    int lightbarCmPerPixel = 2;

    //polygon mode for section drawing
    bool isDrawPolygons = false;

    QColor frameDayColor;
    QColor frameNightColor;
    QColor sectionColorDay;
    QColor fieldColorDay;
    QColor fieldColorNight;

    QColor textColorDay;
    QColor textColorNight;

    QColor vehicleColor;
    double vehicleOpacity;
    uchar vehicleOpacityByte;
    bool isVehicleImage;

    //Is it in 2D or 3D, metric or imperial, display lightbar, display grid etc
    bool isMetric = true, isLightbarOn = true, isGridOn, isFullScreen;
    bool isUTurnAlwaysOn, isCompassOn, isSpeedoOn, isSideGuideLines = true;
    bool isPureDisplayOn = true, isSkyOn = true, isRollMeterOn = false, isTextureOn = true;
    bool isDay = true, isDayTime = true, isBrightnessOn = true;
    bool isKeyboardOn = true, isAutoStartAgIO = true, isSvennArrowOn = true;
    bool isConnectedBlockage = false; //Dim
    bool isUTurnOn = true, isLateralOn = true;

    //sunrise, sunset

    bool isFlashOnOff = false;

private:
public:
    //for animated submenu
    //bool isMenuHid = true;

    // Storage For Our Tractor, implement, background etc Textures
    //Texture particleTexture;

    QElapsedTimer stopwatch; //general stopwatch for debugging purposes.
    //readonly Stopwatch swFrame = new Stopwatch();



    //Time to do fix position update and draw routine
    double HzTime = 5;


    //used to update the screen status bar etc
    int statusUpdateCounter = 1;

    CTram tram;

    CTool tool;

    //boundary instance (singleton)
    CBoundary &bnd = *CBoundary::instance();

    CAHRS ahrs;
    CFieldData fd;

    CHeadLine hdl;

    FormHeadland headland_form;
    FormHeadache headache_form;
    FormTrackDrawer trackdrawer_form;

    /* GUI synchronization lock */
    QReadWriteLock lock;
    bool newframe = false;

    bool bootstrap_field = false;
   /************************
     * Controls.Designer.cs *
     ************************/
public:
    bool isTT;
    bool isABCyled = false;

    void GetHeadland();
    void CloseTopMosts();
    void getAB();
    void FixTramModeButton();

    //other things will be in slots

    /*************************
     *  Position.designer.cs *
     *************************/
public:
    //very first fix to setup grid etc
    bool isFirstFixPositionSet = false, isGPSPositionInitialized = false;
    bool m_forceGPSReinitialization = false;  // PHASE 6.0.41: Force latStart/lonStart update on mode switch even if field open
    bool /*isReverse = false (CVehicle),*/ isSteerInReverse = true, isSuperSlow = false, isAutoSnaptoPivot = false;
    double startGPSHeading = 0;

    //string to record fixes for elevation maps
    QByteArray sbGrid;

    // autosteer variables for sending serial moved to CVehicle
    //short guidanceLineDistanceOff, guidanceLineSteerAngle; --> CVehicle
    double avGuidanceSteerAngle = 0.0;  // Phase 6.0.24 Problem 18

    short errorAngVel = 0;  // Phase 6.0.24 Problem 18
    double setAngVel = 0.0;  // Phase 6.0.24 Problem 18
    double actAngVel = 0.0;  // Phase 6.0.24 Problem 18
    bool isConstantContourOn;

    //for heading or Atan2 as camera
    QString headingFromSource, headingFromSourceBak;

    /* moved to CVehicle:
    Vec3 pivotAxlePos;
    Vec3 steerAxlePos;
    Vec3 toolPos;
    Vec3 tankPos;
    Vec3 hitchPos;
    */

    //history
    Vec2 prevFix;
    Vec2 prevDistFix;
    Vec2 lastReverseFix;

    //headings
    double smoothCamHeading = 0, prevGPSHeading = 0.0;

    //storage for the cos and sin of heading
    //moved to vehicle
    //double cosSectionHeading = 1.0, sinSectionHeading = 0.0;

    //how far travelled since last section was added, section points
    double sectionTriggerDistance = 0, contourTriggerDistance = 0, sectionTriggerStepDistance = 0, gridTriggerDistance;
    Vec2 prevSectionPos;
    Vec2 prevContourPos;
    Vec2 prevGridPos;
    int patchCounter = 0;

    Vec2 prevBoundaryPos;

    //Everything is so wonky at the start
    int startCounter = 0;

    //tally counters for display
    //public double totalSquareMetersWorked = 0, totalUserSquareMeters = 0, userSquareMetersAlarm = 0;


    double /*avgSpeed --> CVehicle,*/ previousSpeed = 0.0;  // Phase 6.0.24 Problem 18
    double crossTrackError = 0.0;  // Phase 6.0.24 Problem 18: for average cross track error

    //youturn
    double _distancePivotToTurnLine = -2222;  // Renamed to avoid Q_PROPERTY conflict
    double distanceToolToTurnLine = -2222;

    //the value to fill in you turn progress bar
    int youTurnProgressBar = 0;

    //IMU
    double rollCorrectionDistance = 0;
    double imuGPS_Offset = 0.0;
    double _imuCorrected = 0.0;  // Renamed to avoid Q_PROPERTY conflict

    //step position - slow speed spinner killer
    int currentStepFix = 0;
    int totalFixSteps = 10;
    VecFix2Fix stepFixPts[10];
    double distanceCurrentStepFix = 0, distanceCurrentStepFixDisplay = 0, minHeadingStepDist, startSpeed = 0.5;
    double fixToFixHeadingDistance = 0, gpsMinimumStepDistance;  // PHASE 6.0.35: Loaded from SettingsManager in loadSettings()

    // isReverseWithIMU moved to Q_OBJECT_BINDABLE_PROPERTY m_isReverseWithIMU (line 1848)

    double nowHz = 0, filteredDelta = 0, delta = 0;

    bool isRTK, isRTK_KillAutosteer;

    double headlandDistanceDelta = 0, boundaryDistanceDelta = 0;

    Vec2 lastGPS;

    double uncorrectedEastingGraph = 0;
    double correctionDistanceGraph = 0;

    double timeSliceOfLastFix = 0;

    bool isMaxAngularVelocity = false;

    int minSteerSpeedTimer = 0;


    void UpdateFixPosition(); //process a new position

    void TheRest();
    void CalculatePositionHeading(); // compute all headings and fixes
    void AddContourPoints();
    void AddSectionOrPathPoints();
    void CalculateSectionLookAhead(double northing, double easting, double cosHeading, double sinHeading);
    void InitializeFirstFewGPSPositions();
    void ResetGPSState(bool toSimMode);  // PHASE 6.0.40: Reset GPS state on sim/real switch

    /************************
     * SaveOpen.Designer.cs *
     ************************/

    //moved to CTool
    //list of the list of patch data individual triangles for field sections
    //QVector<QSharedPointer<QVector<QVector3D>>> patchSaveList;

    //moved to CContour.
    //list of the list of patch data individual triangles for contour tracking
    QVector<QSharedPointer<QVector<Vec3>>> contourSaveList;

    void FileSaveHeadLines();
    void FileLoadHeadLines();
    //moved up to a SLOT: void FileSaveTracks();
    void FileLoadTracks();
    void FileSaveCurveLines();
    void FileLoadCurveLines();
    void FileSaveABLines();
    void FileLoadABLines();
    bool FileOpenField(QString fieldDir, int flags = -1);
    QMap<QString, QVariant> FileFieldInfo(QString fieldDir);
    void FileCreateField();
    void FileCreateElevation();
    void FileSaveSections();
    void FileCreateSections();
    void FileCreateFlags();
    void FileCreateContour();
    void FileSaveContour();
    void FileCreateBoundary();
    void FileSaveBoundary();
    void FileSaveTram();
    void FileSaveBackPic();
    void FileCreateRecPath();
    void FileSaveHeadland();
    void FileSaveRecPath();
    void FileLoadRecPath();
    void FileSaveFlags();
    void FileSaveNMEA();
    void FileSaveElevation();
    void FileSaveSingleFlagKML2(int flagNumber);
    void FileSaveSingleFlagKML(int flagNumber);
    void FileMakeKMLFromCurrentPosition(double lat, double lon);
    void ExportFieldAs_KML();
    void FileUpdateAllFieldsKML();
    QString GetBoundaryPointsLatLon(int bndNum);
    void ExportFieldAs_ISOXMLv3();
    void ExportFieldAs_ISOXMLv4();

    /************************
     * formgps_settimgs.cpp *
     ************************/

    void loadSettings();

    /**********************
     * OpenGL.Designer.cs *
     **********************/
    //extracted Near, Far, Right, Left clipping planes of frustum
    double frustum[24];

    double fovy = 0.7;
    double camDistanceFactor = -2;
    int mouseX = 0, mouseY = 0;
    double mouseEasting = 0, mouseNorthing = 0;
    // Phase 6.0.24 Problem 18: Initialize field boundary variables
    double offX = 0.0, offY = 0.0;

    //data buffer for pixels read from off screen buffer
    //uchar grnPixels[80001];
    LookAheadPixels grnPixels[150001];
    LookAheadPixels *overPixels = new LookAheadPixels[160000]; //400x400
    QImage overPix; //for debugging purposes to show in a window

    /*
    QOpenGLShaderProgram *simpleColorShader = 0;
    QOpenGLShaderProgram *texShader = 0;
    QOpenGLShaderProgram *interpColorShader = 0;
    */
    QOpenGLBuffer skyBuffer;

    /***********************
     * formgps_udpcomm.cpp *
     ***********************/
private:
    // UDP FormGPS REMOVED - Phase 4.6: Workers → AgIOService ONLY source
    // QUdpSocket *udpSocket = NULL;  // ❌ REMOVED - AgIOService Workers only

public:
    // ===== Q_INVOKABLE MODERN ACTIONS - Qt 6.8 Direct QML Calls =====
    // Phase 6.0.20: Modernization of button actions - replace signal/slot with direct calls
    // Batch 9 - 2 actions Snap Track - lines 237-238
    // Batch 10 - 8 actions Modules & Steering - lines 253-266

    // Batch 13 - 7 actions Field Management - lines 1826-1832
    Q_INVOKABLE void fieldUpdateList();
    Q_INVOKABLE void fieldClose();
    Q_INVOKABLE void fieldOpen(const QString& fieldName);
    Q_INVOKABLE void fieldNew(const QString& fieldName);
    Q_INVOKABLE void fieldNewFrom(const QString& fieldName, const QString& sourceField, int fieldType);
    Q_INVOKABLE void fieldNewFromKML(const QString& fieldName, const QString& kmlPath);
    Q_INVOKABLE void fieldDelete(const QString& fieldName);

    // Batch 14 - 11 actions Boundary Management - lines 1843-1854
    Q_INVOKABLE void loadBoundaryFromKML(QString filename);

    // Batch 3 - 8 actions Camera Navigation - lines 201-208
    // Batch 4 - 2 actions Settings - lines 227-228
    Q_INVOKABLE void settingsReload();
    Q_INVOKABLE void settingsSave();

    // AB Lines and Curves management - Qt 6.8 additions
    Q_INVOKABLE void updateABLines();
    Q_INVOKABLE void updateCurves();
    Q_INVOKABLE void setCurrentABCurve(int index);

    // AB Lines Methods - Phase 6.0.20
    Q_INVOKABLE void swapABLineHeading(int index);
    Q_INVOKABLE void deleteABLine(int index);
    Q_INVOKABLE void addABLine(const QString& name);
    Q_INVOKABLE void changeABLineName(int index, const QString& newName);

    // ===== Q_INVOKABLE ALIASES FOR QML CONSISTENCY =====
    Q_INVOKABLE void settings_save() { settingsSave(); }
    Q_INVOKABLE void settings_revert() { settingsReload(); }
    // modules_send_252 not needed - modulesSend252() already exists as Q_INVOKABLE

    /******************
     * formgps_ui.cpp *
     ******************/
    //or should be under formgps_settings.cpp?

   /**********************
     * OpenGL.Designer.cs *
     **********************/
    ulong number = 0, lastNumber = 0;

    bool isHeadlandClose = false;

    double lightbarDistance=0;
    QString strHeading;
    int lenth = 4;

    void MakeFlagMark(QOpenGLFunctions *gl);
    void DrawFlags(QOpenGLFunctions *gl, QMatrix4x4 mvp);
    void DrawTramMarkers();
    void CalcFrustum(const QMatrix4x4 &mvp);
    void calculateMinMax();

    QVector3D mouseClickToField(int mouseX, int mouseY);
    QVector3D mouseClickToPan(int mouseX, int mouseY);

    void loadGLTextures();

private:
    bool toSend = false, isSA = false;

    // Language translation system
    QTranslator* m_translator;

    // PHASE 6.0.42: GPS jump detection for automatic field close and OpenGL regeneration
    double m_lastKnownLatitude = 0;
    double m_lastKnownLongitude = 0;
    const double GPS_JUMP_THRESHOLD_KM = 1.0;  // 1 km threshold for jump detection
    double latK, lonK = 0.0;

private:
    void setupGui();
    void setupAgIOService();
    void connectToAgIOFactoryInstance(); // New: connect to factory-created instance
    void testAgIOConfiguration();
    void connectFormLoopToAgIOService();
    void cleanupAgIOService();

    // PHASE 6.0.42: GPS jump detection helper functions
    bool detectGPSJump(double newLat, double newLon);
    void handleGPSJump(double newLat, double newLon);


    /**************
     * FormGPS.cs *
     **************/
public:
    void JobNew();
    void JobClose();

    /****************
     * form_sim.cpp *
     ****************/
    void simConnectSlots();

    /**************************
     * UI/Qt object callbacks *
     **************************/

public slots:
    // Phase 6.0.21: Receive parsed data from AgIOService
    void onParsedDataReady(const PGNParser::ParsedData& data);

    // Phase 6.0.25: Separated data handlers for optimal performance
    void onNmeaDataReady(const PGNParser::ParsedData& data);    // GPS position updates
    void onImuDataReady(const PGNParser::ParsedData& data);     // External IMU updates
    void onSteerDataReady(const PGNParser::ParsedData& data);   // AutoSteer feedback
    void onMachineDataReady(const PGNParser::ParsedData& data); // Machine
    void onBlockageDataReady(const PGNParser::ParsedData& data); // Blockage data
    void onRateControlDataReady(const PGNParser::ParsedData& data); // Rate control data
    void onAhrsSettingsChanged(); // Update ahrs when IMU settings change


    /*******************
     * from FormGPS.cs *
     *******************/
    void tmrWatchdog_timeout();
    void processSectionLookahead(); //called when section lookahead GL stuff is rendered
    void processOverlapCount(); //called to calculate overlap stats

    void TimedMessageBox(int timeout, QString s1, QString s2);

    void on_qml_created(QObject *object, const QUrl &url);

    // Qt 6.8: Simplified - removed complex property binding slots

    //settings dialog callbacks
    void on_settings_reload();
    void on_settings_save();
    void on_language_changed(); // Dynamic language switching

    //vehicle callbacks
    void vehicle_saveas(QString vehicle_name);
    //void vehicle_load(int index);
    void vehicle_load(QString vehicle_name);
    void vehicle_delete(QString vehicle_name);
    void vehicle_update_list();


    //field callbacks
    void field_update_list();
    void field_close();
    void field_open(QString field_name);
    void field_new(QString field_name);
    void field_new_from(QString existing, QString field_name, int flags);
    void field_new_from_KML(QString field_name, QString file_name);
    void field_delete(QString field_name);
    void field_saveas(QString field_name);
    void field_load_json(QString field_name);
    void FindLatLon(QString filename);
    void LoadKMLBoundary(QString filename);

    //modules ui callback
    // Note: modulesSend238/251/252 are Q_INVOKABLE versions for QML

    //boundary UI for recording new boundary
    void boundary_new_from_KML(QString filename);

    void headland_save();
    void headlines_save();
    void headlines_load();

    //headland creation

    void resetDirection();
    //right column
    void contourPriority(bool isRight);
    //bottom row
    void onBtnTramlines_clicked();

    void snapSideways(double distance);
    void snapToPivot();

    //displaybuttons.qml

    void SwapDirection();
    void turnOffBoundAlarm();

    void centerOgl();

    void deleteAppliedArea();

    /***************************
     * from OpenGL.Designer.cs *
     ***************************/
    void render_main_fbo();
    void oglMain_Paint();
    void openGLControl_Initialized();
    void openGLControl_Shutdown();
    //void openGLControl_Resize();
    void onGLControl_clicked(const QVariant &event);
    void onGLControl_dragged(int startX, int startY, int mouseX, int mouseY);

    void oglBack_Paint();
    void openGLControlBack_Initialized();

    void oglZoom_Paint();

    /***
     * UDPCOMM.Designer.cs
     * formgps_udpcomm.cpp
     ***/
    // void ReceiveFromAgIO(); // ❌ REMOVED - AgIOService Workers handle all UDP communication

    /*******************
     * simulator       *
     * formgps_sim.cpp *
     *******************/
    void onSimNewPosition(double vtgSpeed,
                     double headingTrue,
                     double latitude,
                     double longitude, double hdop,
                     double altitude,
                     double satellitesTracked);

    // Phase 6.0.24: GPS timer callback for real GPS mode (40 Hz fixed rate)
    void onGPSTimerTimeout();

    /*
     * misc
     */
    void FileSaveEverythingBeforeClosingField(bool saveVehicle = true);
    void FileSaveTracks();

    /* formgps_classcallbacks.cpp */
    void onStoppedDriving();

signals:
    void do_processSectionLookahead();
    void do_processOverlapCount();

private:
    // OLD translator removed - now using m_translator

    // PHASE 6.0.33: Two-buffer pattern for position stability
    // Separates RAW GPS position (immutable) from CORRECTED position (computed)
    // Prevents cascade corrections when timer calls UpdateFixPosition() between GPS packets
    Vec2 m_rawGpsPosition;          // RAW GPS position (8 Hz updates, never corrected)
    QMutex m_rawGpsPositionMutex;   // Thread-safe access (onNmeaDataReady writes, UpdateFixPosition reads)

    // ===== Q_PROPERTY MEMBER VARIABLES =====
    // 69 members for optimized properties

};

#endif // FORMGPS_H
