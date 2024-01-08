#include "JupyterXeusKernel.h"
#include <QStandardPaths>

JupyterXeusKernel::JupyterXeusKernel()
{

}

bool JupyterXeusKernel::startJupyterLabServer(QString noteBookDirectory)
{
    QString pythonExecutable = QStandardPaths::findExecutable("python");
    QStringList args;
    args <<  "-m" << "jupyter" << "lab" << "--notebook-dir" << noteBookDirectory;
    jupyterLabServer.setArguments(args);
    jupyterLabServer.start();
    auto success = jupyterLabServer.waitForStarted();

    return success;
}
