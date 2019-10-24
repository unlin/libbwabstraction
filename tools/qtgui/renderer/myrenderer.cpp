#include "myrenderer.h"
#include "mainwindow.h"
#include "utility/utility.h"
#include "ui_mainwindow.h"
#include <glutils.hpp>
#include "shader/shaders.hpp"

#define RENDER_WIDTH 1600
#define RENDER_HEIGHT 1200

MyRenderer::MyRenderer(QWidget *parent) :
    QOpenGLWidget(parent)
{
    viewportSize = QSize(RENDER_WIDTH, RENDER_HEIGHT);
    viewportAspect = (float)RENDER_WIDTH / (float)RENDER_HEIGHT;
    connect(this, SIGNAL(ScreenSaved(QString)), this, SLOT(ShowSaveScreenMessage(QString)));
}

void MyRenderer::AppReady()
{
    app = MyApplication::Instance();
    viewMgr = app->GetViewManager();
    ui = app->GetMainWindow()->GetUI();
    onAppReady();
}

void MyRenderer::MeshChanged(QString modelFilepath)
{
    this->modelFilepath = modelFilepath;
    meshChanged = true;
}

void MyRenderer::SaveFramebuffer()
{
    saveFramebuffer = true;
    update();
}

void MyRenderer::onAppReady()
{

}

void MyRenderer::initializeGL()
{
    multiSampleFboFormat.setAttachment(QOpenGLFramebufferObject::Depth);
    multiSampleFboFormat.setSamples(8);
    multiSampleFboFormat.setTextureTarget(GL_TEXTURE_2D);
    multiSampleFboFormat.setInternalTextureFormat(GL_RGBA8);
    blitedFboFormat.setAttachment(QOpenGLFramebufferObject::Depth);
    blitedFboFormat.setMipmap(true);
    standardShader.Create(texquad_vs_glsl, texquad_fs_glsl);
    quad = StandardVertexAttribute::CreateQuadAttribute();
    myInitializeGL();
}

void MyRenderer::myInitializeGL()
{

}

void MyRenderer::BindRendererFBO()
{
    GetCurrentFbo()->bind();
    glViewport(0, 0, viewportSize.width(), viewportSize.height());
    glDrawBuffer(GL_COLOR_ATTACHMENT0);
}

void MyRenderer::paintGL()
{
    if(meshChanged)
    {
        meshChanged = false;
        onMeshChanged();
    }

    glViewport(0, 0, viewportSize.width(), viewportSize.height());
    glDrawBuffer(GL_COLOR_ATTACHMENT0);
    if(useMultiSampleFbo)
    {
        multiSampleFbo->bind();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        myPaintGL();
        QOpenGLFramebufferObject::blitFramebuffer(blitedFbo, multiSampleFbo, GL_COLOR_BUFFER_BIT, GL_LINEAR);
    }
    else
    {
        blitedFbo->bind();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        myPaintGL();
    }

    glBindTexture(GL_TEXTURE_2D, blitedFbo->texture());
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    QOpenGLFramebufferObject::bindDefault();
    glViewport(0, 0, screenSize.width(), screenSize.height());
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    QMatrix4x4 mvp;
    QMatrix4x4 scale;
    mvp.scale(1, viewportRatio, 1);
    scale.scale(viewportScale);
    mvp = mvp * scale;

    standardShader.Bind();
    glUniformMatrix4fv(glGetUniformLocation(standardShader.program, "mvp"), 1, GL_FALSE, mvp.data());
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, blitedFbo->texture());
    glUniform1i(glGetUniformLocation(standardShader.program, "tex"), 0);
    standardShader.Draw(GL_TRIANGLES, quad);

    // this section should be placed in the end to prevent recursive paint caused by the update when QMessageBox closes
    if(saveFramebuffer)
    {
        saveFramebuffer = false;
        QImage image = blitedFbo->toImage();
        SaveScreenshot(image);
    }
}

QOpenGLFramebufferObject* MyRenderer::GetCurrentFbo()
{
    return useMultiSampleFbo ? multiSampleFbo : blitedFbo;
}

void MyRenderer::SaveScreenshot(QImage &image)
{
    QString path = QFileDialog::getSaveFileName(this, tr("Save Screenshot"), tr(""), tr("Portable Network Graphics (*.png)"));
    if(!path.isNull())
    {
        image.save(path);
        emit ScreenSaved(path); // connects to ShowSaveScreenMessage()
    }
}

void MyRenderer::ShowSaveScreenMessage(QString fp)
{
    ShowMessage(QString("Screenshot has been saved as ") + fp + ".");
}

void MyRenderer::myPaintGL()
{

}

void MyRenderer::onMeshChanged()
{

}

void MyRenderer::resizeGL(int w, int h)
{
    float aspect = (float)w / (float)h;
    viewportRatio = aspect / viewportAspect;
    screenSize = QSize(w, h);

    if(multiSampleFbo != NULL)
    {
        delete multiSampleFbo;
        delete blitedFbo;
    }

    QSize fboSize = viewportSize * devicePixelRatio();
    multiSampleFbo = new QOpenGLFramebufferObject(fboSize, multiSampleFboFormat);
    blitedFbo = new QOpenGLFramebufferObject(fboSize, blitedFboFormat);

    myResizeGL(RENDER_WIDTH, RENDER_HEIGHT);
}

void MyRenderer::myResizeGL(int w, int h)
{
    Q_UNUSED(w);
    Q_UNUSED(h);
}

QPoint MyRenderer::GetLocalPoint(QPoint p)
{
    float px = static_cast<float>(p.x()) / static_cast<float>(screenSize.width());
    float aspect = static_cast<float>(RENDER_WIDTH) / static_cast<float>(RENDER_HEIGHT);
    float h = screenSize.width() / aspect;
    float py = static_cast<float>(p.y() - (screenSize.height() / 2)) / h;
    py = std::min(0.5f, std::max(py, -0.5f));

    int x = RENDER_WIDTH * px;
    int y = RENDER_HEIGHT / 2 + RENDER_HEIGHT * py;

    x = std::min(RENDER_WIDTH - 1, std::max(0, x));
    y = std::min(RENDER_HEIGHT - 1, std::max(0, y));

    return QPoint(x, y);
}
