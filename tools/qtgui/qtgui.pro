QT += core gui widgets

TARGET = bwa-qtgui
TEMPLATE = app
CONFIG += c++11 precompile_header

SOURCES += main.cpp\
    mainwindow.cpp \
    myapplication.cpp \
    renderer/modelviewer.cpp \
    renderer/myrenderer.cpp \
    renderer/viewmanager.cpp \
    utility/utility.cpp \
    renderer/bwarenderer.cpp \
    shader/shaders.cpp

HEADERS  += mainwindow.h \
    pch.h \
    myapplication.h \
    renderer/modelviewer.h \
    renderer/myrenderer.h \
    renderer/viewmanager.h \
    utility/utility.h \
    renderer/bwarenderer.h \
    shader/shaders.hpp

PRECOMPILED_HEADER = pch.h

FORMS += mainwindow.ui

DISTFILES +=

win32 {
    message(win-x64-build)
    QMAKE_CXXFLAGS += /openmp
    INCLUDEPATH += $$PWD/../../include $$PWD/../../src $$PWD/../../external/include
    DEFINES += _USE_MATH_DEFINES # enable math constant definitions in cmath header. M_PI, M_E, ...
    LIBPATH += $$PWD/../../lib $$PWD/../../external/lib64
    Release::LIBS += CGAL_Core-vc140-mt-4.13.lib CGAL_ImageIO-vc140-mt-4.13.lib CGAL-vc140-mt-4.13.lib\
        libgmp-10.lib libmpfr-4.lib opencv_world411.lib OpenMeshCore.lib OpenMeshTools.lib OpenGL32.lib\
        glew32.lib glfw3dll.lib libbwabstraction.lib
    Debug::LIBS += CGAL_Core-vc140-mt-4.13.lib CGAL_ImageIO-vc140-mt-4.13.lib CGAL-vc140-mt-4.13.lib\
        libgmp-10.lib libmpfr-4.lib opencv_world411d.lib OpenMeshCored.lib OpenMeshToolsd.lib OpenGL32.lib\
        glew32.lib glfw3dll.lib libbwabstractiond.lib
}

macx {
    message(macx-x64-build)
    INCLUDEPATH += $$PWD/../../src /usr/local/include /usr/local/include/opencv4 /usr/local/include/eigen3
    DEFINES += _USE_MATH_DEFINES # enable math constant definitions in cmath header. M_PI, M_E, ...
    LIBPATH += /usr/local/lib
    LIBS += -lGLEW -lglfw -lOpenMeshCore -lOpenMeshTools -lCGAL -lopencv_core -lopencv_imgproc -lopencv_highgui\
        -lmpfr -lgmp -llibbwabstraction
}
