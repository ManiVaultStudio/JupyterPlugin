#include "JupyterPlugin.h"

#include <CoreInterface.h>
#include <PointData/PointData.h>

#include <QDebug>
#include <QProcess>
#include <QDir>
#include <QStandardPaths>

#include "XeusKernel.h"

Q_PLUGIN_METADATA(IID "nl.BioVault.JupyterPlugin")

using namespace mv;

class JupyterPlugin::PrivateKernel {
public:
    PrivateKernel() 
    {
        _xeusKernel = std::make_unique<XeusKernel>();
    }

    void startKernel(const QString& connection_path) 
    {
        _xeusKernel->startKernel(connection_path);
    }

public:
    std::unique_ptr<XeusKernel> _xeusKernel;  /** the xeus kernel that manages the jupyter comms and the python interpreter*/
};

JupyterPlugin::JupyterPlugin(const PluginFactory* factory) :
    ViewPlugin(factory),
    _connectionFilePath(this, "Connection file", QDir(QStandardPaths::standardLocations(QStandardPaths::HomeLocation)[0]).filePath("connection.json")),
    _pKernel(std::make_unique<PrivateKernel>())
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
    qDebug() << "JupyterPlugin::init with " << jupyter_configFilepath;
    _pKernel->startKernel(jupyter_configFilepath);
}

ViewPlugin* JupyterPluginFactory::produce()
{
    return new JupyterPlugin(this);
}

mv::DataTypes JupyterPluginFactory::supportedDataTypes() const
{
    return { PointType };
}

mv::gui::PluginTriggerActions JupyterPluginFactory::getPluginTriggerActions(const mv::Datasets& datasets) const
{
    PluginTriggerActions pluginTriggerActions;

    const auto getPluginInstance = [this]() -> JupyterPlugin* {
        return dynamic_cast<JupyterPlugin*>(mv::plugins().requestViewPlugin(getKind()));
    };

    const auto numberOfDatasets = datasets.count();

    if (numberOfDatasets >= 1 && PluginFactory::areAllDatasetsOfTheSameType(datasets, PointType)) {
        auto pluginTriggerAction = new PluginTriggerAction(const_cast<JupyterPluginFactory*>(this), this, "Jupyter Plugin", "Jupyter bridge", getIcon(), [this, getPluginInstance, datasets](PluginTriggerAction& pluginTriggerAction) -> void {
            for (auto dataset : datasets)
                getPluginInstance();
        });

        pluginTriggerActions << pluginTriggerAction;
    }

    return pluginTriggerActions;
}
