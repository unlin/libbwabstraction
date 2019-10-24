#include "bwarenderer.h"
#include "utility/utility.h"
#include "ui_mainwindow.h"
#include "utility/utility.h"
#include <glutils.hpp>
#include <opencv2/opencv.hpp>
#include "shader/shaders.hpp"

using namespace cv;
using namespace std;

BWARenderer::BWARenderer(QWidget *parent) :
    MyRenderer(parent)
{
    this->setContextMenuPolicy(Qt::CustomContextMenu);
    this->setFocusPolicy(Qt::StrongFocus);

    connect(this, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(ShowContextMenu(QPoint)));
    connect(&timer, SIGNAL(timeout()), this, SLOT(TimerUpdate()));
    timer.start(16);

    bwaParam.useHostOpenGL = true;
}

void BWARenderer::TimerUpdate()
{
    update();
}

void BWARenderer::OptimizeOnce()
{
    optimizeOnce = true;
    update();
}

void BWARenderer::ShowContextMenu(QPoint p)
{
    QMenu contextMenu(tr("Contrain Editing"), this);

    QAction forceBlack("Patch: Black", this);
    connect(&forceBlack, SIGNAL(triggered(bool)), this, SLOT(ForceBlack()));
    contextMenu.addAction(&forceBlack);

    QAction forceWhite("Patch: White", this);
    connect(&forceWhite, SIGNAL(triggered(bool)), this, SLOT(ForceWhite()));
    contextMenu.addAction(&forceWhite);

    QAction clearForce("Reset", this);
    connect(&clearForce, SIGNAL(triggered(bool)), this, SLOT(ClearForce()));
    contextMenu.addAction(&clearForce);

    contextMenu.exec(this->mapToGlobal(p));
}

void BWARenderer::ClearForce()
{
    //forceList.clear();
    OptimizeOnce();
}

void BWARenderer::ForceBlack()
{
    //forceList.push_back(pair<QPoint, int>(mouseSelect, 0));
    OptimizeOnce();
}

void BWARenderer::ForceWhite()
{
    //forceList.push_back(pair<QPoint, int>(mouseSelect, 1));
    OptimizeOnce();
}

void BWARenderer::onAppReady()
{
    connect(ui->bwaRenderHeight, SIGNAL(valueChanged(int)), this, SLOT(OptimizeOnce()));
    connect(ui->bwaRenderWidth, SIGNAL(valueChanged(int)), this, SLOT(OptimizeOnce()));
    connect(ui->bwaNeighbourWeight, SIGNAL(valueChanged(double)), this, SLOT(OptimizeOnce()));
    connect(ui->bwaConsistencyWeight, SIGNAL(valueChanged(double)), this, SLOT(OptimizeOnce()));
    connect(ui->bwaBackgroundWeight, SIGNAL(valueChanged(double)), this, SLOT(OptimizeOnce()));
    connect(ui->bwaInclusionWeight, SIGNAL(valueChanged(double)), this, SLOT(OptimizeOnce()));
    connect(ui->bwaContrastWeight, SIGNAL(valueChanged(double)), this, SLOT(OptimizeOnce()));
    connect(ui->bwaScaleFactor, SIGNAL(valueChanged(double)), this, SLOT(OptimizeOnce()));
    connect(ui->bwaFeatureLineThresholdPercent, SIGNAL(valueChanged(double)), this, SLOT(OptimizeOnce()));

    connect(ui->bwaBackgroundColor, SIGNAL(currentIndexChanged(int)), this, SLOT(OptimizeOnce()));
    connect(ui->bwaResultDisplay, SIGNAL(currentIndexChanged(int)), this, SLOT(OptimizeOnce()));
}

void BWARenderer::myInitializeGL()
{
    standardShader.Create(texquad_vs_glsl, texquad_fs_glsl);
    quad = StandardVertexAttribute::CreateQuadAttribute();
    renderTarget.init();
    renderTarget.resize(1600, 1200);
}

void BWARenderer::onMeshChanged()
{
    bwa.LoadModel(modelFilepath.toStdString(), bwaParam);
    optimizeOnce = true;
    //forceList.clear();
}

QVector3D BWARenderer::getBackgroundColor()
{
    if(ui->bwaBackgroundColor->currentIndex() == static_cast<int>(BackgroundColor::White))
    {
        return QVector3D(1.0f, 1.0f, 1.0f);
    }
    else if(ui->bwaBackgroundColor->currentIndex() == static_cast<int>(BackgroundColor::Grey))
    {
        return QVector3D(0.5f, 0.5f, 0.5f);
    }
    return QVector3D(0, 0, 0);
}

void BWARenderer::myPaintGL()
{
    if(optimizeOnce)
    {
        optimizeOnce = false;

        // update params
        bwaParam.scale = ui->bwaScaleFactor->value();
        bwaParam.contrastWeight = ui->bwaContrastWeight->value();
        bwaParam.inclusionWeight = ui->bwaInclusionWeight->value();
        bwaParam.backgroundWeight = ui->bwaBackgroundWeight->value();
        bwaParam.neighbourWeight = ui->bwaNeighbourWeight->value();
        bwaParam.consistencyWeight = ui->bwaConsistencyWeight->value();
        bwaParam.featureWeight = ui->bwaFeatureLineThresholdPercent->value();
        bwaParam.renderWidth = ui->bwaRenderWidth->value();
        bwaParam.renderHeight = ui->bwaRenderHeight->value();
        float aspect = static_cast<float>(bwaParam.renderWidth) / static_cast<float>(bwaParam.renderHeight);
        if(renderTarget.w != bwaParam.renderWidth || renderTarget.h != bwaParam.renderHeight)
        {
            renderTarget.resize(bwaParam.renderWidth, bwaParam.renderHeight);
        }
        memcpy(bwaParam.mvpMatrix, viewMgr->GetModelViewProjectionMatrix(aspect).data(), sizeof(float) * 16);
        QVector3D backgroundColor = getBackgroundColor();
        bwaParam.backgroundColor[0] = backgroundColor[0];
        bwaParam.backgroundColor[1] = backgroundColor[1];
        bwaParam.backgroundColor[2] = backgroundColor[2];

        // render and create OpenGL texture
        const int from_to_bwa[] = { 0,0, 0,1, 0,2, 0,3 };
        const int from_to_other[] = { 0,0, 1,1, 2,2, 0,3 };
        Mat img;
        switch(ui->bwaResultDisplay->currentIndex())
        {
        case 0:
            bwaParam.resultImage = bwabstraction::ResultImage::BWA;
            bwa.Render(&bwaResult, bwaParam);
            img = Mat(bwaParam.renderHeight, bwaParam.renderWidth, CV_8UC4);
            mixChannels(bwaResult.bwaImage, img, from_to_bwa, 4);
            renderTarget.update(img);
            break;
        case 1:
            bwaParam.resultImage = bwabstraction::ResultImage::BWA + bwabstraction::ResultImage::PATCH;
            bwa.Render(&bwaResult, bwaParam);
            img = Mat(bwaParam.renderHeight, bwaParam.renderWidth, CV_8UC4);
            mixChannels(bwaResult.patchImage, img, from_to_other, 4);
            renderTarget.update(img);
            break;
        case 2:
            bwaParam.resultImage = bwabstraction::ResultImage::BWA + bwabstraction::ResultImage::DEPTH_CRITICAL;
            bwa.Render(&bwaResult, bwaParam);
            img = Mat(bwaParam.renderHeight, bwaParam.renderWidth, CV_8UC4);
            mixChannels(bwaResult.depthCriticalLineImage, img, from_to_other, 4);
            renderTarget.update(img);
            break;
        case 3:
            bwaParam.resultImage = bwabstraction::ResultImage::BWA + bwabstraction::ResultImage::BOUNDARY;
            bwa.Render(&bwaResult, bwaParam);
            img = Mat(bwaParam.renderHeight, bwaParam.renderWidth, CV_8UC4);
            mixChannels(bwaResult.boundaryImage, img, from_to_other, 4);
            renderTarget.update(img);
            break;
        case 4:
            bwaParam.resultImage = bwabstraction::ResultImage::BWA + bwabstraction::ResultImage::SHARP_EDGE;
            bwa.Render(&bwaResult, bwaParam);
            img = Mat(bwaParam.renderHeight, bwaParam.renderWidth, CV_8UC4);
            mixChannels(bwaResult.sharpEdgeLineImage, img, from_to_other, 4);
            renderTarget.update(img);
            break;
        case 5:
            bwaParam.resultImage = bwabstraction::ResultImage::BWA + bwabstraction::ResultImage::FEATURE_LINE;
            bwa.Render(&bwaResult, bwaParam);
            img = Mat(bwaParam.renderHeight, bwaParam.renderWidth, CV_8UC4);
            mixChannels(bwaResult.featureLineImage, img, from_to_other, 4);
            renderTarget.update(img);
            break;
        case 6:
            bwaParam.resultImage = bwabstraction::ResultImage::BWA + bwabstraction::ResultImage::CONSISTENCY;
            bwa.Render(&bwaResult, bwaParam);
            img = Mat(bwaParam.renderHeight, bwaParam.renderWidth, CV_8UC4);
            mixChannels(bwaResult.consistencyImage, img, from_to_other, 4);
            renderTarget.update(img);
            break;
        case 7:
            bwaParam.resultImage = bwabstraction::ResultImage::BWA + bwabstraction::ResultImage::COMPONENT;
            bwa.Render(&bwaResult, bwaParam);
            img = Mat(bwaParam.renderHeight, bwaParam.renderWidth, CV_8UC4);
            mixChannels(bwaResult.componentImage, img, from_to_other, 4);
            renderTarget.update(img);
            break;
        case 8:
            bwaParam.resultImage = bwabstraction::ResultImage::BWA + bwabstraction::ResultImage::INCLUSION;
            bwa.Render(&bwaResult, bwaParam);
            img = Mat(bwaParam.renderHeight, bwaParam.renderWidth, CV_8UC4);
            mixChannels(bwaResult.inclusionImage, img, from_to_other, 4);
            renderTarget.update(img);
            break;
        case 9:
            bwaParam.resultImage = bwabstraction::ResultImage::BWA + bwabstraction::ResultImage::DISTANCE_TRANSFORM;
            bwa.Render(&bwaResult, bwaParam);
            img = Mat(bwaParam.renderHeight, bwaParam.renderWidth, CV_8UC4);
            mixChannels(bwaResult.distanceTransformImage, img, from_to_other, 4);
            renderTarget.update(img);
            break;
        }
    }

    QMatrix4x4 mvp;
    BindRendererFBO();
    standardShader.Bind();
    glUniformMatrix4fv(glGetUniformLocation(standardShader.program, "mvp"), 1, GL_FALSE, mvp.data());
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, renderTarget.color_id_buffer);
    glUniform1i(glGetUniformLocation(standardShader.program, "tex"), 0);
    standardShader.Draw(GL_TRIANGLES, quad);
}

void BWARenderer::myResizeGL(int w, int h)
{
    Q_UNUSED(w);
    Q_UNUSED(h);
    mouseSelect = QPoint(0, 0);
    optimizeOnce = true;
}

void BWARenderer::mousePressEvent(QMouseEvent *event)
{
    mouseSelect = event->pos();
    mouseSelect = GetLocalPoint(mouseSelect);
    mouseSelect.setY(viewportSize.height() - 1 - mouseSelect.y());
    if(QGuiApplication::keyboardModifiers() & Qt::ShiftModifier)
    {
        viewMgr->mousePressEvent(event);
        //forceList.clear();
        OptimizeOnce();
    }
    update();
}

void BWARenderer::mouseMoveEvent(QMouseEvent *event)
{
    if(QGuiApplication::keyboardModifiers() & Qt::ShiftModifier)
    {
        viewMgr->mouseMoveEvent(event, viewportSize);
        //forceList.clear();
        OptimizeOnce();
    }
    update();
}

void BWARenderer::mouseReleaseEvent(QMouseEvent *event)
{
    if(QGuiApplication::keyboardModifiers() & Qt::ShiftModifier)
    {
        viewMgr->mouseReleaseEvent(event);
        //forceList.clear();
        OptimizeOnce();
    }
    update();
}

void BWARenderer::wheelEvent(QWheelEvent *event)
{
    if(QGuiApplication::keyboardModifiers() & Qt::ShiftModifier)
    {
        viewMgr->wheelEvent(event);
        //forceList.clear();
        OptimizeOnce();
    }
    update();
}

void BWARenderer::keyPressEvent(QKeyEvent *event)
{
    Q_UNUSED(event);
    update();
}
