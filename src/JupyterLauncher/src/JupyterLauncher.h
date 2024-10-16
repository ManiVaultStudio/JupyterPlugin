#pragma once

#include <BackgroundTask.h>
#include <Dataset.h>
#include <PluginFactory.h>
#include <ViewPlugin.h>

#include <actions/HorizontalGroupAction.h>
#include <actions/StringAction.h>
#include <actions/PluginStatusBarAction.h>

#include <PointData/PointData.h>

#include <QWidget>
#include <QProcess>

using namespace mv::plugin;
using namespace mv::gui;

class QLabel;

/**
 * Transitive JupyterPlugin Loader
 *
 * The user needs to choose a python directory that will be added
 * to the path before the JupyterPlugin is loader.
 * This plugin, JupyterLauncher fulfills that function.
 * The user selected pat is saved in the settings.
 *
 * @authors B. van Lew
 */
class JupyterLauncher : public ViewPlugin
{
    Q_OBJECT

public:

    /**
     * Constructor
     * @param factory Pointer to the plugin factory
     */
    JupyterLauncher(const PluginFactory* factory);

    /** Destructor */
    ~JupyterLauncher();
    
    /** This function is called by the core after the view plugin has been created */
    void init() override;

     bool validatePythonEnvironment();

    void loadJupyterPythonKernel(const QString version);

protected:
    mv::Dataset<Points>     _points;                    /** Points smart pointer */
    QString                 _currentDatasetName;        /** Name of the current dataset */
    QLabel*                 _currentDatasetNameLabel;   /** Label that show the current dataset name */
    mv::BackgroundTask*     _serverBackgroundTask;      /** The background task monitoring the Jupyter Server */
    QProcess                _serverProcess;             /** A detached process for running the Jupyter server */
    QTimer*                 _serverPollTimer;           /** Poll the server process output at a regular interval */

public slots:
    void jupyterServerError(QProcess::ProcessError error);
    void jupyterServerFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void jupyterServerStateChanged(QProcess::ProcessState newState);
    void jupyterServerStarted();
    void shutdownJupyterServer();

private:
    QHash<QString, PluginFactory*>                      _pluginFactories;   /** All loaded plugin factories */
    std::vector<std::unique_ptr<mv::plugin::Plugin>>    _plugins;           /** Vector of plugin instances */
    // TBD merge the two runPythonScript signatures
    /** Run a python script from the resources return the exit code and stderr and stdout */
    int runPythonScript(const QString scriptName, QString& sout, QString& serr, const QString version, const QStringList params = {}); 
    bool runPythonCommand(const QStringList params, const QString version);

    void setPythonEnv(const QString version);
    bool installKernel(const QString version);
    bool optionallyInstallMVWheel(const QString version);

    bool startJupyterServerProcess(const QString version);

    void logProcessOutput();

    const QString getVirtDir(const QString);
    QString getPythonExePath();
    QString getPythonConfigPath();
};

/**
 * Jupyter Launcher view plugin factory class
 */
class JupyterLauncherFactory : public ViewPluginFactory
{
    Q_INTERFACES(mv::plugin::ViewPluginFactory mv::plugin::PluginFactory)
    Q_OBJECT
    Q_PLUGIN_METADATA(IID   "nl.BioVault.JupyterLauncher"
                      FILE  "JupyterLauncher.json")

public:

    /** Default constructor */
    JupyterLauncherFactory();

    /** Destructor */
    ~JupyterLauncherFactory() override {}

    /** Perform post-construction initialization */
    void initialize() override;

    /** Get plugin icon */
    QIcon getIcon(const QColor& color = Qt::black) const override;
    
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

private:
    PluginStatusBarAction*  _statusBarAction;               /** For global action in a status bar */
    HorizontalGroupAction   _statusBarPopupGroupAction;     /** Popup group action for status bar action */
    StringAction            _statusBarPopupAction;          /** Popup action for the status bar */
};