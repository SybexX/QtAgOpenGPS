#include "lineproperties.h"

LineProperties::LineProperties(QObject *parent)
    : QObject{parent}
{
    // Connect all properties to the lineChanged signal
    connect(this, &LineProperties::pointsChanged, this, &LineProperties::lineChanged);
    connect(this, &LineProperties::widthChanged, this, &LineProperties::lineChanged);
    connect(this, &LineProperties::dashedChanged, this, &LineProperties::lineChanged);
    connect(this, &LineProperties::loopChanged, this, &LineProperties::lineChanged);
    connect(this, &LineProperties::colorChanged, this, &LineProperties::lineChanged);
}