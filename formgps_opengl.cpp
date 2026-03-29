// Copyright (C) 2024 Michael Torrie and the QtAgOpenGPS Dev Team
// SPDX-License-Identifier: GNU General Public License v3.0 or later
//
// Main loop OpenGL stuff
//#include <QtOpenGL>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <assert.h>
#include "formgps.h"
#include "csection.h"
#include "cvehicle.h"
#include "ccontour.h"
#include "cabline.h"
#include "cboundary.h"
#include "aogrenderer.h"
#include "cnmea.h"
#include "qmlutil.h"
#include "glm.h"
#include "glutils.h"
#include "qmlutil.h"
#include "classes/agioservice.h"  // For zero-latency GPS access
#include "classes/settingsmanager.h"
#include "backend.h"
#include "backendaccess.h"
#include "boundaryinterface.h"
#include "mainwindowstate.h"
#include "flagsinterface.h"
#include "recordedpath.h"
#include "siminterface.h"
#include "modulecomm.h"
#include "worldgrid.h"
#include "camera.h"
#include "cpgn.h"
#include "rendering.h"

#include <assert.h>
#include <QElapsedTimer>  // For latency profiling

#include <QLabel>

Q_LOGGING_CATEGORY (form_opengl_log, "formgps_opengl.qtagopengps")

//TODO remote this #define
#define mc CModuleComm::instance()

extern QLabel *overlapPixelsWindow;
extern QOpenGLShaderProgram *interpColorShader; //pull from glutils.h

// Helper function to safely get OpenGL control properties with defaults
struct OpenGLViewport {
    QOpenGLContext *context = nullptr;
    int width = 800;
    int height = 600;
    double shiftX = 0.0;
    double shiftY = 0.0;
};

struct DrawElementsIndirectCommand {
    GLuint count;         // Number of indices
    GLuint instanceCount; // Number of instances
    GLuint firstIndex;    // Offset in index buffer
    GLuint baseVertex;    // Added to each index
    GLuint baseInstance;  // Base instance for instanced attributes
};

#define PATCHBUFFER_LENGTH 16 * 1024 * 1024 //16 MB
#define VERTEX_SIZE sizeof(ColorVertex) //combined vertex and color, 7 floats

QOpenGLContext *getGLContext(QQuickWindow *window) {
    auto *ri = window->rendererInterface();
    if (ri->graphicsApi() == QSGRendererInterface::OpenGL) {
        return static_cast<QOpenGLContext *>(
            ri->getResource(window, QSGRendererInterface::OpenGLContextResource));
    }
    return nullptr; // Not using OpenGL
}

OpenGLViewport getOpenGLViewport() {
    OpenGLViewport viewport;
    QObject *openglControl = Backend::instance()->aogRenderer;

    if (openglControl) {
        viewport.width = openglControl->property("width").toReal();
        viewport.height = openglControl->property("height").toReal();
        // ✅ PHASE 6.3.0 FIX: shiftX/shiftY are now Q_PROPERTY in AOGRendererInSG
        viewport.shiftX = openglControl->property("shiftX").toDouble();
        viewport.shiftY = openglControl->property("shiftY").toDouble();
        viewport.context = getGLContext(qobject_cast<QQuickItem *>(openglControl)->window());
    } else {
        qWarning() << "⚠️ OpenGL control not found - using default viewport settings";
        // Defaults already set in struct definition
    }

    return viewport;
}
/*
*/
// Latency profiler for real-time validation
class LatencyProfiler {
public:
    static void measure(const QString& operation) {
        static QElapsedTimer timer;
        static bool initialized = false;
        if (!initialized) {
            timer.start();
            initialized = true;
            return;
        }
        qint64 elapsed = timer.nsecsElapsed();
        if (elapsed > 1000) { // >1μs warning for AutoSteer safety
            qWarning() << "⚠️ LATENCY:" << operation << elapsed << "ns";
        }
        timer.restart();
    }
};

QVector3D FormGPS::mouseClickToPan(int mouseX, int mouseY)
{
    /* returns easting and northing relative to the tractor's hitch position,
     * useful for drag to pan
     */

    Camera &camera = *Camera::instance();

    QMatrix4x4 modelview;
    QMatrix4x4 projection;

    // Safe access to OpenGL viewport properties
    OpenGLViewport viewport = getOpenGLViewport();
    int width = viewport.width;
    int height = viewport.height;
    double shiftX = viewport.shiftX;
    double shiftY = viewport.shiftY;

    projection.setToIdentity();

    //to shift, translate projection here. -1,0,0 is far left, 1,0,0 is far right.
    projection.translate(shiftX,shiftY,0);

    //  Create a perspective transformation.
    projection.perspective(glm::toDegrees(fovy), width / (double)height, 1.0f, camDistanceFactor * camera.camSetDistance());
    modelview.setToIdentity();

    //camera does translations and rotations
    camera.SetWorldCam(modelview, CVehicle::instance()->pivotAxlePos.easting, CVehicle::instance()->pivotAxlePos.northing);
    modelview.translate(CVehicle::instance()->hitchPos.easting, CVehicle::instance()->hitchPos.northing, 0);
    //modelview.translate(sin(CVehicle::instance()->fixHeading()) * tool.hitchLength,
    //                        cos(CVehicle::instance()->fixHeading()) * tool.hitchLength, 0);
    if (camera.camFollowing())
        modelview.rotate(glm::toDegrees(-CVehicle::instance()->fixHeading()), 0.0, 0.0, 1.0);

    float x,y;
    x = mouseX;
    y = height - mouseY;

    //get point on the near plane
    QVector3D worldpoint_near = QVector3D( { x, y, 0} ).unproject(modelview,projection,QRect(0,0,width, height));
    //get point on the far plane
    QVector3D worldpoint_far = QVector3D( { x, y, 1} ).unproject(modelview, projection,QRect(0,0,width, height));
    //get direction vector from near to far
    QVector3D direction = worldpoint_far - worldpoint_near;
    //determine intercept with z=0 plane, and calculate easting and northing
    double lambda = -(worldpoint_near.z()) / direction.z();

    mouseEasting = worldpoint_near.x() + lambda * direction.x();
    mouseNorthing = worldpoint_near.y() + lambda * direction.y();

    QMatrix4x4 m;
    m.rotate(-CVehicle::instance()->fixHeading(), 0,0,1);

    QVector3D relative = QVector3D( { (float)mouseEasting, (float)mouseNorthing, 0 } );
    return relative;
}

QVector3D FormGPS::mouseClickToField(int mouseX, int mouseY)
{
    /* returns the field easting and northing position of a
     * mouse click
     */
    Camera &camera = *Camera::instance();

    QMatrix4x4 modelview;
    QMatrix4x4 projection;

    // Safe access to OpenGL viewport properties
    OpenGLViewport viewport = getOpenGLViewport();
    int width = viewport.width;
    int height = viewport.height;
    double shiftX = viewport.shiftX;
    double shiftY = viewport.shiftY;

    projection.setToIdentity();

    //to shift, translate projection here. -1,0,0 is far left, 1,0,0 is far right.
    projection.translate(shiftX,shiftY,0);

    //  Create a perspective transformation.
    projection.perspective(glm::toDegrees(fovy), width / (double)height, 1.0f, camDistanceFactor * camera.camSetDistance());
    modelview.setToIdentity();

    //camera does translations and rotations
    camera.SetWorldCam(modelview, CVehicle::instance()->pivotAxlePos.easting, CVehicle::instance()->pivotAxlePos.northing);
    //modelview.translate(CVehicle::instance()->pivotAxlePos.easting, CVehicle::instance()->pivotAxlePos.northing, 0);
    //modelview.translate(sin(CVehicle::instance()->fixHeading()) * tool.hitchLength,
    //                        cos(CVehicle::instance()->fixHeading()) * tool.hitchLength, 0);
    //if (camera.camFollowing)
    //    modelview.rotate(glm::toDegrees(-CVehicle::instance()->fixHeading()), 0.0, 0.0, 1.0);

    float x,y;
    x = mouseX;
    y = height - mouseY;

    //get point on the near plane
    QVector3D worldpoint_near = QVector3D( { x, y, 0} ).unproject(modelview,projection,QRect(0,0,width, height));
    //get point on the far plane
    QVector3D worldpoint_far = QVector3D( { x, y, 1} ).unproject(modelview, projection,QRect(0,0,width, height));
    //get direction vector from near to far
    QVector3D direction = worldpoint_far - worldpoint_near;
    //determine intercept with z=0 plane, and calculate easting and northing
    double lambda = -(worldpoint_near.z()) / direction.z();

    mouseEasting = worldpoint_near.x() + lambda * direction.x();
    mouseNorthing = worldpoint_near.y() + lambda * direction.y();

    QMatrix4x4 m;
    m.rotate(-CVehicle::instance()->fixHeading(), 0,0,1);

    QVector3D fieldCoord = QVector3D( { (float)mouseEasting, (float)mouseNorthing, 0 } );
    return fieldCoord;
}

void FormGPS::render_main_fbo()
{
    QOpenGLContext *glContext = QOpenGLContext::currentContext();
    QOpenGLFunctions *gl = glContext->functions();
    //int width = glContext->surface()->size().width();
    //int height = glContext->surface()->size().height();
    QMatrix4x4 projection;
    QMatrix4x4 modelview;
    QColor color;
    GLHelperTextureBack gldrawtex;
    //GLHelperOneColorBack gldrawtex;

    initializeBackShader();

    // Set The Blending Function For Translucency
    gl->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    gl->glCullFace(GL_BACK);

    gl->glEnable(GL_BLEND);
    gl->glClearColor(0.25122f, 0.258f, 0.275f, 1.0f);
    gl->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    gl->glDisable(GL_DEPTH_TEST);

    modelview.setToIdentity();
    projection.setToIdentity();

    // Safe access to OpenGL viewport properties
    OpenGLViewport viewport = getOpenGLViewport();
    int width = viewport.width;
    int height = viewport.height;
    double shiftX = viewport.shiftX;
    double shiftY = viewport.shiftY;

    //gl->glViewport(0,0,width,height);
    //qDebug(qgl) << "viewport is " << width << height;

    if (active_fbo >= 0) {

        gl->glActiveTexture(GL_TEXTURE0);
        gl->glBindTexture(GL_TEXTURE_2D, mainFBO[active_fbo]->texture());

        //qDebug(qgl) << "Texture is " << overPix.size();
        //QOpenGLTexture texture = QOpenGLTexture(grnPix.mirrored(false, true));
        //texture.bind();


        gldrawtex.append( { QVector3D(1, 1, 0), QVector2D(1,1) } ); //Top Right
        gldrawtex.append( { QVector3D(-1, 1, 0), QVector2D(0,1) } ); //Top Left
        gldrawtex.append( { QVector3D(1, -1, 0), QVector2D(1,0) } ); //Bottom Right
        gldrawtex.append( { QVector3D(-1, -1, 0), QVector2D(0,0) } ); //Bottom Left
        /*
        gldrawtex.append( QVector3D(0.75, 0.75, 0)); //Top Right
        gldrawtex.append( QVector3D(-0.75, 0.75, 0)); //Top Left
        gldrawtex.append( QVector3D(0.75, -0.75, 0)); //Bottom Right
        gldrawtex.append( QVector3D(-0.75, -0.75, 0)); //Bottom Left
        */

        gldrawtex.draw(gl, projection * modelview, GL_TRIANGLE_STRIP, false);
        //gldrawtex.draw(gl, projection * modelview, QColor::fromRgb(255,0,0), GL_LINE_STRIP, 1.0f );
        //texture.release();
        gl->glBindTexture(GL_TEXTURE_2D, 0); //unbind the texture
        //texture.destroy();
    }
    gl->glFlush();
}

void FormGPS::oglMain_Paint()
{
    OpenGLViewport viewport = getOpenGLViewport();
    BACKEND_TRACK(track);
    BACKEND_YT(yt);
    CContour &ct = track.contour;

    Camera &camera = *Camera::instance();

    QMatrix4x4 projection;
    QMatrix4x4 modelview;
    QColor color;
    GLHelperTexture gldrawtex;
    GLHelperColors gldrawcolors;
    GLHelperOneColor gldraw1;

    float lineWidth = SettingsManager::instance()->display_lineWidth();
    
    // Safe access to OpenGL viewport properties
    int width = viewport.width;
    int height = viewport.height;
    double shiftX = viewport.shiftX;
    double shiftY = viewport.shiftY;
    //gl->glViewport(oglX,oglY,width,height);
    //qDebug(qgl) << "viewport is " << width << height;

#ifdef USE_INDIRECT_RENDERING
    //If we're rendering to a texture, this method is being called
    //by the main thread from UpdateFixPostion and needs to create
    //its own OpenGL context.
    if (!mainOpenGLContext.isValid()) {
        if (viewport.context)
            mainOpenGLContext.setShareContext(viewport.context);
        mainOpenGLContext.create();
    }

    if (!mainSurface.isValid()) {
        QSurfaceFormat format = mainOpenGLContext.format();
        mainSurface.setFormat(format);
        mainSurface.create();
        auto r = mainSurface.isValid();
        qDebug(form_opengl_log) << "main surface creation: " << r;
    }

    auto result = mainOpenGLContext.makeCurrent(&mainSurface);

    QOpenGLFunctions *gl = mainOpenGLContext.functions();
#else
    QOpenGLContext *glContext = QOpenGLContext::currentContext();
    QOpenGLFunctions *gl = glContext->functions();
#endif

    initializeTextures();
    initializeShaders();

#ifdef USE_INDIRECT_RENDERING
    //we will work on the unused texture in case QML is rendering on
    //another core
    int working_fbo = (active_fbo < 0 ? 0 : active_fbo + 1 % 1);


    if (!mainFBO[working_fbo] || mainFBO[working_fbo]->size() != QSize(width,height)) {
        QOpenGLFramebufferObjectFormat format;
        format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
        // ✅ C++17 RAII: automatic memory management, no manual delete needed
        mainFBO[working_fbo].reset(new QOpenGLFramebufferObject(QSize(width,height), format));
    }

    mainFBO[working_fbo]->bind();

    mainOpenGLContext.functions()->glViewport(0,0,width,height);
#endif

    // Set The Blending Function For Translucency
    gl->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    gl->glCullFace(GL_BACK);

    //gl->glDisable(GL_CULL_FACE);

    //set the camera to right distance
    camera.SetZoom();

    //now move the "camera" to the calculated zoom settings
    //I had to move these functions here because if setZoom is called
    //from elsewhere in the GUI (say a button press), there's no GL
    //context to work with.
    projection.setToIdentity();

    //to shift, translate projection here. -1,0,0 is far left, 1,0,0 is far right.

    //warning.  Moving in the Y direction alters the way the field tilts in 3D view.
    //would need to adjust the camera.setWorldCam stuff.
    //But can move in X direction without issue
    projection.translate(shiftX,shiftY,0);


    //  Create a perspective transformation.
    projection.perspective(glm::toDegrees(fovy), width / (double)height, 1.0f, camDistanceFactor * camera.camSetDistance());

    //oglMain rendering, Draw

    int deadCam = 0;

    gl->glEnable(GL_BLEND);
    gl->glClearColor(0.25122f, 0.258f, 0.275f, 1.0f);
    gl->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    gl->glDisable(GL_DEPTH_TEST);
    //gl->glDisable(GL_TEXTURE_2D);

    if(Backend::instance()->m_fixFrame.sentenceCounter < 299)
    {
        if (isGPSPositionInitialized)
        {

            //  Clear the color and depth buffer.
            gl->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            if (isDay) gl->glClearColor(0.27f, 0.4f, 0.7f, 1.0f);
            else gl->glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

            modelview.setToIdentity();

            //camera does translations and rotations
            camera.SetWorldCam(modelview, CVehicle::instance()->pivotAxlePos.easting, CVehicle::instance()->pivotAxlePos.northing);

            //calculate the frustum planes for culling
            CalcFrustum(projection*modelview);

            QColor fieldcolor;
            if(isDay) {
                fieldcolor = fieldColorDay;
            } else {
                fieldcolor = fieldColorNight;
            }
            //draw the field ground images
            WorldGrid::instance()->DrawFieldSurface(gl, projection *modelview, isTextureOn, fieldcolor, camera.gridZoom());

            ////if grid is on draw it
            if (isGridOn)
                WorldGrid::instance()->DrawWorldGrid(gl, modelview, projection, camera.gridZoom(), QColor::fromRgbF(0,0,0,1));

            //OpenGL ES does not support wireframe in this way. If we want wireframe,
            //we'll have to do it with LINES
            //if (isDrawPolygons) gl->glPolygonMode(GL_FRONT, GL_LINE);

            gl->glEnable(GL_BLEND);

            //draw patches of sections
#ifndef Q_OS_ANDROID
            tool.DrawPatchesTrianglesGL(gl, projection*modelview, patchCounter, swFrame);
#else
            tool.DrawPatchesGL(gl,projection*modelview,patchCounter, camera.camSetDistance(), swFrame);
#endif

            qDebug(form_opengl_log) << "time after painting patches " << (float)swFrame.nsecsElapsed() / 1000000;

            if (tram.displayMode != 0) tram.DrawTram(gl,projection*modelview,camera.camSetDistance());

            //draw contour line if button on
            if (MainWindowState::instance()->isContourBtnOn())
            {
                ct.DrawContourLine(gl, projection*modelview, swFrame);
            }
            else// draw the current and reference AB Lines or CurveAB Ref and line
            {
                //when switching lines, draw the ghost
                track.DrawTrack(gl, projection*modelview, isFontOn, false, camera.camSetDistance(), yt);
            }

            track.DrawTrackNew(gl, projection*modelview);

            if (RecordedPath::instance()->isRecordOn) {
                RecordedPath::instance()->DrawRecordedLine(gl, projection*modelview);
                RecordedPath::instance()->DrawDubins(gl, projection*modelview);
            }

            if (bnd.bndList.count() > 0 || BoundaryInterface::instance()->isBndBeingMade() == true)
            {
                //draw Boundaries
                bnd.DrawFenceLines(CVehicle::instance()->pivotAxlePos, gl, projection*modelview);

                //draw the turnLines
                if (MainWindowState::instance()->isYouTurnBtnOn() && ! MainWindowState::instance()->isContourBtnOn())
                {
                    bnd.DrawFenceLines(CVehicle::instance()->pivotAxlePos, gl,projection*modelview);

                    color.setRgbF(0.3555f, 0.6232f, 0.20f); //TODO: not sure what color turnLines should actually be

                    for (int i = 0; i < bnd.bndList.count(); i++)
                    {
                        DrawPolygon(gl,projection*modelview,bnd.bndList[i].turnLine,lineWidth,color);
                    }
                }

                //Draw headland
                if (MainWindowState::instance()->isHeadlandOn())
                {
                    color.setRgbF(0.960f, 0.96232f, 0.30f);
                    DrawPolygon(gl,projection*modelview,bnd.bndList[0].hdLine,lineWidth,color);
                }
            }

            DrawFlags(gl, projection*modelview);

            //draw the vehicle/implement
            QMatrix4x4 mv = modelview; //push matrix
            // ✅ PHASE 6.3.0: InterfaceProperty guaranteed to be initialized before rendering

            QMatrix4x4 vehiclemv = modelview;
            vehiclemv.translate(CVehicle::instance()->pivotAxlePos.easting,
                                CVehicle::instance()->pivotAxlePos.northing, 0);

            //setup for tool rendering
            QMatrix4x4 toolmv = vehiclemv;
            //translate down to the hitch pin
            toolmv.translate(sin(CVehicle::instance()->fixHeading()) * tool.hitchLength,
                         cos(CVehicle::instance()->fixHeading()) * tool.hitchLength, 0);

            tool.DrawToolGL(gl,toolmv, projection,
                          Backend::instance()->isJobStarted(),
                          CVehicle::instance()->isHydLiftOn(),
                          camera.camSetDistance(),tram);
            double steerangle;


            //TODO: this is redundant. The simulator sets mc.actualSteerangleDegrees
            if(SimInterface::instance()->isRunning()) steerangle = SimInterface::instance()->steerAngleActual();
            else steerangle = ModuleComm::instance()->actualSteerAngleDegrees();

            double markLeft, markRight;
            if (BoundaryInterface::instance()->isBndBeingMade()) {
                if (BoundaryInterface::instance()->isDrawRightSide()) {
                    markLeft = 0;
                    markRight = BoundaryInterface::instance()->createBndOffset();
                } else {
                    markLeft = BoundaryInterface::instance()->createBndOffset();
                    markRight = 0;
                }
            } else {
                markLeft = 0;
                markRight = 0;
            }

            CVehicle::instance()->DrawVehicle(gl, vehiclemv,
                                              projection, steerangle,
                                              markLeft, markRight,
                                              camera.camSetDistance(),
                                              QRect(0,0,width,height)
                                              );

            if (camera.camSetDistance() > -150)
            {
                track.DrawTrackGoalPoint(gl, projection * modelview);
            }

            // 2D Ortho --------------------------
            //no need to "push" matrix since it will be regenerated next time
            projection.setToIdentity();
            //negative and positive on width, 0 at top to bottom ortho view
            projection.ortho(-(double)width / 2, (double)width / 2, (double)height, 0, -1, 1);

            //  Create the appropriate modelview matrix.
            modelview.setToIdentity();

            //lightbar, button status, compasstext, steer circle, Hyd Lift, RTK
            // alarm, etc are all done in QML based on properties in Interface.qml

            //uturn buttons drawn in QML

            if (tool.isDisplayTramControl && tram.displayMode != 0) { DrawTramMarkers(); }

            //if this is on, VehicleInterface.isHydLiftOn is true
            //why is this here? Should be somewhere in UpdateFixPosition()
            if (ModuleComm::instance()->p_239.pgn[CPGN_EF::hydLift] == 2)
            {
                CVehicle::instance()->setHydLiftDown(false); //VehicleInterface.hydLiftDown in QML
            }
            else
            {
                CVehicle::instance()->setHydLiftDown(true);
            }

            //Reverse indicator in QML
            //RTK alarm implemented in QML
            //RTK age implemented in QML
            //guidance line text implemented in QML

            //draw the zoom window
            if (Backend::instance()->isJobStarted())
            {
                /*TODO implement floating zoom windo
                if (threeSeconds != zoomUpdateCounter)
                {
                    zoomUpdateCounter = threeSeconds;
                    oglZoom.Refresh();
                }
                */
            }
            //this probably can be in an event handler from QML land
            if (leftMouseDownOnOpenGL) MakeFlagMark(gl); //TODO: not working, fix!
        }
        else
        {
            gl->glClear(GL_COLOR_BUFFER_BIT);
        }

    }
    else
    {
        modelview.setToIdentity();
        modelview.translate(0,0.3,-10);
        //rotate the camera down to look at fix
        //modelview.rotate(-60, 1.0, 0.0, 0.0);
        modelview.rotate(deadCam, 0.0, 1.0, 0.0);
        deadCam += 5;

        // 2D Ortho ---------------------------------------////////-------------------------------------------------

        //we don't need to save the matrix since it's regenerated every time through this method
        projection.setToIdentity();

        //negative and positive on width, 0 at top to bottom ortho view
        projection.ortho(-(double)width / 2, (double)width / 2, (double)height, 0, -1, 1);

        //  Create the appropriate modelview matrix.
        modelview.setToIdentity();

        color.setRgbF(0.98f, 0.98f, 0.70f);
        int edge = -(double)width / 2 + 10;

        drawText(gl,projection * modelview,edge,height - 240, "<-- AgIO ?",1.0,true,color);


        //GUI widgets have to be updated elsewhere
    }

    gl->glFlush();
    qDebug() << "End of all main rendering" << swFrame.elapsed();

    /*
    if (SettingsManager::instance()->display_showBack()) {
        overPix = mainFBO[working_fbo]->toImage().convertToFormat(QImage::Format_RGBX8888);
        qDebug(qgl) << "image size is: " << overPix.size();
        overlapPixelsWindow->setPixmap(QPixmap::fromImage(overPix));
    }
    */
#ifdef USE_INDIRECT_RENDERING

    mainFBO[working_fbo]->bindDefault();

    //tell GUI to swich to new texture;
    active_fbo = working_fbo;
#endif
}

/// Handles the OpenGLInitialized event of the openGLControl control.
void FormGPS::openGLControl_Initialized()
{
    //QOpenGLContext *glContext = QOpenGLContext::currentContext();

    //qmlview->resetOpenGLState();

    //Load all the textures
    //qDebug(qgl) << "initializing Open GL.";
    //initializeTextures();
    //qDebug(qgl) << "textures loaded.";
    //initializeShaders();
    //qDebug(qgl) << "shaders loaded.";

}

void FormGPS::openGLControl_Shutdown()
{

    //qDebug(qgl) << "OpenGL shutting down... destroying buffers and shaders";
    //qDebug(qgl) << QOpenGLContext::currentContext();
    //We should have a valid OpenGL context here so we can clean up shaders and buffers
    destroyShaders();
    destroyTextures();
    //destroy any openGL buffers.
    WorldGrid::instance()->destroyGLBuffers();
}

//back buffer openGL draw function
void FormGPS::oglBack_Paint()
{
    BACKEND_TRACK(track);

    //QOpenGLContext *glContext = QOpenGLContext::currentContext();
    QMatrix4x4 projection;
    QMatrix4x4 modelview;

    GLHelperOneColorBack gldraw;

    GLint oldviewport[4];

    //if there's no context we need to create one because
    //the qml renderer is in a different thread.
    if (!backOpenGLContext.isValid()) {
        backOpenGLContext.create();
    }

    if (!backSurface.isValid()) {
        QSurfaceFormat format = backOpenGLContext.format();
        backSurface.setFormat(format);
        backSurface.create();
        auto r = backSurface.isValid();
        qDebug(form_opengl_log) << "back surface creation: " << r;
    }

    auto result = backOpenGLContext.makeCurrent(&backSurface);

    initializeBackShader(); //compiler the shader we need if necessary

    /* use the QML context with an offscreen surface to draw
     * the lookahead triangles
     */
    if (!backFBO) {
        QOpenGLFramebufferObjectFormat format;
        format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
        // ✅ C++17 RAII: automatic memory management, no manual delete needed
        backFBO = std::make_unique<QOpenGLFramebufferObject>(QSize(500,300), format);
    }

    backFBO->bind();

    //save current viewport settings in case it conflicts with QML
    GLint origview[4];
    backOpenGLContext.functions()->glGetIntegerv(GL_VIEWPORT, origview);


    backOpenGLContext.functions()->glViewport(0,0,500,300);
    QOpenGLFunctions *gl = backOpenGLContext.functions();

    //int width = glContext->surface()->size().width();
    //int height = glContext->surface()->size().height();

    //  Load the identity.
    projection.setToIdentity();

    //projection.perspective(6.0f,1,1,6000);
    projection.perspective(glm::toDegrees((double)0.06f), 1.666666666666f, 50.0f, 520.0f);

    gl->glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    gl->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);	// Clear The Screen And The Depth Buffer
    //gl->glLoadIdentity();					// Reset The View
    modelview.setToIdentity();

    //back the camera up
    modelview.translate(0, 0, -500);

    //rotate camera so heading matched fix heading in the world
    //gl->glRotated(toDegrees(CVehicle::instance()->fixHeadingSection), 0, 0, 1);
    modelview.rotate(glm::toDegrees(tool.toolPos.heading), 0, 0, 1);

    modelview.translate(-tool.toolPos.easting - sin(tool.toolPos.heading) * 15,
                        -tool.toolPos.northing - cos(tool.toolPos.heading) * 15,
                        0);

    //patch color
    QColor patchColor = QColor::fromRgbF(0.0f, 0.5f, 0.0f);

    //to draw or not the triangle patch
    bool isDraw;

    double pivEplus = CVehicle::instance()->pivotAxlePos.easting + 50;
    double pivEminus = CVehicle::instance()->pivotAxlePos.easting - 50;
    double pivNplus = CVehicle::instance()->pivotAxlePos.northing + 50;
    double pivNminus = CVehicle::instance()->pivotAxlePos.northing - 50;

    //draw patches j= # of sections
    for (int j = 0; j < tool.triStrip.count(); j++)
    {
        //every time the section turns off and on is a new patch
        int patchCount = tool.triStrip[j].patchList.size();

        if (patchCount > 0)
        {
            //for every new chunk of patch
            for (QSharedPointer<QVector<QVector3D>> &triList: tool.triStrip[j].patchList)
            {
                isDraw = false;
                int count2 = triList->size();
                for (int i = 1; i < count2; i+=3)
                {
                    //determine if point is in frustum or not
                    if ((*triList)[i].x() > pivEplus)
                        continue;
                    if ((*triList)[i].x() < pivEminus)
                        continue;
                    if ((*triList)[i].y() > pivNplus)
                        continue;
                    if ((*triList)[i].y() < pivNminus)
                        continue;

                    //point is in frustum so draw the entire patch
                    isDraw = true;
                    break;
                }

                if (isDraw)
                {
                    QOpenGLBuffer triBuffer;

                    triBuffer.create();
                    triBuffer.bind();

                    //triangle lists are now using QVector3D, so we can allocate buffers
                    //directly from list data.

                    //first vertice is color, so we should skip it
                    triBuffer.allocate(triList->data() + 1, (count2-1) * sizeof(QVector3D));
                    //triBuffer.allocate(triList->data(), count2 * sizeof(QVector3D));
                    triBuffer.release();

                    //draw the triangles in each triangle strip
                    glDrawArraysColorBack(gl,projection*modelview,
                                      GL_TRIANGLE_STRIP, patchColor,
                                      triBuffer,GL_FLOAT,count2-1);

                    triBuffer.destroy();
                }
            }
        }
    }

    //draw tool bar for debugging
    //gldraw.clear();
    //gldraw.append(QVector3D(tool.section[0].leftPoint.easting, tool.section[0].leftPoint.northing,0.5));
    //gldraw.append(QVector3D(tool.section[tool.numOfSections-1].rightPoint.easting, tool.section[tool.numOfSections-1].rightPoint.northing,0.5));
    //gldraw.draw(gl,projection*modelview,QColor::fromRgb(255,0,0),GL_LINE_STRIP,1);

    //draw 245 green for the tram tracks

    if (tram.displayMode !=0 && tram.displayMode !=0 && (track.idx() > -1))
    {
        if ((tram.displayMode == 1 || tram.displayMode == 2))
        {
            for (int i = 0; i < tram.tramList.count(); i++)
            {
                gldraw.clear();
                for (int h = 0; h < tram.tramList[i]->count(); h++)
                    gldraw.append(QVector3D((*tram.tramList[i])[h].easting, (*tram.tramList[i])[h].northing, 0));
                gldraw.draw(gl,projection*modelview,QColor::fromRgb(0,245,0),GL_LINE_STRIP,8);
            }
        }

        if (tram.displayMode == 1 || tram.displayMode == 3)
        {
            gldraw.clear();
            for (int h = 0; h < tram.tramBndOuterArr.count(); h++)
                gldraw.append(QVector3D(tram.tramBndOuterArr[h].easting, tram.tramBndOuterArr[h].northing, 0));
            for (int h = 0; h < tram.tramBndInnerArr.count(); h++)
                gldraw.append(QVector3D(tram.tramBndInnerArr[h].easting, tram.tramBndInnerArr[h].northing, 0));
            gldraw.draw(gl,projection*modelview,QColor::fromRgb(0,245,0),GL_LINE_STRIP,8);
        }
    }

    //draw 240 green for boundary
    if (bnd.bndList.count() > 0)
    {
        ////draw the bnd line
        if (bnd.bndList[0].fenceLine.count() > 3)
        {
            DrawPolygonBack(gl,projection*modelview,bnd.bndList[0].fenceLine,3,QColor::fromRgb(0,240,0));
        }


        //draw 250 green for the headland
        if (MainWindowState::instance()->isHeadlandOn() && tool.isSectionControlledByHeadland)
        {
            DrawPolygonBack(gl,projection*modelview,bnd.bndList[0].hdLine,3,QColor::fromRgb(0,250,0));
        }
    }

    //finish it up - we need to read the ram of video card
    gl->glFlush();

    qDebug(form_opengl_log) << "Time after drawing back buffer: " << swFrame.elapsed();

    //read the whole block of pixels up to max lookahead, one read only
    //we'll use Qt's QImage function to grab it.
#if QT_VERSION < QT_VERSION_CHECK(6,9,0)
    tool.grnPixWindow = backFBO->toImage().mirrored(false, true).convertToFormat(QImage::Format_RGBX8888);
#else
    tool.grnPixWindow = backFBO->toImage().flipped().convertToFormat(QImage::Format_RGBX8888);
#endif

    qDebug(form_opengl_log) << "Time after glReadPixels: " << swFrame.elapsed();
    //qDebug(qgl) << grnPix.size();
    //QImage temp = grnPix.copy(tool.rpXPosition, 250, tool.rpWidth, 290 /*(int)rpHeight*/);
    //TODO: is thisn right?
    QImage temp = tool.grnPixWindow.copy(tool.rpXPosition, 0, tool.rpWidth, 290 /*(int)rpHeight*/);
    temp.setPixelColor(0,0,QColor::fromRgb(255,128,0));
    //grnPix = temp; //only show clipped image
    memcpy(grnPixels, temp.constBits(), temp.size().width() * temp.size().height() * 4);

    //The remaining code from the original method in the C# code is
    //broken out into a callback in formgps.cpp called
    //processSectionLookahead().

    backOpenGLContext.functions()->glFlush();

    //restore viewport
    backOpenGLContext.functions()->glViewport(origview[0], origview[1], origview[2], origview[3]);

    //restore QML's context
    backFBO->bindDefault();
    //resetOpenGLState();
}

//Draw section OpenGL window, not visible
void FormGPS::openGLControlBack_Initialized()
{
}

//back buffer openGL draw function
void FormGPS::oglZoom_Paint()
{
    //Because this is potentially running in another thread, we cannot
    //safely make any GUI calls to set buttons, etc.  So instead, we
    //do the GL drawing here and get the lookahead pixmap from GL here.
    //After this, this widget will emit a finished signal, where the main
    //thread can then run the second part of this function, which I've
    //split out into its own function.
    QOpenGLContext *glContext = QOpenGLContext::currentContext();
    QMatrix4x4 projection;
    QMatrix4x4 modelview;

    GLHelperOneColorBack gldraw;

    initializeBackShader(); //make sure shader is loaded

    /* use the QML context with an offscreen surface to draw
     * the lookahead triangles
     */
    if (!zoomFBO) {
        QOpenGLFramebufferObjectFormat format;
        format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
        // ✅ C++17 RAII: automatic memory management, no manual delete needed
        zoomFBO = std::make_unique<QOpenGLFramebufferObject>(QSize(400,400), format);
    }

    zoomFBO->bind();
    glContext->functions()->glViewport(0,0,400,400);
    QOpenGLFunctions *gl = glContext->functions();

    //int width = glContext->surface()->size().width();
    //int height = glContext->surface()->size().height();

    gl->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    gl->glCullFace(GL_BACK);
    gl->glClearColor(0, 0, 0, 1.0f);

    if (Backend::instance()->isJobStarted())
    {
        gl->glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
        projection.setToIdentity(); //Reset the view
        projection.perspective(glm::toDegrees((double)1.0f), 1.0f, 100.0f, 5000.0f);
        modelview.setToIdentity();

        calculateMinMax();
        //back the camera up

        modelview.translate(0, 0, -Backend::instance()->m_currentField.maxDistance);

        //translate to that spot in the world
        modelview.translate(-Backend::instance()->m_currentField.centerX, -Backend::instance()->m_currentField.centerY, 0);

        //draw patches
        int count2;

        for (int j = 0; j < tool.triStrip.count(); j++)
        {
            //every time the section turns off and on is a new patch
            int patchCount = tool.triStrip[j].patchList.count();

            if (patchCount > 0)
            {
                //for every new chunk of patch
                for (auto &triList: tool.triStrip[j].patchList)
                {
                    //draw the triangle in each triangle strip
                    gldraw.clear();

                    count2 = triList->count();
                    int mipmap = 2;

                    //if large enough patch and camera zoomed out, fake mipmap the patches, skip triangles
                    if (count2 >= (mipmap))
                    {
                        int step = mipmap;
                        for (int i = 1; i < count2; i += step)
                        {
                            gldraw.append(QVector3D((*triList)[i].x(), (*triList)[i].y(), 0)); i++;
                            gldraw.append(QVector3D((*triList)[i].x(), (*triList)[i].y(), 0)); i++;

                            //too small to mipmap it
                            if (count2 - i <= (mipmap))
                                break;
                        }
                    }

                    else
                    {
                        for (int i = 1; i < count2; i++) gldraw.append(QVector3D((*triList)[i].x(), (*triList)[i].y(), 0));
                    }

                    gldraw.draw(gl,projection*modelview,QColor::fromRgbF(0.5, 0.5, 0.5, 0.5),GL_TRIANGLE_STRIP,1.0f);
                }
            }
        } //end of section patches

        gl->glFlush();

        //oglZoom.SwapBuffers();

        //QImage overPix;

#if QT_VERSION < QT_VERSION_CHECK(6,9,0)
        overPix = zoomFBO->toImage().mirrored(false, true).convertToFormat(QImage::Format_RGBX8888);
#else
        overPix = zoomFBO->toImage().flipped().convertToFormat(QImage::Format_RGBX8888);
#endif
        memcpy(overPixels, overPix.constBits(), overPix.size().width() * overPix.size().height() * 4);
    }

    glContext->functions()->glFlush();

    //restore QML's context
    zoomFBO->bindDefault();
}

void FormGPS::MakeFlagMark(QOpenGLFunctions *gl)
{
    leftMouseDownOnOpenGL = false;

    qDebug(form_opengl_log) << "mouse down at " << mouseX << ", " << mouseY;

    //figure out what the screen coordinates of all the flags are
    //check for clicks within a bounding box.
    //select it.  popup box to edit name or allow delete?

    //if no flag is clicked on, deselect flag. Maybe do this in
    //qml and javascript?
}

//DrawTramMarkers moved to QML
void FormGPS::DrawTramMarkers()
{
    /*3 states. On, off, manual on
     *  0 and 1 are automatic
     *  0 off
     *  1 on
     *  2 manual
     *  "flash" happens in qml.
     *  I just changed dot_color to have the 3 states
     */
    int dot_color;

    if (((tram.controlByte) & 2) == 2) dot_color = 1;// set to green #49FD49
    else dot_color = 0;//

    if ((bool)tram.isLeftManualOn())
    {
        dot_color = 2;
    }
    CVehicle::instance()->setLeftTramIndicator(dot_color);
    //done with left

    if (((tram.controlByte) & 1) == 1) dot_color = 1;
    else dot_color = 0;

    if ((bool)tram.isRightManualOn())
    {
        dot_color = 2;
    }
    CVehicle::instance()->setRightTramIndicator(dot_color);
}

void FormGPS::DrawFlags(QOpenGLFunctions *gl, QMatrix4x4 mvp)
{
    GLHelperOneColor gldraw;

    int flagCnt = FlagsInterface::instance()->count();
    for (int f = 0; f < flagCnt; f++)
    {
        QColor color;
        QString flagColor = "&";
        FlagModel::Flag flag = FlagsInterface::instance()->flagModel()->flagAt(f+1);

        if (flag.color == 0) {
            color = QColor::fromRgb(255, 0, flag.id);
        }
        if (flag.color == 1) {
            color = QColor::fromRgb(0, 255, flag.id);
            flagColor = "|";
        }
        if (flag.color == 2) {
            color = QColor::fromRgb(255, 255, flag.id);
            flagColor = "~";
        }

        gldraw.append(QVector3D(flag.easting, flag.northing, 0));
        gldraw.draw(gl, mvp, color, GL_POINTS, 8.0f);
        flagColor += flag.notes;

        drawText3D(gl, mvp, flag.easting, flag.northing, flagColor,1,true,color);

        if (FlagsInterface::instance()->currentFlag() == (f + 1)) {
            ////draw the box around flag
            gldraw.clear();

            double zoomValue = SettingsManager::instance()->display_camZoom();

            double offSet = (zoomValue * zoomValue * 0.01);
            gldraw.append(QVector3D(flag.easting, flag.northing + offSet, 0));
            gldraw.append(QVector3D(flag.easting - offSet, flag.northing, 0));
            gldraw.append(QVector3D(flag.easting, flag.northing - offSet, 0));
            gldraw.append(QVector3D(flag.easting + offSet, flag.northing, 0));
            gldraw.append(QVector3D(flag.easting, flag.northing + offSet, 0));
            gldraw.draw(gl, mvp, QColor::fromRgbF(0.980f, 0.0f, 0.980f),
                        GL_LINE_STRIP, 4.0);

            //draw line to vehicle
            gldraw.clear();
            gl->glLineWidth(2);
            //TODO: implement with shader: GL.LineStipple(1, 0x0707);
            gldraw.append(QVector3D(CVehicle::instance()->pivotAxlePos.easting, CVehicle::instance()->pivotAxlePos.northing, 0));
            gldraw.append(QVector3D(flag.easting, flag.northing, 0));
            gldraw.draw(gl, mvp,
                        QColor::fromRgbF(0.930f, 0.72f, 0.32f),
                        GL_LINES, 1.0);

        }
    }
}

void FormGPS::CalcFrustum(const QMatrix4x4 &mvp)
{
    //const float *clip = mvp.constData(); //column major order

    // Extract the RIGHT clipping plane
    frustum[0] = mvp(3,0) - mvp(0,0);
    frustum[1] = mvp(3,1) - mvp(0,1);
    frustum[2] = mvp(3,2) - mvp(0,2);
    frustum[3] = mvp(3,3) - mvp(0,3);
    //frustum[0] = clip[3] - clip[0];
    //frustum[1] = clip[7] - clip[4];
    //frustum[2] = clip[11] - clip[8];
    //frustum[3] = clip[15] - clip[12];

    // Extract the LEFT clipping plane
    frustum[4] = mvp(0,0) + mvp(3,0);
    frustum[5] = mvp(0,1) + mvp(3,1);
    frustum[6] = mvp(0,2) + mvp(3,2);
    frustum[7] = mvp(0,3) + mvp(3,3);
    //frustum[4] = clip[3] + clip[0];
    //frustum[5] = clip[7] + clip[4];
    //frustum[6] = clip[11] + clip[8];
    //frustum[7] = clip[15] + clip[12];

    // Extract the FAR clipping plane
    frustum[8] = mvp(3,0) - mvp(2,0);
    frustum[9] = mvp(3,1) - mvp(2,1);
    frustum[10] = mvp(3,2) - mvp(2,2);
    frustum[11] = mvp(3,3) - mvp(2,3);
    //frustum[8] = clip[3] - clip[2];
    //frustum[9] = clip[7] - clip[6];
    //frustum[10] = clip[11] - clip[10];
    //frustum[11] = clip[15] - clip[14];

    // Extract the NEAR clipping plane.  This is last on purpose (see pointinfrustum() for reason)
    frustum[12] = mvp(2,0) + mvp(3,0);
    frustum[13] = mvp(2,1) + mvp(3,1);
    frustum[14] = mvp(2,2) + mvp(3,2);
    frustum[15] = mvp(2,3) + mvp(3,3);
    //frustum[12] = clip[3] + clip[2];
    //frustum[13] = clip[7] + clip[6];
    //frustum[14] = clip[11] + clip[10];
    //frustum[15] = clip[15] + clip[14];

    // Extract the BOTTOM clipping plane
    frustum[16] = mvp(1,0) + mvp(3,0);
    frustum[17] = mvp(1,1) + mvp(3,1);
    frustum[18] = mvp(1,2) + mvp(3,2);
    frustum[19] = mvp(1,3) + mvp(3,3);
    //frustum[16] = clip[3] + clip[1];
    //frustum[17] = clip[7] + clip[5];
    //frustum[18] = clip[11] + clip[9];
    //frustum[19] = clip[15] + clip[13];

    // Extract the TOP clipping plane
    frustum[20] = mvp(3,0) - mvp(1,0);
    frustum[21] = mvp(3,1) - mvp(1,1);
    frustum[22] = mvp(3,2) - mvp(1,2);
    frustum[23] = mvp(3,3) - mvp(1,3);
    //frustum[20] = clip[3] - clip[1];
    //frustum[21] = clip[7] - clip[5];
    //frustum[22] = clip[11] - clip[9];
    //frustum[23] = clip[15] - clip[13];
}

//determine mins maxs of patches and whole field.
void FormGPS::calculateMinMax()
{

    //calculate field extents from boundary
    if (! bnd.CalculateMinMax() ) {
        //otherwise calculate from coverage
        //draw patches j= # of sections
        for (int j = 0; j < tool.triStrip.count(); j++)
        {
            //every time the section turns off and on is a new patch
            int patchCount = tool.triStrip[j].patchList.count();

            if (patchCount > 0)
            {
                //for every new chunk of patch
                for (QSharedPointer<PatchTriangleList> &triList: tool.triStrip[j].patchList)
                {
                    int count2 = triList->count();
                    for (int i = 0; i < count2; i += 3)
                    {
                        double x = (*triList)[i].x();
                        double y = (*triList)[i].y();

                        //also tally the max/min of field x and z
                        if (Backend::instance()->m_currentField.minX > x) Backend::instance()->m_currentField.minX = x;
                        if (Backend::instance()->m_currentField.maxX < x) Backend::instance()->m_currentField.maxX = x;
                        if (Backend::instance()->m_currentField.minY > y) Backend::instance()->m_currentField.minY = y;
                        if (Backend::instance()->m_currentField.maxY < y) Backend::instance()->m_currentField.maxY = y;
                    }
                }
            }
        }
    }

    if (Backend::instance()->m_currentField.minX == -9999999 ||
        Backend::instance()->m_currentField.maxX ==  9999999 ||
        Backend::instance()->m_currentField.minY == -9999999 ||
        Backend::instance()->m_currentField.maxY ==  9999999 )
    {
        //there was no boundary and no coverage
        Backend::instance()->m_currentField.minX = 0;
        Backend::instance()->m_currentField.minY = 0;
        Backend::instance()->m_currentField.maxX = 0;
        Backend::instance()->m_currentField.maxY = 0;
    }
    else
    {
        double maxFieldDistance;
        //the largest distancew across field
        double dist = fabs(Backend::instance()->m_currentField.minX - Backend::instance()->m_currentField.maxX);
        double dist2 = fabs(Backend::instance()->m_currentField.minY - Backend::instance()->m_currentField.maxY);

        if (dist > dist2) maxFieldDistance = (dist);
        else maxFieldDistance = (dist2);

        if (maxFieldDistance < 100) maxFieldDistance = 100;
        if (maxFieldDistance > 19900) maxFieldDistance = 19900;

        Backend::instance()->m_currentField.calcCenter();
        Backend::instance()->m_currentField.maxDistance = maxFieldDistance;
    }

    Backend::instance()->currentFieldChanged();
}
