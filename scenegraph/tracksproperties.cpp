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
    connect(this, &TracksProperties::lookaheadPointsChanged, this, &TracksProperties::tracksPropertiesChanged);
    connect(this, &TracksProperties::pursuitCircleChanged, this, &TracksProperties::tracksPropertiesChanged);
    connect(this, &TracksProperties::smoothedCurveChanged, this, &TracksProperties::tracksPropertiesChanged);
    connect(this, &TracksProperties::showCurrentLineDotsChanged, this, &TracksProperties::tracksPropertiesChanged);
}
