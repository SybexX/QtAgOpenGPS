#ifndef FORMTRACKDRAWER_H
#define FORMTRACKDRAWER_H

#include <QObject>
#include <QVector>
#include <QMatrix4x4>
#include "vec3.h"

class CBoundary;
class CTrack;

class FormTrackDrawer : public QObject
{
    Q_OBJECT
protected:
    bool isA = true;
    int start = 99999, end = 99999;
    int bndSelect = 0, mode;

    QVector<Vec3> sliceArr;

    bool zoomToggle = false;
    bool isLinesVisible = true;

    QVector3D mouseClickToField(int mouseX, int mouseY);

    void setup_matrices(QMatrix4x4 &modelview, QMatrix4x4 &projection);

public:
    CBoundary *bnd = nullptr;
    CTrack *track = nullptr;

    explicit FormTrackDrawer(QObject *parent = nullptr);

public slots:
    void load();
    void close();
    void updateLines();
    void mouseClicked(int mouseX, int mouseY);
    void mouseDragged(int fromX, int fromY, int mouseX, int mouseY);

    void btnCycleForward_Click();
    void btnCycleBackward_Click();
    void btnDeleteTrack_Click();
    void btnCancelTouch_Click();

    void createABLine();
    void createCurve();
    void createBoundaryCurve();
    void cancelTrackCreation();

    void alength_Click();
    void blength_Click();
    void ashrink_Click();
    void bshrink_Click();
    void setTrackVisible_Click(bool visible);

    void saveExit();

signals:
    void timedMessageBox(int, QString, QString);
    void saveTracks();

private:
    void updateABPoints();
    void updateCurrentTrackLine(int idx);
};

#endif // FORMTRACKDRAWER_H
