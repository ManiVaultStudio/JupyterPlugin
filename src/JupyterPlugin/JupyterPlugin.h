#pragma once

#include <ViewPlugin.h>

#include <memory>
#include <unordered_set>

#include <QString>
#include <QStringList>

#undef slots
#include <pybind11/embed.h>
#include <pybind11/pybind11.h>
#define slots Q_SLOTS

class XeusKernel;
using PyScopedInterpreterPtr = std::unique_ptr<pybind11::scoped_interpreter>;
using PyModulePtr = std::unique_ptr<pybind11::module>;

/**
 * Jupyter plugin class
 *
 * This view plugin class hosts a Xeus kernel and python interpreter.
 * We open this plugin via the jupyter launcher plugin.
 *
 * @authors B. van Lew
 */
class JupyterPlugin : public mv::plugin::ViewPlugin
{
    Q_OBJECT

public:

    /**
     * Constructor
     * @param factory Pointer to the plugin factory
     */
    JupyterPlugin(const mv::plugin::PluginFactory* factory);
    ~JupyterPlugin();

    void init() override;

    Q_INVOKABLE void startJupyterNotebook() const;
    Q_INVOKABLE void runScriptWithArgs(const QString& scriptPath, const QStringList& args);

    Q_INVOKABLE void setConnectionFilePath(const QString& scriptPath) {
        _connectionFilePath = scriptPath;
    };

private:
    std::unique_ptr<XeusKernel>     _xeusKernel;
    QString                         _connectionFilePath = {};
    std::unordered_set<std::string> _baseModules = {};
    PyScopedInterpreterPtr          _mainPyInterpreter = {};

public:
    static PyModulePtr mvCommunicationModule;
    static void initMvCommunicationModule();
};


// =============================================================================
// Factory
// =============================================================================

class JupyterPluginFactory : public mv::plugin::ViewPluginFactory
{
    Q_INTERFACES(mv::plugin::ViewPluginFactory mv::plugin::PluginFactory)
    Q_OBJECT
    Q_PLUGIN_METADATA(IID   "studio.manivault.JupyterPlugin"
                      FILE  "PluginInfo.json")

public:
    /** Creates an instance of the example view plugin */
    mv::plugin::ViewPlugin* produce() override;

    /** Returns the data types that are supported by the example view plugin */
    mv::DataTypes supportedDataTypes() const override;
};
