#ifndef CFIELDDATA_H
#define CFIELDDATA_H

#include <QVector>
#include "cboundarylist.h"
#include "glm.h"
#include <QLocale>
#include "qmlutil.h"

class CVehicle;

class CFieldData
{

public:
    //all the section area added up;

    //just a cumulative tally based on distance and eq width.

    //accumulated user distance

    double barPercent = 0;

    double overlapPercent = 0;

    //Outside area minus inner boundaries areas (m)

    //used for overlap calcs - total done minus overlap

    //Inner area of outer boundary(m)

    //not really used - but if needed

    CFieldData();

};

#endif // CFIELDDATA_H
