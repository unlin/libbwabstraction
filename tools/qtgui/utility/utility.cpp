#include <vector>
#include "utility/utility.h"

void CreateFolderIfNotExists(QString path)
{
    QDir dir(path);
    if(!dir.exists())
    {
        dir.mkpath(".");
    }
}

void ShowMessage(QString message)
{
    QMessageBox msgBox;
    msgBox.setText(message);
    msgBox.exec();
}
