#include "GlobalSettingsAction.h"

#include <QHBoxLayout>
#include <QStandardPaths>
#include <QOperatingSystemVersion>

using namespace mv;
using namespace mv::gui;

GlobalSettingsAction::GlobalSettingsAction(QObject* parent, const plugin::PluginFactory* pluginFactory) :
    PluginGlobalSettingsGroupAction(parent, pluginFactory),
    _defaultPythonPathAction(this, "Python interpreter", "")
{
    _defaultPythonPathAction.setToolTip("A python (.exe on Windows) interpreter for Jupyter Plugin");

    QStringList connectionFilter = { "Connection file (connection.json)" };

    QStringList pythonFilter = {};
    if (QOperatingSystemVersion::currentType() == QOperatingSystemVersion::Windows)
        pythonFilter = { "Python interpreter (python*.exe)" };
    else
        pythonFilter = { "Python interpreter (python*)" };

    _defaultPythonPathAction.setNameFilters(pythonFilter);

    // The add action automatically assigns a settings prefix to _pointSizeAction so there is no need to do this manually
    addAction(&_defaultPythonPathAction);
}
