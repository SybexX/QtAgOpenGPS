#ifndef TRACKINTERFACE_H
#define TRACKINTERFACE_H

#include <QObject>
#include <QQmlEngine>
#include <QMutex>
#include <QPropertyBinding>
#include <QPointF>
#include <QColor>
#include <QRect>
#include "simpleproperty.h"
#include "fencelinemodel.h"

class TrackInterface : public QObject
{
    Q_OBJECT
    QML_SINGLETON
    QML_ELEMENT
private:
    explicit TrackInterface(QObject *parent = nullptr);
    ~TrackInterface() override = default;

    TrackInterface(const TrackInterface &) = delete;
    TrackInterface &operator=(const TrackInterface &) = delete;

    static TrackInterface *s_instance;
    static QMutex s_mutex;
    static bool s_cpp_created;

public:
    static TrackInterface *instance();
    static TrackInterface *create(QQmlEngine *qmlEngine, QJSEngine *jsEngine);

    SIMPLE_BINDABLE_PROPERTY(int, sliceCount)
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

    Q_PROPERTY(FenceLineModel* boundaryLineModel READ boundaryLineModel CONSTANT)
    FenceLineModel* boundaryLineModel() const { return m_boundaryLineModel; }

    SIMPLE_BINDABLE_PROPERTY(QVariantList, sliceLine)
    SIMPLE_BINDABLE_PROPERTY(int, viewportWidth)
    SIMPLE_BINDABLE_PROPERTY(int, viewportHeight)
    SIMPLE_BINDABLE_PROPERTY(QString, trackName)

    SIMPLE_BINDABLE_PROPERTY(int, trackCount)
    SIMPLE_BINDABLE_PROPERTY(int, currentTrackIndex)
    SIMPLE_BINDABLE_PROPERTY(QVariantList, currentTrackLine)
    SIMPLE_BINDABLE_PROPERTY(QVariantList, refLine)
    SIMPLE_BINDABLE_PROPERTY(bool, isTrackVisible)

private:
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(TrackInterface, int, m_sliceCount, 0, &TrackInterface::sliceCountChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(TrackInterface, bool, m_curveLine, true, &TrackInterface::curveLineChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(TrackInterface, double, m_lineDistance, 0, &TrackInterface::lineDistanceChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(TrackInterface, double, m_zoom, 1, &TrackInterface::zoomChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(TrackInterface, double, m_sX, 0, &TrackInterface::sXChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(TrackInterface, double, m_sY, 0, &TrackInterface::sYChanged)

    Q_OBJECT_BINDABLE_PROPERTY(TrackInterface, QPoint, m_apoint, &TrackInterface::apointChanged)
    Q_OBJECT_BINDABLE_PROPERTY(TrackInterface, QPoint, m_bpoint, &TrackInterface::bpointChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(TrackInterface, bool, m_showa, false, &TrackInterface::showaChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(TrackInterface, bool, m_showb, false, &TrackInterface::showbChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(TrackInterface, QColor, m_acolor, Qt::red, &TrackInterface::acolorChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(TrackInterface, QColor, m_bcolor, Qt::blue, &TrackInterface::bcolorChanged)

    Q_OBJECT_BINDABLE_PROPERTY(TrackInterface, QPoint, m_vehiclePoint, &TrackInterface::vehiclePointChanged)
    Q_OBJECT_BINDABLE_PROPERTY(TrackInterface, QVariantList, m_sliceLine, &TrackInterface::sliceLineChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(TrackInterface, int, m_viewportWidth, 0, &TrackInterface::viewportWidthChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(TrackInterface, int, m_viewportHeight, 0, &TrackInterface::viewportHeightChanged)
    Q_OBJECT_BINDABLE_PROPERTY(TrackInterface, QString, m_trackName, &TrackInterface::trackNameChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(TrackInterface, int, m_trackCount, 0, &TrackInterface::trackCountChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(TrackInterface, int, m_currentTrackIndex, -1, &TrackInterface::currentTrackIndexChanged)
    Q_OBJECT_BINDABLE_PROPERTY(TrackInterface, QVariantList, m_currentTrackLine, &TrackInterface::currentTrackLineChanged)
    Q_OBJECT_BINDABLE_PROPERTY(TrackInterface, QVariantList, m_refLine, &TrackInterface::refLineChanged)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(TrackInterface, bool, m_isTrackVisible, true, &TrackInterface::isTrackVisibleChanged)

    FenceLineModel *m_boundaryLineModel;

signals:
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

    void createABLine();
    void createCurve();
    void createBoundaryCurve();
    void cancelTrackCreation();

    void cycleForward();
    void cycleBackward();
    void deleteTrack();
    void setTrackVisible(bool visible);
    void cancelTouch();

    void boundaryLinesChanged();
};

#endif // TRACKINTERFACE_H
