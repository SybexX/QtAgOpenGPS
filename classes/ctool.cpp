#include "ctool.h"
#include "glm.h"
#include "glutils.h"
#include "classes/settingsmanager.h"
#include "glutils.h"
#include "ctram.h"
#include "cboundary.h"
#include "cvehicle.h"
#include "backend.h"
#include "mainwindowstate.h"
#include "modulecomm.h"
#include "tools.h"
#include "layerservice.h"

#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLBuffer>
#include <QLabel>
#include <QPainter>
#include <QLoggingCategory>

extern QLabel *overlapPixelsWindow;
extern QOpenGLShaderProgram *interpColorShader;

Q_LOGGING_CATEGORY (ctool_log, "ctool.qtagopengps")

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

struct PatchBuffer {
    QOpenGLBuffer patchBuffer;
    int length;
};

struct PatchInBuffer {
    int which;
    int offset;
    int length;
};

#define PATCHBUFFER_LENGTH 16 * 1024 * 1024 //16 MB
#define VERTEX_SIZE sizeof(ColorVertex) //combined vertex and color, 7 floats

void CTool::loadSettings()
{
    //from settings grab the vehicle specifics
    trailingToolToPivotLength = SettingsManager::instance()->tool_trailingToolToPivotLength();

    width = SettingsManager::instance()->vehicle_toolWidth();
    overlap = SettingsManager::instance()->vehicle_toolOverlap();

    offset = SettingsManager::instance()->vehicle_toolOffset();

    trailingHitchLength = SettingsManager::instance()->tool_toolTrailingHitchLength();
    tankTrailingHitchLength = SettingsManager::instance()->vehicle_tankTrailingHitchLength();
    hitchLength = SettingsManager::instance()->vehicle_hitchLength();

    isToolRearFixed = SettingsManager::instance()->tool_isToolRearFixed();
    isToolTrailing = SettingsManager::instance()->tool_isToolTrailing();
    isToolTBT = SettingsManager::instance()->tool_isTBT();
    isToolFrontFixed = SettingsManager::instance()->tool_isToolFront();

    lookAheadOnSetting = SettingsManager::instance()->vehicle_toolLookAheadOn();
    lookAheadOffSetting = SettingsManager::instance()->vehicle_toolLookAheadOff();
    turnOffDelay = SettingsManager::instance()->vehicle_toolOffDelay();

    isSectionOffWhenOut = SettingsManager::instance()->tool_isSectionOffWhenOut();

    isSectionsNotZones = SettingsManager::instance()->tool_isSectionsNotZones();

    if (isSectionsNotZones) {
        numOfSections = SettingsManager::instance()->vehicle_numSections();
    } else {
        numOfSections = SettingsManager::instance()->tool_numSectionsMulti();
    }
    LayerService::instance()->layers()->setSectionCount(numOfSections);

    minCoverage = SettingsManager::instance()->vehicle_minCoverage();
    isMultiColoredSections = SettingsManager::instance()->color_isMultiColorSections();

    secColors[0] = SettingsManager::instance()->color_sec01();
    secColors[1] = SettingsManager::instance()->color_sec02();
    secColors[2] = SettingsManager::instance()->color_sec03();
    secColors[3] = SettingsManager::instance()->color_sec04();
    secColors[4] = SettingsManager::instance()->color_sec05();
    secColors[5] = SettingsManager::instance()->color_sec06();
    secColors[6] = SettingsManager::instance()->color_sec07();
    secColors[7] = SettingsManager::instance()->color_sec08();
    secColors[8] = SettingsManager::instance()->color_sec09();
    secColors[9] = SettingsManager::instance()->color_sec10();
    secColors[10] = SettingsManager::instance()->color_sec11();
    secColors[11] = SettingsManager::instance()->color_sec12();
    secColors[12] = SettingsManager::instance()->color_sec13();
    secColors[13] = SettingsManager::instance()->color_sec14();
    secColors[14] = SettingsManager::instance()->color_sec15();
    secColors[15] = SettingsManager::instance()->color_sec16();

    for (int c=0 ; c < 16; c++) {
        //check setColor[C] to make sure there's nothing over 254
    }

    zoneRanges = SettingsManager::instance()->tool_zones();
    if (zoneRanges.size() > 0) {
        zones = zoneRanges[0];
    } else {
        qWarning(ctool_log) << "ERROR: tool_zones is empty! Size:" << zoneRanges.size();
        zones = 2; // valeur par défaut
    }
    //zoneRanges.removeAt(0); //remove first element since it was a count

    isDisplayTramControl = SettingsManager::instance()->tool_isDisplayTramControl();

}


CTool::CTool()
{
    // Initialize all section button states to Off
    for (int i = 0; i < 65; i++) {
        sectionButtonState[i] = SectionState::Off;
    }

    //get notified when the UI button changes state
    connect(MainWindowState::instance(), &MainWindowState::autoBtnStateChanged,
            this, &CTool::on_autoBtnChanged);

    connect(Tools::instance(), &Tools::sectionButtonStateChanged,
            this, &CTool::onSectionButtonStatechanged);

    connect(Backend::instance(), &Backend::resetTool,
            this, &CTool::resetTool);

    //load our local cached copies whenever the settings change
    connect(SettingsManager::instance(), &SettingsManager::toolGroupChanged, this, &CTool::loadSettings);
    connect(SettingsManager::instance(), &SettingsManager::vehicleGroupChanged, this, &CTool::loadSettings);
    connect(SettingsManager::instance(), &SettingsManager::colorGroupChanged, this, &CTool::loadSettings);

    loadSettings();
}

void CTool::saveSettings()
{
    // Save all tool settings to SettingsManager (mirror of loadSettings)
    SettingsManager::instance()->setTool_trailingToolToPivotLength(trailingToolToPivotLength);

    SettingsManager::instance()->setVehicle_toolWidth(width);
    SettingsManager::instance()->setVehicle_toolOverlap(overlap);
    SettingsManager::instance()->setVehicle_toolOffset(offset);

    SettingsManager::instance()->setTool_toolTrailingHitchLength(trailingHitchLength);
    SettingsManager::instance()->setVehicle_tankTrailingHitchLength(tankTrailingHitchLength);
    SettingsManager::instance()->setVehicle_hitchLength(hitchLength);

    SettingsManager::instance()->setTool_isToolRearFixed(isToolRearFixed);
    SettingsManager::instance()->setTool_isToolTrailing(isToolTrailing);
    SettingsManager::instance()->setTool_isTBT(isToolTBT);
    SettingsManager::instance()->setTool_isToolFront(isToolFrontFixed);

    SettingsManager::instance()->setVehicle_toolLookAheadOn(lookAheadOnSetting);
    SettingsManager::instance()->setVehicle_toolLookAheadOff(lookAheadOffSetting);
    SettingsManager::instance()->setVehicle_toolOffDelay(turnOffDelay);

    SettingsManager::instance()->setTool_isSectionOffWhenOut(isSectionOffWhenOut);
    SettingsManager::instance()->setTool_isSectionsNotZones(isSectionsNotZones);

    if (isSectionsNotZones) {
        SettingsManager::instance()->setVehicle_numSections(numOfSections);
        // Note: sectionWidthMulti is not a member variable, it's accessed directly from settings when needed
    } else {
        SettingsManager::instance()->setTool_numSectionsMulti(numOfSections);
        // Save zone ranges
        SettingsManager::instance()->setTool_zones(zoneRanges);
    }

    SettingsManager::instance()->setTool_isDisplayTramControl(isDisplayTramControl);
}

void CTool::DrawToolGL(QOpenGLFunctions *gl, QMatrix4x4 mv,
                       QMatrix4x4 projection,
                       bool isJobStarted,
                       bool isHydLiftOn,
                       double camSetDistance, CTram &tram)
{
    double tram_halfWheelTrack = SettingsManager::instance()->vehicle_trackWidth() * 0.5;
    bool tool_isDisplayTramControl = SettingsManager::instance()->tool_isDisplayTramControl();
    //translate and rotate at pivot axle, caller's mvp will be changed
    //all subsequent draws will be based on this point


    GLHelperOneColor gldraw;

    //settings doesn't change trailing hitch length if set to rigid, so do it here
    double trailingTank, trailingTool;
    if (isToolTrailing)
    {
        trailingTank = tankTrailingHitchLength;
        trailingTool = trailingHitchLength;
    }
    else { trailingTank = 0; trailingTool = 0; }

    //there is a trailing tow between hitch
    if (isToolTBT && isToolTrailing)
    {
        //rotate to tank heading
        mv.rotate(glm::toDegrees(-tankPos.heading), 0.0, 0.0, 1.0);


        //draw the tank hitch
        //draw the rigid hitch
        gldraw.append(QVector3D(-0.57, trailingTank, 0.0));
        gldraw.append(QVector3D(0, 0, 0));
        gldraw.append(QVector3D(0.57,trailingTank, 0.0));
        gldraw.draw(gl,projection*mv,QColor::fromRgbF(0.0, 0.0, 0.0),GL_LINE_LOOP, 6.0f);

        //draw the rigid hitch
        gldraw.draw(gl,projection*mv,QColor::fromRgbF(0.765f, 0.76f, 0.32f),GL_LINE_LOOP, 6.0f);

        //move down the tank hitch, unwind, rotate to section heading
        mv.translate(0.0, trailingTank, 0.0);
        mv.rotate(glm::toDegrees(tankPos.heading), 0.0, 0.0, 1.0);
        mv.rotate(glm::toDegrees(-toolPos.heading), 0.0, 0.0, 1.0);
    }

    //no tow between hitch
    else
    {
        mv.rotate(glm::toDegrees(-toolPos.heading), 0.0, 0.0, 1.0);
    }

    //draw the hitch if trailing
    if (isToolTrailing)
    {
        gldraw.clear();
        gldraw.append(QVector3D(-0.4 + offset, trailingTool, 0.0));
        gldraw.append(QVector3D(0,0,0));
        gldraw.append(QVector3D(0.4 + offset, trailingTool, 0.0));
        gldraw.draw(gl,projection*mv,QColor::fromRgbF(0.0f, 0.0f, 0.0f),GL_LINE_STRIP, 6.0f);

        gldraw.draw(gl,projection*mv,QColor::fromRgbF(0.7f, 0.4f, 0.2f),GL_LINE_STRIP, 1.0f);

        GLHelperTexture gldrawtex;

        gldrawtex.append( { QVector3D(1.5 + offset, trailingTool + 1, 0), QVector2D(1,0) } ); //Top Right
        gldrawtex.append( { QVector3D(-1.5 + offset, trailingTool + 1, 0), QVector2D(0,0) } ); //Top Left
        gldrawtex.append( { QVector3D(1.5 + offset, trailingTool - 1, 0), QVector2D(1,1) } ); //Bottom Right
        gldrawtex.append( { QVector3D(-1.5 + offset, trailingTool - 1, 0), QVector2D(0,1) } ); //Bottom LEft
        gldrawtex.draw(gl, projection*mv,Textures::TOOLWHEELS, GL_TRIANGLE_STRIP, true, QColor::fromRgbF(1,1,1,0.75));
    }

    trailingTool -= trailingToolToPivotLength;

    //look ahead lines
    GLHelperColors gldrawcolors;
    ColorVertex cv;
    QColor color;


    if (isJobStarted)
    {
        //lookahead section on
        cv.color = QVector4D(0.20f, 0.7f, 0.2f, 1);
        cv.vertex = QVector3D(farLeftPosition, (lookAheadDistanceOnPixelsLeft) * 0.1 + trailingTool, 0);
        gldrawcolors.append(cv);
        cv.vertex = QVector3D(farRightPosition, (lookAheadDistanceOnPixelsRight) * 0.1 + trailingTool, 0);
        gldrawcolors.append(cv);

        //lookahead section off
        cv.color = QVector4D(0.70f, 0.2f, 0.2f, 1);
        cv.vertex = QVector3D(farLeftPosition, (lookAheadDistanceOffPixelsLeft) * 0.1 + trailingTool, 0);
        gldrawcolors.append(cv);
        cv.vertex = QVector3D(farRightPosition, (lookAheadDistanceOffPixelsRight) * 0.1 + trailingTool, 0);
        gldrawcolors.append(cv);


        if (isHydLiftOn)
        {
            cv.color = QVector4D(0.70f, 0.2f, 0.72f, 1);
            cv.vertex = QVector3D(section[0].positionLeft, (hydLiftLookAheadDistanceLeft * 0.1) + trailingTool, 0);
            gldrawcolors.append(cv);
            cv.vertex = QVector3D(section[numOfSections - 1].positionRight, (hydLiftLookAheadDistanceRight * 0.1) + trailingTool, 0);
            gldrawcolors.append(cv);
        }

        gldrawcolors.draw(gl, projection * mv, GL_LINES, 1.0);
    }
    //draw the sections
    //line width 2 now

    double hite = camSetDistance / -150;
    if (hite > 12) hite = 12;
    if (hite < 1) hite = 1;

    for (int j = 0; j < numOfSections; j++)
    {
        //if section is on, green, if off, red color
        if (sectionButtonState[j] == SectionState::Auto)
        {
            // Mode Auto: couleur dépend de si section vraiment active (dans le champ)
            if (section[j].isSectionOn)
            {
                if (section[j].isMappingOn) color.setRgbF(0.0f, 0.95f, 0.0f, 1.0f);  // Vert si dans champ
                else color.setRgbF(0.970f, 0.30f, 0.970f);  // Magenta si pas de mapping
            }
            else
            {
                color.setRgbF(0.950f, 0.2f, 0.2f, 1.0f);  // Rouge si hors champ
            }
        }
        else if (sectionButtonState[j] == SectionState::On)
        {
            color.setRgbF(0.97, 0.97, 0, 1.0f);  // Jaune pour On (forçé)
        }
        else if (section[j].isSectionOn)
        {
            // Logic originale pour sections actives automatiquement
            if (section[j].isMappingOn) color.setRgbF(0.0f, 0.95f, 0.0f, 1.0f);
            else color.setRgbF(0.970f, 0.30f, 0.970f);
        }
        else
        {
            if (!section[j].isMappingOn)
                color.setRgbF(0.950f, 0.2f, 0.2f, 1.0f);
            else
                color.setRgbF(0.0f, 0.250f, 0.97f, 1.0f);
        }

        double mid = (section[j].positionRight - section[j].positionLeft) / 2 + section[j].positionLeft;

        gldraw.clear();
        gldraw.append(QVector3D(section[j].positionLeft, trailingTool, 0));
        gldraw.append(QVector3D(section[j].positionLeft, trailingTool - hite, 0));

        gldraw.append(QVector3D(mid, trailingTool - hite * 1.5, 0));

        gldraw.append(QVector3D(section[j].positionRight, trailingTool - hite, 0));
        gldraw.append(QVector3D(section[j].positionRight, trailingTool, 0));

        gldraw.draw(gl, projection * mv, color, GL_TRIANGLE_FAN, 2.0f);

        if (camSetDistance > -width * 200)
        {
            color.setRgbF(0.0, 0.0, 0.0);
            gldraw.draw(gl,projection * mv, color, GL_LINE_LOOP, 1.0);
        }
    }

    //zones
    if (!isSectionsNotZones && zones > 0 && camSetDistance > -150)
    {
        gldraw.clear();
        color.setRgbF(0.5f, 0.80f, 0.950f);
        for (int i = 1; i < zones; i++)
        {
            gldraw.append(QVector3D(section[zoneRanges[i]].positionLeft, trailingTool - 0.4, 0));
            gldraw.append(QVector3D(section[zoneRanges[i]].positionLeft, trailingTool + 0.2, 0));
        }
        gldraw.draw(gl, projection * mv, color, GL_LINES, 2.0f);
    }

    float pointSize;

    //tram Dots
    if ( tool_isDisplayTramControl && tram.displayMode != 0)
    {
        if (camSetDistance > -300)
        {
            if (camSetDistance > -100)
                pointSize = 16;
            else pointSize = 12;

            if (tram.isOuter)
            {
                //section markers
                gldrawcolors.clear();
                //right side
                if (((tram.controlByte) & 1) == 1) cv.color = QVector4D(0.0f, 0.900f, 0.39630f,1.0f);
                else cv.color = QVector4D(0, 0, 0, 1.0f);
                cv.vertex = QVector3D(farRightPosition - tram_halfWheelTrack, trailingTool, 0);
                gldrawcolors.append(cv);

                //left side
                if ((tram.controlByte & 2) == 2) cv.color = QVector4D(0.0f, 0.900f, 0.3930f, 1.0f);
                else cv.color = QVector4D(0, 0, 0, 1.0f);
                cv.vertex = QVector3D(farLeftPosition + tram_halfWheelTrack, trailingTool, 0);
                gldrawcolors.append(cv);

                gldrawcolors.draw(gl, projection * mv,GL_POINTS, pointSize);
            }
            else
            {
                gldrawcolors.clear();

                //right side
                if (((tram.controlByte) & 1) == 1) cv.color = QVector4D(0.0f, 0.900f, 0.39630f, 1.0f);
                else cv.color = QVector4D(0, 0, 0, 1.0f);
                cv.vertex = QVector3D(tram_halfWheelTrack, trailingTool, 0);
                gldrawcolors.append(cv);

                //left side
                if ((tram.controlByte & 2) == 2) cv.color = QVector4D(0.0f, 0.900f, 0.3930f, 1.0f);
                else cv.color = QVector4D(0, 0, 0, 1.0f);
                cv.vertex = QVector3D(-tram_halfWheelTrack, trailingTool, 0);
                gldrawcolors.append(cv);
                gldrawcolors.draw(gl, projection * mv, GL_POINTS, pointSize);
            }
        }
    }
}

void CTool::DrawPatchesGL(QOpenGLFunctions *gl,
                          QMatrix4x4 mvp,
                          int patchCounter,
                          double camSetDistance,
                          QElapsedTimer &swFrame)
{
    GLHelperOneColor gldraw1;
    int currentPatchBuffer = 0;
    double frustum[24];
    QColor color;

    double sinSectionHeading = sin(-toolPivotPos.heading);
    double cosSectionHeading = cos(-toolPivotPos.heading);

    CalcFrustum(mvp, frustum);

    if (patchesBufferDirty) {
        //destroy all GPU patches buffers
        patchBuffer.clear();
        patchesInBuffer.clear();

        for (int j = 0; j < triStrip.count(); j++) {
            patchesInBuffer.append(QVector<PatchInBuffer>());
            for (int k=0; k < triStrip[j].patchList.size()-1 ; k++) {
                patchesInBuffer[j].append({ -1, -1, -1});
            }
        }

        patchBuffer.append( { QOpenGLBuffer(), 0} );
        patchBuffer[0].patchBuffer.create();
        patchBuffer[0].patchBuffer.bind();
        patchBuffer[0].patchBuffer.allocate(PATCHBUFFER_LENGTH); //16 MB
        patchBuffer[0].patchBuffer.release();
        if (!patchesInBuffer.count()) {
            patchesInBuffer.append(QVector<PatchInBuffer>());
            patchesInBuffer[0].append({ -1, -1, -1});
        }
        currentPatchBuffer = 0;

        patchesBufferDirty = false;
    } else {
        currentPatchBuffer = patchBuffer.count() - 1;
    }

    bool draw_patch = false;
    //int total_vertices = 0;

    //initialize the steps for mipmap of triangles (skipping detail while zooming out)
    int mipmap = 0;
    if (camSetDistance < -800) mipmap = 2;
    if (camSetDistance < -1500) mipmap = 4;
    if (camSetDistance < -2400) mipmap = 8;
    if (camSetDistance < -5000) mipmap = 16;

    if (mipmap > 1)
        qDebug(ctool_log) << "mipmap is" << mipmap;

    //QVector<GLuint> indices;
    //indices.reserve(PATCHBUFFER_LENGTH / 28 * 3);  //enough to index 16 MB worth of vertices
    QVector<QVector<GLuint>> indices2;
    for (int i=0; i < patchBuffer.size(); i++) {
        indices2.append(QVector<GLuint>());
        indices2[i].reserve(PATCHBUFFER_LENGTH / 28 * 3);
    }

    bool enough_indices = false;

    //draw patches j= # of sections
    for (int j = 0; j < triStrip.count(); j++)
    {
        //every time the section turns off and on is a new patch
        int patchCount = triStrip[j].patchList.size();

        for (int k=0; k < patchCount; k++) {
            QSharedPointer<PatchTriangleList> triList = triStrip[j].patchList[k];
            QVector3D *triListRaw = triList->data();
            int count2 = triList->size();
            //total_vertices += count2;

            draw_patch = false;
            for (int i = 1; i < count2; i += 3) //first vertice is color
            {
                //determine if point is in frustum or not, if < 0, its outside so abort, z always is 0
                //x is easting, y is northing
                if (frustum[0] * triListRaw[i].x() + frustum[1] * triListRaw[i].y() + frustum[3] <= 0)
                    continue;//right
                if (frustum[4] * triListRaw[i].x() + frustum[5] * triListRaw[i].y() + frustum[7] <= 0)
                    continue;//left
                if (frustum[16] * triListRaw[i].x() + frustum[17] * triListRaw[i].y() + frustum[19] <= 0)
                    continue;//bottom
                if (frustum[20] * triListRaw[i].x() + frustum[21] * triListRaw[i].y() + frustum[23] <= 0)
                    continue;//top
                if (frustum[8] * triListRaw[i].x() + frustum[9] * triListRaw[i].y() + frustum[11] <= 0)
                    continue;//far
                if (frustum[12] * triListRaw[i].x() + frustum[13] * triListRaw[i].y() + frustum[15] <= 0)
                    continue;//near

                //point is in frustum so draw the entire patch. The downside of triangle strips.
                draw_patch = true;
                break;
            }

            if (!draw_patch) continue;
            color.setRgbF((*triList)[0].x(), (*triList)[0].y(), (*triList)[0].z(), 0.596 );

            if (k == patchCount - 1) {
                //If this is the last patch in the list, it's currently being worked on
                //so we don't save this one.
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
                glDrawArraysColor(gl,mvp,
                                  GL_TRIANGLE_STRIP, color,
                                  triBuffer,GL_FLOAT,count2-1);

                triBuffer.destroy();
                //qDebug(ctool) << "Last patch, not cached.";
                continue;
            } else {
                while (j >= patchesInBuffer.size())
                    patchesInBuffer.append(QVector<PatchInBuffer>());
                while (k >= patchesInBuffer[j].size())
                    patchesInBuffer[j].append({ -1, -1, -1});

                if (patchesInBuffer[j][k].which == -1) {
                    //patch is not in one of the big buffers yet, so allocate it.
                    if ((patchBuffer[currentPatchBuffer].length + (count2-1) * VERTEX_SIZE) >= PATCHBUFFER_LENGTH ) {
                        //add a new buffer because the current one is full.
                        currentPatchBuffer ++;
                        patchBuffer.append( { QOpenGLBuffer(), 0 });
                        patchBuffer[currentPatchBuffer].patchBuffer.create();
                        patchBuffer[currentPatchBuffer].patchBuffer.bind();
                        patchBuffer[currentPatchBuffer].patchBuffer.allocate(PATCHBUFFER_LENGTH); //4MB
                        patchBuffer[currentPatchBuffer].patchBuffer.release();
                        indices2.append(QVector<GLuint>());
                        indices2[currentPatchBuffer].reserve(PATCHBUFFER_LENGTH / 28 * 3);
                    }

                    //there's room for it in the current patch buffer
                    patchBuffer[currentPatchBuffer].patchBuffer.bind();
                    QVector<ColorVertex> temp_patch;
                    temp_patch.reserve(count2-1);
                    for (int i=1; i < count2; i++) {
                        temp_patch.append( { triListRaw[i], QVector4D(triListRaw[0], 0.596) } );
                    }
                    patchBuffer[currentPatchBuffer].patchBuffer.write(patchBuffer[currentPatchBuffer].length,
                                                                      temp_patch.data(),
                                                                      (count2-1) * VERTEX_SIZE);
                    patchesInBuffer[j][k].which = currentPatchBuffer;
                    patchesInBuffer[j][k].offset = patchBuffer[currentPatchBuffer].length / VERTEX_SIZE;
                    patchesInBuffer[j][k].length = count2 - 1;
                    patchBuffer[currentPatchBuffer].length += (count2 - 1) * VERTEX_SIZE;
                    qDebug(ctool_log) << "buffering" << j << k << patchesInBuffer[j][k].which << ", " << patchBuffer[currentPatchBuffer].length;
                    patchBuffer[currentPatchBuffer].patchBuffer.release();
                }
                //generate list of indices for this patch
                int index_offset = patchesInBuffer[j][k].offset;
                int which_buffer = patchesInBuffer[j][k].which;

                int step = mipmap;
                if (count2 - 1 < mipmap + 2) {
                    for (int i = 1; i < count2 - 2 ; i ++)
                    {
                        if (i % 2) {  //preserve winding order
                            indices2[which_buffer].append(i-1 + index_offset);
                            indices2[which_buffer].append(i   + index_offset);
                            indices2[which_buffer].append(i+1 + index_offset);
                        } else {
                            indices2[which_buffer].append(i-1 + index_offset);
                            indices2[which_buffer].append(i+1   + index_offset);
                            indices2[which_buffer].append(i + index_offset);
                        }

                    }
                } else {
                    //use mipmap to make fewer triangles
                    int last_index2 = indices2[which_buffer].count();

                    int vertex_count = 0;
                    for (int i=1; i < count2; i += step) {
                        //convert triangle strip to triangles
                        if (vertex_count > 2 ) { //even, normal winding
                            indices2[which_buffer].append(indices2[which_buffer][last_index2 - 1]);
                            indices2[which_buffer].append(indices2[which_buffer][last_index2 - 2]);
                            last_index2+=3;
                        } else {
                            last_index2 ++;
                        }
                        indices2[which_buffer].append(i-1 + index_offset);

                        i++;
                        vertex_count++;

                        if (vertex_count > 2) { //odd, reverse winding
                            indices2[which_buffer].append(indices2[which_buffer][last_index2 - 2]);
                        }
                        indices2[which_buffer].append(i-1 + index_offset);

                        if (vertex_count > 2) {
                            indices2[which_buffer].append(indices2[which_buffer][last_index2 - 1 ]);
                            last_index2 += 3;
                        } else {
                            last_index2 ++;
                        }
                        i++;
                        vertex_count++;

                        if (count2 - i <= (mipmap + 2))
                            //too small to mipmap, so add each one
                            //individually.
                            step = 0;
                    }
                }
                if (indices2[which_buffer].count() > 2)
                    enough_indices = true;
            }
        }

        qDebug(ctool_log) << "time after preparing patches for drawing" << swFrame.nsecsElapsed() / 1000000;

        if (enough_indices) {
            interpColorShader->bind();
            interpColorShader->setUniformValue("mvpMatrix", mvp);
            interpColorShader->setUniformValue("pointSize", 0.0f);

            //glDrawElements needs a vertex array object to hold state
            QOpenGLVertexArrayObject vao;
            vao.create();
            vao.bind();

            //create ibo
            QOpenGLBuffer ibo{QOpenGLBuffer::IndexBuffer};
            ibo.create();

            for (int i=0; i < indices2.count(); i++) {
                if (indices2[i].count() > 2) {
                    patchBuffer[i].patchBuffer.bind();

                    //set up vertex positions in buffer for the shader
                    gl->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), nullptr); //3D vector
                    gl->glEnableVertexAttribArray(0);

                    gl->glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 7 * sizeof(float), reinterpret_cast<void*>(3 * sizeof(float))); //color
                    gl->glEnableVertexAttribArray(1);


                    ibo.bind();
                    ibo.allocate(indices2[i].data(), indices2[i].size() * sizeof(GLuint));

                    gl->glDrawElements(GL_TRIANGLES, indices2[i].count(), GL_UNSIGNED_INT, nullptr);
                    patchBuffer[i].patchBuffer.release();

                    ibo.release();
                }
            }
            ibo.destroy();
            vao.release();
            vao.destroy();
            interpColorShader->release();
        }
    }

    // the follow up to sections patches
    int patchCount = 0;

    if (patchCounter > 0)
    {
        color = SettingsManager::instance()->display_colorSectionsDay();
        if (SettingsManager::instance()->display_isDayMode())
            color.setAlpha(152);
        else
            color.setAlpha(76);

        for (int j = 0; j < triStrip.count(); j++)
        {
            if (triStrip[j].isDrawing)
            {
                if (isMultiColoredSections)
                {
                    color = secColors[j];
                    color.setAlpha(152);
                }
                patchCount = triStrip[j].patchList.count();

               //draw the triangle in each triangle strip
                gldraw1.clear();

                //left side of triangle
                QVector3D pt((cosSectionHeading * section[triStrip[j].currentStartSectionNum].positionLeft) + toolPos.easting,
                             (sinSectionHeading * section[triStrip[j].currentStartSectionNum].positionLeft) + toolPos.northing, 0);
                gldraw1.append(pt);

                //Right side of triangle
                pt = QVector3D((cosSectionHeading * section[triStrip[j].currentEndSectionNum].positionRight) + toolPos.easting,
                               (sinSectionHeading * section[triStrip[j].currentEndSectionNum].positionRight) + toolPos.northing, 0);
                gldraw1.append(pt);

                int last = triStrip[j].patchList[patchCount -1]->count();
                //antenna
                gldraw1.append(QVector3D((*triStrip[j].patchList[patchCount-1])[last-2].x(), (*triStrip[j].patchList[patchCount-1])[last-2].y(),0));
                gldraw1.append(QVector3D((*triStrip[j].patchList[patchCount-1])[last-1].x(), (*triStrip[j].patchList[patchCount-1])[last-1].y(),0));

                gldraw1.draw(gl, mvp, color, GL_TRIANGLE_STRIP, 1.0f);
            }
        }
    }

}

void CTool::DrawPatchesTrianglesGL(QOpenGLFunctions *gl,
                                   QMatrix4x4 mvp,
                                   int patchCounter,
                                   QElapsedTimer &swFrame)
{
    GLHelperOneColor gldraw1;
    int currentPatchBuffer = 0;
    double frustum[24];
    QColor color;

    double sinSectionHeading = sin(-toolPivotPos.heading);
    double cosSectionHeading = cos(-toolPivotPos.heading);

    CalcFrustum(mvp, frustum);
    if (patchesBufferDirty) {
        //destroy all GPU patches buffers
        patchBuffer.clear();
        patchesInBuffer.clear();

        for (int j = 0; j < triStrip.count(); j++) {
            patchesInBuffer.append(QVector<PatchInBuffer>());
            for (int k=0; k < triStrip[j].patchList.size()-1 ; k++) {
                patchesInBuffer[j].append({ -1, -1, -1});
            }
        }

        patchBuffer.append( { QOpenGLBuffer(), 0} );
        patchBuffer[0].patchBuffer.create();
        patchBuffer[0].patchBuffer.bind();
        patchBuffer[0].patchBuffer.allocate(PATCHBUFFER_LENGTH); //16 MB
        patchBuffer[0].patchBuffer.release();
        if (!patchesInBuffer.count()) {
            patchesInBuffer.append(QVector<PatchInBuffer>());
            patchesInBuffer[0].append({ -1, -1, -1});
        }
        currentPatchBuffer = 0;

        patchesBufferDirty = false;
    } else {
        currentPatchBuffer = patchBuffer.count() - 1;
    }

    bool draw_patch = false;
    //int total_vertices = 0;

    bool enough_indices = false;

    //draw patches j= # of sections
    for (int j = 0; j < triStrip.count(); j++)
    {
        //every time the section turns off and on is a new patch
        int patchCount = triStrip[j].patchList.size();

        for (int k=0; k < patchCount; k++) {
            QSharedPointer<PatchTriangleList> triList = triStrip[j].patchList[k];
            QVector3D *triListRaw = triList->data();
            int count2 = triList->size();
            //total_vertices += count2;
            draw_patch = false;

            draw_patch = false;
            for (int i = 1; i < count2; i += 3) //first vertice is color
            {
                //determine if point is in frustum or not, if < 0, its outside so abort, z always is 0
                //x is easting, y is northing
                if (frustum[0] * triListRaw[i].x() + frustum[1] * triListRaw[i].y() + frustum[3] <= 0)
                    continue;//right
                if (frustum[4] * triListRaw[i].x() + frustum[5] * triListRaw[i].y() + frustum[7] <= 0)
                    continue;//left
                if (frustum[16] * triListRaw[i].x() + frustum[17] * triListRaw[i].y() + frustum[19] <= 0)
                    continue;//bottom
                if (frustum[20] * triListRaw[i].x() + frustum[21] * triListRaw[i].y() + frustum[23] <= 0)
                    continue;//top
                if (frustum[8] * triListRaw[i].x() + frustum[9] * triListRaw[i].y() + frustum[11] <= 0)
                    continue;//far
                if (frustum[12] * triListRaw[i].x() + frustum[13] * triListRaw[i].y() + frustum[15] <= 0)
                    continue;//near

                //point is in frustum so draw the entire patch. The downside of triangle strips.
                draw_patch = true;
                break;
            }

            if (!draw_patch) continue;
            color.setRgbF((*triList)[0].x(), (*triList)[0].y(), (*triList)[0].z(), 0.596 );

            if (k == patchCount - 1) {
                //If this is the last patch in the list, it's currently being worked on
                //so we don't save this one.
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
                glDrawArraysColor(gl,mvp,
                                  GL_TRIANGLE_STRIP, color,
                                  triBuffer,GL_FLOAT,count2-1);

                triBuffer.destroy();
                //qDebug(ctool) << "Last patch, not cached.";
                continue;
            } else {
                while (j >= patchesInBuffer.size())
                    patchesInBuffer.append(QVector<PatchInBuffer>());
                while (k >= patchesInBuffer[j].size())
                    patchesInBuffer[j].append({ -1, -1, -1});

                if (patchesInBuffer[j][k].which == -1) {
                    //patch is not in one of the big buffers yet, so allocate it.
                    if ((patchBuffer[currentPatchBuffer].length + (count2-3) * 3 * VERTEX_SIZE) >= PATCHBUFFER_LENGTH ) {
                        //add a new buffer because the current one is full.
                        currentPatchBuffer ++;
                        patchBuffer.append( { QOpenGLBuffer(), 0 });
                        patchBuffer[currentPatchBuffer].patchBuffer.create();
                        patchBuffer[currentPatchBuffer].patchBuffer.bind();
                        patchBuffer[currentPatchBuffer].patchBuffer.allocate(PATCHBUFFER_LENGTH); //4MB
                        patchBuffer[currentPatchBuffer].patchBuffer.release();
                    }

                    //there's room for it in the current patch buffer
                    patchBuffer[currentPatchBuffer].patchBuffer.bind();
                    QVector<ColorVertex> temp_patch;
                    temp_patch.reserve(count2-1);
                    for (int i=1; i < count2-2; i++) {
                        if (i % 2) {  //preserve winding order
                            temp_patch.append( { triListRaw[i], QVector4D(triListRaw[0], 0.596) } );
                            temp_patch.append( { triListRaw[i+1], QVector4D(triListRaw[0], 0.596) } );
                            temp_patch.append( { triListRaw[i+2], QVector4D(triListRaw[0], 0.596) } );
                        } else {
                            temp_patch.append( { triListRaw[i], QVector4D(triListRaw[0], 0.596) } );
                            temp_patch.append( { triListRaw[i+2], QVector4D(triListRaw[0], 0.596) } );
                            temp_patch.append( { triListRaw[i+1], QVector4D(triListRaw[0], 0.596) } );
                        }
                    }
                    patchBuffer[currentPatchBuffer].patchBuffer.write(patchBuffer[currentPatchBuffer].length,
                                                                      temp_patch.data(),
                                                                      (count2-3) * 3 * VERTEX_SIZE);
                    patchesInBuffer[j][k].which = currentPatchBuffer;
                    patchesInBuffer[j][k].offset = patchBuffer[currentPatchBuffer].length / VERTEX_SIZE;
                    patchesInBuffer[j][k].length = count2 - 1;
                    patchBuffer[currentPatchBuffer].length += (count2 - 1) * 3 * VERTEX_SIZE;
                    qDebug(ctool_log) << "buffering" << j << k << patchesInBuffer[j][k].which << ", " << patchBuffer[currentPatchBuffer].length;
                    patchBuffer[currentPatchBuffer].patchBuffer.release();
                }
            }
        }

        qDebug(ctool_log) << "time after preparing patches for drawing" << swFrame.nsecsElapsed() / 1000000;

        if (patchBuffer.size() && patchBuffer[0].length) {
            interpColorShader->bind();
            interpColorShader->setUniformValue("mvpMatrix", mvp);
            interpColorShader->setUniformValue("pointSize", 0.0f);

            for (int i=0; i < patchBuffer.count(); i++) {
                if (patchBuffer[i].length) {
                    //draw entire buffer of triangles since it's fast.

                    patchBuffer[i].patchBuffer.bind();

                    //set up vertex positions in buffer for the shader
                    gl->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), nullptr); //3D vector
                    gl->glEnableVertexAttribArray(0);

                    gl->glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 7 * sizeof(float), reinterpret_cast<void*>(3 * sizeof(float))); //color
                    gl->glEnableVertexAttribArray(1);

                    gl->glDrawArrays(GL_TRIANGLES,0,patchBuffer[i].length / VERTEX_SIZE);
                    patchBuffer[i].patchBuffer.release();
                }
            }
            interpColorShader->release();
        }
    }

    // the follow up to sections patches
    int patchCount = 0;

    if (patchCounter > 0)
    {
        color = SettingsManager::instance()->display_colorSectionsDay();
        if (SettingsManager::instance()->display_isDayMode())
            color.setAlpha(152);
        else
            color.setAlpha(76);

        for (int j = 0; j < triStrip.count(); j++)
        {
            if (triStrip[j].isDrawing)
            {
                if (isMultiColoredSections)
                {
                    color = secColors[j];
                    color.setAlpha(152);
                }
                patchCount = triStrip[j].patchList.count();

               //draw the triangle in each triangle strip
                gldraw1.clear();

                //left side of triangle
                QVector3D pt((cosSectionHeading * section[triStrip[j].currentStartSectionNum].positionLeft) + toolPos.easting,
                             (sinSectionHeading * section[triStrip[j].currentStartSectionNum].positionLeft) + toolPos.northing, 0);
                gldraw1.append(pt);

                //Right side of triangle
                pt = QVector3D((cosSectionHeading * section[triStrip[j].currentEndSectionNum].positionRight) + toolPos.easting,
                               (sinSectionHeading * section[triStrip[j].currentEndSectionNum].positionRight) + toolPos.northing, 0);
                gldraw1.append(pt);

                int last = triStrip[j].patchList[patchCount -1]->count();
                //antenna
                gldraw1.append(QVector3D((*triStrip[j].patchList[patchCount-1])[last-2].x(), (*triStrip[j].patchList[patchCount-1])[last-2].y(),0));
                gldraw1.append(QVector3D((*triStrip[j].patchList[patchCount-1])[last-1].x(), (*triStrip[j].patchList[patchCount-1])[last-1].y(),0));

                gldraw1.draw(gl, mvp, color, GL_TRIANGLE_STRIP, 1.0f);
            }
        }
    }
}

void CTool::DrawPatchesBack(QOpenGLFunctions *gl, QMatrix4x4 mvp)
{


}

void CTool::DrawPatchesBackQP(const CTram &tram,
                                const CBoundary &bnd,
                                Vec3 pivotAxlePos,
                                bool isHeadlandOn,
                                bool onTrack
                                )
{
    QMatrix4x4 projection;
    QMatrix4x4 modelview;
    QImage grnPix;

    //  Load the identity.
    projection.setToIdentity();

    //projection.perspective(6.0f,1,1,6000);
    projection.perspective(glm::toDegrees((double)0.06f), 1.666666666666f, 50.0f, 520.0f);

    grnPix = QImage(QSize(500,300), QImage::Format_RGBX8888);

    grnPix.fill(0);

    //gl->glLoadIdentity();					// Reset The View
    modelview.setToIdentity();

    //back the camera up
    modelview.translate(0, 0, -500);

    //rotate camera so heading matched fix heading in the world
    //gl->glRotated(toDegrees(CVehicle::instance()->fixHeadingSection), 0, 0, 1);
    modelview.rotate(glm::toDegrees(toolPos.heading), 0, 0, 1);

    modelview.translate(-toolPos.easting - sin(toolPos.heading) * 15,
                        -toolPos.northing - cos(toolPos.heading) * 15,
                        0);

    // Viewport: NDC to pixel coordinates
    QMatrix4x4 viewport;
    viewport.translate(500 / 2.0f, 300 / 2.0f, 0);
    viewport.scale(500 / 2.0f, -300 / 2.0f, 1);  // negative Y to flip

    QMatrix4x4 mvp = projection * modelview;


    //patch color
    QColor patchColor = QColor::fromRgbF(0.0f, 0.5f, 0.0f);

    QPainter painter;
    if (!painter.begin(&grnPix)) {
        qWarning() << "New GPS frame but back buffer painter is still working on the last one.";
        return;
    }

    painter.setRenderHint(QPainter::Antialiasing, false);

    painter.setPen(Qt::NoPen);

    QMatrix4x4 vmvp = viewport * mvp;

    painter.setTransform(vmvp.toTransform());
    painter.setBrush(QBrush(patchColor));

    QPolygonF triangle;
    QList<QLineF> lines;

    //to draw or not the triangle patch
    bool isDraw;

    double pivEplus = pivotAxlePos.easting + 50;
    double pivEminus = pivotAxlePos.easting - 50;
    double pivNplus = pivotAxlePos.northing + 50;
    double pivNminus = pivotAxlePos.northing - 50;

    //QPolygonF frustum({{pivEminus, pivNplus}, {pivEplus, pivNplus },
    //                   { pivEplus, pivNminus}, {pivEminus, pivNminus }});

    //draw patches j= # of sections
    for (int j = 0; j < triStrip.count(); j++)
    {
        //every time the section turns off and on is a new patch
        int patchCount = triStrip[j].patchList.size();

        if (patchCount > 0)
        {
            //for every new chunk of patch
            for (int k = 0; k < triStrip[j].patchList.size() ; k++)
            {
                isDraw = false;
                QSharedPointer<PatchTriangleList> triList = triStrip[j].patchList[k];
                QSharedPointer<PatchBoundingBox> bb = triStrip[j].patchBoundingBoxList[k];

                /*
                QPolygonF patchBox({{ (*bb).minx, (*bb).miny }, {(*bb).maxx, (*bb).miny},
                                    { (*bb).maxx, (*bb).maxy }, { (*bb).minx, (*bb).maxy } });

                if (frustum.intersects(patchBox))
                    isDraw = true;
                */

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
                    triangle.clear();
                    //triangle strip to polygon:
                    //first two vertices, then every other one to the end
                    //then from the end back to vertex #3, but every other one.

                    triangle.append(QPointF((*triList)[1].x(), (*triList)[1].y()));
                    triangle.append(QPointF((*triList)[2].x(), (*triList)[2].y()));

                    //even vertices after first two
                    for (int i=4; i < count2; i+=2) {
                        triangle.append(QPointF((*triList)[i].x(), (*triList)[i].y()));
                    }

                    //odd remaining vertices
                    for (int i=count2 - (count2 % 2 ? 2 : 1) ; i >2 ; i -=2) {
                        triangle.append(QPointF((*triList)[i].x(), (*triList)[i].y()));
                    }

                    painter.drawPolygon(triangle);

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
    QPen pen(QColor::fromRgb(0,245,0));
    pen.setCosmetic(true);
    pen.setWidth(8);
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);

    if (tram.displayMode !=0 && tram.displayMode !=0 && onTrack)
    {
        if ((tram.displayMode == 1 || tram.displayMode == 2))
        {

            for (int i = 0; i < tram.tramList.count(); i++)
            {
                lines.clear();
                for (int h = 1; h < tram.tramList[i]->count(); h++) {
                    lines.append(QLineF(vec2point((*tram.tramList[i])[h-1]),
                                        vec2point((*tram.tramList[i])[h])));
                }

                painter.drawLines(lines);
            }
        }

        if (tram.displayMode == 1 || tram.displayMode == 3)
        {
            lines.clear();
            for (int h = 0; h < tram.tramBndOuterArr.count(); h++) {
                lines.append(QLineF(vec2point(tram.tramBndOuterArr[h-1]),
                                    vec2point(tram.tramBndOuterArr[h])));
            }

            for (int h = 0; h < tram.tramBndInnerArr.count(); h++) {
                lines.append(QLineF(vec2point(tram.tramBndInnerArr[h-1]),
                                    vec2point(tram.tramBndInnerArr[h])));
            }

            painter.drawLines(lines);
        }
    }

    //draw 240 green for boundary
    if (bnd.bndList.count() > 0)
    {
        ////draw the bnd line
        if (bnd.bndList[0].fenceLine.count() > 3)
        {
            DrawPolygonBack(painter, bnd.bndList[0].fenceLine,3,QColor::fromRgb(0,240,0));
        }


        //draw 250 green for the headland
        if (MainWindowState::instance()->isHeadlandOn() && bnd.isSectionControlledByHeadland)
        {
            DrawPolygonBack(painter, bnd.bndList[0].hdLine,3,QColor::fromRgb(0,250,0));
        }
    }

    painter.end();

    //TODO adjust coordinate transformations above to eliminate this step
#if QT_VERSION < QT_VERSION_CHECK(6,9,0)
    grnPix = grnPix.mirrored(false, true).convertToFormat(QImage::Format_RGBX8888);
#else
    grnPix = grnPix.flipped().convertToFormat(QImage::Format_RGBX8888);
#endif

    QImage temp = grnPix.copy(rpXPosition, 0, rpWidth, 290 /*(int)rpHeight*/);
    temp.setPixelColor(0,0,QColor::fromRgb(255,128,0));
    //grnPix = temp; //only show clipped image
    //TODO: should be guarded with a lock
    memcpy(grnPixels, temp.constBits(), temp.size().width() * temp.size().height() * 4);
    grnPixWindow = grnPix.copy();
}


void CTool::sectionCalcWidths()
{
    if (isSectionsNotZones)
    {
        for (int j = 0; j < MAXSECTIONS; j++)
        {
            section[j].sectionWidth = (section[j].positionRight - section[j].positionLeft);
            section[j].rpSectionPosition = 250 + (int)(glm::roundMidAwayFromZero(section[j].positionLeft * 10));
            section[j].rpSectionWidth = (int)(glm::roundMidAwayFromZero(section[j].sectionWidth * 10));
            if (section[j].isSectionOn) applyWidth += section[j].sectionWidth;
        }

        //calculate tool width based on extreme right and left values
        double width = fabs(section[0].positionLeft) + fabs(section[numOfSections - 1].positionRight);
        SettingsManager::instance()->setVehicle_toolWidth(width);

        //left and right tool position
        farLeftPosition = section[0].positionLeft;
        farRightPosition = section[numOfSections - 1].positionRight;

        //find the right side pixel position
        rpXPosition = 250 + (int)(glm::roundMidAwayFromZero(farLeftPosition * 10));
        rpWidth = (int)(glm::roundMidAwayFromZero(width * 10));
    }
}

//moved from main form to here
void CTool::sectionCalcMulti()
{
    double leftside = width / -2.0;
    double defaultSectionWidth = SettingsManager::instance()->tool_sectionWidthMulti();
    double offset = SettingsManager::instance()->vehicle_toolOffset();
    section[0].positionLeft = leftside+offset;

    for (int i = 0; i < numOfSections - 1; i++)
    {
        leftside += defaultSectionWidth;

        section[i].positionRight = leftside + offset;
        section[i + 1].positionLeft = leftside + offset;
        section[i].sectionWidth = defaultSectionWidth;
        section[i].rpSectionPosition = 250 + (int)(glm::roundMidAwayFromZero(section[i].positionLeft * 10));
        section[i].rpSectionWidth = (int)(glm::roundMidAwayFromZero(section[i].sectionWidth * 10));
    }

    leftside += defaultSectionWidth;
    section[numOfSections - 1].positionRight = leftside + offset;
    section[numOfSections - 1].sectionWidth = defaultSectionWidth;
    section[numOfSections - 1].rpSectionPosition = 250 + (int)(glm::roundMidAwayFromZero(section[numOfSections - 1].positionLeft * 10));
    section[numOfSections - 1].rpSectionWidth = (int)(glm::roundMidAwayFromZero(section[numOfSections - 1].sectionWidth * 10));

    //calculate tool width based on extreme right and left values
    width = (section[numOfSections - 1].positionRight) - (section[0].positionLeft);
    SettingsManager::instance()->setVehicle_toolWidth(width);

    //left and right tool position
    farLeftPosition = section[0].positionLeft;
    farRightPosition = section[numOfSections - 1].positionRight;

    //find the right side pixel position
    rpXPosition = 250 + (int)(glm::roundMidAwayFromZero(farLeftPosition * 10));
    rpWidth = (int)(glm::roundMidAwayFromZero(width * 10));

}


void CTool::sectionSetPositions()
{
    double vehicle_toolOffset = SettingsManager::instance()->vehicle_toolOffset();
    double section_position1 = SettingsManager::instance()->section_position1();
    double section_position2 = SettingsManager::instance()->section_position2();
    double section_position3 = SettingsManager::instance()->section_position3();
    double section_position4 = SettingsManager::instance()->section_position4();
    double section_position5 = SettingsManager::instance()->section_position5();
    double section_position6 = SettingsManager::instance()->section_position6();
    double section_position7 = SettingsManager::instance()->section_position7();
    double section_position8 = SettingsManager::instance()->section_position8();
    double section_position9 = SettingsManager::instance()->section_position9();
    double section_position10 = SettingsManager::instance()->section_position10();
    double section_position11 = SettingsManager::instance()->section_position11();
    double section_position12 = SettingsManager::instance()->section_position12();
    double section_position13 = SettingsManager::instance()->section_position13();
    double section_position14 = SettingsManager::instance()->section_position14();
    double section_position15 = SettingsManager::instance()->section_position15();
    double section_position16 = SettingsManager::instance()->section_position16();
    double section_position17 = SettingsManager::instance()->section_position17();

    section[0].positionLeft = section_position1 + vehicle_toolOffset;
    section[0].positionRight = section_position2 + vehicle_toolOffset;

    section[1].positionLeft = section_position2 + vehicle_toolOffset;
    section[1].positionRight = section_position3 + vehicle_toolOffset;

    section[2].positionLeft = section_position3 + vehicle_toolOffset;
    section[2].positionRight = section_position4 + vehicle_toolOffset;

    section[3].positionLeft = section_position4 + vehicle_toolOffset;
    section[3].positionRight = section_position5 + vehicle_toolOffset;

    section[4].positionLeft = section_position5 + vehicle_toolOffset;
    section[4].positionRight = section_position6 + vehicle_toolOffset;

    section[5].positionLeft = section_position6 + vehicle_toolOffset;
    section[5].positionRight = section_position7 + vehicle_toolOffset;

    section[6].positionLeft = section_position7 + vehicle_toolOffset;
    section[6].positionRight = section_position8 + vehicle_toolOffset;

    section[7].positionLeft = section_position8 + vehicle_toolOffset;
    section[7].positionRight = section_position9 + vehicle_toolOffset;

    section[8].positionLeft = section_position9 + vehicle_toolOffset;
    section[8].positionRight = section_position10 + vehicle_toolOffset;

    section[9].positionLeft = section_position10 + vehicle_toolOffset;
    section[9].positionRight = section_position11 + vehicle_toolOffset;

    section[10].positionLeft = section_position11 + vehicle_toolOffset;
    section[10].positionRight = section_position12 + vehicle_toolOffset;

    section[11].positionLeft = section_position12 + vehicle_toolOffset;
    section[11].positionRight = section_position13 + vehicle_toolOffset;

    section[12].positionLeft = section_position13 + vehicle_toolOffset;
    section[12].positionRight = section_position14 + vehicle_toolOffset;

    section[13].positionLeft = section_position14 + vehicle_toolOffset;
    section[13].positionRight = section_position15 + vehicle_toolOffset;

    section[14].positionLeft = section_position15 + vehicle_toolOffset;
    section[14].positionRight = section_position16 + vehicle_toolOffset;

    section[15].positionLeft = section_position16 + vehicle_toolOffset;
    section[15].positionRight = section_position17 + vehicle_toolOffset;
}

void CTool::ProcessLookAhead(int gpsHz,
                             SectionState::State autoBtnState,
                             const CBoundary &bnd,
                             CTram &tram)
{
      //determine where the tool is wrt to headland
    if (MainWindowState::instance()->isHeadlandOn()) WhereAreToolCorners(bnd);

    //set the look ahead for hyd Lift in pixels per second
    hydLiftLookAheadDistanceLeft = farLeftSpeed * SettingsManager::instance()->vehicle_hydraulicLiftLookAhead() * 10;
    hydLiftLookAheadDistanceRight = farRightSpeed * SettingsManager::instance()->vehicle_hydraulicLiftLookAhead() * 10;

    if (hydLiftLookAheadDistanceLeft > 200) hydLiftLookAheadDistanceLeft = 200;
    if (hydLiftLookAheadDistanceRight > 200) hydLiftLookAheadDistanceRight = 200;

    lookAheadDistanceOnPixelsLeft = farLeftSpeed * lookAheadOnSetting * 10;
    lookAheadDistanceOnPixelsRight = farRightSpeed * lookAheadOnSetting * 10;

    if (lookAheadDistanceOnPixelsLeft > 200) lookAheadDistanceOnPixelsLeft = 200;
    if (lookAheadDistanceOnPixelsRight > 200) lookAheadDistanceOnPixelsRight = 200;

    lookAheadDistanceOffPixelsLeft = farLeftSpeed * lookAheadOffSetting * 10;
    lookAheadDistanceOffPixelsRight = farRightSpeed * lookAheadOffSetting * 10;

    if (lookAheadDistanceOffPixelsLeft > 160) lookAheadDistanceOffPixelsLeft = 160;
    if (lookAheadDistanceOffPixelsRight > 160) lookAheadDistanceOffPixelsRight = 160;

    //determine if section is in boundary and headland using the section left/right positions
    bool isLeftIn = true, isRightIn = true;

    if (bnd.bndList.count() > 0)
    {
        for (int j = 0; j < numOfSections; j++)
        {
            //only one first left point, the rest are all rights moved over to left
            isLeftIn = j == 0 ? bnd.IsPointInsideFenceArea(section[j].leftPoint) : isRightIn;
            isRightIn = bnd.IsPointInsideFenceArea(section[j].rightPoint);

            if (isSectionOffWhenOut)
            {
                //merge the two sides into in or out
                if (isLeftIn || isRightIn) section[j].isInBoundary = true;
                else section[j].isInBoundary = false;
            }
            else
            {
                //merge the two sides into in or out
                if (!isLeftIn || !isRightIn) section[j].isInBoundary = false;
                else section[j].isInBoundary = true;
            }
        }
    }

    //determine farthest ahead lookahead - is the height of the readpixel line
    double rpHeight = 0;
    double rpOnHeight = 0;
    double rpToolHeight = 0;

    //pick the larger side
    if (hydLiftLookAheadDistanceLeft > hydLiftLookAheadDistanceRight) rpToolHeight = hydLiftLookAheadDistanceLeft;
    else rpToolHeight = hydLiftLookAheadDistanceRight;

    if (lookAheadDistanceOnPixelsLeft > lookAheadDistanceOnPixelsRight) rpOnHeight = lookAheadDistanceOnPixelsLeft;
    else rpOnHeight = lookAheadDistanceOnPixelsRight;

    isHeadlandClose = false;

    //clamp the height after looking way ahead, this is for switching off super section only
    rpOnHeight = fabs(rpOnHeight);
    rpToolHeight = fabs(rpToolHeight);

    //10 % min is required for overlap, otherwise it never would be on.
    int pixLimit = (int)((double)(section[0].rpSectionWidth * rpOnHeight) / (double)(5.0));
    //bnd.isSectionControlledByHeadland = true;
    if ((rpOnHeight < rpToolHeight && MainWindowState::instance()->isHeadlandOn() && bnd.isSectionControlledByHeadland)) rpHeight = rpToolHeight + 2;
    else rpHeight = rpOnHeight + 2;
    //qDebug(qpos) << bnd.isSectionControlledByHeadland << "headland sections";

    if (rpHeight > 290) rpHeight = 290;
    if (rpHeight < 8) rpHeight = 8;

    //read the whole block of pixels up to max lookahead, one read only
    //pixels are already read in another thread.

    //determine if headland is in read pixel buffer left middle and right.
    int start = 0, end = 0, tagged = 0, totalPixel = 0;

    //slope of the look ahead line
    double mOn = 0, mOff = 0;

    //tram and hydraulics
    if (tram.displayMode > 0 && width > SettingsManager::instance()->vehicle_trackWidth())
    {
        tram.controlByte = 0;
        //1 pixels in is there a tram line?
        if (tram.isOuter)
        {
            if (grnPixels[(int)(tram.halfWheelTrack * 10)].green == 245) tram.controlByte += 2;
            if (grnPixels[rpWidth - (int)(tram.halfWheelTrack * 10)].green == 245) tram.controlByte += 1;
        }
        else
        {
            if (grnPixels[rpWidth / 2 - (int)(tram.halfWheelTrack * 10)].green == 245) tram.controlByte += 2;
            if (grnPixels[rpWidth / 2 + (int)(tram.halfWheelTrack * 10)].green == 245) tram.controlByte += 1;
        }
    }
    else tram.controlByte = 0;

    //determine if in or out of headland, do hydraulics if on
    if (MainWindowState::instance()->isHeadlandOn())
    {
        //calculate the slope
        double m = (hydLiftLookAheadDistanceRight - hydLiftLookAheadDistanceLeft) / rpWidth;
        int height = 1;

        for (int pos = 0; pos < rpWidth; pos++)
        {
            height = (int)(hydLiftLookAheadDistanceLeft + (m * pos)) - 1;
            for (int a = pos; a < height * rpWidth; a += rpWidth)
            {
                if (grnPixels[a].green == 250)
                {
                    isHeadlandClose = true;
                    goto GetOutTool;
                }
            }
        }

    GetOutTool: //goto

        //is the tool completely in the headland or not
        isToolInHeadland = isToolOuterPointsInHeadland && !isHeadlandClose;

        //set hydraulics based on tool in headland or not
        emit SetHydPosition(autoBtnState);

        //set hydraulics based on tool in headland or not
        emit SetHydPosition(autoBtnState);

    }

    ///////////////////////////////////////////   Section control        ssssssssssssssssssssss

    int endHeight = 1, startHeight = 1;

    if (MainWindowState::instance()->isHeadlandOn() && bnd.isSectionControlledByHeadland)
        WhereAreToolLookOnPoints(bnd);

    for (int j = 0; j < numOfSections; j++)
    {
        //Off or too slow or going backwards
        if (sectionButtonState[j] == SectionState::Off || CVehicle::instance()->avgSpeed() < SettingsManager::instance()->vehicle_slowSpeedCutoff() || section[j].speedPixels < 0)
        {
            section[j].sectionOnRequest = false;
            section[j].sectionOffRequest = true;

            // Manual on, force the section On
            if (sectionButtonState[j] == SectionState::On)
            {
                section[j].sectionOnRequest = true;
                section[j].sectionOffRequest = false;
                continue;
            }
            continue;
        }

        // Manual on, force the section On
        if (sectionButtonState[j] == SectionState::On)
        {
            section[j].sectionOnRequest = true;
            section[j].sectionOffRequest = false;
            continue;
        }


        //AutoSection - If any nowhere applied, send OnRequest, if its all green send an offRequest
        section[j].isSectionRequiredOn = false;

        //calculate the slopes of the lines
        mOn = (lookAheadDistanceOnPixelsRight - lookAheadDistanceOnPixelsLeft) / rpWidth;
        mOff = (lookAheadDistanceOffPixelsRight - lookAheadDistanceOffPixelsLeft) / rpWidth;

        start = section[j].rpSectionPosition - section[0].rpSectionPosition;
        end = section[j].rpSectionWidth - 1 + start;

        if (end >= rpWidth)
            end = rpWidth - 1;

        totalPixel = 1;
        tagged = 0;

        for (int pos = start; pos <= end; pos++)
        {
            startHeight = (int)(lookAheadDistanceOffPixelsLeft + (mOff * pos)) * rpWidth + pos;
            endHeight = (int)(lookAheadDistanceOnPixelsLeft + (mOn * pos)) * rpWidth + pos;

            for (int a = startHeight; a <= endHeight; a += rpWidth)
            {
                totalPixel++;
                if (grnPixels[a].green == 0) tagged++;
            }
        }

        //determine if meeting minimum coverage
        section[j].isSectionRequiredOn = ((tagged * 100) / totalPixel > (100 - minCoverage));

        //logic if in or out of boundaries or headland
        if (bnd.bndList.count() > 0)
        {
            //if out of boundary, turn it off
            if (!section[j].isInBoundary)
            {
                section[j].isSectionRequiredOn = false;
                section[j].sectionOffRequest = true;
                section[j].sectionOnRequest = false;
                section[j].sectionOffTimer = 0;
                section[j].sectionOnTimer = 0;
                continue;
            }
            else
            {
                //is headland coming up
                if (MainWindowState::instance()->isHeadlandOn() && isSectionControlledByHeadland)
                {
                    bool isHeadlandInLookOn = false;

                    //is headline in off to on area
                    mOn = (lookAheadDistanceOnPixelsRight - lookAheadDistanceOnPixelsLeft) / rpWidth;
                    mOff = (lookAheadDistanceOffPixelsRight - lookAheadDistanceOffPixelsLeft) / rpWidth;

                    start = section[j].rpSectionPosition - section[0].rpSectionPosition;

                    end = section[j].rpSectionWidth - 1 + start;

                    if (end >= rpWidth)
                        end = rpWidth - 1;

                    tagged = 0;

                    for (int pos = start; pos <= end; pos++)
                    {
                        startHeight = (int)(lookAheadDistanceOffPixelsLeft + (mOff * pos)) * rpWidth + pos;
                        endHeight = (int)(lookAheadDistanceOnPixelsLeft + (mOn * pos)) * rpWidth + pos;

                        for (int a = startHeight; a <= endHeight; a += rpWidth)
                        {
                            if (a < 0)
                                mOn = 0;
                            if (grnPixels[a].green == 250)
                            {
                                isHeadlandInLookOn = true;
                                goto GetOutHdOn;
                            }
                        }
                    }
                GetOutHdOn:

                    //determine if look ahead points are completely in headland
                    if (section[j].isSectionRequiredOn && section[j].isLookOnInHeadland && !isHeadlandInLookOn)
                    {
                        section[j].isSectionRequiredOn = false;
                        section[j].sectionOffRequest = true;
                        section[j].sectionOnRequest = false;
                    }

                    if (section[j].isSectionRequiredOn && !section[j].isLookOnInHeadland && isHeadlandInLookOn)
                    {
                        section[j].isSectionRequiredOn = true;
                        section[j].sectionOffRequest = false;
                        section[j].sectionOnRequest = true;
                    }
                }
            }
        }


        //global request to turn on section
        section[j].sectionOnRequest = section[j].isSectionRequiredOn;
        section[j].sectionOffRequest = !section[j].sectionOnRequest;

    }  // end of go thru all sections "for"

    //Set all the on and off times based from on off section requests
    for (int j = 0; j < numOfSections; j++)
    {
        //SECTION timers

        if (section[j].sectionOnRequest) {
            bool wasOn = section[j].isSectionOn;
            section[j].isSectionOn = true;
            // PHASE 6.0.36: sectionButtonState (user preference) should NOT be modified here
            // Only isSectionOn (calculated state) changes - matches C# original architecture
        }

        //turn off delay
        if (turnOffDelay > 0)
        {
            if (!section[j].sectionOffRequest) section[j].sectionOffTimer = (int)(gpsHz / 2.0 * turnOffDelay);

            if (section[j].sectionOffTimer > 0) section[j].sectionOffTimer--;

            if (section[j].sectionOffRequest && section[j].sectionOffTimer == 0)
            {
                if (section[j].isSectionOn) {
                    section[j].isSectionOn = false;
                    // PHASE 6.0.36: sectionButtonState (user preference) NOT modified
                    // Only isSectionOn (calculated state) changes - matches C# original
                }
            }
        }
        else
        {
            if (section[j].sectionOffRequest) {
                bool wasOn = section[j].isSectionOn;
                section[j].isSectionOn = false;
                // PHASE 6.0.36: sectionButtonState (user preference) NOT modified here
                // Only isSectionOn (calculated state) changes - matches C# original architecture
                // sectionButtonState controlled ONLY by user actions: button clicks, Master Auto
            }
        }

        //Mapping timers
        if (section[j].sectionOnRequest && !section[j].isMappingOn && section[j].mappingOnTimer == 0)
        {
            section[j].mappingOnTimer = (int)(lookAheadOnSetting * (gpsHz / 2) - 1);
        }
        else if (section[j].sectionOnRequest && section[j].isMappingOn && section[j].mappingOffTimer > 1)
        {
            section[j].mappingOffTimer = 0;
            section[j].mappingOnTimer = (int)(lookAheadOnSetting * (gpsHz / 2) - 1);
        }

        if (lookAheadOffSetting > 0)
        {
            if (section[j].sectionOffRequest && section[j].isMappingOn && section[j].mappingOffTimer == 0)
            {
                section[j].mappingOffTimer = (int)(lookAheadOffSetting * (gpsHz / 2) + 4);
            }
        }
        else if (turnOffDelay > 0)
        {
            if (section[j].sectionOffRequest && section[j].isMappingOn && section[j].mappingOffTimer == 0)
                section[j].mappingOffTimer = (int)(turnOffDelay * gpsHz / 2);
        }
        else
        {
            section[j].mappingOffTimer = 0;
        }

        //MAPPING - Not the making of triangle patches - only status - on or off
        if (section[j].sectionOnRequest)
        {
            section[j].mappingOffTimer = 0;
            if (section[j].mappingOnTimer > 1)
                section[j].mappingOnTimer--;
            else
            {
                section[j].isMappingOn = true;
            }
        }

        if (section[j].sectionOffRequest)
        {
            section[j].mappingOnTimer = 0;
            if (section[j].mappingOffTimer > 1)
                section[j].mappingOffTimer--;
            else
            {
                section[j].isMappingOn = false;
            }
        }
    }

    //Checks the workswitch or steerSwitch if required
    if (SettingsManager::instance()->as_isAutoSteerAutoOn() || SettingsManager::instance()->f_isRemoteWorkSystemOn())
        ModuleComm::instance()->CheckWorkAndSteerSwitch(MainWindowState::instance()->isBtnAutoSteerOn());

    // check if any sections have changed status
    number = 0;

    for (int j = 0; j < numOfSections; j++)
    {
        if (section[j].isMappingOn)
        {
            number |= 1ul << j;
        }
    }

    //there has been a status change of section on/off
    if (number != lastNumber)
    {
        int sectionOnOffZones = 0, patchingZones = 0;

        //everything off
        if (number == 0)
        {
            for (int j = 0; j < triStrip.count(); j++)
            {
                if (triStrip[j].isDrawing)
                    triStrip[j].TurnMappingOff(secColors[j],
                                               section[triStrip[j].currentStartSectionNum].leftPoint,
                                               section[triStrip[j].currentEndSectionNum].rightPoint,
                                               patchSaveList);
            }

            LayerService::instance()->flushPendingSections();
        }
        else if (!isMultiColoredSections)
        {

            QColor sectionColor = SettingsManager::instance()->display_colorSectionsDay();

            //set the start and end positions from section points
            for (int j = 0; j < numOfSections; j++)
            {
                //LayerService has no concept of patches, just sections
                //if the section was on but is now off, we need to add
                //the current left/right points before we flush the section
                //otherwise we'll miss a small area.
                if (section[j].isMappingOn || LayerService::instance()->isSectionPending(j)) {
                    LayerService::instance()->addSectionVertices(
                        j,
                        section[j].leftPoint,
                        section[j].rightPoint,
                        sectionColor);
                }

                if (!section[j].isMappingOn) {
                   LayerService::instance()->flushPendingSection(j);
                }

                //skip till first mapping section
                if (!section[j].isMappingOn) continue;

                //do we need more patches created
                if (triStrip.count() < sectionOnOffZones + 1)
                    triStrip.append(CPatches());

                //set this strip start edge to edge of this section
                triStrip[sectionOnOffZones].newStartSectionNum = j;

                while ((j + 1) < numOfSections && section[j + 1].isMappingOn)
                {
                    j++;
                }

                //set the edge of this section to be end edge of strp
                triStrip[sectionOnOffZones].newEndSectionNum = j;
                sectionOnOffZones++;
            }

            //count current patch strips being made
            for (int j = 0; j < triStrip.count(); j++)
            {
                if (triStrip[j].isDrawing) patchingZones++;
            }

            //tests for creating new strips or continuing
            bool isOk = (patchingZones == sectionOnOffZones && sectionOnOffZones < 3);

            if (isOk)
            {
                for (int j = 0; j < sectionOnOffZones; j++)
                {
                    if (triStrip[j].newStartSectionNum > triStrip[j].currentEndSectionNum
                        || triStrip[j].newEndSectionNum < triStrip[j].currentStartSectionNum)
                        isOk = false;
                }
            }

            if (isOk)
            {
                for (int j = 0; j < sectionOnOffZones; j++)
                {
                    if (triStrip[j].newStartSectionNum != triStrip[j].currentStartSectionNum
                        || triStrip[j].newEndSectionNum != triStrip[j].currentEndSectionNum)
                    {
                        //if (isSectionsNotZones)
                        {
                            triStrip[j].AddMappingPoint(secColors[j],
                                                        section[triStrip[j].currentStartSectionNum].leftPoint,
                                                        section[triStrip[j].currentEndSectionNum].rightPoint,
                                                        patchSaveList);
                        }

                        triStrip[j].currentStartSectionNum = triStrip[j].newStartSectionNum;
                        triStrip[j].currentEndSectionNum = triStrip[j].newEndSectionNum;
                        triStrip[j].AddMappingPoint(secColors[j],
                                                    section[triStrip[j].currentStartSectionNum].leftPoint,
                                                    section[triStrip[j].currentEndSectionNum].rightPoint,
                                                    patchSaveList);
                    }
                }
            }
            else
            {
                //too complicated, just make new strips
                for (int j = 0; j < triStrip.count(); j++)
                {
                    if (triStrip[j].isDrawing)
                        triStrip[j].TurnMappingOff(secColors[j],
                                                   section[triStrip[j].currentStartSectionNum].leftPoint,
                                                   section[triStrip[j].currentEndSectionNum].rightPoint,
                                                   patchSaveList);
                }

                for (int j = 0; j < sectionOnOffZones; j++)
                {
                    triStrip[j].currentStartSectionNum = triStrip[j].newStartSectionNum;
                    triStrip[j].currentEndSectionNum = triStrip[j].newEndSectionNum;
                    triStrip[j].TurnMappingOn(secColors[j],
                                              section[triStrip[j].currentStartSectionNum].leftPoint,
                                              section[triStrip[j].currentEndSectionNum].rightPoint);
                }
            }
        }
        else if (isMultiColoredSections) //could be else only but this is more clear
        {
            //set the start and end positions from section points
            for (int j = 0; j < numOfSections; j++)
            {
                //do we need more patches created
                if (triStrip.count() < sectionOnOffZones + 1)
                    triStrip.append(CPatches());

                //set this strip start edge to edge of this section
                triStrip[sectionOnOffZones].newStartSectionNum = j;

                //set the edge of this section to be end edge of strp
                triStrip[sectionOnOffZones].newEndSectionNum = j;
                sectionOnOffZones++;

                if (!section[j].isMappingOn)
                {
                    if (triStrip[j].isDrawing)
                        triStrip[j].TurnMappingOff(secColors[j],
                                                   section[triStrip[j].currentStartSectionNum].leftPoint,
                                                   section[triStrip[j].currentEndSectionNum].rightPoint,
                                                   patchSaveList);
                }
                else
                {
                    triStrip[j].currentStartSectionNum = triStrip[j].newStartSectionNum;
                    triStrip[j].currentEndSectionNum = triStrip[j].newEndSectionNum;
                    triStrip[j].TurnMappingOn(secColors[j],
                                              section[triStrip[j].currentStartSectionNum].leftPoint,
                                              section[triStrip[j].currentEndSectionNum].rightPoint);
                }
            }
        }


        lastNumber = number;
    }
}

void CTool::BuildMachineByte(CTram &tram) {
    CPGN_FE &p_254 = ModuleComm::instance()->p_254;
    CPGN_EF &p_239 = ModuleComm::instance()->p_239;
    CPGN_E5 &p_229 = ModuleComm::instance()->p_229;

    if (isSectionsNotZones)
    {
        p_254.pgn[CPGN_FE::sc1to8] = 0;
        p_254.pgn[CPGN_FE::sc9to16] = 0;

        int number = 0;
        for (int j = 0; j < 8; j++)
        {
            if (section[j].isSectionOn)
                number |= 1 << j;
        }
        p_254.pgn[CPGN_FE::sc1to8] = (char)number;
        number = 0;

        for (int j = 8; j < 16; j++)
        {
            if (section[j].isSectionOn)
                number |= 1 << (j-8);
        }
        p_254.pgn[CPGN_FE::sc9to16] = (char)number;

        //machine pgn
        p_239.pgn[CPGN_EF::sc9to16] = p_254.pgn[CPGN_FE::sc9to16];
        p_239.pgn[CPGN_EF::sc1to8] = p_254.pgn[CPGN_FE::sc1to8];
        p_229.pgn[CPGN_E5::sc1to8] = p_254.pgn[CPGN_FE::sc1to8];
        p_229.pgn[CPGN_E5::sc9to16] = p_254.pgn[CPGN_FE::sc9to16];
        p_229.pgn[CPGN_E5::toolLSpeed] = (char)(farLeftSpeed * 10);
        p_229.pgn[CPGN_E5::toolRSpeed] = (char)(farRightSpeed * 10);
    }
    else
    {
        //zero all the bytes - set only if on
        for (int i = 5; i < 13; i++)
        {
            p_229.pgn[i] = 0;
        }

        int number = 0;
        for (int k = 0; k < 8; k++)
        {
            for (int j = 0; j < 8; j++)
            {
                if (section[j + k * 8].isSectionOn)
                    number |= 1 << j;
            }
            p_229.pgn[5 + k] = (char)number;
            number = 0;
        }

        //tool speed to calc ramp
        p_229.pgn[CPGN_E5::toolLSpeed] = (char)(farLeftSpeed * 10);
        p_229.pgn[CPGN_E5::toolRSpeed] = (char)(farRightSpeed * 10);

        p_239.pgn[CPGN_EF::sc1to8] = p_229.pgn[CPGN_E5::sc1to8];
        p_239.pgn[CPGN_EF::sc9to16] = p_229.pgn[CPGN_E5::sc9to16];

        p_254.pgn[CPGN_FE::sc1to8] = p_229.pgn[CPGN_E5::sc1to8];
        p_254.pgn[CPGN_FE::sc9to16] = p_229.pgn[CPGN_E5::sc9to16];

    }

    p_239.pgn[CPGN_EF::speed] = (char)(CVehicle::instance()->avgSpeed() * 10);
    p_239.pgn[CPGN_EF::tram] = (char)tram.controlByte;

    emit ModuleComm::instance()->p_239_changed();
    emit ModuleComm::instance()->p_254_changed();

}

void CTool::DoRemoteSwitches() {
    COMPILER_WARNING("This method is not called anywhere. Check AgOpenGPS or Twol to find out what we missed.")
    ModuleComm &mc = *ModuleComm::instance();

    // Check if AgIOService is ON - if OFF, skip all hardware switch processing
    SettingsManager* settings = SettingsManager::instance();
    if (!settings->feature_isAgIOOn()) {
        // AgIOService is OFF - manual QML controls have priority
        return;
    }

    bool sectionsChanged = false; // Track if any section state changed
    if (Backend::instance()->isJobStarted())
    {
        //MainSW was used
        if (mc.ss[ModuleComm::swMain] != mc.ssP[ModuleComm::swMain])
        {
            //Main SW pressed
            if ((mc.ss[ModuleComm::swMain] & 1) == 1)
            {
                MainWindowState::instance()->set_autoBtnState(SectionState::Off);
            } // if Main SW ON

            //if Main SW in Arduino is pressed OFF
            if ((mc.ss[ModuleComm::swMain] & 2) == 2)
            {
                MainWindowState::instance()->set_autoBtnState(SectionState::Auto);
            } // if Main SW OFF

            mc.ssP[ModuleComm::swMain] = mc.ss[ModuleComm::swMain];
        }  //Main or Rate SW


        if (isSectionsNotZones)
        {
            if (mc.ss[ModuleComm::swOnGr0] != 0)
            {
                // ON Signal from Arduino
                if ((mc.ss[ModuleComm::swOnGr0] & 128) == 128 && numOfSections > 7)
                {
                    sectionButtonState[7] = SectionState::On;
                    section[7].sectionBtnState = SectionState::On;
                    sectionsChanged = true;
                    //TODO: not sure why we have redundant states like that
                }
                if ((mc.ss[ModuleComm::swOnGr0] & 64) == 64 && numOfSections > 6)
                {
                    sectionButtonState[6] = SectionState::On;
                    section[6].sectionBtnState = SectionState::On;
                }
                if ((mc.ss[ModuleComm::swOnGr0] & 32) == 32 && numOfSections > 5)
                {
                    sectionButtonState[5] = SectionState::On;
                    section[5].sectionBtnState = SectionState::On;
                }
                if ((mc.ss[ModuleComm::swOnGr0] & 16) == 16 && numOfSections > 4)
                {
                    sectionButtonState[4] = SectionState::On;
                    section[4].sectionBtnState = SectionState::On;
                }
                if ((mc.ss[ModuleComm::swOnGr0] & 8) == 8 && numOfSections > 3)
                {
                    sectionButtonState[3] = SectionState::On;
                    section[3].sectionBtnState = SectionState::On;
                }
                if ((mc.ss[ModuleComm::swOnGr0] & 4) == 4 && numOfSections > 2)
                {
                    sectionButtonState[2] = SectionState::On;
                    section[2].sectionBtnState = SectionState::On;
                }
                if ((mc.ss[ModuleComm::swOnGr0] & 2) == 2 && numOfSections > 1)
                {
                    sectionButtonState[1] = SectionState::On;
                    section[1].sectionBtnState = SectionState::On;
                }
                if ((mc.ss[ModuleComm::swOnGr0] & 1) == 1)
                {
                    sectionButtonState[0] = SectionState::On;
                    section[0].sectionBtnState = SectionState::On;
                    sectionsChanged = true;
                }
                mc.ssP[ModuleComm::swOnGr0] = mc.ss[ModuleComm::swOnGr0];
            } //if swONLo != 0
            else { if (mc.ssP[ModuleComm::swOnGr0] != 0) { mc.ssP[ModuleComm::swOnGr0] = 0; } }

            if (mc.ss[ModuleComm::swOnGr1] != 0)
            {
                // sections ON signal from Arduino
                if ((mc.ss[ModuleComm::swOnGr1] & 128) == 128 && numOfSections > 15)
                {
                    sectionButtonState[15] = SectionState::On;
                    section[15].sectionBtnState = SectionState::On;
                }
                if ((mc.ss[ModuleComm::swOnGr1] & 64) == 64 && numOfSections > 14)
                {
                    sectionButtonState[14] = SectionState::On;
                    section[14].sectionBtnState = SectionState::On;
                }
                if ((mc.ss[ModuleComm::swOnGr1] & 32) == 32 && numOfSections > 13)
                {
                    sectionButtonState[13] = SectionState::On;
                    section[13].sectionBtnState = SectionState::On;
                }
                if ((mc.ss[ModuleComm::swOnGr1] & 16) == 16 && numOfSections > 12)
                {
                    sectionButtonState[12] = SectionState::On;
                    section[12].sectionBtnState = SectionState::On;
                }

                if ((mc.ss[ModuleComm::swOnGr1] & 8) == 8 && numOfSections > 11)
                {
                    sectionButtonState[11] = SectionState::On;
                    section[11].sectionBtnState = SectionState::On;
                }
                if ((mc.ss[ModuleComm::swOnGr1] & 4) == 4 && numOfSections > 10)
                {
                    sectionButtonState[10] = SectionState::On;
                    section[10].sectionBtnState = SectionState::On;
                }
                if ((mc.ss[ModuleComm::swOnGr1] & 2) == 2 && numOfSections > 9)
                {
                    sectionButtonState[9] = SectionState::On;
                    section[9].sectionBtnState = SectionState::On;
                }
                if ((mc.ss[ModuleComm::swOnGr1] & 1) == 1 && numOfSections > 8)
                {
                    sectionButtonState[8] = SectionState::On;
                    section[8].sectionBtnState = SectionState::On;
                }
                mc.ssP[ModuleComm::swOnGr1] = mc.ss[ModuleComm::swOnGr1];
            } //if swONHi != 0
            else { if (mc.ssP[ModuleComm::swOnGr1] != 0) { mc.ssP[ModuleComm::swOnGr1] = 0; } }

            // Switches have changed
            if (mc.ss[ModuleComm::swOffGr0] != mc.ssP[ModuleComm::swOffGr0])
            {
                //if Main = Auto then change section to Auto if Off signal from Arduino stopped
                if (MainWindowState::instance()->autoBtnState() == SectionState::Auto)
                {

                    for(int s=0; s< 8; s++) {
                        if ((mc.ssP[ModuleComm::swOffGr0] & (1 << s)) && !(mc.ss[ModuleComm::swOffGr0] & (1 << s)) && (sectionButtonState[s] == SectionState::Off))
                        {
                            sectionButtonState[s] = SectionState::Auto;
                            section[s].sectionBtnState = SectionState::Auto;
                            sectionsChanged = true;
                        }
                    }
                }
                mc.ssP[ModuleComm::swOffGr0] = mc.ss[ModuleComm::swOffGr0];
            }

            if (mc.ss[ModuleComm::swOffGr1] != mc.ssP[ModuleComm::swOffGr1])
            {
                //if Main = Auto then change section to Auto if Off signal from Arduino stopped
                if (MainWindowState::instance()->autoBtnState() == SectionState::Auto)
                {
                    for(int s=8; s< 16; s++) {
                        if ((mc.ssP[ModuleComm::swOffGr1] & (1 << s)) && !(mc.ss[ModuleComm::swOffGr1] & (1 << s)) && (sectionButtonState[s+8] == SectionState::Off))
                        {
                            sectionButtonState[s+8] = SectionState::Auto;
                            section[s+8].sectionBtnState = SectionState::Auto;
                            sectionsChanged = true;
                        }
                    }
                }
                mc.ssP[ModuleComm::swOffGr1] = mc.ss[ModuleComm::swOffGr1];
            }

            // OFF Signal from Arduino
            if (mc.ss[ModuleComm::swOffGr0] != 0)
            {
                //if section SW in Arduino is switched to OFF; check always, if switch is locked to off GUI should not change
                for(int s=0; s< 8; s++) {
                    if ((mc.ss[ModuleComm::swOffGr0] & (1 << s)) && sectionButtonState[s] != SectionState::Off)
                    {
                        // Hardware switch override
                        sectionButtonState[s] = SectionState::Off;
                        section[s].sectionBtnState = SectionState::Off;
                        sectionsChanged = true;
                    }
                }
            } // if swOFFLo !=0
            if (mc.ss[ModuleComm::swOffGr1] != 0)
            {
                //if section SW in Arduino is switched to OFF; check always, if switch is locked to off GUI should not change
                for (int s=0; s<8; s++) {
                    if ((mc.ss[ModuleComm::swOffGr0] & (1 << s)) && sectionButtonState[s+8] != SectionState::Off)
                    {
                        sectionButtonState[s+8] = SectionState::Off;
                        section[s+8].sectionBtnState = SectionState::Off;
                        sectionsChanged = true;
                    }
                }
            } // if swOFFHi !=0
        }//if serial or udp port open
        else
        {
            //DoZones
            int Bit;
            // zones to on
            if (mc.ss[ModuleComm::swOnGr0] != 0)
            {
                for (int i = 0; i < 8; i++)
                {
                    Bit = 1 << i;
                    if ((zoneRanges[i + 1] > 0) && ((mc.ss[ModuleComm::swOnGr0] & Bit) == Bit))
                    {
                        section[zoneRanges[i + 1] - 1].sectionBtnState = SectionState::On;
                        sectionButtonState[zoneRanges[i + 1] - 1] = SectionState::On;
                        sectionsChanged = true;
                    }
                }

                mc.ssP[ModuleComm::swOnGr0] = mc.ss[ModuleComm::swOnGr0];
            }
            else { if (mc.ssP[ModuleComm::swOnGr0] != 0) { mc.ssP[ModuleComm::swOnGr0] = 0; } }

            // zones to auto
            if (mc.ss[ModuleComm::swOffGr0] != mc.ssP[ModuleComm::swOffGr0])
            {
                if (MainWindowState::instance()->autoBtnState() == SectionState::Auto)
                {
                    for (int i = 0; i < 8; i++)
                    {
                        Bit = 1 << i;
                        if ((zoneRanges[i + 1] > 0) && ((mc.ssP[ModuleComm::swOffGr0] & Bit) == Bit)
                            && ((mc.ss[ModuleComm::swOffGr0] & Bit) != Bit) && (section[zoneRanges[i + 1] - 1].sectionBtnState == SectionState::Off))
                        {
                            section[zoneRanges[i + 1] - 1].sectionBtnState = SectionState::Auto;
                            sectionButtonState[zoneRanges[i + 1] - 1] = SectionState::Auto;
                            sectionsChanged = true;
                        }
                    }
                }
                mc.ssP[ModuleComm::swOffGr0] = mc.ss[ModuleComm::swOffGr0];
            }

            // zones to off
            if (mc.ss[ModuleComm::swOffGr0] != 0)
            {
                for (int i = 0; i < 8; i++)
                {
                    Bit = 1 << i;
                    if ((zoneRanges[i + 1] > 0) && ((mc.ss[ModuleComm::swOffGr0] & Bit) == Bit) && (section[zoneRanges[i + 1] - 1].sectionBtnState != SectionState::Off))
                    {
                        section[zoneRanges[i + 1] - 1].sectionBtnState = SectionState::Off;
                        sectionButtonState[zoneRanges[i + 1] - 1] = SectionState::Off;
                        sectionsChanged = true;
                    }
                }
            }
        }
    }
}

void CTool::WhereAreToolCorners(const CBoundary &bnd)
{
    if (bnd.bndList.count() > 0 && bnd.bndList[0].hdLine.count() > 0)
    {
        bool isLeftInWk, isRightInWk = true;

        for (int j = 0; j < numOfSections; j++)
        {
            isLeftInWk = j == 0 ? bnd.IsPointInsideHeadArea(section[j].leftPoint) : isRightInWk;
            isRightInWk = bnd.IsPointInsideHeadArea(section[j].rightPoint);

            //save left side
            if (j == 0)
                isLeftSideInHeadland = !isLeftInWk;

            //merge the two sides into in or out
            section[j].isInHeadlandArea = !isLeftInWk && !isRightInWk;
        }

        //save right side
        isRightSideInHeadland = !isRightInWk;

        //is the tool in or out based on endpoints
        isToolOuterPointsInHeadland = isLeftSideInHeadland && isRightSideInHeadland;
    }

}

void CTool::WhereAreToolLookOnPoints(const CBoundary &bnd)
{
    if (bnd.bndList.count() > 0 && bnd.bndList[0].hdLine.count() > 0)
    {
        bool isLookRightIn = false;

        Vec3 toolFix = toolPos;
        double sinAB = sin(toolFix.heading);
        double cosAB = cos(toolFix.heading);

        //generated box for finding closest point
        double pos = 0;
        double mOn = (lookAheadDistanceOnPixelsRight - lookAheadDistanceOnPixelsLeft) / rpWidth;

        for (int j = 0; j < numOfSections; j++)
        {
            bool isLookLeftIn = j == 0 ? bnd.IsPointInsideHeadArea(Vec2(
                                    section[j].leftPoint.easting + (sinAB * lookAheadDistanceOnPixelsLeft * 0.1),
                                    section[j].leftPoint.northing + (cosAB * lookAheadDistanceOnPixelsLeft * 0.1))) : isLookRightIn;

            pos += section[j].rpSectionWidth;
            double endHeight = (lookAheadDistanceOnPixelsLeft + (mOn * pos)) * 0.1;

            isLookRightIn = bnd.IsPointInsideHeadArea(Vec2(
                section[j].rightPoint.easting + (sinAB * endHeight),
                section[j].rightPoint.northing + (cosAB * endHeight)));

            section[j].isLookOnInHeadland = !isLookLeftIn && !isLookRightIn;
        }
    }

}

void CTool::resetTool() {
   tankPos.heading = CVehicle::instance()->fixHeading();
   tankPos.easting = CVehicle::instance()->hitchPos.easting + (sin(tankPos.heading) * (tankTrailingHitchLength));
   tankPos.northing = CVehicle::instance()->hitchPos.northing + (cos(tankPos.heading) * (tankTrailingHitchLength));

   toolPivotPos.heading = tankPos.heading;
   toolPivotPos.easting = tankPos.easting + (sin(toolPivotPos.heading) * (trailingHitchLength));
   toolPivotPos.northing = tankPos.northing + (cos(toolPivotPos.heading) * (trailingHitchLength));
}

void CTool::on_autoBtnChanged() {
    SectionState::State autoBtnState = MainWindowState::instance()-> autoBtnState();

    // When Master Auto button activated, set all sections to Auto mode
    // This allows automatic section activation based on boundary and coverage
    // Only changes sections currently in Off state - respects manual On state
    if (autoBtnState == SectionState::Auto && Backend::instance()->isJobStarted()) {
        for (int j = 0; j < numOfSections; j++) {
            if (sectionButtonState[j] == SectionState::Off) {
                sectionButtonState[j] = SectionState::Auto;
                section[j].sectionBtnState = SectionState::Auto;
            }
        }
    }
    // When Master Auto turned off, set all Auto sections back to Off
    // Respects manual On state
    else if (autoBtnState == SectionState::Off && Backend::instance()->isJobStarted()) {
        for (int j = 0; j < numOfSections; j++) {
            if (sectionButtonState[j] == SectionState::Auto) {
                sectionButtonState[j] = SectionState::Off;
                section[j].sectionBtnState = SectionState::Off;
            }
        }
    }
}

void CTool::onSectionButtonStatechanged(int toolIndex, int sectionButtonNo, SectionState::State new_state) {
    //For now toolIndex doesn't matter. It will always 0 if no TBT, 1 with TBT
    //in the future toolIndex will start to matter.
    Q_UNUSED(toolIndex);

    if (SettingsManager::instance()->tool_isSectionsNotZones()) {
        //1:1 correlationb etween buttons and sections
        sectionButtonState[sectionButtonNo] = new_state;
    } else {
        //Zones mode -- one button controls multiple sections
        if (sectionButtonNo >= zones ) {
            qWarning(ctool_log) << "ERROR! section button changed but it was out size of the number of zones defined";
        }

        int zone_left = (sectionButtonNo == 0 ? 0 : zoneRanges[sectionButtonNo]);
        int zone_right = (sectionButtonNo + 1 < zones ? zoneRanges[sectionButtonNo + 1] : numOfSections);

        //update all sections in the zone
        if (zone_left >=0 && zone_right > zone_left && zone_right <= numOfSections) {
            for (int j = zone_left ; j < zone_right; j++) {
                sectionButtonState[j] = new_state;
                bool newSectionOn = (new_state == SectionState::On || new_state == SectionState::Auto);
                section[j].isSectionOn = newSectionOn;
                section[j].sectionOnRequest = newSectionOn;
                section[j].sectionOffRequest = !newSectionOn;
            }
        } else {
            qWarning(ctool_log) << "Something is wrong with zones.  Zone" << sectionButtonNo << " start and end section numbers not sane.";
        }
    }
}
