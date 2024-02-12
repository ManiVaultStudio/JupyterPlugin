#include "SettingsAction.h"
#include "GlobalSettingsAction.h"

#include "JupyterLauncher.h"

#include <QHBoxLayout>

using namespace mv::gui;

SettingsAction::SettingsAction(QObject* parent, const QString& title) :
    GroupAction(parent, title),
    _jupyterLauncher(dynamic_cast<JupyterLauncher*>(parent)),
    _pythonPath(this, "Python directory")
{
    setText("Settings");

    addAction(&_pythonPath);

    _pythonPath.setToolTip("Directory containing the Python shared library ");

    _pythonPath.setEnabled(true);


    _pythonPath.setDirectory(mv::settings().getPluginGlobalSettingsGroupAction<GlobalSettingsAction>(_jupyterLauncher)->getDefaultPythonPathAction().getDirectory());
 

}