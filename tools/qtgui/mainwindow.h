#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "myapplication.h"
#include "renderer/myrenderer.h"

// Forward declaration of class Ui::MainWindow.
namespace Ui
{
    class MainWindow;
}

class MainWindow : public QMainWindow
{

    Q_OBJECT

public:

    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    void NotifyAppReady();
    Ui::MainWindow *GetUI() { return ui; }

private slots:

    void on_actionSave_Screen_triggered();
    void on_actionImportMesh_triggered();
    void on_actionToggle_Ortho_Projection_triggered();
    void on_actionReset_Camera_triggered();
    void on_actionExit_triggered();

private:

    MyApplication *app;
    QList<MyRenderer *> renderers;
    Ui::MainWindow *ui;

    void UpdateAllRenderers();
    void NotifyMeshChanged(QString modelFilepath);

};

#endif // MAINWINDOW_H
