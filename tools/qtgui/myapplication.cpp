#include "pch.h"
#include "myapplication.h"
#include "mainwindow.h"
#include "renderer/viewmanager.h"
#include "ui_mainwindow.h"
#include <glutils.hpp>

MyApplication *MyApplication::instance = NULL;

MyApplication::MyApplication(int argc, char *argv[]) :
    QApplication(argc, argv)
{
    instance = this;
    SetDefaultSurfaceFormat();
    InitGLEW();
    viewManager = new ViewManager();
    mainWindow = new MainWindow();
    mainWindow->NotifyAppReady();
    mainWindow->show();
}

void MyApplication::SetDefaultSurfaceFormat()
{
    QSurfaceFormat format;
    format.setVersion(4, 5);
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setDepthBufferSize(24);
    format.setStencilBufferSize(8);
    QSurfaceFormat::setDefaultFormat(format);
}

void MyApplication::InitGLEW()
{
    QOpenGLContext *context = new QOpenGLContext();
    QOffscreenSurface *surface = new QOffscreenSurface();
    context->create();
    surface->create();
    context->makeCurrent(surface);
    glewExperimental = GL_TRUE;
    glewInit();
    glPrintContextInfo();
}
