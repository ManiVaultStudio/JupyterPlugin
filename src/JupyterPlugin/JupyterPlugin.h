#pragma once

#include <ViewPlugin.h>

#include <actions/FilePickerAction.h>
#include <Dataset.h>

#include <memory>

using namespace mv::plugin;
using namespace mv::gui;

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

    /** Destructor */
    ~JupyterPlugin();

    /** This function is called by the core after the view plugin has been created */
    void init() override;

    void setConnectionPath(const QString& connection_path);

private:
    // shield the implementation dependencies
    class PrivateKernel;
    std::unique_ptr<PrivateKernel>  _pKernel;

    FilePickerAction                _connectionFilePath;        /** Settings action */
};

/**
 * Jupyter plugin factory class
 */
class JupyterPluginFactory : public ViewPluginFactory
{
    Q_INTERFACES(mv::plugin::ViewPluginFactory mv::plugin::PluginFactory)
    Q_OBJECT
    Q_PLUGIN_METADATA(IID   "studio.manivault.JupyterPlugin"
                      FILE  "JupyterPlugin.json")

public:

    /** Default constructor */
    JupyterPluginFactory() {}

    /** Destructor */
    ~JupyterPluginFactory() override {}
    
    /** Creates an instance of the example view plugin */
    ViewPlugin* produce() override;

    /** Returns the data types that are supported by the example view plugin */
    mv::DataTypes supportedDataTypes() const override;

    /**
     * Get plugin trigger actions given \p datasets
     * @param datasets Vector of input datasets
     * @return Vector of plugin trigger actions
     */
    PluginTriggerActions getPluginTriggerActions(const mv::Datasets& datasets) const override;
};
