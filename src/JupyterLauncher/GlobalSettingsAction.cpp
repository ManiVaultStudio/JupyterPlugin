#include "GlobalSettingsAction.h"

#include "JupyterLauncher.h"

#include <QHBoxLayout>
#include <QStandardPaths>

using namespace mv;
using namespace mv::gui;

GlobalSettingsAction::GlobalSettingsAction(QObject* parent, const plugin::PluginFactory* pluginFactory) :
    PluginGlobalSettingsGroupAction(parent, pluginFactory),
    _defaultPythonPathAction(this, "Python interpreter", "")
{
    _defaultPythonPathAction.setToolTip("A python (.exe on Windows) interpreter for Jupyter Plugin");

    const auto pythonFilter = pythonInterpreterFilters();
    _defaultPythonPathAction.setNameFilters(pythonFilter);

    // The add action automatically assigns a settings prefix to _pointSizeAction so there is no need to do this manually
    addAction(&_defaultPythonPathAction);
}
