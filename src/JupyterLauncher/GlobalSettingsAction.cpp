#include "GlobalSettingsAction.h"

#include <QHBoxLayout>
#include <QStandardPaths>

using namespace mv;
using namespace mv::gui;

GlobalSettingsAction::GlobalSettingsAction(QObject* parent, const plugin::PluginFactory* pluginFactory) :
    PluginGlobalSettingsGroupAction(parent, pluginFactory),
    _defaultPythonPathAction(this, "Default python interpreter", ""),
    _defaultConnectionPathAction(this, "Connection file path", "")
{
    _defaultPythonPathAction.setToolTip("A python(.exe) interpreter for Jupyter Plugin");
    _defaultConnectionPathAction.setToolTip("The file used to store and communicate the kernel connection parameters");

    QString pythonString = QStandardPaths::findExecutable("python");
    if (!pythonString.isEmpty()) {
        _defaultPythonPathAction.setFilePath(QFileInfo(pythonString).path());
    }

    QStringList connectionFilter = { "Connection file (connection.json)" };
#ifdef _WIN32
    QStringList pythonFilter = { "Python interpreter (python*.exe)" };
#else
    QStringList pythonFilter = { "Python interpreter (python*)" };
#endif
    _defaultPythonPathAction.setNameFilters(pythonFilter);
    _defaultConnectionPathAction.setNameFilters(connectionFilter);

    // The add action automatically assigns a settings prefix to _pointSizeAction so there is no need to do this manually
    addAction(&_defaultPythonPathAction);
    addAction(&_defaultConnectionPathAction);
}