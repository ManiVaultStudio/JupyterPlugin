#include "JupyterPlugin.h"

#include <QDebug>
#include <QDir>
#include <QStandardPaths>

#include "XeusKernel.h"

Q_PLUGIN_METADATA(IID "studio.manivault.JupyterPlugin")

using namespace mv;

JupyterPlugin::JupyterPlugin(const PluginFactory* factory) :
    ViewPlugin(factory),
    _pKernel(std::make_unique<XeusKernel>()),
    _connectionFilePath(this, "Connection file", QDir(QStandardPaths::standardLocations(QStandardPaths::HomeLocation)[0]).filePath("connection.json"))
{
}

JupyterPlugin::~JupyterPlugin()
{
    _pKernel->stopKernel();
}

void JupyterPlugin::setConnectionPath(const QString& connection_path)
{
    _connectionFilePath.setFilePath(connection_path);
}

void JupyterPlugin::init()
{
    QString jupyter_configFilepath = _connectionFilePath.getFilePath();
    QString pluginVersion = QString::fromStdString(getVersion().getVersionString());

    qDebug() << "JupyterPlugin::init with " << jupyter_configFilepath;
    _pKernel->startKernel(jupyter_configFilepath, pluginVersion);
}

// =============================================================================
// Plugin Factory
// =============================================================================

ViewPlugin* JupyterPluginFactory::produce()
{
    return new JupyterPlugin(this);
}

mv::DataTypes JupyterPluginFactory::supportedDataTypes() const
{
    return { };
}
