#ifndef BOUNDARYINTERFACE_H
#define BOUNDARYINTERFACE_H

#include <QObject>
#include <QPropertyBinding>
#include <QQmlEngine>
#include <QMutex>
#include "simpleproperty.h"
#include "boundariesproperties.h"

class BoundaryInterface : public QObject
{
    Q_OBJECT
    QML_SINGLETON
    QML_ELEMENT
private:
    explicit BoundaryInterface(QObject *parent = nullptr);
    ~BoundaryInterface() override=default;

    //prevent copying
    BoundaryInterface(const BoundaryInterface &) = delete;
    BoundaryInterface &operator=(const BoundaryInterface &) = delete;

    static BoundaryInterface *s_instance;
    static QMutex s_mutex;
    static bool s_cpp_created;

public:
    static BoundaryInterface *instance();
    static BoundaryInterface *create (QQmlEngine *qmlEngine, QJSEngine *jsEngine);

    SIMPLE_BINDABLE_PROPERTY(bool, isOutOfBounds)
    SIMPLE_BINDABLE_PROPERTY(double, createBndOffset)
    SIMPLE_BINDABLE_PROPERTY(bool, isDrawRightSide)

    SIMPLE_BINDABLE_PROPERTY(bool, isBndBeingMade)
    SIMPLE_BINDABLE_PROPERTY(bool, isRecording)
    SIMPLE_BINDABLE_PROPERTY(double, area)
    SIMPLE_BINDABLE_PROPERTY(int, pointCount)

    SIMPLE_BINDABLE_PROPERTY(int, count)

    SIMPLE_BINDABLE_PROPERTY(QList<QVariant>, list)

    SIMPLE_BINDABLE_PROPERTY_PTR(BoundariesProperties*, properties)

signals:
    // QML can call these signals directly - no need for Q_INVOKABLE wrappers
    void calculateArea();
    void updateList();
    void start();
    void stop();
    void addPoint();
    void deleteLastPoint();
    void pause();
    void record();
    void reset();
    void deleteBoundary(int id);
    void setDriveThrough(int id, bool drive_thru);
    void deleteAll();

    void loadBoundaryFromKML(QString filename);
    void addBoundaryOSMPoint(double latitude, double longitude);


private:
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(BoundaryInterface, bool, m_isOutOfBounds, false, &BoundaryInterface::isOutOfBoundsChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(BoundaryInterface, double, m_createBndOffset, 0, &BoundaryInterface::createBndOffsetChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(BoundaryInterface, bool, m_isDrawRightSide, false, &BoundaryInterface::isDrawRightSideChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(BoundaryInterface, bool, m_isBndBeingMade, false, &BoundaryInterface::isBndBeingMadeChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(BoundaryInterface, bool, m_isRecording, false, &BoundaryInterface::isRecordingChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(BoundaryInterface, double, m_area, false, &BoundaryInterface::areaChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(BoundaryInterface, int, m_pointCount, false, &BoundaryInterface::pointCountChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(BoundaryInterface, int, m_count, false, &BoundaryInterface::countChanged)
    Q_OBJECT_BINDABLE_PROPERTY(BoundaryInterface, QList<QVariant>, m_list, &BoundaryInterface::listChanged)
    Q_OBJECT_BINDABLE_PROPERTY(BoundaryInterface, BoundariesProperties*, m_properties, &BoundaryInterface::propertiesChanged)
};

#endif // BOUNDARYINTERFACE_H
