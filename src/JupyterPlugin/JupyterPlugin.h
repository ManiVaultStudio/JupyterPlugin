#pragma once

#include <ViewPlugin.h>

#include <actions/FilePickerAction.h>

#include <memory>

#include <QStringList>
#include <QThread>

#undef slots
#include <pybind11/embed.h>
#include <pybind11/pybind11.h>
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

    static std::unique_ptr<pybind11::module> mv_communication_module;
    static void init_mv_communication_module();

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

class PythonWorker : public QThread {
    Q_OBJECT
public:
    explicit PythonWorker(QObject* parent, const QString& scriptPath, const QStringList& args);
protected:
    void run() override;
signals:
    void resultReady(const QString& s);
    void errorMessage(const QString& s);
private:
    QString     _scriptPath;
    QStringList _args;
};

