#ifndef CTOOL_H
#define CTOOL_H

#include <QString>
#include "csection.h"
#include "cpatches.h"
#include "common.h"
#include <QColor>
#include <QElapsedTimer>
#include <QImage>
#include "vec3.h"
#include "sectionstate.h"

class QOpenGLFunctions;
class QMatrix4x4;
class CVehicle;
class CTram;
class CBoundary;


struct PatchBuffer;
struct PatchInBuffer;


class CTool: public QObject
{
    Q_OBJECT
public:
    ///---- in settings
    double width;
    double applyWidth;
    ///----
    double halfWidth, contourWidth;
    double farLeftPosition = 0;
    double farLeftSpeed = 0;
    double farRightPosition = 0;
    double farRightSpeed = 0;

    ///---- in settings
    double overlap;
    double trailingHitchLength, tankTrailingHitchLength;
    double trailingToolToPivotLength;
    double offset;

    double lookAheadOffSetting, lookAheadOnSetting;
    double turnOffDelay;
    ///----

    double lookAheadDistanceOnPixelsLeft, lookAheadDistanceOnPixelsRight;
    double lookAheadDistanceOffPixelsLeft, lookAheadDistanceOffPixelsRight;

    ///---- in settings
    bool isToolTrailing, isToolTBT;
    bool isToolRearFixed, isToolFrontFixed;

    bool isMultiColoredSections, isSectionOffWhenOut;
    ///----

    QString toolAttachType;

    ///---- in settings
    double hitchLength;

    //how many individual sections
    int numOfSections;

    //used for super section off on
    int minCoverage;
    ///----

    bool areAllSectionBtnsOn = true;

    bool isLeftSideInHeadland = true, isRightSideInHeadland = true, isSectionsNotZones;
    bool isHeadlandClose = false;
    bool isToolInHeadland, isToolOuterPointsInHeadland=false, isSectionControlledByHeadland;

    ulong number = 0, lastNumber = 0;

    //read pixel values
    int rpXPosition;

    int rpWidth;

    ///---- in settings
    QColor secColors[16];

    int zones;
    QVector<int> zoneRanges;
    ///----

    bool isDisplayTramControl;

    double hydLiftLookAheadDistanceLeft = 0.0;
    double hydLiftLookAheadDistanceRight = 0.0;

    Vec3 toolPivotPos;
    Vec3 toolPos;
    Vec3 tankPos;

    //moved the following from the main form to here
    CSection section[MAXSECTIONS+1];
    SectionState::State sectionButtonState[65];

    //list of patches to save to disk at next opportunity
    QVector<QSharedPointer<PatchTriangleList>> patchSaveList;

    //list of patches, one per section.  each one has a list of
    //individual patches.
    QVector<CPatches> triStrip = QVector<CPatches>( { CPatches() } );

    bool patchesBufferDirty = true;
    QImage grnPixWindow;

    void sectionCalcWidths();
    void sectionCalcMulti();
    void sectionSetPositions();
    void loadSettings();
    void saveSettings();

    CTool();
    //this class needs modelview and projection as separate matrices because some
    //additiona transformations need to be done.
    void DrawToolGL(QOpenGLFunctions *gl,
                  QMatrix4x4 modelview,
                  QMatrix4x4 projection,
                  bool isJobStarted, bool isHydLiftOn,
                  double camSetDistance, CTram &tram);

    void DrawPatchesGL(QOpenGLFunctions *gl,
                     QMatrix4x4 mvp,
                     int patchCounter,
                     double camSetDistance,
                     QElapsedTimer &swFrame
                     );

    void DrawPatchesTrianglesGL(QOpenGLFunctions *gl,
                                QMatrix4x4 mvp,
                                int patchCounter,
                                QElapsedTimer &swFrame
                                );

    void DrawPatchesBack(QOpenGLFunctions *gl,
                               QMatrix4x4 mvp);

    void DrawPatchesBackQP(const CTram &tram, const CBoundary &bnd, Vec3 pivotAxlePos, bool isHeadlandOn, bool onTrack);

    void NewPosition();
    void ProcessLookAhead(int gpsHz, SectionState::State autoBtnState,
                          const CBoundary &bnd, CTram &tram);
    void BuildMachineByte(CTram &tram);
    void DoRemoteSwitches();

    void clearPatches();
    void loadPatches();

    void WhereAreToolCorners(const CBoundary &bnd);
    void WhereAreToolLookOnPoints(const CBoundary &bnd);


private:

    QVector<QVector<PatchInBuffer>> patchesInBuffer;
    QVector<PatchBuffer> patchBuffer;
    LookAheadPixels grnPixels[150001];
    LookAheadPixels *overPixels = new LookAheadPixels[160000]; //400x400

public slots:
    void on_autoBtnChanged();
    void onSectionButtonStatechanged(int toolIndex, int sectionButtonNo, SectionState::State new_state);
    void resetTool();

signals:
    void SetHydPosition(SectionState::State autoBtnState);

};

#endif // CTOOL_H
