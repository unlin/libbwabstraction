#ifndef VIEWMANAGER_H
#define VIEWMANAGER_H

#include "pch.h"

/**
 * @brief The ViewManager class
 * The ViewManager class provides viewing manipulation related functionalities.
 *
 * To use the ViewManager class, call mousePressEvent(),
 * mouseReleaseEvent(), mouseMoveEvent(), wheelEvent() in your event handlers
 * with the event objects.
 *
 * The viewing manipulation will be done for you in the class. When you are ready
 * to render something, call GetModelMatrix(), GetViewMatrix(), GetProjectionMatrix()
 * and their composite versions to get the MVP matrices which ecode current viewing
 * properties.
 */
class ViewManager
{

public:

    ViewManager();

    // property getters
    QMatrix4x4 GetModelMatrix();
    QMatrix4x4 GetViewMatrix();
    QMatrix4x4 GetProjectionMatrix(float aspect);
    QMatrix4x4 GetViewProjectionMatrix(float aspect);
    QMatrix4x4 GetModelViewProjectionMatrix(float aspect);
    QVector3D GetEyePosition();
    QVector3D GetViewVector();
    bool IsOrthoProjection();

    // Qt event handlers
    void mousePressEvent(QMouseEvent *);
    void mouseReleaseEvent(QMouseEvent *);
    void mouseMoveEvent(QMouseEvent *, QSize);
    void wheelEvent(QWheelEvent *);

    // setters
    void SetRotation(float theta, float phi);
    void SetRotation(float x, float y, float z);
    bool ToggleOrtho();
    void Zoom(float distance);
    void Reset();

private:

    bool ortho = false;
    float zoom = 1.0f;

    QMatrix4x4 translationMatrix;
    QMatrix4x4 rotationMatrix;
    QMatrix4x4 viewMatrix;
    QVector3D viewVector;
    QVector3D rotateXAxis;
    QVector3D rotateYAxis;
    QVector3D eyePosition;
    QVector3D eyeLookPosition;

    bool lmbDown = false;
    bool midDown = false;
    QVector2D lmbDownCoord;
    QVector2D midDownCoord;

};

#endif // VIEWMANAGER_H
