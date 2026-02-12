#pragma once

#include "UserDialogActions.h"
#include "ScriptDialogAction.h"

#include <BackgroundTask.h>
#include <ViewPlugin.h>

#include <actions/HorizontalGroupAction.h>
#include <actions/StringAction.h>
#include <actions/TriggerAction.h>
#include <actions/PluginStatusBarAction.h>

#include <QOperatingSystemVersion>
#include <QProcess>
#include <QStringList>
#include <QTimer>

#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

using namespace mv::plugin;
using namespace mv::gui;

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

    // The pyversion should correspond to a python major.minor version
    // e.g. "3.11", "3.12"
    // There  must be a JupyterPlugin (a kernel provider) that matches the python version for this to work.
    void launchJupyterKernelAndNotebook(const QString& version);

    // The pyversion should correspond to a python major.minor version
    // e.g. "3.11", "3.12"
    void initPythonScripts(const QString& version);

    /**
     * Get script trigger actions given \p datasets
     * @param datasets Vector of input datasets
     * @return Vector of script trigger actions
     */
    mv::gui::ScriptTriggerActions getScriptTriggerActions(const mv::Datasets& datasets) const override;

public: // Global settings
    // Python interpreter path
    static QString getPythonInterpreterPath();

    static void setPythonInterpreterPath(const QString& p);

    static bool getShowInterpreterPathDialog();

public: // Call python
    // TBD merge the two runPythonScript signatures
    /** Run a python script from the resources return the exit code and stderr and stdout */
    static int runPythonScript(const QString& scriptName, QString& out, QString& err, const QStringList& params = {});
    static bool runPythonCommand(const QStringList& params, bool verbose = true);

    bool runScriptInKernel(const QString& scriptPath, const QString& interpreterVersion, const QStringList& params = {});

private:
    void jupyterServerError(const QProcess::ProcessError& error) const;
    void jupyterServerFinished(const int exitCode, const QProcess::ExitStatus& exitStatus) const;
    void jupyterServerStateChanged(const QProcess::ProcessState& newState) const;
    void jupyterServerStarted() const;
    void shutdownJupyterServer() const;

private:
    bool ensureMvWheelIsInstalled() const;
    bool initLauncher(const QString& version, const int mode);
    bool initPython(const bool activateXeus = true);

    bool checkPythonVersion();

    void startJupyterServerProcess();

    void logProcessOutput();

    void createPythonPluginAndStartNotebook();
    void addPythonScripts();

    void setLaunchTriggersEnabled(bool const enabled) const;

private:
    QString                         _connectionFilePath = {};
    QString                         _selectedInterpreterVersion = {};
    QString                         _currentInterpreterPatchVersion = {};
    QString                         _jupyterPluginFolder = {};

    using LoadedPythonInterpreters  = std::unordered_map<QString, mv::plugin::Plugin*>;
    LoadedPythonInterpreters        _initializedPythonInterpreters = {};

    using PythonScripts             = std::vector<std::shared_ptr<PythonScript>>;
    PythonScripts                   _scriptTriggerActions = {};

    mv::BackgroundTask*             _serverBackgroundTask = nullptr;    /** The background task monitoring the Jupyter Server */
    QProcess                        _serverProcess;                     /** A detached process for running the Jupyter server */
    QTimer*                         _serverPollTimer = nullptr;         /** Poll the server process output at a regular interval */
    std::unique_ptr<LauncherDialog> _launcherDialog;

};


// =============================================================================
// Factory
// =============================================================================

class JupyterLauncherFactory : public ViewPluginFactory
{
    Q_INTERFACES(mv::plugin::ViewPluginFactory mv::plugin::PluginFactory)
    Q_OBJECT
    Q_PLUGIN_METADATA(IID   "studio.manivault.JupyterLauncher"
                      FILE  "PluginInfo.json")

public:

    JupyterLauncherFactory();
    ~JupyterLauncherFactory() = default;

    /** Perform post-construction initialization */
    void initialize() override;

    /**
     * Get the read me markdown file URL
     * @return URL of the read me markdown file
     */
    QUrl getReadmeMarkdownUrl() const override;

    /**
     * Get the URL of the GitHub repository
     * @return URL of the GitHub repository
     */
    QUrl getRepositoryUrl() const override;

    /** Creates an instance of the example view plugin */
    ViewPlugin* produce() override;

    /** Returns the data types that are supported by the example view plugin */
    mv::DataTypes supportedDataTypes() const override;

public:
	std::vector<TriggerAction*> getLaunchTriggersEnabled() const { return _launchTriggerActions; };

private:
    PluginStatusBarAction*  _statusBarAction;               /** For global action in a status bar */
    HorizontalGroupAction   _statusBarPopupGroupAction;     /** Popup group action for status bar action */
    StringAction            _statusBarPopupAction;          /** Popup action for the status bar */

    using TriggerActions = std::vector<TriggerAction*>;
    TriggerActions          _launchTriggerActions = {};     /** Trigger actions in the popup menu */
};