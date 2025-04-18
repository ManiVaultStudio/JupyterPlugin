#include "GlobalSettingsAction.h"

#include "JupyterLauncher.h"

#include <QHBoxLayout>
#include <QOperatingSystemVersion>
#include <QStandardPaths>

using namespace mv;
using namespace mv::gui;

GlobalSettingsAction::GlobalSettingsAction(QObject* parent, const plugin::PluginFactory* pluginFactory) :
    PluginGlobalSettingsGroupAction(parent, pluginFactory),
    _defaultPythonPathAction(this, "Python interpreter", ""),
    _doNotShowAgainButton(this, "Do not show interpreter path picker on start", false)
{
    _defaultPythonPathAction.setToolTip("A python (.exe on Windows) interpreter for Jupyter Plugin");
    _doNotShowAgainButton.setToolTip("Whether to show the interpreter path picker on start of the plugin");

    const auto pythonFilter = pythonInterpreterFilters();
    _defaultPythonPathAction.setNameFilters(pythonFilter);
    _defaultPythonPathAction.setUseNativeFileDialog(true);
    _defaultPythonPathAction.getFilePathAction().setText("Python interpreter");

    const auto [isConda, pyVersion] = isCondaEnvironmentActive();

    if(QOperatingSystemVersion::currentType() != QOperatingSystemVersion::Windows && isConda)
    {
        _defaultPythonPathAction.setDisabled(true);
        _defaultPythonPathAction.getFilePathAction().setDisabled(true);
        _defaultPythonPathAction.getPickAction().setDisabled(true);
    }

    addAction(&_defaultPythonPathAction);
    addAction(&_doNotShowAgainButton);
}
