#include "trackproperties.h"

TrackProperties::TrackProperties() {
    //if any of the non-geometry properties change, use single signal
    connect(this, &TrackProperties::visibleChanged, this, &TrackProperties::trackPropertiesUniformsChanged);
    connect(this, &TrackProperties::loopChanged, this, &TrackProperties::trackPropertiesUniformsChanged);
    connect(this, &TrackProperties::colorChanged, this, &TrackProperties::trackPropertiesUniformsChanged);
    connect(this, &TrackProperties::lineWidthChanged, this, &TrackProperties::trackPropertiesUniformsChanged);

    connect(this, &TrackProperties::dashedChanged, this, &TrackProperties::trackPropertiesGeometryChanged);
    connect(this, &TrackProperties::pointsChanged, this, &TrackProperties::trackPropertiesGeometryChanged);
}
