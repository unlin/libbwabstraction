#ifndef MODELVIEWER_H
#define MODELVIEWER_H

#include "pch.h"
#include <trimesh.hpp>
#include "myrenderer.h"
#include "viewmanager.h"

class ModelViewer : public MyRenderer
{
    Q_OBJECT
public:
    enum DisplayMode {
        TRIANGLE = 1,
        VERTEX = 2,
        EDGE = 4
    };

    ModelViewer(QWidget* parent = 0);
    void SetDisplayMode(int mode);
    void SetLightDirection(int phi, int theta);
protected slots:
    void lightDirectionChanged();
    void modelDisplayChanged();
protected:
    void onAppReady();
    void myInitializeGL();
    void myPaintGL();
    void onMeshChanged();
    void mousePressEvent(QMouseEvent *);
    void mouseReleaseEvent(QMouseEvent *);
    void mouseMoveEvent(QMouseEvent *);
    void wheelEvent(QWheelEvent *);

    StandardShader standardShader;
    StandardShader manMadeObjShader;
    StandardVertexAttribute vertexAttribute;
    bwabstraction::TriMesh triMesh;

    std::vector<float> contours;
    StandardVertexAttribute contourVertexAttribute;

    QVector3D lightvec;
    int displayMode = TRIANGLE;
};

#endif // MODELVIEWER_H
