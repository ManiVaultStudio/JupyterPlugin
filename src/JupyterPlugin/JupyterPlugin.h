#pragma once

#include <ViewPlugin.h>

#include <actions/FilePickerAction.h>

#include <memory>

#include <QStringList>

#undef slots
#include <pybind11/embed.h>
#define slots Q_SLOTS

using namespace mv::plugin;
using namespace mv::gui;

class XeusKernel;

/**
 * Jupyter plugin class
 *
 * This view plugin class hosts a Xeus kernel and python interpreter.
 * We open this plugin via the jupyter launcher plugin.
 *
 * @authors B. van Lew
 */
class JupyterPlugin : public ViewPlugin
{
    Q_OBJECT

public:

    /**
     * Constructor
     * @param factory Pointer to the plugin factory
     */
    JupyterPlugin(const PluginFactory* factory);
    ~JupyterPlugin();

    void init() override;

    Q_INVOKABLE void runScriptWithArgs(const QString& scriptPath, const QStringList& args);

private:
    std::unique_ptr<XeusKernel>     _pKernel;
    FilePickerAction                _connectionFilePath;        /** Settings action */

    std::unique_ptr<pybind11::scoped_interpreter>   _init_guard = {};

};


// =============================================================================
// Factory
// =============================================================================

class JupyterPluginFactory : public ViewPluginFactory
{
    Q_INTERFACES(mv::plugin::ViewPluginFactory mv::plugin::PluginFactory)
    Q_OBJECT
    Q_PLUGIN_METADATA(IID   "studio.manivault.JupyterPlugin"
                      FILE  "JupyterPlugin.json")

public:

    /** Default constructor */
    JupyterPluginFactory() = default;

    /** Destructor */
    ~JupyterPluginFactory() = default;
    
    /** Creates an instance of the example view plugin */
    ViewPlugin* produce() override;

    /** Returns the data types that are supported by the example view plugin */
    mv::DataTypes supportedDataTypes() const override;
};
