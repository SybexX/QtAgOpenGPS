#ifndef GLUTILS_H
#define GLUTILS_H

#include <QOpenGLFunctions>
#include <QMatrix4x4>
#include <QColor>
#include <QOpenGLBuffer>
#include <QOpenGLTexture>
#include <QVector3D>
#include <QVector4D>
#include <QVector2D>
#include "vec2.h"
#include "vec3.h"

class CCamera;
class QPainter;

struct ColorVertex {
    QVector3D vertex;
    QVector4D color;
};

struct VertexTexcoord {
    QVector3D vertex;
    QVector2D texcoord;
};

enum Textures {
    FLOOR,
    FONT,
    HYDLIFT,
    TRACTOR,
    QUESTION_MARK,
    FRONT_WHEELS,
    TRACTOR_4WD_FRONT,
    TRACTOR_4WD_REAR,
    HARVESTER,
    TOOLWHEELS,
    TIRE
};

extern bool isFontOn;
extern QVector<QOpenGLTexture *> texture;


//thinking about putting GL buffer drawing routines here
//like Draw box, etc. Do I put the shaders as module globals here?

void initializeBackShader();

void initializeShaders();
void initializeTextures();

void destroyShaders();
void destroyTextures();

//Simple wrapper to draw primitives using lists of Vec3 or QVector3Ds
//with a single color to the back frame buffer.
void glDrawArraysColorBack(QOpenGLFunctions *gl, QMatrix4x4 mvp,
                       GLenum operation, QColor color,
                       QOpenGLBuffer &vertexBuffer, GLenum glType,
                       int count,
                       float pointSize=1.0f);

//Simple wrapper to draw primitives using lists of Vec3 or QVector3Ds
//with a single color.
void glDrawArraysColor(QOpenGLFunctions *gl, QMatrix4x4 mvp,
                       GLenum operation, QColor color,
                       QOpenGLBuffer &vertexBuffer, GLenum glType,
                       int count,
                       float pointSize=1.0f);
//Simple wrapper to draw primitives using lists of vec3s or QVector3Ds
//with a color per vertex. Buffer format is 7 values per vertice:
//x,y,z,r,g,b,a
void glDrawArraysColors(QOpenGLFunctions *gl, QMatrix4x4 mvp,
                       GLenum operation,
                       QOpenGLBuffer &vertexBuffer, GLenum glType,
                       int count,
                       float pointSize=1.0f);

//Buffer format is 5 values per vertice:
//x,y,z,texX,texY
void glDrawArraysTexture(QOpenGLFunctions *gl, QMatrix4x4 mvp,
                         GLenum operation,
                         QOpenGLBuffer &vertexBuffer, GLenum glType,
                         int count, bool useColor, QColor color);

//Buffer format is 5 values per vertice:
//x,y,z,texX,texY
void glDrawArraysTextureBack(QOpenGLFunctions *gl, QMatrix4x4 mvp,
                         GLenum operation,
                         QOpenGLBuffer &vertexBuffer, GLenum glType,
                         int count, bool useColor, QColor color);

//draw arrays
void drawText(QOpenGLFunctions *gl, QMatrix4x4 mvp, double x, double y, QString text, double size = 1.0, bool colorize = false, QColor color = QColor::fromRgbF(1,1,1));
void drawText3D(QOpenGLFunctions *gl, QMatrix4x4 mvp, double x1, double y1, QString text, double size = 1.0, bool colorize = false, QColor color = QColor::fromRgbF(1,1,1));
void drawTextVehicle(const CCamera &camera, QOpenGLFunctions *gl, QMatrix4x4 mvp, double x, double y, QString text, double size = 1.0, bool colorize = false, QColor color = QColor::fromRgbF(1,1,1));

void DrawPolygon(QOpenGLFunctions *gl, QMatrix4x4 mvp, QVector<Vec2> &polygon, float size, QColor color);
void DrawPolygon(QOpenGLFunctions *gl, QMatrix4x4 mvp, QVector<Vec3> &polygon, float size, QColor color);

void DrawPolygonBack(QOpenGLFunctions *gl, QMatrix4x4 mvp, const QVector<Vec2> &polygon, float size, QColor color);
void DrawPolygonBack(QOpenGLFunctions *gl, QMatrix4x4 mvp, const QVector<Vec3> &polygon, float size, QColor color);
void DrawPolygonBack(QPainter &painter, const QVector<Vec2> &polygon, float size, QColor color);
void DrawPolygonBack(QPainter &painter, const QVector<Vec3> &polygon, float size, QColor color);

class GLHelperOneColorBack: public QVector<QVector3D>
{
public:
    GLHelperOneColorBack();
    void draw(QOpenGLFunctions *gl, QMatrix4x4 mvp, QColor color, GLenum operation, float point_size);
};

class GLHelperOneColor: public QVector<QVector3D>
{
public:
    GLHelperOneColor();
    void draw(QOpenGLFunctions *gl, QMatrix4x4 mvp, QColor color, GLenum operation, float point_size);
};

class GLHelperColors: public QVector<ColorVertex>
{
public:
    GLHelperColors();
    void draw(QOpenGLFunctions *gl, QMatrix4x4 mvp, GLenum operation, float point_size);
};

class GLHelperTextureBack: public QVector<VertexTexcoord>
{
public:
    GLHelperTextureBack();
    void draw(QOpenGLFunctions *gl, QMatrix4x4 mvp, GLenum operation,
              bool colorize=false, QColor color = QColor::fromRgbF(1,1,1));
};

class GLHelperTexture: public QVector<VertexTexcoord>
{
public:
    GLHelperTexture();
    void draw(QOpenGLFunctions *gl, QMatrix4x4 mvp, GLenum operation,
              bool colorize=false, QColor color = QColor::fromRgbF(1,1,1));
    void draw(QOpenGLFunctions *gl, QMatrix4x4 mvp, Textures textureno, GLenum operation,
              bool colorize=false, QColor color = QColor::fromRgbF(1,1,1));
};

inline static void CalcFrustum(QMatrix4x4 mvp, double *frustum) {
    // Extract the RIGHT clipping plane
    frustum[0] = mvp(3,0) - mvp(0,0);
    frustum[1] = mvp(3,1) - mvp(0,1);
    frustum[2] = mvp(3,2) - mvp(0,2);
    frustum[3] = mvp(3,3) - mvp(0,3);

    // Extract the LEFT clipping plane
    frustum[4] = mvp(0,0) + mvp(3,0);
    frustum[5] = mvp(0,1) + mvp(3,1);
    frustum[6] = mvp(0,2) + mvp(3,2);
    frustum[7] = mvp(0,3) + mvp(3,3);

    // Extract the FAR clipping plane
    frustum[8] = mvp(3,0) - mvp(2,0);
    frustum[9] = mvp(3,1) - mvp(2,1);
    frustum[10] = mvp(3,2) - mvp(2,2);
    frustum[11] = mvp(3,3) - mvp(2,3);

    // Extract the NEAR clipping plane.  This is last on purpose (see pointinfrustum() for reason)
    frustum[12] = mvp(2,0) + mvp(3,0);
    frustum[13] = mvp(2,1) + mvp(3,1);
    frustum[14] = mvp(2,2) + mvp(3,2);
    frustum[15] = mvp(2,3) + mvp(3,3);

    // Extract the BOTTOM clipping plane
    frustum[16] = mvp(1,0) + mvp(3,0);
    frustum[17] = mvp(1,1) + mvp(3,1);
    frustum[18] = mvp(1,2) + mvp(3,2);
    frustum[19] = mvp(1,3) + mvp(3,3);

    // Extract the TOP clipping plane
    frustum[20] = mvp(3,0) - mvp(1,0);
    frustum[21] = mvp(3,1) - mvp(1,1);
    frustum[22] = mvp(3,2) - mvp(1,2);
    frustum[23] = mvp(3,3) - mvp(1,3);
}

inline static QPointF vec2point(Vec2 v) {
    return QPointF(v.easting, v.northing);
}

inline static QPointF vec2point(Vec3 v) {
    return QPointF(v.easting, v.northing);
}

#endif // GLUTILS_H
