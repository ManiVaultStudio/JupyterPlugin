#include "GlobalSettingsAction.h"

#include <QHBoxLayout>
#include <QStandardPaths>

using namespace mv;
using namespace mv::gui;

GlobalSettingsAction::GlobalSettingsAction(QObject* parent, const plugin::PluginFactory* pluginFactory) :
    PluginGlobalSettingsGroupAction(parent, pluginFactory),
    _defaultPythonPathAction(this, "Python Directory ()", "")
{
    _defaultPythonPathAction.setToolTip("The directory must contain the python 3.11 shared library for the Jupyter Plugin");

    QString pythonString = QStandardPaths::findExecutable("python");
    if (!pythonString.isEmpty()) {
        _defaultPythonPathAction.setDirectory(QFileInfo(pythonString).absoluteDir().path());
    }
    // The add action automatically assigns a settings prefix to _pointSizeAction so there is no need to do this manually
    addAction(&_defaultPythonPathAction);
}