#ifndef HEADLANDINTERFACE_H
#define HEADLANDINTERFACE_H

#include <QObject>
#include <QQmlEngine>
#include <QMutex>
#include <QPropertyBinding>
#include <QPointF>
#include <QColor>
#include <QRect>
#include "simpleproperty.h"
#include "fencelinemodel.h"

class HeadlandInterface : public QObject
{
    Q_OBJECT
    QML_SINGLETON
    QML_ELEMENT
private:
    explicit HeadlandInterface(QObject *parent = nullptr);
    ~HeadlandInterface() override=default;

    //prevent copying
    HeadlandInterface(const HeadlandInterface &) = delete;
    HeadlandInterface &operator=(const HeadlandInterface &) = delete;

    static HeadlandInterface *s_instance;
    static QMutex s_mutex;
    static bool s_cpp_created;

public:
    static HeadlandInterface *instance();
    static HeadlandInterface *create (QQmlEngine *qmlEngine, QJSEngine *jsEngine);

    SIMPLE_BINDABLE_PROPERTY(int, sliceCount)
    SIMPLE_BINDABLE_PROPERTY(int, backupCount)
    SIMPLE_BINDABLE_PROPERTY(bool, curveLine)
    SIMPLE_BINDABLE_PROPERTY(double, lineDistance)
    SIMPLE_BINDABLE_PROPERTY(double, zoom)
    SIMPLE_BINDABLE_PROPERTY(double, sX)
    SIMPLE_BINDABLE_PROPERTY(double, sY)

    SIMPLE_BINDABLE_PROPERTY(QPoint, apoint)
    SIMPLE_BINDABLE_PROPERTY(QPoint, bpoint)
    SIMPLE_BINDABLE_PROPERTY(bool, showa)
    SIMPLE_BINDABLE_PROPERTY(bool, showb)
    SIMPLE_BINDABLE_PROPERTY(QColor, acolor)
    SIMPLE_BINDABLE_PROPERTY(QColor, bcolor)

    SIMPLE_BINDABLE_PROPERTY(QPoint, vehiclePoint)

    // Model-based boundary lines (new approach)
    Q_PROPERTY(FenceLineModel* boundaryLineModel READ boundaryLineModel CONSTANT)
    FenceLineModel* boundaryLineModel() const { return m_boundaryLineModel; }

   /*
    property var boundaryLines: [
        {
            index: 0,
            color: "#FF0000",
            width: 4,
            points: [
                Qt.point(50, 50),
                Qt.point(100, 50),
                Qt.point(100, 100),
                Qt.point(50, 100),
                Qt.point(50, 50)
            ]
        },
        {
            index: 1,
            color: "#00FF00",
            width: 4,
            points: [
                Qt.point(25, 25),
                Qt.point(75, 25),
                Qt.point(75, 75),
                Qt.point(25, 75),
                Qt.point(25, 25)
            ]
        }
    ]
    */

    SIMPLE_BINDABLE_PROPERTY(QVariantList, headlandLine)
    /*
    property var headlandLine: [
        Qt.point(0,0),
        Qt.point(100,20)
    ]
    */

    SIMPLE_BINDABLE_PROPERTY(QVariantList, sliceLine)
    /*
    property var sliceLine: [
        Qt.point(0,0),
        Qt.point(20,100)
    ]
    */

    SIMPLE_BINDABLE_PROPERTY(int, viewportWidth)
    SIMPLE_BINDABLE_PROPERTY(int, viewportHeight)

private:
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(HeadlandInterface, int, m_sliceCount, 0, &HeadlandInterface::sliceCountChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(HeadlandInterface, int, m_backupCount, 0, &HeadlandInterface::backupCountChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(HeadlandInterface, bool, m_curveLine, true, &HeadlandInterface::curveLineChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(HeadlandInterface, double, m_lineDistance, 0, &HeadlandInterface::lineDistanceChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(HeadlandInterface, double, m_zoom, 1, &HeadlandInterface::zoomChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(HeadlandInterface, double, m_sX, 0, &HeadlandInterface::sXChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(HeadlandInterface, double, m_sY, 0, &HeadlandInterface::sYChanged)

    Q_OBJECT_BINDABLE_PROPERTY(HeadlandInterface, QPoint, m_apoint, &HeadlandInterface::apointChanged)
    Q_OBJECT_BINDABLE_PROPERTY(HeadlandInterface, QPoint, m_bpoint, &HeadlandInterface::bpointChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(HeadlandInterface, bool, m_showa, false, &HeadlandInterface::showaChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(HeadlandInterface, bool, m_showb, false, &HeadlandInterface::showbChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(HeadlandInterface, QColor, m_acolor, Qt::red, &HeadlandInterface::acolorChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(HeadlandInterface, QColor, m_bcolor, Qt::blue, &HeadlandInterface::bcolorChanged)

    Q_OBJECT_BINDABLE_PROPERTY(HeadlandInterface, QPoint, m_vehiclePoint, &HeadlandInterface::vehiclePointChanged)

    Q_OBJECT_BINDABLE_PROPERTY(HeadlandInterface, QVariantList, m_headlandLine, &HeadlandInterface::headlandLineChanged)
    Q_OBJECT_BINDABLE_PROPERTY(HeadlandInterface, QVariantList, m_sliceLine, &HeadlandInterface::sliceLineChanged)

    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(HeadlandInterface, int, m_viewportWidth, 0, &HeadlandInterface::viewportWidthChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(HeadlandInterface, int, m_viewportHeight, 0, &HeadlandInterface::viewportHeightChanged)

    FenceLineModel *m_boundaryLineModel;

signals:
    // These can be called by QML to initiate action in formheadache.cpp
    void load();
    void updateLines();
    void saveExit();
    void mouseClicked(int x, int y);
    void close();
    void slice();
    void deletePoints();
    void createHeadland();
    void undo();
    void ashrink();
    void alength();
    void bshrink();
    void blength();
    void headlandOff();
    void isSectionControlled(bool wellIsIt);
    void mouseDragged(int fromX, int fromY, int mouseX, int mouseY);

};

#endif // HEADLANDINTERFACE_H
