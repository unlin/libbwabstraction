#include "pch.h"
#include "viewmanager.h"
#include "myapplication.h"
#include "utility/utility.h"

ViewManager::ViewManager()
{
    eyePosition = QVector3D(0, 0, 5.0f);
    eyeLookPosition = QVector3D(0, 0, 0);
    QVector3D up = QVector3D(0, 1, 0);
    viewMatrix.lookAt(eyePosition, eyeLookPosition, up);
    viewVector = eyePosition - eyeLookPosition;
    viewVector.normalize();
}

QMatrix4x4 ViewManager::GetModelMatrix()
{
    return translationMatrix * rotationMatrix;
}

QMatrix4x4 ViewManager::GetViewMatrix()
{
    return viewMatrix;
}

QMatrix4x4 ViewManager::GetProjectionMatrix(float aspect)
{
    QMatrix4x4 projectionMatrix;
    projectionMatrix.setToIdentity();
    float nearVal = 0.1f;
    float farVal = 15.0f;
    if(ortho)
    {
        float size = 1.5f * zoom;
        projectionMatrix.ortho(-aspect * size, aspect * size, -size, size, nearVal, farVal);
    }
    else
    {
        projectionMatrix.perspective(30.0f * zoom, aspect, nearVal, farVal);
    }
    return projectionMatrix;
}

QMatrix4x4 ViewManager::GetViewProjectionMatrix(float aspect)
{
    return GetProjectionMatrix(aspect) * viewMatrix;
}

QMatrix4x4 ViewManager::GetModelViewProjectionMatrix(float aspect)
{
    return GetViewProjectionMatrix(aspect) * GetModelMatrix();
}

QVector3D ViewManager::GetEyePosition()
{
    return eyePosition;
}

QVector3D ViewManager::GetViewVector()
{
    return viewVector;
}

bool ViewManager::IsOrthoProjection()
{
    return ortho;
}

void ViewManager::mousePressEvent(QMouseEvent *event)
{
    if( event->button() == Qt::LeftButton ) {
        lmbDown = true;
        lmbDownCoord = QVector2D( event->x(), event->y() );
        QMatrix4x4 invrtRot = rotationMatrix.inverted();
        rotateYAxis = (invrtRot * QVector4D(0, 1, 0, 0)).toVector3D();
        rotateXAxis = (invrtRot * QVector4D(1, 0, 0, 0)).toVector3D();
    } else if(event->button() == Qt::MiddleButton) {
        midDown = true;
        midDownCoord = QVector2D(event->x(), event->y());
    }
}

void ViewManager::mouseReleaseEvent(QMouseEvent *event)
{
    if( event->button() == Qt::LeftButton ) {
        lmbDown = false;
    } else if(event->button() == Qt::MiddleButton) {
        midDown = false;
    }
}

void ViewManager::mouseMoveEvent(QMouseEvent *event, QSize viewportSize)
{
    if( lmbDown ) {
        QVector2D coord = QVector2D( event->x(), event->y() );
        QVector2D diff = coord - lmbDownCoord;
        double factor = 0.2;
        rotationMatrix.rotate(diff.x() * factor, rotateYAxis);
        rotationMatrix.rotate(diff.y() * factor, rotateXAxis);
        lmbDownCoord = coord;
    } else if(midDown) {
        QVector2D coord = QVector2D(event->x(), event->y());
        QVector2D diff = coord - midDownCoord;
        diff *= QVector2D(1.0 / viewportSize.width(), -1.0 / viewportSize.height()) * zoom * 3.0;
        translationMatrix.translate(QVector3D(diff, 0));
        midDownCoord = coord;
    }
}

void ViewManager::wheelEvent(QWheelEvent *event)
{
    QPoint wheelPosition = event->angleDelta();
    Zoom(wheelPosition.y() / 120.0f);
}

void ViewManager::SetRotation(float theta, float phi)
{
    rotationMatrix.setToIdentity();
    rotationMatrix.rotate(theta, QVector3D(0, 1, 0));
    rotationMatrix.rotate(phi, QVector3D(1, 0, 0));
}

void ViewManager::SetRotation(float x, float y, float z)
{
    QVector3D v(x, y, z);
    v.normalize();
    QVector3D o(0, 0, 1);
    double angle = acos(QVector3D::dotProduct(v, o));
    rotationMatrix.setToIdentity();
    rotationMatrix.rotate(qRadiansToDegrees(angle), QVector3D::crossProduct(o, v));
}

bool ViewManager::ToggleOrtho()
{
    return ortho = !ortho;
}

void ViewManager::Zoom(float distance)
{
    zoom *= (1.0f + 0.05f * distance);
    zoom = qBound(0.1f, zoom, 2.0f);
}

void ViewManager::Reset()
{
    zoom = 1.0f;
    translationMatrix.setToIdentity();
    rotationMatrix.setToIdentity();
}
