#ifndef HEADACHEINTERFACE_H
#define HEADACHEINTERFACE_H

#include <QObject>
#include <QQmlEngine>
#include <QMutex>
#include <QPropertyBinding>
#include <QPointF>
#include <QColor>
#include <QRect>
#include "simpleproperty.h"
#include "fencelinemodel.h"

class HeadacheInterface : public QObject
{
    Q_OBJECT
    QML_SINGLETON
    QML_ELEMENT
private:
    explicit HeadacheInterface(QObject *parent = nullptr);
    ~HeadacheInterface() override=default;

    //prevent copying
    HeadacheInterface(const HeadacheInterface &) = delete;
    HeadacheInterface &operator=(const HeadacheInterface &) = delete;

    static HeadacheInterface *s_instance;
    static QMutex s_mutex;
    static bool s_cpp_created;

public:
    static HeadacheInterface *instance();
    static HeadacheInterface *create (QQmlEngine *qmlEngine, QJSEngine *jsEngine);

    SIMPLE_BINDABLE_PROPERTY(int, count)
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

    Q_PROPERTY(FenceLineModel* headacheLineModel READ headacheLineModel CONSTANT)
    FenceLineModel* headacheLineModel() const { return m_headacheLineModel; }

    /*
    property var headacheLines: [
        {
            index: 0,
            color: "#FF0000",
            width: 4,
            points: [
                Qt.point(150, 150),
                Qt.point(200, 150),
                Qt.point(200, 200),
                Qt.point(150, 200),
                Qt.point(150, 150)
            ],
            dashed: false
        },
        {
            index: 1,
            color: "#00FF00",
            width: 4,
            points: [
                Qt.point(125, 125),
                Qt.point(175, 125),
                Qt.point(175, 175),
                Qt.point(125, 175),
                Qt.point(125, 125)
            ],
            dashed: true
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

    SIMPLE_BINDABLE_PROPERTY(int, viewportWidth)
    SIMPLE_BINDABLE_PROPERTY(int, viewportHeight)

private:
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(HeadacheInterface, int, m_count, 0, &HeadacheInterface::countChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(HeadacheInterface, bool, m_curveLine, true, &HeadacheInterface::curveLineChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(HeadacheInterface, double, m_lineDistance, 0, &HeadacheInterface::lineDistanceChanged)

    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(HeadacheInterface, double, m_zoom, 1, &HeadacheInterface::zoomChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(HeadacheInterface, double, m_sX, 0, &HeadacheInterface::sXChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(HeadacheInterface, double, m_sY, 0, &HeadacheInterface::sYChanged)

    Q_OBJECT_BINDABLE_PROPERTY(HeadacheInterface, QPoint, m_apoint, &HeadacheInterface::apointChanged)
    Q_OBJECT_BINDABLE_PROPERTY(HeadacheInterface, QPoint, m_bpoint, &HeadacheInterface::bpointChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(HeadacheInterface, bool, m_showa, false, &HeadacheInterface::showaChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(HeadacheInterface, bool, m_showb, false, &HeadacheInterface::showbChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(HeadacheInterface, QColor, m_acolor, Qt::red, &HeadacheInterface::acolorChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(HeadacheInterface, QColor, m_bcolor, Qt::blue, &HeadacheInterface::bcolorChanged)

    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(HeadacheInterface, int, m_viewportWidth, 0, &HeadacheInterface::viewportWidthChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(HeadacheInterface, int, m_viewportHeight, 0, &HeadacheInterface::viewportHeightChanged)

    Q_OBJECT_BINDABLE_PROPERTY(HeadacheInterface, QVariantList, m_headlandLine, &HeadacheInterface::headlandLineChanged)

    FenceLineModel *m_boundaryLineModel;
    FenceLineModel *m_headacheLineModel;

signals:
    // These can be called by QML to initiate action in formheadache.cpp
    void load();
    void close();
    void updateLines();
    void saveExit();

    void mouseClicked(int x, int y);
    void mouseDragged(int fromX, int fromY, int mouseX, int mouseY);

    void createHeadland();
    void deleteHeadland();

    void ashrink();
    void alength();
    void bshrink();
    void blength();
    void headlandOff();

    void cycleForward();
    void cycleBackward();
    void deleteCurve();
    void cancelTouch();

    void isSectionControlled(bool wellIsIt);

};

#endif // HEADLANDINTERFACE_H
