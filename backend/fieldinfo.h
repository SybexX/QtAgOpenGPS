#ifndef FIELDINFO_H
#define FIELDINFO_H

#include <QObject>

struct FieldInfo
{
    Q_GADGET
    Q_PROPERTY(double areaOuterBoundary MEMBER areaOuterBoundary)
    Q_PROPERTY(double areaBoundaryOuterLessInner MEMBER areaBoundaryOuterLessInner)
    Q_PROPERTY(double distanceUser MEMBER distanceUser)
    Q_PROPERTY(double workedAreaTotal MEMBER workedAreaTotal)
    Q_PROPERTY(double workedAreaTotalUser MEMBER workedAreaTotalUser)

    Q_PROPERTY(double actualAreaCovered MEMBER actualAreaCovered)
    Q_PROPERTY(double userSquareMetersAlarm MEMBER userSquareMetersAlarm)
    Q_PROPERTY(double minX MEMBER minX)
    Q_PROPERTY(double maxX MEMBER maxX)
    Q_PROPERTY(double minY MEMBER minY)
    Q_PROPERTY(double maxY MEMBER maxY)
    Q_PROPERTY(double maxDistance MEMBER maxDistance)
    Q_PROPERTY(double centerX MEMBER centerX)
    Q_PROPERTY(double centerY MEMBER centerY)

public:

    double areaOuterBoundary = 0.0;
    double areaBoundaryOuterLessInner = 0.0;
    double distanceUser = 0.0;

    double workedAreaTotal = 0.0;
    double workedAreaTotalUser = 0.0;

    double actualAreaCovered = 0.0;
    double userSquareMetersAlarm = 0.0;

    //field extents in metres from field start
    double minX = 0;
    double maxX = 0;
    double minY = 0;
    double maxY = 0;

    double maxDistance = 0.0;
    double centerX = 0.0;
    double centerY = 0.0;

    inline void calcCenter() {
        centerX = (maxX + minX) / 2.0;
        centerY = (maxY + minY) / 2.0;
    }



};

Q_DECLARE_METATYPE(FieldInfo)

#endif // FIELDINFO_H
