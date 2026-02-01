#include "tracksproperties.h"

TracksProperties::TracksProperties(QObject *parent) : QObject(parent)
{
    connect(this, &TracksProperties::newTrackChanged, this, &TracksProperties::tracksPropertiesChanged);
    connect(this, &TracksProperties::refLineChanged, this, &TracksProperties::tracksPropertiesChanged);
    connect(this, &TracksProperties::showRefFlagsChanged, this, &TracksProperties::tracksPropertiesChanged);
    connect(this, &TracksProperties::aRefFlagChanged, this, &TracksProperties::tracksPropertiesChanged);
    connect(this, &TracksProperties::bRefFlagChanged, this, &TracksProperties::tracksPropertiesChanged);

}
