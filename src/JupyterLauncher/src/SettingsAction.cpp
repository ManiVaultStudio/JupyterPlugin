#include "SettingsAction.h"
#include "GlobalSettingsAction.h"

#include "JupyterLauncher.h"

#include <QHBoxLayout>

using namespace mv::gui;

SettingsAction::SettingsAction(QObject* parent, const QString& title) :
    GroupAction(parent, title),
    _jupyterLauncher(dynamic_cast<JupyterLauncher*>(parent)),
    _python311Path(this, "Python interpreter"),
    _connection311FilePath(this, "Connection file path"),
    _python312Path(this, "Python interpreter"),
    _connection312FilePath(this, "Connection file path")
{
    setText("Settings");

    addAction(&_python311Path);
    addAction(&_connection311FilePath);

    _python311Path.setToolTip("Directory containing the Python shared library ");
    _connection311FilePath.setToolTip("Path to the kernel connection file");

    _python311Path.setEnabled(true);
    _connection311FilePath.setEnabled(true);

    QStringList connectionFilter = { "Connection file (connection.json)" };
#ifdef _WIN32
    QStringList pythonFilter = { "Python interpreter (python*.exe)"};
#else
    QStringList pythonFilter = { "Python interpreter (python*)" };
#endif

    _python311Path.setNameFilters(pythonFilter);
    _connection311FilePath.setNameFilters(connectionFilter);

    _python311Path.setFilePath(mv::settings().getPluginGlobalSettingsGroupAction<GlobalSettingsAction>(_jupyterLauncher)->getDefaultPythonPathAction().getFilePath());
    _connection311FilePath.setFilePath(mv::settings().getPluginGlobalSettingsGroupAction<GlobalSettingsAction>(_jupyterLauncher)->getDefaultConnectionPathAction().getFilePath());
    //_connectionFilePath.setString(mv::settings().getPluginGlobalSettingsGroupAction<GlobalSettingsAction>(_jupyterLauncher)->getDefaultConnectionPathAction().getString());
 
}

// pyversion either 311 or 312
void SettingsAction::enableVersion(const QString& pyversion)
{
    // T.B.D.
}