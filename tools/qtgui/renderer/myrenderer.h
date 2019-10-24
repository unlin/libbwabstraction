#ifndef MYRENDERER_H
#define MYRENDERER_H

#include "pch.h"
#include <standardshader.hpp>
#include "viewmanager.h"
#include "myapplication.h"

namespace Ui
{
    class MainWindow;
}

class MyRenderer : public QOpenGLWidget
{

    Q_OBJECT

public:

    MyRenderer(QWidget *parent = NULL);
    void AppReady();
    void MeshChanged(QString modelFilepath);
    void SaveFramebuffer();

protected:

    virtual void myInitializeGL();
    virtual void myPaintGL();
    virtual void myResizeGL(int w, int h);

    virtual void onMeshChanged();
    void BindRendererFBO();
    virtual void onAppReady();
    QOpenGLFramebufferObject* GetCurrentFbo();
    QPoint GetLocalPoint(QPoint p);

    QSize viewportSize;
    float viewportAspect;
    float viewportRatio;
    ViewManager *viewMgr = NULL;
    MyApplication *app = NULL;
    Ui::MainWindow *ui = NULL;
    bool useMultiSampleFbo = false;
    float viewportScale = 1.0f;
    QSize screenSize;
    QString modelFilepath = "";

protected slots:

    void ShowSaveScreenMessage(QString);

signals:

    void ScreenSaved(QString);

private:

    bool meshChanged = false;
    bool saveFramebuffer = false;
    QOpenGLFramebufferObjectFormat multiSampleFboFormat;
    QOpenGLFramebufferObjectFormat blitedFboFormat;
    QOpenGLFramebufferObject *multiSampleFbo = NULL;
    QOpenGLFramebufferObject *blitedFbo = NULL;
    StandardShader standardShader;
    StandardVertexAttribute quad;

    void SaveScreenshot(QImage &image);
    void initializeGL();
    void paintGL();
    void resizeGL(int w, int h);

};

#endif // MYRENDERER_H
