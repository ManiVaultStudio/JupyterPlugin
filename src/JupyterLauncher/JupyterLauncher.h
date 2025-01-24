#pragma once

#include <BackgroundTask.h>
#include <ViewPlugin.h>

#include <actions/HorizontalGroupAction.h>
#include <actions/StringAction.h>
#include <actions/PluginStatusBarAction.h>

#include <QProcess>
#include <QTimer>

#include <utility>
#include <vector>

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

    // TBD
    bool validatePythonEnvironment() {
        return true;
    }

    void loadJupyterPythonKernel(const QString& version);

public slots:
    void jupyterServerError(QProcess::ProcessError error);
    void jupyterServerFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void jupyterServerStateChanged(QProcess::ProcessState newState);
    void jupyterServerStarted();
    void shutdownJupyterServer();

private:
    // TBD merge the two runPythonScript signatures
    /** Run a python script from the resources return the exit code and stderr and stdout */
    int runPythonScript(const QString& scriptName, QString& sout, QString& serr, const QString& version, const QStringList& params = {}); 
    bool runPythonCommand(const QStringList& params, const QString& version);

    void setPythonEnv(const QString& version);
    bool installKernel(const QString& version);
    bool optionallyInstallMVWheel(const QString& version);

    bool startJupyterServerProcess(const QString& version);

    void logProcessOutput();

    // Python interpreter path
    QString getPythonInterpreterPath();

    // Distinguish between python in a regular or conda directory and python in a venv
    std::pair<bool, QString> getPythonHomePath(const QString& pyInterpreterPath);
    
private:
    QString                 _connectionFilePath;
    mv::BackgroundTask*     _serverBackgroundTask;      /** The background task monitoring the Jupyter Server */
    QProcess                _serverProcess;             /** A detached process for running the Jupyter server */
    QTimer*                 _serverPollTimer;           /** Poll the server process output at a regular interval */

};


// =============================================================================
// Factory
// =============================================================================

class JupyterLauncherFactory : public ViewPluginFactory
{
    Q_INTERFACES(mv::plugin::ViewPluginFactory mv::plugin::PluginFactory)
    Q_OBJECT
    Q_PLUGIN_METADATA(IID   "studio.manivault.JupyterLauncher"
                      FILE  "JupyterLauncher.json")

public:

    /** Default constructor */
    JupyterLauncherFactory();

    /** Destructor */
    ~JupyterLauncherFactory() = default;

    /** Perform post-construction initialization */
    void initialize() override;

    /** Get plugin icon */
    QIcon getIcon(const QColor& color = Qt::black) const override;
    
    /** Creates an instance of the example view plugin */
    ViewPlugin* produce() override;

    /** Returns the data types that are supported by the example view plugin */
    mv::DataTypes supportedDataTypes() const override;

private:
    PluginStatusBarAction*  _statusBarAction;               /** For global action in a status bar */
    HorizontalGroupAction   _statusBarPopupGroupAction;     /** Popup group action for status bar action */
    StringAction            _statusBarPopupAction;          /** Popup action for the status bar */
};