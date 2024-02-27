#include "SettingsAction.h"
#include "GlobalSettingsAction.h"

#include "JupyterLauncher.h"

#include <QHBoxLayout>

using namespace mv::gui;

SettingsAction::SettingsAction(QObject* parent, const QString& title) :
    GroupAction(parent, title),
    _jupyterLauncher(dynamic_cast<JupyterLauncher*>(parent)),
    _pythonPath(this, "Python directory"),
    _connectionFilePath(this, "Connection file path")
{
    setText("Settings");

    addAction(&_pythonPath);
    addAction(&_connectionFilePath);

    _pythonPath.setToolTip("Directory containing the Python shared library ");
    _connectionFilePath.setToolTip("Path to the kernel connection file");

    _pythonPath.setEnabled(true);
    _connectionFilePath.setEnabled(true);

    _pythonPath.setDirectory(mv::settings().getPluginGlobalSettingsGroupAction<GlobalSettingsAction>(_jupyterLauncher)->getDefaultPythonPathAction().getDirectory());
    //_connectionFilePath.setFilePath(mv::settings().getPluginGlobalSettingsGroupAction<GlobalSettingsAction>(_jupyterLauncher)->getDefaultConnectionPathAction().getFilePath());
    _connectionFilePath.setString(mv::settings().getPluginGlobalSettingsGroupAction<GlobalSettingsAction>(_jupyterLauncher)->getDefaultConnectionPathAction().getString());
 
}