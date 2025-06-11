#pragma once

#include "UserDialogActions.h"

#include <BackgroundTask.h>
#include <ViewPlugin.h>

#include <actions/HorizontalGroupAction.h>
#include <actions/StringAction.h>
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

inline QStringList pythonInterpreterFilters()
{
    QStringList pythonFilter = {};
    if (QOperatingSystemVersion::currentType() == QOperatingSystemVersion::Windows)
        pythonFilter = { "Python interpreter (python*.exe)" };
    else
        pythonFilter = { "Python interpreter (python*)" };

    return pythonFilter;
}

std::pair<bool, QString> isCondaEnvironmentActive();

QString getPythonVersion(const QString& pythonInterpreterPath);

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

    // The pyversion should correspond to a python major.minor version
    // e.g. "3.11", "3.12"
    // There  must be a JupyterPlugin (a kernel provider) that matches the python version for this to work.
    void launchJupyterKernelAndNotebook(const QString& version);

    // The pyversion should correspond to a python major.minor version
    // e.g. "3.11", "3.12"
    void initPythonScripts(const QString& version);

public: // Global settings
    // Python interpreter path
    static QString getPythonInterpreterPath();

    static void setPythonInterpreterPath(const QString& p);

    static bool getShowInterpreterPathDialog();

public: // Call python
    // TBD merge the two runPythonScript signatures
    /** Run a python script from the resources return the exit code and stderr and stdout */
    static int runPythonScript(const QString& scriptName, QString& sout, QString& serr, const QStringList& params = {});
    static bool runPythonCommand(const QStringList& params, bool verbose = true);

    bool runScriptInKernel(const QString& scriptPath, QString interpreterVersion, const QStringList& params = {});

private:
    void jupyterServerError(QProcess::ProcessError error);
    void jupyterServerFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void jupyterServerStateChanged(QProcess::ProcessState newState);
    void jupyterServerStarted();
    void shutdownJupyterServer();

private:
    void setPythonEnv();
    bool installKernel();
    bool optionallyInstallMVWheel();
    bool initPython(bool activateXeus = true);

    bool checkPythonVersion() const;

    void startJupyterServerProcess();

    void logProcessOutput();

    // Distinguish between python in a regular or conda directory and python in a venv
    std::pair<bool, QString> getPythonHomePath(const QString& pyInterpreterPath);
    
    void createPythonPluginAndStartNotebook();
    void addPythonScripts();

private:
    QString                         _connectionFilePath;
    QString                         _selectedInterpreterVersion;
    QString                         _jupyterPluginFolder;

    using LoadedPythonInterpreters = std::unordered_map<QString, mv::plugin::Plugin*>;
    LoadedPythonInterpreters        _initializedPythonInterpreters;

    mv::BackgroundTask*             _serverBackgroundTask = nullptr;      /** The background task monitoring the Jupyter Server */
    QProcess                        _serverProcess;             /** A detached process for running the Jupyter server */
    QTimer*                         _serverPollTimer = nullptr;           /** Poll the server process output at a regular interval */
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

    /** Default constructor */
    JupyterLauncherFactory();

    /** Destructor */
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

private:
    PluginStatusBarAction*  _statusBarAction;               /** For global action in a status bar */
    HorizontalGroupAction   _statusBarPopupGroupAction;     /** Popup group action for status bar action */
    StringAction            _statusBarPopupAction;          /** Popup action for the status bar */
};