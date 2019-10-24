#ifndef MYAPPLICATION_H
#define MYAPPLICATION_H

class MainWindow;
class ViewManager;

class MyApplication : public QApplication
{

    Q_OBJECT

public:

    MyApplication(int argc, char *argv[]);
    static MyApplication *Instance() { return instance; }
    MainWindow *GetMainWindow() { return mainWindow; }
    ViewManager *GetViewManager() { return viewManager; }

private:

    void InitGLEW();
    void SetDefaultSurfaceFormat();

    static MyApplication *instance;
    MainWindow *mainWindow = NULL;
    ViewManager *viewManager = NULL;

};

#endif // MYAPPLICATION_H
