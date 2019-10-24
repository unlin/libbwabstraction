#include "modelviewer.h"
#include "mainwindow.h"
#include "utility/utility.h"
#include "ui_mainwindow.h"
#include <glutils.hpp>
#include "shader/shaders.hpp"

using namespace std;
using namespace bwabstraction;

ModelViewer::ModelViewer(QWidget *parent) :
    MyRenderer(parent)
{
    SetLightDirection(0, 0);
    useMultiSampleFbo = true;
}

void ModelViewer::onAppReady()
{
    connect(ui->horizontalSlider, SIGNAL(valueChanged(int)), this, SLOT(lightDirectionChanged()));
    connect(ui->horizontalSlider_2, SIGNAL(valueChanged(int)), this, SLOT(lightDirectionChanged()));

    connect(ui->mv_display_triangle, SIGNAL(toggled(bool)), this, SLOT(modelDisplayChanged()));
    connect(ui->mv_display_edge, SIGNAL(toggled(bool)), this, SLOT(modelDisplayChanged()));
    connect(ui->mv_display_vertex, SIGNAL(toggled(bool)), this, SLOT(modelDisplayChanged()));
}

void ModelViewer::lightDirectionChanged()
{
    SetLightDirection(ui->horizontalSlider->value(), ui->horizontalSlider_2->value());
    update();
}

void ModelViewer::modelDisplayChanged()
{
    int mode = 0;
    if(ui->mv_display_triangle->isChecked())
    {
        mode = mode | ModelViewer::DisplayMode::TRIANGLE;
    }
    if(ui->mv_display_edge->isChecked())
    {
        mode = mode | ModelViewer::DisplayMode::EDGE;
    }
    if(ui->mv_display_vertex->isChecked())
    {
        mode = mode | ModelViewer::DisplayMode::VERTEX;
    }
    SetDisplayMode(mode);
}

void ModelViewer::myInitializeGL()
{
    glClearColor(0.85f, 0.85f, 0.85f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    standardShader.Create(colored_vs_glsl, colored_fs_glsl);
    manMadeObjShader.Create(manmadeobj_vs_glsl, manmadeobj_fs_glsl);
    vertexAttribute.Create();
    contourVertexAttribute.Create();
}

void ModelViewer::onMeshChanged()
{
    triMesh.LoadOBJ(modelFilepath.toStdString());
    vector<float> vertices;
    vector<float> normals;
    vector<unsigned int> indices;
    vector<float> empty;
    triMesh.GetMeshData(vertices, normals, indices);
    vertexAttribute.BufferData(vertices, normals, empty, empty, indices);

    // update mesh information
    ui->mvNVertices->setText(QString::asprintf("%zu", triMesh.n_vertices()));
    ui->mvNFaces->setText(QString::asprintf("%zu", triMesh.n_faces()));

    /////////////////////////////

    const float angleThreshold = 80.0f;
    TriMesh &mesh = triMesh;
    contours.clear();
    for(TriMesh::EdgeIter eit = mesh.edges_begin(); eit != mesh.edges_end(); eit++)
    {
        TriMesh::FaceHandle f1 = mesh.face_handle(mesh.halfedge_handle(*eit, 0));
        TriMesh::FaceHandle f2 = mesh.face_handle(mesh.halfedge_handle(*eit, 1));
        if(f1.is_valid() && f2.is_valid())
        {
            TriMesh::Normal n1 = mesh.normal(f1).normalized();
            TriMesh::Normal n2 = mesh.normal(f2).normalized();
            if(dot(n1, n2) <= cos(angleThreshold / 180.0 * M_PI))
            {
                TriMesh::HalfedgeHandle he = mesh.halfedge_handle(*eit, 0);
                TriMesh::VertexHandle v1 = mesh.from_vertex_handle(he);
                TriMesh::VertexHandle v2 = mesh.to_vertex_handle(he);
                TriMesh::Point p1 = mesh.point(v1);
                TriMesh::Point p2 = mesh.point(v2);
                contours.push_back((float)p1[0]);
                contours.push_back((float)p1[1]);
                contours.push_back((float)p1[2]);
                contours.push_back((float)p2[0]);
                contours.push_back((float)p2[1]);
                contours.push_back((float)p2[2]);
            }
        }
    }

    vector<unsigned int> emptyIdx;
    contourVertexAttribute.BufferData(contours, empty, empty, empty, emptyIdx);
}

void ModelViewer::myPaintGL()
{
    if(displayMode & DisplayMode::TRIANGLE)
    {
        QVector3D eye = viewMgr->GetEyePosition();
        QMatrix4x4 m = viewMgr->GetModelMatrix();
        QMatrix4x4 o2w = m.inverted().transposed();
        manMadeObjShader.Bind();
        glUniformMatrix4fv(glGetUniformLocation(manMadeObjShader.program, "m"), 1, GL_FALSE, m.data());
        glUniformMatrix4fv(glGetUniformLocation(manMadeObjShader.program, "o2w"), 1, GL_FALSE, o2w.data());
        glUniformMatrix4fv(glGetUniformLocation(manMadeObjShader.program, "mvp"), 1, GL_FALSE, viewMgr->GetModelViewProjectionMatrix(viewportAspect).data());
        glUniform3f(glGetUniformLocation(manMadeObjShader.program, "light"), lightvec[0], lightvec[1], lightvec[2]);
        glUniform3f(glGetUniformLocation(manMadeObjShader.program, "eye"), eye[0], eye[1], eye[2]);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(2.0f, 2.0f);
        manMadeObjShader.Draw(GL_TRIANGLES, vertexAttribute);
        glDisable(GL_POLYGON_OFFSET_FILL);
    }
    if(displayMode & DisplayMode::EDGE)
    {
        vertexAttribute.Color3f(0.0f, 0.0f, 1.0f);
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glLineWidth(1.0f);
        standardShader.Bind();
        glUniformMatrix4fv(glGetUniformLocation(standardShader.program, "mvp"), 1, GL_FALSE, viewMgr->GetModelViewProjectionMatrix(viewportAspect).data());
        standardShader.Draw(GL_TRIANGLES, vertexAttribute);

        contourVertexAttribute.Color3f(1.0f, 0.0f, 0.0f);
        glLineWidth(3.0f);
        standardShader.Draw(GL_LINES, contourVertexAttribute);
    }
    if(displayMode & DisplayMode::VERTEX)
    {
        vertexAttribute.Color3f(0.0f, 1.0f, 0.0f);
        glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
        glPointSize(5.0f);
        standardShader.Bind();
        glUniformMatrix4fv(glGetUniformLocation(standardShader.program, "mvp"), 1, GL_FALSE, viewMgr->GetModelViewProjectionMatrix(viewportAspect).data());
        standardShader.Draw(GL_TRIANGLES, vertexAttribute);
    }
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void ModelViewer::mousePressEvent(QMouseEvent *event)
{
    viewMgr->mousePressEvent(event);
}

void ModelViewer::mouseReleaseEvent(QMouseEvent *event)
{
    viewMgr->mouseReleaseEvent(event);
}

void ModelViewer::mouseMoveEvent(QMouseEvent *event)
{
    viewMgr->mouseMoveEvent(event, viewportSize);
    update();
}

void ModelViewer::wheelEvent(QWheelEvent *event)
{
    viewMgr->wheelEvent(event);
    update();
}

void ModelViewer::SetDisplayMode(int mode)
{
    displayMode = mode;
    update();
}

void ModelViewer::SetLightDirection(int phi, int theta)
{
    double theta_rad = theta / 180.0 * 3.1415926;
    double phi_rad = (phi + 45) / 180.0 * 3.1415926;
    lightvec = QVector3D(cos(theta_rad) * cos(phi_rad), sin(theta_rad) * cos(phi_rad), sin(phi_rad));
}
