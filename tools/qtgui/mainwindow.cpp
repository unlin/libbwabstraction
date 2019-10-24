#include "mainwindow.h"
#include "ui_mainwindow.h"

using namespace std;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    app = MyApplication::Instance();
    ui->setupUi(this);
    renderers = this->findChildren<MyRenderer *>();
    ui->tabWidget->setCurrentWidget(ui->tab_basic);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::NotifyAppReady()
{
    for(auto renderer = renderers.begin(); renderer != renderers.end(); ++renderer)
    {
        (*renderer)->AppReady();
    }
}

void MainWindow::NotifyMeshChanged(QString modelFilepath)
{
    for(auto renderer = renderers.begin(); renderer != renderers.end(); ++renderer)
    {
        (*renderer)->MeshChanged(modelFilepath);
    }
}

void MainWindow::UpdateAllRenderers()
{
    for(auto renderer = renderers.begin(); renderer != renderers.end(); ++renderer)
    {
        (*renderer)->update();
        if(BWARenderer *sr = dynamic_cast<BWARenderer*>(*renderer))
        {
            sr->OptimizeOnce();
        }
    }
}

void MainWindow::on_actionSave_Screen_triggered()
{
    for(auto renderer = renderers.begin(); renderer != renderers.end(); ++renderer)
    {
        if((*renderer)->isVisible())
        {
            (*renderer)->SaveFramebuffer();
            break;
        }
    }
}

void MainWindow::on_actionImportMesh_triggered()
{
    QString modelFilepath = QFileDialog::getOpenFileName(this, tr("Open Model"), tr(""), tr("Wavefront OBJ (*.obj)"));
    if(!modelFilepath.isNull())
    {
        ui->tabWidget->setCurrentWidget(ui->tab_basic);
        NotifyMeshChanged(modelFilepath);
        UpdateAllRenderers();
    }
}

void MainWindow::on_actionToggle_Ortho_Projection_triggered()
{
    app->GetViewManager()->ToggleOrtho();
    UpdateAllRenderers();
}

void MainWindow::on_actionReset_Camera_triggered()
{
    app->GetViewManager()->Reset();
    UpdateAllRenderers();
}

void MainWindow::on_actionExit_triggered()
{
    close();
}
