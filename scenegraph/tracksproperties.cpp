#include "tracksproperties.h"

TracksProperties::TracksProperties(QObject *parent) : QObject(parent)
{
    connect(this, &TracksProperties::newTrackChanged, this, &TracksProperties::tracksPropertiesChanged);
    connect(this, &TracksProperties::refLineChanged, this, &TracksProperties::tracksPropertiesChanged);
    connect(this, &TracksProperties::currentLineChanged, this, &TracksProperties::tracksPropertiesChanged);
    connect(this, &TracksProperties::showRefFlagsChanged, this, &TracksProperties::tracksPropertiesChanged);
    connect(this, &TracksProperties::aRefFlagChanged, this, &TracksProperties::tracksPropertiesChanged);
    connect(this, &TracksProperties::bRefFlagChanged, this, &TracksProperties::tracksPropertiesChanged);

    connect(this, &TracksProperties::shadowQuadChanged, this, &TracksProperties::tracksPropertiesChanged);
    connect(this, &TracksProperties::sideGuideLinesChanged, this, &TracksProperties::tracksPropertiesChanged);
    connect(this, &TracksProperties::curveGuideLinesChanged, this, &TracksProperties::tracksPropertiesChanged);

    connect(this, &TracksProperties::smoothedCurveChanged, this, &TracksProperties::tracksPropertiesChanged);
    connect(this, &TracksProperties::showCurrentLineDotsChanged, this, &TracksProperties::tracksPropertiesChanged);
    connect(this, &TracksProperties::youTurnPointsChanged, this, &TracksProperties::tracksPropertiesChanged);

    // Contour properties
    connect(this, &TracksProperties::contourLineChanged, this, &TracksProperties::tracksPropertiesChanged);
    connect(this, &TracksProperties::stripPointsNearbyChanged, this, &TracksProperties::tracksPropertiesChanged);
    connect(this, &TracksProperties::stripPointsSparseChanged, this, &TracksProperties::tracksPropertiesChanged);
    connect(this, &TracksProperties::contourCurrentPointChanged, this, &TracksProperties::tracksPropertiesChanged);
    connect(this, &TracksProperties::contourGoalPointChanged, this, &TracksProperties::tracksPropertiesChanged);
    connect(this, &TracksProperties::isContourOnChanged, this, &TracksProperties::tracksPropertiesChanged);
}
