#include "SettingsAction.h"
#include "JupyterPlugin.h"

#include <QHBoxLayout>
#include <QStandardPaths>
#include <QDir>

using namespace mv::gui;

SettingsAction::SettingsAction(QObject* parent, const QString& title) :
    GroupAction(parent, title),
    _jupyterPlugin(dynamic_cast<JupyterPlugin*>(parent)),
    _connectionFilePath(this, "Connection file path")
{
    setText("JupyterPlugin settings");

    addAction(&_connectionFilePath);

    _connectionFilePath.setToolTip("Path to the kernel connection file");

    _connectionFilePath.setEnabled(true);

    _connectionFilePath.setFilePath(QDir(QStandardPaths::standardLocations(QStandardPaths::HomeLocation)[0]).filePath("connection.json"));
 
}