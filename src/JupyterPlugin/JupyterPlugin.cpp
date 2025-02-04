#include "JupyterPlugin.h"

#include <QDebug>
#include <QDir>
#include <QStandardPaths>

#include "XeusKernel.h"

#undef slots
#include <pybind11/embed.h>
#define slots Q_SLOTS

Q_PLUGIN_METADATA(IID "studio.manivault.JupyterPlugin")

using namespace mv;
namespace py = pybind11;

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

// https://pybind11.readthedocs.io/en/stable/reference.html#_CPPv422initialize_interpreterbiPPCKcb
void JupyterPlugin::init()
{  
    QString jupyter_configFilepath = _connectionFilePath.getFilePath();
    QString pluginVersion = QString::fromStdString(getVersion().getVersionString());

    _init_guard = std::make_unique<pybind11::scoped_interpreter>(); // start the interpreter and keep it alive

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
