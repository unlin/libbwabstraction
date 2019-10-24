#ifndef MIXEDRENDERER_H
#define MIXEDRENDERER_H

#include "pch.h"
#include <trimesh.hpp>
#include <rendertarget.hpp>
#include "viewmanager.h"
#include "myrenderer.h"

class BWARenderer : public MyRenderer
{

    Q_OBJECT

public:

    BWARenderer(QWidget *parent = NULL);

    enum class BackgroundColor
    {
        White = 0,
        Grey = 1,
        Black = 2
    };

    enum class HaloType
    {
        ScaleAware = 0,
        Solid1px = 1,
        NoHalo = 2
    };

    enum class OptimizationMethod
    {
        BeliefPropagation = 0,
        RuleBased = 1,
        AllBlack = 2
    };

    enum class ValidationLayer
    {
        Optimized = 0,
        BlinnPhong = 1,
        ColoredSegment = 2,
        Colored3DSegment = 3,
        DistanceField = 4,
        Patch = 5,
        Inclusion = 6,
        Similarity= 7
    };

public slots:

    void OptimizeOnce();
    void ShowContextMenu(QPoint p);
    void TimerUpdate();
    void ForceBlack();
    void ForceWhite();
    void ClearForce();

protected:

    void onAppReady();
    void myPaintGL();
    void onMeshChanged();
    void myResizeGL(int w, int h);
    void myInitializeGL();
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void wheelEvent(QWheelEvent *event);
    void keyPressEvent(QKeyEvent *event);

private:

    QVector3D getBackgroundColor();

    QPoint mouseSelect;
    QTimer timer;
    bool optimizeOnce = false;
    bool shiftDown = false;
    bwabstraction::BWAbstraction bwa;
    bwabstraction::Parameters bwaParam;
    bwabstraction::Result bwaResult;
    StandardShader standardShader;
    StandardVertexAttribute quad;
    RGBADTarget renderTarget;

};

#endif // MIXEDRENDERER_H
