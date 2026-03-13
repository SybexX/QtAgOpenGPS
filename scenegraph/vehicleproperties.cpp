#include "vehicleproperties.h"

VehicleProperties::VehicleProperties(QObject *parent)
    : QObject{parent}
{

    //anything that would trigger geometry reneration ties to vehicleChanged
    connect (this, &VehicleProperties::visibleChanged, this, &VehicleProperties::vehicleChanged);
    connect (this, &VehicleProperties::directionSetChanged, this, &VehicleProperties::vehicleChanged);
    connect (this, &VehicleProperties::firstHeadingSetChanged, this, &VehicleProperties::vehicleChanged);
    connect (this, &VehicleProperties::typeChanged, this, &VehicleProperties::vehicleChanged);
    connect (this, &VehicleProperties::wheelBaseChanged, this, &VehicleProperties::vehicleChanged);
    connect (this, &VehicleProperties::trackWidthChanged, this, &VehicleProperties::vehicleChanged);
    connect (this, &VehicleProperties::drawbarLengthChanged, this, &VehicleProperties::vehicleChanged);
    connect (this, &VehicleProperties::threePtLengthChanged, this, &VehicleProperties::vehicleChanged);
    connect (this, &VehicleProperties::frontHitchLengthChanged, this, &VehicleProperties::vehicleChanged);
    connect (this, &VehicleProperties::markBoundaryChanged, this, &VehicleProperties::vehicleChanged);
    connect (this, &VehicleProperties::antennaForwardChanged, this, &VehicleProperties::vehicleChanged);
    connect (this, &VehicleProperties::antennaOffsetChanged, this, &VehicleProperties::vehicleChanged);
    connect (this, &VehicleProperties::markBoundaryChanged, this, &VehicleProperties::vehicleChanged);
    connect (this, &VehicleProperties::svennArrowChanged, this, &VehicleProperties::vehicleChanged);
    connect (this, &VehicleProperties::opacityChanged, this, &VehicleProperties::vehicleChanged);

}
