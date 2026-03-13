#include <QtGlobal>
#include "cvehicle.h"
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QRgb>
#include <QQmlEngine>
#include <QJSEngine>
#include <QStandardPaths>
#include <QDir>
#include <QCoreApplication>
#include "classes/settingsmanager.h"
#include "cboundary.h"
#include "qmlutil.h"
#include "ctool.h"
#include "glm.h"
#include "glutils.h"
#include "cnmea.h"
#include "cabline.h"
#include "cabcurve.h"
#include "ccontour.h"
#include "ctrack.h"
#include "modulecomm.h"
#include "vehicleproperties.h"
#include "boundaryinterface.h"
#include <QLoggingCategory>

Q_LOGGING_CATEGORY (cvehicle, "cvehicle.qtagopengps")

// SomcoSoftware approach: Qt manages the singleton automatically

CVehicle* CVehicle::s_instance = nullptr;
QMutex CVehicle::s_mutex;
bool CVehicle::s_cpp_created = false;

CVehicle* CVehicle::instance()
{
    QMutexLocker locker(&s_mutex);
    if (!s_instance) {
        s_instance = new CVehicle();
        qDebug(cvehicle) << "Singleton created by C++ code.";
        s_cpp_created = true;
        // ensure cleanup on app exit
        QObject::connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit,
                         s_instance, []() {
                             delete s_instance; s_instance = nullptr;
                         });
    }
    return s_instance;
}

CVehicle *CVehicle::create(QQmlEngine *qmlEngine, QJSEngine *jsEngine) {
    Q_UNUSED(jsEngine)

    QMutexLocker locker(&s_mutex);

    if(!s_instance) {
        s_instance = new CVehicle();
        qDebug(cvehicle) << "Singleton created by QML engine.";
    } else if (s_cpp_created) {
        qmlEngine->setObjectOwnership(s_instance, QQmlEngine::CppOwnership);
    }

    return s_instance;
}

CVehicle::CVehicle(QObject* parent)
    : QObject(parent)
{
    // Initialize Qt 6.8 Q_OBJECT_BINDABLE_PROPERTY members
    m_isHydLiftOn = false;
    m_hydLiftDown = false;
    m_isChangingDirection = false;
    m_isReverse = false;
    m_leftTramState = 0;
    m_rightTramState = 0;
    m_vehicleList = QList<QVariant>{};

    // Create scene graph properties object
    m_vehicleProperties = new VehicleProperties(this);

    // Bind vehicle properties to SettingsManager
    auto *settings = SettingsManager::instance();

    m_vehicleProperties->bindable_type().setBinding(
        settings->bindablevehicle_vehicleType().makeBinding()
    );
    m_vehicleProperties->bindable_wheelBase().setBinding([settings]() {
        return static_cast<float>(settings->vehicle_wheelbase());
    });
    m_vehicleProperties->bindable_trackWidth().setBinding([settings]() {
        return static_cast<float>(settings->vehicle_trackWidth());
    });
    m_vehicleProperties->bindable_drawbarLength().setBinding([settings]() {
        if (settings->tool_isToolFront())
            return 0.0f;
        if (settings->tool_isToolRearFixed())
            return 0.0f;
        return static_cast<float>(settings->vehicle_hitchLength());
    });
    m_vehicleProperties->bindable_threePtLength().setBinding([settings]() {
        if (settings->tool_isToolFront())
            return 0.0f;
        if (settings->tool_isToolRearFixed())
            return static_cast<float>(settings->vehicle_hitchLength());
        return 0.0f;
    });
    m_vehicleProperties->bindable_frontHitchLength().setBinding([settings]() {
        if (settings->tool_isToolFront())
            return static_cast<float>(settings->vehicle_hitchLength());
        else
            return 0.0f;
    });
    m_vehicleProperties->bindable_antennaOffset().setBinding([settings]() {
        return static_cast<float>(settings->vehicle_antennaOffset());
    });
    m_vehicleProperties->bindable_antennaForward().setBinding([settings]() {
        return static_cast<float>(settings->vehicle_antennaPivot());
    });
    m_vehicleProperties->bindable_svennArrow().setBinding(
        settings->bindabledisplay_isSvennArrowOn().makeBinding()
    );
    m_vehicleProperties->bindable_steerAngle().setBinding(
        ModuleComm::instance()->bindable_actualSteerAngleDegrees().makeBinding()
        );

    m_vehicleProperties->bindable_markBoundary().setBinding([]() {
        float markBoundary = 0;

        if (BoundaryInterface::instance()->isBndBeingMade()) {
            markBoundary = BoundaryInterface::instance()->createBndOffset();

            if (!BoundaryInterface::instance()->isDrawRightSide()) {
                markBoundary = -markBoundary;
            }

        }
        return markBoundary;
    });

}


QRect find_bounding_box(int viewport_height, QVector3D p1, QVector3D p2, QVector3D p3, QVector3D p4) {
    float x_min = glm::FLOAT_MAX;
    float x_max = glm::FLOAT_MIN;
    float y_min = glm::FLOAT_MAX;
    float y_max = glm::FLOAT_MIN;

    if(p1.x() < x_min) x_min = p1.x();
    if(p1.x() > x_max) x_max = p1.x();
    if(p1.y() < y_min) y_min = p1.y();
    if(p1.y() > y_max) y_max = p1.y();

    if(p2.x() < x_min) x_min = p2.x();
    if(p2.x() > x_max) x_max = p2.x();
    if(p2.y() < y_min) y_min = p2.y();
    if(p2.y() > y_max) y_max = p2.y();

    if(p3.x() < x_min) x_min = p3.x();
    if(p3.x() > x_max) x_max = p3.x();
    if(p3.y() < y_min) y_min = p3.y();
    if(p3.y() > y_max) y_max = p3.y();

    if(p4.x() < x_min) x_min = p4.x();
    if(p4.x() > x_max) x_max = p4.x();
    if(p4.y() < y_min) y_min = p4.y();
    if(p4.y() > y_max) y_max = p4.y();

    return QRect(x_min, viewport_height - y_max, x_max-x_min, y_max-y_min);
}

void CVehicle::loadSettings()
{
    vehicleType = SettingsManager::instance()->vehicle_vehicleType();

    isPivotBehindAntenna = SettingsManager::instance()->vehicle_isPivotBehindAntenna();
    isSteerAxleAhead = SettingsManager::instance()->vehicle_isSteerAxleAhead();

    antennaHeight = SettingsManager::instance()->vehicle_antennaHeight();
    antennaPivot = SettingsManager::instance()->vehicle_antennaPivot();
    antennaOffset = SettingsManager::instance()->vehicle_antennaOffset();

    trackWidth = SettingsManager::instance()->vehicle_trackWidth();
    wheelbase = SettingsManager::instance()->vehicle_wheelbase();

    maxAngularVelocity = SettingsManager::instance()->vehicle_maxAngularVelocity();
    maxSteerAngle = SettingsManager::instance()->vehicle_maxSteerAngle();
    slowSpeedCutoff = SettingsManager::instance()->vehicle_slowSpeedCutoff();
    panicStopSpeed = SettingsManager::instance()->vehicle_panicStopSpeed();

    //hydLiftLookAheadTime = SettingsManager::instance()->vehicle_hydraulicLiftLookAhead();

    goalPointLookAhead = SettingsManager::instance()->vehicle_goalPointLookAhead();
    goalPointLookAheadHold = SettingsManager::instance()->vehicle_goalPointLookAheadHold();
    goalPointLookAheadMult = SettingsManager::instance()->vehicle_goalPointLookAheadMult();

    stanleyDistanceErrorGain = SettingsManager::instance()->vehicle_stanleyDistanceErrorGain();
    stanleyHeadingErrorGain = SettingsManager::instance()->vehicle_stanleyHeadingErrorGain();
    stanleyIntegralGainAB = SettingsManager::instance()->vehicle_stanleyIntegralGainAB();
    stanleyIntegralDistanceAwayTriggerAB = SettingsManager::instance()->vehicle_stanleyIntegralDistanceAwayTriggerAB();
    purePursuitIntegralGain = SettingsManager::instance()->vehicle_purePursuitIntegralGainAB();

    //how far from line before it becomes Hold
    modeXTE = SettingsManager::instance()->as_modeXTE();

    //how long before hold is activated
    modeTime = SettingsManager::instance()->as_modeTime();

    functionSpeedLimit = SettingsManager::instance()->as_functionSpeedLimit();
    maxSteerSpeed = SettingsManager::instance()->as_maxSteerSpeed();
    minSteerSpeed = SettingsManager::instance()->as_minSteerSpeed();

    uturnCompensation = SettingsManager::instance()->as_uTurnCompensation();
}

void CVehicle::saveSettings()
{
    // Save all vehicle settings to SettingsManager (mirror of loadSettings)
    SettingsManager::instance()->setVehicle_vehicleType(vehicleType);

    SettingsManager::instance()->setVehicle_isPivotBehindAntenna(isPivotBehindAntenna);
    SettingsManager::instance()->setVehicle_isSteerAxleAhead(isSteerAxleAhead);

    SettingsManager::instance()->setVehicle_antennaHeight(antennaHeight);
    SettingsManager::instance()->setVehicle_antennaPivot(antennaPivot);
    SettingsManager::instance()->setVehicle_antennaOffset(antennaOffset);

    SettingsManager::instance()->setVehicle_trackWidth(trackWidth);
    SettingsManager::instance()->setVehicle_wheelbase(wheelbase);

    SettingsManager::instance()->setVehicle_maxAngularVelocity(maxAngularVelocity);
    SettingsManager::instance()->setVehicle_maxSteerAngle(maxSteerAngle);
    SettingsManager::instance()->setVehicle_slowSpeedCutoff(slowSpeedCutoff);
    SettingsManager::instance()->setVehicle_panicStopSpeed(panicStopSpeed);

    //SettingsManager::instance()->setVehicle_hydraulicLiftLookAhead(hydLiftLookAheadTime);

    // ⚡ PHASE 6.0.20 FIX 3: Do NOT overwrite Pure Pursuit settings
    // These are already updated by QML → SettingsManager directly
    // Writing member variables (loaded at startup) would overwrite QML changes
    // REMOVED: setVehicle_goalPointLookAhead(goalPointLookAhead);
    // REMOVED: setVehicle_goalPointLookAheadHold(goalPointLookAheadHold);
    // REMOVED: setVehicle_goalPointLookAheadMult(goalPointLookAheadMult);

    SettingsManager::instance()->setVehicle_stanleyDistanceErrorGain(stanleyDistanceErrorGain);
    SettingsManager::instance()->setVehicle_stanleyHeadingErrorGain(stanleyHeadingErrorGain);
    SettingsManager::instance()->setVehicle_stanleyIntegralGainAB(stanleyIntegralGainAB);
    SettingsManager::instance()->setVehicle_stanleyIntegralDistanceAwayTriggerAB(stanleyIntegralDistanceAwayTriggerAB);
    // ⚡ PHASE 6.0.20 FIX 3: Do NOT overwrite purePursuitIntegralGainAB
    // REMOVED: setVehicle_purePursuitIntegralGainAB(purePursuitIntegralGain);

    SettingsManager::instance()->setAs_maxSteerSpeed(maxSteerSpeed);
    SettingsManager::instance()->setAs_minSteerSpeed(minSteerSpeed);
    SettingsManager::instance()->setAs_uTurnCompensation(uturnCompensation);

    // Qt6 Pure Architecture: All setters auto-sync to INI immediately via macros
}


//called from various Classes, always needs current speed
double CVehicle::UpdateGoalPointDistance()
{
    ensureSettingsLoaded();  // Qt 6.8 MIGRATION: Lazy load settings

    // ⚡ PHASE 6.0.20 FIX 1: Load directly from SettingsManager for real-time updates
    // Member variables are not updated when QML changes settings → read fresh values
    double goalPointLookAhead = SettingsManager::instance()->vehicle_goalPointLookAhead();
    double goalPointLookAheadHold = SettingsManager::instance()->vehicle_goalPointLookAheadHold();
    double goalPointLookAheadMult = SettingsManager::instance()->vehicle_goalPointLookAheadMult();
    double modeXTE = SettingsManager::instance()->as_modeXTE();
    double modeTime = SettingsManager::instance()->as_modeTime();

    double xTE = fabs(m_modeActualXTE);

    //how far should goal point be away  - speed * seconds * kmph -> m/s then limit min value
    double goalPointDistance = m_avgSpeed * goalPointLookAhead * 0.05 * goalPointLookAheadMult;
    goalPointDistance += goalPointLookAhead;

    if (xTE < modeXTE)
    {
        if (modeTimeCounter > modeTime * 10)
        {
            goalPointDistance = m_avgSpeed * goalPointLookAheadHold * 0.05 * goalPointLookAheadMult;
            goalPointDistance += goalPointLookAheadHold;
        }
        else
        {
            modeTimeCounter++;
        }
    }
    else
    {
        modeTimeCounter = 0;
    }

    if (goalPointDistance < 1) goalPointDistance = 1;
    goalDistance = goalPointDistance;

    return goalPointDistance;


}

void CVehicle::DrawVehicle(QOpenGLFunctions *gl, QMatrix4x4 modelview,
                           QMatrix4x4 projection,
                           double steerAngle,
                           double markLeft,
                           double markRight,
                           double camSetDistance,
                           QRect viewport
                           )
{
    ensureSettingsLoaded();  // Qt 6.8 MIGRATION: Lazy load settings

    float display_lineWidth = 2.0f; //GL doesn't honor this
    bool display_isVehicleImage = SettingsManager::instance()->display_isVehicleImage();
    bool display_isSvennArrowOn = SettingsManager::instance()->display_isSvennArrowOn();
    display_lineWidth = SettingsManager::instance()->display_lineWidth();

    if (!std::isfinite(m_fixHeading.value()) || fabs(m_fixHeading) > 1000.0) {
        qWarning() << "DrawVehicle skipped: invalid fixHeading =" << m_fixHeading
                   << "(garbage not yet replaced by valid GPS/IMU data)";
        return;
    }

    //draw vehicle
    modelview.rotate(glm::toDegrees(-m_fixHeading), 0.0, 0.0, 1.0);

    GLHelperColors glcolors;
    GLHelperOneColor gldraw;
    GLHelperTexture gltex;
    //VertexTexcoord tc;

    QColor color;
    ColorVertex cv;

    QMatrix4x4 savedModelView = modelview;
    QMatrix4x4 mvp = projection*modelview;

    float vehicleOpacity = 1.0f; //TODO: mf.vehicleOpacity

    QVector3D p1;
    QVector3D p2;
    QVector3D p3;
    QVector3D p4;
    QVector3D s;

    s = QVector3D(0,0,0);
    p1 = s.project(modelview, projection, viewport);
    m_screenCoord = QPoint(p1.x(), p1.y());

    if (m_vehicleProperties->firstHeadingSet() && !SettingsManager::instance()->tool_isToolFront())
    {
        if (!SettingsManager::instance()->tool_isToolRearFixed())
        {
            //draw the rigid hitch
            color.setRgbF(0, 0, 0);
            gldraw.append(QVector3D(0, SettingsManager::instance()->vehicle_hitchLength(), 0));
            gldraw.append(QVector3D(0, 0, 0));
            gldraw.draw(gl,mvp,color,GL_LINES,4);

            gldraw.clear();
            color.setRgbF(1.237f, 0.037f, 0.0397f);
            gldraw.append(QVector3D(0, SettingsManager::instance()->vehicle_hitchLength(), 0));
            gldraw.append(QVector3D(0, 0, 0));
            gldraw.draw(gl,mvp,color,GL_LINES,1);
        }
        else
        {
            //draw the rigid hitch
            color.setRgbF(0, 0, 0);
            gldraw.append(QVector3D(-0.35, SettingsManager::instance()->vehicle_hitchLength(), 0));
            gldraw.append(QVector3D(-0.350, 0, 0));
            gldraw.append(QVector3D(0.35, SettingsManager::instance()->vehicle_hitchLength(), 0));
            gldraw.append(QVector3D(0.350, 0, 0));
            gldraw.draw(gl,mvp,color,GL_LINES,4);

            color.setRgbF(1.237f, 0.037f, 0.0397f);
            gldraw.append(QVector3D(-0.35, SettingsManager::instance()->vehicle_hitchLength(), 0));
            gldraw.append(QVector3D(-0.35, 0, 0));
            gldraw.append(QVector3D(0.35, SettingsManager::instance()->vehicle_hitchLength(), 0));
            gldraw.append(QVector3D(0.35, 0, 0));
            gldraw.draw(gl,mvp,color,GL_LINES,1);
        }
    }

    //draw the vehicle Body

    if (!m_vehicleProperties->firstHeadingSet())
    {

        //using texture 14, Textures::QUESTION_MARK
        gltex.append( { QVector3D(5,5,0), QVector2D(1,0) } );
        gltex.append( { QVector3D(1,5,0), QVector2D(0,0) } );
        gltex.append( { QVector3D(5,1,0), QVector2D(1,1) } );
        gltex.append( { QVector3D(1,1,0), QVector2D(0,1) } );
        gltex.draw(gl,mvp,Textures::QUESTION_MARK,GL_TRIANGLE_STRIP,false);

    }

    //3 vehicle types  tractor=0 harvestor=1 4wd=2
    if (display_isVehicleImage)
    {
        //get an on-screen coordinate and bounding box for the
        //vehicle for detecting when the vehicle is clicked
        s = QVector3D(0,0,0); //should be pivot axle
        p1 = s.project(modelview, projection, viewport);
        m_screenCoord = QPoint(p1.x(), viewport.height() - p1.y());

        if (vehicleType == 0)
        {
            //vehicle body
            //texture is 13, or Textures::TRACTOR, Wheel is 15

            //TODO: color.setRgbF(mf.vehicleColor.R, mf.vehicleColor.G, mf.vehicleColor.B, mf.vehicleOpacityByte);

            double leftAckerman, rightAckerman;

            if (steerAngle < 0)
            {
                leftAckerman = 1.25 * -steerAngle;
                rightAckerman = -steerAngle;
            }
            else
            {
                leftAckerman = -steerAngle;
                rightAckerman = 1.25 * -steerAngle;
            }

            //tractor body
            gltex.clear();
            gltex.append( { QVector3D(trackWidth, wheelbase * 1.5, 0.0),   QVector2D(1, 0) } );
            gltex.append( { QVector3D(-trackWidth, wheelbase * 1.5, 0.0),  QVector2D(0, 0) } );
            gltex.append( { QVector3D(trackWidth, -wheelbase * 0.5, 0.0),  QVector2D(1, 1) } );
            gltex.append( { QVector3D(-trackWidth, -wheelbase * 0.5, 0.0), QVector2D(0, 1) } );
            gltex.draw(gl,mvp,Textures::TRACTOR,GL_TRIANGLE_STRIP,false); //TODO: colorize

            s = QVector3D(-trackWidth, wheelbase * 1.5, 0.0); //front left corner
            p1 = s.project(modelview,projection, viewport);

            s = QVector3D(trackWidth, wheelbase * 1.5, 0.0); //front right corner
            p2 = s.project(modelview, projection, viewport);

            s = QVector3D(-trackWidth, -wheelbase * 0.5, 0.0); //rear left corne
            p3 = s.project(modelview, projection, viewport);

            s = QVector3D(trackWidth, -wheelbase * 0.5, 0.0); //rear right corner
            p4 = s.project(modelview, projection, viewport);

            m_screenBounding = find_bounding_box(viewport.height(),p1, p2, p3, p4);

            //right wheel
            //push modelview... nop because savedModelView already has a copy

            gltex.clear();
            gltex.append( { QVector3D(trackWidth * 0.5, wheelbase * 0.75, 0.0),   QVector2D(1, 0) } ); //Top Right
            gltex.append( { QVector3D(-trackWidth * 0.5, wheelbase * 0.75, 0.0),  QVector2D(0, 0) } ); //Top Left
            gltex.append( { QVector3D(trackWidth * 0.5, -wheelbase * 0.75, 0.0),  QVector2D(1, 1) } ); //Bottom Right
            gltex.append( { QVector3D(-trackWidth * 0.5, -wheelbase * 0.75, 0.0), QVector2D(0, 1) } ); //Bottom Left

            modelview.translate(trackWidth * 0.5, wheelbase, 0);
            modelview.rotate(rightAckerman, 0, 0, 1);

            gltex.draw(gl,projection*modelview,Textures::FRONT_WHEELS,GL_TRIANGLE_STRIP,false); //TODO: colorize

            modelview = savedModelView; //pop matrix

            //Left Wheel
            //push modelview, nop because savedModelView already has a copy

            gltex.clear();
            gltex.append( { QVector3D(trackWidth * 0.5, wheelbase * 0.75, 0.0),   QVector2D(1, 0) } ); //Top Right
            gltex.append( { QVector3D(-trackWidth * 0.5, wheelbase * 0.75, 0.0),  QVector2D(0, 0) } ); //Top Left
            gltex.append( { QVector3D(trackWidth * 0.5, -wheelbase * 0.75, 0.0),  QVector2D(1, 1) } ); //Bottom Right
            gltex.append( { QVector3D(-trackWidth * 0.5, -wheelbase * 0.75, 0.0), QVector2D(0, 1) } ); //Bottom Left

            modelview.translate(-trackWidth * 0.5, wheelbase, 0);
            modelview.rotate(leftAckerman, 0, 0, 1);

            gltex.draw(gl,projection*modelview,Textures::FRONT_WHEELS,GL_TRIANGLE_STRIP,false); //TODO: colorize

            modelview = savedModelView; //pop matrix
        }
        else if (vehicleType == 1) //Harvester
        {
            //TODO: color.setRgbF(0.078, 0.078, 0.078, mf.vehicleOpacityByte / 255.0);

            double leftAckerman, rightAckerman;

            if (steerAngle < 0)
            {
                leftAckerman = 1.25 * steerAngle;
                rightAckerman = steerAngle;
            }
            else
            {
                leftAckerman = steerAngle;
                rightAckerman = 1.25 * steerAngle;
            }

            //right wheel
            //push modelview... nop because savedModelView already has a copy

            gltex.clear();
            gltex.append( { QVector3D(trackWidth * 0.25, wheelbase * 0.5, 0.0),   QVector2D(1, 0) } ); //Top Right
            gltex.append( { QVector3D(-trackWidth * 0.25, wheelbase * 0.5, 0.0),  QVector2D(0, 0) } ); //Top Left
            gltex.append( { QVector3D(trackWidth * 0.25, -wheelbase * 0.5, 0.0),  QVector2D(1, 1) } ); //Bottom Right
            gltex.append( { QVector3D(-trackWidth * 0.25, -wheelbase * 0.5, 0.0), QVector2D(0, 1) } ); //Bottom Left

            modelview.translate(trackWidth * 0.5, -wheelbase, 0);
            modelview.rotate(rightAckerman, 0, 0, 1);

            gltex.draw(gl,projection*modelview,Textures::FRONT_WHEELS,GL_TRIANGLE_STRIP,false); //TODO: colorize

            modelview = savedModelView; //pop matrix

            //Left Wheel
            //push modelview, nop because savedModelView already has a copy

            gltex.clear();
            gltex.append( { QVector3D(trackWidth * 0.25, wheelbase * 0.5, 0.0),   QVector2D(1, 0) } ); //Top Right
            gltex.append( { QVector3D(-trackWidth * 0.25, wheelbase * 0.5, 0.0),  QVector2D(0, 0) } ); //Top Left
            gltex.append( { QVector3D(trackWidth * 0.25, -wheelbase * 0.5, 0.0),  QVector2D(1, 1) } ); //Bottom Right
            gltex.append( { QVector3D(-trackWidth * 0.25, -wheelbase * 0.5, 0.0), QVector2D(0, 1) } ); //Bottom Left

            modelview.translate(-trackWidth * 0.5, -wheelbase, 0);
            modelview.rotate(leftAckerman, 0, 0, 1);

            gltex.draw(gl,projection*modelview,Textures::FRONT_WHEELS,GL_TRIANGLE_STRIP,false); //TODO: colorize

            modelview = savedModelView; //pop matrix
            //harvester body
            //TODO colorize
            //color.setRgbF(mf.vehicleColor.R, mf.vehicleColor.G, mf.vehicleColor.B, mf.vehicleOpacityByte);
            gltex.clear();
            gltex.append( { QVector3D(trackWidth, wheelbase * 1.5, 0.0),   QVector2D(1, 0) } );
            gltex.append( { QVector3D(-trackWidth, wheelbase * 1.5, 0.0),  QVector2D(0, 0) } );
            gltex.append( { QVector3D(trackWidth, -wheelbase * 1.5, 0.0),  QVector2D(1, 1) } );
            gltex.append( { QVector3D(-trackWidth, -wheelbase * 1.5, 0.0), QVector2D(0, 1) } );
            gltex.draw(gl,mvp,Textures::HARVESTER,GL_TRIANGLE_STRIP,false); //TODO: colorize

            s = QVector3D(-trackWidth, wheelbase * 1.5, 0.0); //front left corner
            p1 = s.project(modelview,projection, viewport);

            s = QVector3D(trackWidth, wheelbase * 1.5, 0.0); //front right corner
            p2 = s.project(modelview, projection, viewport);

            s = QVector3D(-trackWidth, -wheelbase * 1.5, 0.0); //rear left corne
            p3 = s.project(modelview, projection, viewport);

            s = QVector3D(trackWidth, -wheelbase * 1.5, 0.0); //rear right corner
            p4 = s.project(modelview, projection, viewport);

            m_screenBounding = find_bounding_box(viewport.height(),p1, p2, p3, p4);
        }
        else if (vehicleType == 2) //4WD tractor, articulated
        {

            double modelSteerAngle = 0.5 * steerAngle;
            //TODO: color.setRgbF(mf.vehicleColor.R, mf.vehicleColor.G, mf.vehicleColor.B, mf.vehicleOpacityByte);

            //tractor rear
            //push modelview... nop because savedModelView already has a copy

            gltex.clear();
            gltex.append( { QVector3D(trackWidth, wheelbase * 0.65, 0.0),   QVector2D(1, 0) } ); //Top Right
            gltex.append( { QVector3D(-trackWidth, wheelbase * 0.65, 0.0),  QVector2D(0, 0) } ); //Top Left
            gltex.append( { QVector3D(trackWidth, -wheelbase * 0.65, 0.0),  QVector2D(1, 1) } ); //Bottom Right
            gltex.append( { QVector3D(-trackWidth, -wheelbase * 0.65, 0.0), QVector2D(0, 1) } ); //Bottom Left

            modelview.translate(0, -wheelbase * 0.5, 0);
            modelview.rotate(modelSteerAngle, 0, 0, 1);

            gltex.draw(gl,projection*modelview,Textures::TRACTOR_4WD_REAR,GL_TRIANGLE_STRIP,false); //TODO: colorize

            s = QVector3D(-trackWidth, -wheelbase * 0.65, 0.0); //rear left corner
            p3 = s.project(modelview, projection, viewport);

            s = QVector3D(trackWidth, -wheelbase * 0.65, 0.0); //rear right corner
            p4 = s.project(modelview, projection, viewport);

            modelview = savedModelView; //pop matrix

            //tractor front
            //push modelview, nop because savedModelView already has a copy

            gltex.clear();
            gltex.append( { QVector3D(trackWidth, wheelbase * 0.65, 0.0),   QVector2D(1, 0) } ); //Top Right
            gltex.append( { QVector3D(-trackWidth, wheelbase * 0.65, 0.0),  QVector2D(0, 0) } ); //Top Left
            gltex.append( { QVector3D(trackWidth, -wheelbase * 0.65, 0.0),  QVector2D(1, 1) } ); //Bottom Right
            gltex.append( { QVector3D(-trackWidth, -wheelbase * 0.65, 0.0), QVector2D(0, 1) } ); //Bottom Left

            modelview.translate(0, wheelbase*0.5, 0);
            modelview.rotate(-modelSteerAngle, 0, 0, 1);

            gltex.draw(gl,projection*modelview,Textures::TRACTOR_4WD_FRONT,GL_TRIANGLE_STRIP,false); //TODO: colorize

            s = QVector3D(-trackWidth, wheelbase * 0.65, 0.0); //front left corner
            p1 = s.project(modelview,projection, viewport);
            s = QVector3D(-trackWidth, wheelbase * 0.65, 0.0); //front right corner
            p2 = s.project(modelview,projection, viewport);

            m_screenBounding = find_bounding_box(viewport.height(),p1, p2, p3, p4);

            modelview = savedModelView; //pop matrix
        }
    }
    else
    { //just draw a triangle
        color.setRgbF(1.2, 1.20, 0.0, vehicleOpacity);
        glcolors.clear();
        glcolors.append( { QVector3D(0, antennaPivot, -0.0), QVector4D(0.0, 1.20, 1.22, vehicleOpacity) } );
        glcolors.append( { QVector3D(0, wheelbase, 0.0),     QVector4D(1.220, 0.0, 1.2, vehicleOpacity) } );
        glcolors.append( { QVector3D(-1.0, -0, 0.0), QVector4D(1.220, 0.0, 1.2, vehicleOpacity) } );
        glcolors.append( { QVector3D(1.0, -0, 0.0),  QVector4D(1.220, 0.0, 1.2, vehicleOpacity) } );
        glcolors.draw(gl,mvp,GL_TRIANGLE_FAN,1.0f);

        gldraw.clear();
        color.setRgbF(0.12, 0.12, 0.12);
        gldraw.append(QVector3D(-1.0, 0, 0));
        gldraw.append(QVector3D(1.0, 0, 0));
        gldraw.append(QVector3D(0, wheelbase, 0));
        gldraw.draw(gl,mvp,color,GL_LINE_LOOP, 3.0f);

        s = QVector3D(0, wheelbase, 0); //front point
        p1 = s.project(modelview,projection, viewport);

        s = QVector3D(-1.0, 0, 0.0); //rear left corne
        p3 = s.project(modelview, projection, viewport);

        s = QVector3D(1.0, 0, 0.0); //rear right corner
        p4 = s.project(modelview, projection, viewport);

        m_screenBounding = find_bounding_box(viewport.height(), p1, p1, p3, p4);
    }

    if (camSetDistance > -75 && m_vehicleProperties->firstHeadingSet())
    {
        //draw the bright antenna dot
        gldraw.clear();
        color.setRgbF(0,0,0);
        gldraw.append(QVector3D(-antennaOffset, antennaPivot, 0.1));
        gldraw.draw(gl,mvp,color,GL_POINTS,16.0f);

        gldraw.clear();
        color.setRgbF(0.2,0.98,0.98);
        gldraw.append(QVector3D(-antennaOffset, antennaPivot, 0.1));
        gldraw.draw(gl,mvp,color,GL_POINTS,10.0f);
    }

    glcolors.clear();

    gl->glLineWidth(2);
    cv.color = QVector4D(0.0, 1.270, 0.0, 1.0);


    if (markLeft) {
        cv.vertex = QVector3D(0.0, 0, 0);
        glcolors.append(cv);

        cv.color = QVector4D(1.270, 1.220, 0.20, 1.0);
        cv.vertex = QVector3D(markLeft, 0.0f, 0.0f);
        glcolors.append(cv);

        cv.vertex = QVector3D(markLeft * 0.75, 0.25f, 0.0f);
        glcolors.append(cv);

        glcolors.draw(gl,mvp,GL_LINE_STRIP, 2);
    } else if (markRight) {
        cv.vertex = QVector3D(0.0, 0, 0);
        glcolors.append(cv);

        cv.color = QVector4D(1.270, 1.220, 0.20, 1.0);
        cv.vertex = QVector3D(-markRight, 0.0f, 0.0f);
        glcolors.append(cv);

        cv.vertex = QVector3D(-markRight * 0.75, 0.25f, 0.0f);
        glcolors.append(cv);

        glcolors.draw(gl,mvp,GL_LINE_STRIP, 2);
    }

    //Svenn Arrow

    if (display_isSvennArrowOn && camSetDistance > -1000)
    {
        //double offs = mf.curve.distanceFromCurrentLinePivot * 0.3;
        double svennDist = camSetDistance * -0.07;
        double svennWidth = svennDist * 0.22;

        gldraw.clear();
        color.setRgbF(1.2, 1.25, 0.10);
        gldraw.append(QVector3D(svennWidth, wheelbase + svennDist, 0.0));
        gldraw.append(QVector3D(0, wheelbase + svennWidth + 0.5 + svennDist, 0.0));
        gldraw.append(QVector3D(-svennWidth, wheelbase + svennDist, 0.0));

        gldraw.draw(gl,mvp,color,GL_LINE_STRIP,display_lineWidth);
    }

    //Track number and nudge offset done in QML
}

void CVehicle::AverageTheSpeed(double newSpeed) {
    // Phase 6.0.34: Fixed formula to match C# original (CNMEA.cs:50)
    // C#: mf.avgSpeed = (mf.avgSpeed * 0.75) + (speed * 0.25);
    // BEFORE (WRONG): avgSpeed = newSpeed * 0.75 + avgSpeed * 0.25;  // Inverted weights!
    m_avgSpeed = m_avgSpeed * 0.75 + newSpeed * 0.25;  // ✅ CORRECTED: 75% old + 25% new
}

// ===== Qt 6.8 QProperty Migration =====
// Qt 6.8 FIX: External implementations for Qt 6.8 QObject + QML_ELEMENT compatibility
void CVehicle::setIsHydLiftOn(bool value) {
    m_isHydLiftOn = value;
}

void CVehicle::setHydLiftDown(bool value) {
    m_hydLiftDown = value;
}

void CVehicle::setIsChangingDirection(bool value) {
    m_isChangingDirection = value;
}

void CVehicle::setIsReverse(bool value) {
    m_isReverse = value;
}

void CVehicle::setLeftTramState(int value) {
    m_leftTramState = value;
}

void CVehicle::setRightTramState(int value) {
    m_rightTramState = value;
}

void CVehicle::setVehicleList(const QList<QVariant>& value) {
    m_vehicleList = value;
}

void CVehicle::setIsInFreeDriveMode(bool new_mode) {
    m_isInFreeDriveMode = new_mode;
}

void CVehicle::setDriveFreeSteerAngle(double new_angle) {
    m_driveFreeSteerAngle = new_angle;
}

// ===== Qt 6.8 Rectangle Pattern Getters =====
bool CVehicle::isHydLiftOn() const {
    return m_isHydLiftOn;
}

bool CVehicle::hydLiftDown() const {
    return m_hydLiftDown;
}

bool CVehicle::isChangingDirection() const {
    return m_isChangingDirection;
}

bool CVehicle::isReverse() const {
    return m_isReverse;
}

int CVehicle::leftTramState() const {
    return m_leftTramState;
}

int CVehicle::rightTramState() const {
    return m_rightTramState;
}

bool CVehicle::isInFreeDriveMode() const {
    return m_isInFreeDriveMode;
}

double CVehicle::driveFreeSteerAngle() const {
    return m_driveFreeSteerAngle;
}

QList<QVariant> CVehicle::vehicleList() const {
    return m_vehicleList;
}

// ===== Qt 6.8 Rectangle Pattern Bindables =====
QBindable<bool> CVehicle::bindableIsHydLiftOn() {
    return QBindable<bool>(&m_isHydLiftOn);
}

QBindable<bool> CVehicle::bindableHydLiftDown() {
    return QBindable<bool>(&m_hydLiftDown);
}

QBindable<bool> CVehicle::bindableIsChangingDirection() {
    return QBindable<bool>(&m_isChangingDirection);
}

QBindable<bool> CVehicle::bindableIsReverse() {
    return QBindable<bool>(&m_isReverse);
}

QBindable<int> CVehicle::bindableLeftTramState() {
    return QBindable<int>(&m_leftTramState);
}

QBindable<int> CVehicle::bindableRightTramState() {
    return QBindable<int>(&m_rightTramState);
}

QBindable<QList<QVariant>> CVehicle::bindableVehicleList() {
    return QBindable<QList<QVariant>>(&m_vehicleList);
}

QBindable<bool> CVehicle::bindableIsInFreeDriveMode() {
    return QBindable<bool>(&m_isInFreeDriveMode);
}

QBindable<double> CVehicle::bindableDriveFreeSteerAngle() {
    return QBindable<double>(&m_driveFreeSteerAngle);
}

// ===== Thread-Safe Vehicle Management (Phase 1 Architecture) =====
// CVehicle emits signals that are connected to FormGPS slots thread-safely
// Q_INVOKABLE wrappers in .h emit signals for QML access

// Removed QML_SINGLETON factory function - using qmlRegisterSingletonInstance instead
