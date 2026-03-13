#ifndef FORMHEADLAND_H
#define FORMHEADLAND_H

#include <QObject>
#include <QVector>
#include "vec3.h"
#include <QTimer>

class CBoundary;
class CHeadLine;

class FormHeadland : public QObject
{
    Q_OBJECT
protected:
    int fixX, fixY;
    //InterfaceProperty<HeadlandDesigner,bool> isA = InterfaceProperty<HeadlandDesigner,bool>("isA");
    bool isA = false;
    bool draw = false;
    int start = 99999, end = 99999;
    int bndSelect = 0, mode;
    QVector<Vec3> sliceArr;
    QVector<Vec3> backupList;


    bool zoomToggle;

    Vec3 pint = Vec3(0,1,0);

    QTimer updateVehiclePositionTimer;

    void SetLineDistance();
    QVector3D mouseClickToField(int mouseX, int mouseY);

    void setup_matrices(QMatrix4x4 &modelview, QMatrix4x4 &projection);

public:
    explicit FormHeadland(QObject *parent = nullptr);
    double iE = 0, iN = 0;
    QVector<int> crossings;

    CBoundary *bnd = nullptr;
    CHeadLine *hdl = nullptr;

public slots:
    void load_headline();
    void update_lines(); //update the boundary lines in GUI
    void update_headland(); //update headland line
    void update_slice(); //update slice and A B points in GUI
    void FormHeadLine_FormClosing();
    void clicked(int mouseX, int mouseY);

    void btn_Exit_Click();
    void isSectionControlled(bool wellIsIt);
    void btnBndLoop_Click();
    void btnSlice_Click();
    void btnDeletePoints_Click();
    void btnUndo_Click();
    void btnALength_Click();
    void btnBLength_Click();
    void btnBShrink_Click();
    void btnAShrink_Click();
    void btnHeadlandOff_Click();

    void updateVehiclePosition();

signals:
    void timedMessageBox(int, QString, QString);
    void saveHeadland();
};

#endif // FORMHEADLAND_H
