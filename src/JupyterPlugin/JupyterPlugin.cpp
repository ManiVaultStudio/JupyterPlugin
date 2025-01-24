#include "JupyterPlugin.h"

#include <CoreInterface.h>
#include <PointData/PointData.h>

#include <QDebug>
#include <QDir>
#include <QStandardPaths>

#include "XeusKernel.h"

Q_PLUGIN_METADATA(IID "studio.manivault.JupyterPlugin")

using namespace mv;

class JupyterPlugin::PrivateKernel {
public:
    PrivateKernel() 
    {
        _xeusKernel = std::make_unique<XeusKernel>();
    }

    void startKernel(const QString& connection_path, const QString& pluginVersion = "")
    {
        _xeusKernel->startKernel(connection_path, pluginVersion);
    }

public:
    std::unique_ptr<XeusKernel> _xeusKernel;  /** the xeus kernel that manages the jupyter comms and the python interpreter*/
};

JupyterPlugin::JupyterPlugin(const PluginFactory* factory) :
    ViewPlugin(factory),
    _pKernel(std::make_unique<PrivateKernel>()),
    _connectionFilePath(this, "Connection file", QDir(QStandardPaths::standardLocations(QStandardPaths::HomeLocation)[0]).filePath("connection.json"))
{
}

JupyterPlugin::~JupyterPlugin() = default;

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

ViewPlugin* JupyterPluginFactory::produce()
{
    return new JupyterPlugin(this);
}

mv::DataTypes JupyterPluginFactory::supportedDataTypes() const
{
    return { };
}
