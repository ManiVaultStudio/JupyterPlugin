#include "JupyterLauncher.h"

#include "GlobalSettingsAction.h"
#include "ScriptDialogAction.h"

#include "FileHandlingUtils.h"
#include "JsonUtils.h"
#include "PythonUtils.h"

#include <Application.h>
#include <CoreInterface.h>
#include <Set.h>

#include <actions/StatusBarAction.h>
#include <actions/WidgetAction.h>
#include <util/LearningCenterTutorial.h>

#include <QByteArray>
#include <QDebug>
#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLibrary>
#include <QMainWindow>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QOperatingSystemVersion>
#include <QPluginLoader>
#include <QProcess>
#include <QStringList>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QThread>
#include <QUrl>

#include <algorithm>
#include <iostream>

using namespace mv;

Q_PLUGIN_METADATA(IID "studio.manivault.JupyterLauncher")

// =============================================================================
// JupyterLauncher
// =============================================================================

JupyterLauncher::JupyterLauncher(const PluginFactory* factory) :
    ViewPlugin(factory),
    _jupyterPluginFolder(QCoreApplication::applicationDirPath() + "/PluginDependencies/JupyterLauncher/bin/"),
    _serverProcess(this),
    _launcherDialog(std::make_unique<LauncherDialog>(nullptr, this))
{
    setObjectName("Jupyter kernel plugin launcher");

    const QDir temporaryApplicationDirectory = Application::current()->getTemporaryDir().path();
    const auto temporaryPluginDirectory = QDir(temporaryApplicationDirectory.absolutePath() + QDir::separator() + "JupyterLauncher");
    if (!temporaryPluginDirectory.exists())
        [[maybe_unused]] const bool createdDir = temporaryPluginDirectory.mkpath(".");

    _connectionFilePath = temporaryPluginDirectory.absolutePath() + QDir::separator() + "connection.json";

    if (auto connectionFile = QFile(_connectionFilePath);
        connectionFile.open(QIODevice::WriteOnly))
        connectionFile.close();
    else
        qWarning() << "JupyterLauncher: Could not create connection file at " << _connectionFilePath;

    connect(_launcherDialog.get(), &LauncherDialog::accepted, this, [this]() {
        if (_launcherDialog->getMode() == 0)
            createPythonPluginAndStartNotebook();
        else
            addPythonScripts();
        });

    connect(&_launcherDialog->getDoNotShowAgainButton(), &mv::gui::ToggleAction::toggled, this, [this](bool toggled) {
        mv::settings().getPluginGlobalSettingsGroupAction<GlobalSettingsAction>(this)->getDoNotShowAgainButton().setChecked(toggled);
        });

}

JupyterLauncher::~JupyterLauncher()
{
    // Just in case this hasn't happened yet 
    // now would be a good time.
    this->shutdownJupyterServer();
}

void JupyterLauncher::init()
{
}

bool JupyterLauncher::checkPythonVersion()
{
    const QString currentInterpreterVersion = getPythonVersion(getPythonInterpreterPath());
    const QString currentShortVersion = extractShortVersionNumber(currentInterpreterVersion);

    const bool match = (_selectedInterpreterVersion == currentShortVersion);

    if (match) {
        _currentInterpreterPatchVersion = extractPatchVersionNumber(currentInterpreterVersion);
    }
    else {
        qDebug() << "Version of Python interpreter: " << currentInterpreterVersion;
        qDebug() << "Version selected in launcher:  " << _selectedInterpreterVersion;
    }

    return match;
}

QString JupyterLauncher::getPythonInterpreterPath()
{
    return mv::settings().getPluginGlobalSettingsGroupAction<GlobalSettingsAction>("Jupyter Launcher")->getDefaultPythonPathAction().getFilePath();
}

void JupyterLauncher::setPythonInterpreterPath(const QString& p)
{
    mv::settings().getPluginGlobalSettingsGroupAction<GlobalSettingsAction>("Jupyter Launcher")->getDefaultPythonPathAction().setFilePath(p);
}

bool JupyterLauncher::getShowInterpreterPathDialog()
{
    return !mv::settings().getPluginGlobalSettingsGroupAction<GlobalSettingsAction>("Jupyter Launcher")->getDoNotShowAgainButton().isChecked();
}



bool JupyterLauncher::runScriptInKernel(const QString& scriptPath, const QStringList& params)
{
    if (!_initializedPythonInterpreters.contains(_selectedInterpreterVersion)) {
        qWarning() << "JupyterLauncher::runScriptInKernel: version not initialized: " << _selectedInterpreterVersion;
        return false;
    }

    mv::plugin::Plugin* interpreterPlugin = _initializedPythonInterpreters[_selectedInterpreterVersion];

    if (!interpreterPlugin) {
        qWarning() << "JupyterLauncher::runScriptInKernel: version not initialized: " << _selectedInterpreterVersion;
        return false;
    }

    const bool success = QMetaObject::invokeMethod(
        interpreterPlugin,
        "runScriptWithArgs",
        Q_ARG(QString, scriptPath),
        Q_ARG(QStringList, params)
    );

    if (!success) {
        qWarning() << "Failed to invoke runScriptWithArgs";
    }

    return true;
}

bool JupyterLauncher::ensureMvWheelIsInstalled() const
{
    const QString pluginVersion = QString::fromStdString(getVersion().getVersionString());
    const QMessageBox::StandardButton reply = QMessageBox::question(
        nullptr, 
        "Python modules missing", 
        "mvstudio.kernel " + pluginVersion + " and mvstudio.data " + pluginVersion + " are required for passing data between Python and ManiVault.\nDo you wish to install them now?",
        QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes
    );

    if (reply == QMessageBox::Yes) {
        const auto pythonInterpreter = getPythonInterpreterPath();

        // Bootstrap the pip installer - does nothing if pip is available
        // https://docs.python.org/3/library/ensurepip.html
        if (const auto res = runPythonCommand({ "-m", "ensurepip" }, pythonInterpreter, false);
			!res.result) {
          qWarning() << "Installing pip failed. See logging for more information";
          return false;
        }

        // Install the manivault wheels
        const QString mvWheelPath = QDir::toNativeSeparators(QCoreApplication::applicationDirPath() + "/PluginDependencies/JupyterLauncher/py/");
        const QString kernelWheel = mvWheelPath + "mvstudio_kernel-" + pluginVersion + "-py3-none-any.whl";
        const QString dataWheel = mvWheelPath + "mvstudio_data-" + pluginVersion + "-py3-none-any.whl";

        for (const QString& wheelpath : { kernelWheel , dataWheel }) {
          qDebug() << "wheel paths: " << wheelpath;

          if (const auto res = runPythonCommand({ "-m", "pip", "install", wheelpath, "--only-binary=:all:" }, pythonInterpreter, false);
              !res.result) {
            qWarning() << "Installing the given package failed. See logging for more information";
            return false;
          }

        }

        if (const auto res = runPythonCommand(QStringList({ "-m", "jupyter", "kernelspec", "install", createKernelDir(), "--user" }), pythonInterpreter, false);
			!res.result) {
            qWarning() << "Installing the ManiVaultStudio Jupyter kernel failed. See logging for more information";
            return false;
        }

    } else {
        return false;
    }
    return true;
}

void JupyterLauncher::shutdownJupyterServer() const
{
    switch (_serverProcess.state()) {
    case QProcess::Starting: [[fallthrough]];
    case QProcess::Running:
    {
        // Use the shutdown api with our fixed Authorization token to close the server
        auto request = QNetworkRequest(QUrl("http://127.0.0.1:8888/api/shutdown"));
        request.setRawHeader("Authorization", "token manivaultstudio_jupyterkerneluser");
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
 
        QNetworkAccessManager manager;
        const auto reply = QSharedPointer<QNetworkReply>(manager.post(request, QByteArray("")));
        
        std::cerr << "Waiting for Jupyter Server shutdown";
        int wait = 50; // usually done in 10 seconds
        while (!reply->isFinished() && wait > 0 ) {
            QThread::msleep(250);
            QCoreApplication::processEvents();
            std::cerr << ".";
            wait -= 1;
        }
        break;
    }
    case QProcess::NotRunning:
        break; // nothing to do
    }
}

void JupyterLauncher::jupyterServerError(const QProcess::ProcessError& error) const
{
    qWarning() << "Jupyter Server error: " << _serverProcess.errorString();
    switch (error) {
    case QProcess::FailedToStart:
        qWarning() << "Could not start.";
        break;
    case QProcess::Crashed:
        qWarning() << "Crashed";
        break;
    case QProcess::Timedout:
        qWarning() << "Timed out";
        break;
    case QProcess::WriteError:
        qWarning() << "Write error";
        break;
    case QProcess::ReadError:
        qWarning() << "Read error";
        break;
    default:
        qWarning() << "Reason unknown";
        break;
    }
    
    _serverBackgroundTask->setFinished();
}

void JupyterLauncher::jupyterServerFinished(const int exitCode, const QProcess::ExitStatus& exitStatus) const
{
    switch (exitStatus) {
    case QProcess::NormalExit :
        qDebug() << "Jupyter server exited normally with exitCode " << exitCode ;
        break;
    default:
        qWarning() << "Jupyter server exited abnormally with exitCode " << exitCode ;
        qWarning() << "With error: " << _serverProcess.errorString();
    }
    
    _serverBackgroundTask->setFinished();
}

void JupyterLauncher::jupyterServerStateChanged(const QProcess::ProcessState& newState) const
{
    switch (newState) {
    case QProcess::NotRunning:
        _serverBackgroundTask->setUndefined();
        break;
    case QProcess::Starting:
        _serverBackgroundTask->setRunning();
        _serverBackgroundTask->setProgress(0.1f);
        break;
    case QProcess::Running:
        _serverBackgroundTask->setRunning();
        _serverBackgroundTask->setProgress(1.0f);
        break;
    }
}

void JupyterLauncher::jupyterServerStarted() const
{
    _serverBackgroundTask->setProgress(1.0f);
}

void JupyterLauncher::startJupyterServerProcess()
{
    _serverBackgroundTask = new BackgroundTask(this, "JupyterLab Server");
    _serverBackgroundTask->setProgressMode(Task::ProgressMode::Manual);
    _serverBackgroundTask->setIdle();

    _serverProcess.setProcessChannelMode(QProcess::MergedChannels);
    _serverProcess.setProgram(getPythonInterpreterPath());
    _serverProcess.setArguments({ "-m", "jupyter", "lab", "--config", _connectionFilePath });

    connect(&_serverProcess, &QProcess::stateChanged, this, &JupyterLauncher::jupyterServerStateChanged);
    connect(&_serverProcess, &QProcess::errorOccurred, this, &JupyterLauncher::jupyterServerError);

    connect(&_serverProcess, &QProcess::started, this, &JupyterLauncher::jupyterServerStarted);
    connect(&_serverProcess, &QProcess::finished, this, &JupyterLauncher::jupyterServerFinished);

    connect(Application::current(), &QCoreApplication::aboutToQuit, this, &JupyterLauncher::shutdownJupyterServer);

    _serverProcess.start();
    _serverPollTimer = new QTimer(this);
    _serverPollTimer->setInterval(20); // poll every 20ms

    connect(_serverPollTimer, &QTimer::timeout, [this]() { 
        logProcessOutput(); 
        });

    _serverPollTimer->start();
}

void JupyterLauncher::logProcessOutput()
{
    if (!_serverProcess.canReadLine())
        return; 

    const auto output = _serverProcess.readAll();
    _serverBackgroundTask->setProgressDescription(output);

}

bool JupyterLauncher::initPython(const bool activateXeus)
{
    if (_initializedPythonInterpreters.contains(_selectedInterpreterVersion)) {
        qDebug() << "JupyterLauncher::initPython: already initialized: " << _selectedInterpreterVersion;
        return true;
    }

    // Check if the user set a python interpreter path
    if (getPythonInterpreterPath() == "") {
        [[maybe_unused]] QMessageBox::StandardButton reply = QMessageBox::information(
            nullptr,
            "No Python interpreter",
            "Please provide a path to a python interpreter in the plugin settings.\n Go to File -> Settings -> Plugin: Jupyter Launcher -> Python interpreter");
        return false;
    }

    if (!checkPythonVersion()) {
        qDebug() << "The given python interpreter does not match the selected python version";
        return false;
    }

    setPythonEnv(getPythonInterpreterPath(), _selectedInterpreterVersion, _connectionFilePath);

	// Check the path to see if the correct version of mvstudio is installed
    const QString pluginVersion = QString::fromStdString(getVersion().getVersionString());
    const auto pythonResEnv = runPythonScript(":/text/check_env.py", getPythonInterpreterPath(), { pluginVersion });

    if (pythonResEnv.result != QProcess::NormalExit) {
        qDebug() << "JupyterLauncher::createPythonPluginAndStartNotebook: Checking environment failed";
        qDebug() << pythonResEnv.out;
        qDebug() << pythonResEnv.err;

        // error like time out or other failed connection
        if (pythonResEnv.result >= 2)
            return false;

        // we might just miss the communication modules
        if (pythonResEnv.result == 1)
        {
            if (!ensureMvWheelIsInstalled())
            {
                qDebug() << "Could not install ManiVault JupyterPythonKernel modules";
                return false;
            }
        }
    }

    // Determine the path to the python library
    const auto pythonResLib = runPythonScript(":/text/find_libpython.py", getPythonInterpreterPath());
    if (pythonResLib.result != QProcess::NormalExit) {
        qDebug() << "JupyterLauncher::createPythonPluginAndStartNotebook: Finding python library failed";
        qDebug() << pythonResLib.out;
        qDebug() << pythonResLib.err;
        return false;
    }

    const auto pythonLibrary = QFileInfo(QDir::toNativeSeparators(pythonResLib.out));

    qDebug() << "Using python communication plugin library " << pythonLibrary.fileName() << " at: " << pythonLibrary.dir().absolutePath();
    if (!loadDynamicLibrary(pythonLibrary)) {
        qWarning() << "Failed to load/locate python communication plugin";
    }

    QString jupyterPluginPath = QCoreApplication::applicationDirPath() + "/PluginDependencies/JupyterLauncher/bin/JupyterPlugin" + _selectedInterpreterVersion.remove(".");

    // plugin lib version suffix
    const auto coreVersion  = mv::Application::current()->getVersion();
    const QString coreVersionStr  = QString("%1.%2.%3").arg(QString::number(coreVersion.getMajor()), QString::number(coreVersion.getMinor()), QString::number(coreVersion.getPatch()));

    const QString versionSuffix = QString("_p%1_c%2").arg(
        /*1: plugin version */ QString::fromStdString(getVersion().getVersionString()),
        /*1: core version   */ coreVersionStr
    );
    jupyterPluginPath.append(versionSuffix);

    const auto jupyterPluginLib     = QLibrary(jupyterPluginPath);
    auto jupyterPluginLoader        = QPluginLoader(jupyterPluginLib.fileName());

    qDebug() << "Using python plugin at: " << jupyterPluginLib.fileName();

    // Check if the plugin was loaded successfully
    if (!jupyterPluginLoader.load()) {
        qWarning() << "Failed to load plugin:" << jupyterPluginLoader.errorString();
        return false;
    }

    // If pluginFactory is a nullptr then loading of the plugin failed for some reason. Print the reason to output.
    auto jupyterPluginFactory = dynamic_cast<PluginFactory*>(jupyterPluginLoader.instance());
    if (!jupyterPluginFactory) {
        qWarning() << "Failed to load plugin: " << jupyterPluginPath << jupyterPluginLoader.errorString();
        return false;
    }

    // Create the jupyter plugin
    mv::plugin::Plugin* jupyterPluginInstance = jupyterPluginFactory->produce();

    // Communicate the connection file path via the child action in the JupyterPlugin
    const bool setConnectionFilePath = QMetaObject::invokeMethod(
        jupyterPluginInstance,
        "setConnectionFilePath",
        Q_ARG(QString, _connectionFilePath)
    );

    if (!setConnectionFilePath) {
        qWarning() << "Failed to invoke JupyterPlugin::setConnectionFilePath";
    }

    jupyterPluginInstance->init();

    if (activateXeus) {
        const bool startedJupyterNotebook = QMetaObject::invokeMethod(
            jupyterPluginInstance,
            "startJupyterNotebook"
        );

        if (!startedJupyterNotebook) {
            qWarning() << "Failed to invoke JupyterPlugin::startJupyterNotebook";
        }
    }

    _initializedPythonInterpreters.insert({ _selectedInterpreterVersion, jupyterPluginInstance });

    return true;
}

void JupyterLauncher::setLaunchTriggersEnabled(const bool enabled) const
{
  auto launchTriggerActions = dynamic_cast<const JupyterLauncherFactory*>(getFactory())->getLaunchTriggersEnabled();

  for (TriggerAction* triggerAction : launchTriggerActions)
    triggerAction->setEnabled(enabled);
}

bool JupyterLauncher::initLauncher(const QString& version, const int mode)
{
    _selectedInterpreterVersion = version;

    if (getShowInterpreterPathDialog()) {
        _launcherDialog->setMode(mode);
        _launcherDialog->show();                // by default ask user for python path
        return false;
    }

    return true;
}

void JupyterLauncher::launchJupyterKernelAndNotebook(const QString& version)
{
    if (!initLauncher(version, /*mode*/ 0))
        return;

    createPythonPluginAndStartNotebook();   // open notebook immediately if user has set do-not-show-dialog option
}

void JupyterLauncher::createPythonPluginAndStartNotebook()
{
    if (!initPython(/* activateXeus */ true))
        return;

    startJupyterServerProcess();

    setLaunchTriggersEnabled(false);
}

void JupyterLauncher::initPythonScripts(const QString& version)
{
    if (!initLauncher(version, /*mode*/ 1))
        return;

    addPythonScripts();
}

void JupyterLauncher::addPythonScripts()
{
    if (!initPython(/* activateXeus */ false))
        return;

    // Look for all available scripts
    const auto jupyterPluginDir = QDir(QCoreApplication::applicationDirPath() + "/examples/JupyterPlugin/scripts");

    qDebug() << "JupyterLauncher: Loading scripts from " << jupyterPluginDir;

    std::vector<QString> scriptFilePaths;
    for (const QFileInfo& fileInfo : jupyterPluginDir.entryInfoList(QStringList{ "*.json" }, QDir::Files | QDir::NoSymLinks)) {
        scriptFilePaths.push_back(fileInfo.absoluteFilePath());
    }

    // Add UI entries for the scripts
    auto checkRequirements = [](const QString& requirementsFilePath) -> bool {

        if (const QFileInfo requirementsFileInfo(requirementsFilePath);
            !requirementsFileInfo.exists() || !requirementsFileInfo.isFile()) {
            qDebug() << "Requirements file is listed but not found: " << requirementsFileInfo;
            return false;
        }

        const QStringList params = { "-m", "pip", "install", "-r", requirementsFilePath, "--dry-run" };

#ifdef NDEBUG
        constexpr bool verbose = false;
#else
        constexpr bool verbose = true;
#endif

        const auto res = runPythonCommand(params, getPythonInterpreterPath(), verbose);

        return res.result == 0;
        };

    uint32_t numLoadedScripts = 0;
    _scriptTriggerActions.clear();

    for (const auto& scriptFilePath : scriptFilePaths) {
        auto scriptFile = QFile(scriptFilePath);
        const auto scriptFileInfo = QFileInfo(scriptFile);
        const auto scriptFileDir = QDir(scriptFileInfo.absolutePath());

        if (!scriptFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qWarning() << "Couldn't open file:" << scriptFilePath;
            continue;
        }

        const QByteArray jsonData = scriptFile.readAll();
        scriptFile.close();

        QJsonParseError parseError;
        QJsonDocument document = QJsonDocument::fromJson(jsonData, &parseError);
        if (parseError.error != QJsonParseError::NoError) {
            qWarning() << "JSON parse error:" << parseError.errorString();
            continue;
        }

        if (!document.isObject()) {
            qWarning() << "JSON is not an object.";
            continue;
        }

        const QJsonObject json = document.object();

        if (const std::vector<QString> requiredEntries = { "script", "name", "type" };
            !std::all_of(requiredEntries.begin(), requiredEntries.end(), [&json](const QString& entry) { return containsMemberString(json, entry); })) {
            qWarning() << "Does not contain all required entries: " << scriptFilePath;
            qWarning().noquote() << document.toJson(QJsonDocument::Indented);
            continue;
        }

        // check requirements
        if (containsMemberString(json, "requirements")) {
            QString requirementsFilePath = scriptFileDir.filePath(json["requirements"].toString());

            qDebug() << "Checking and installing requirements for: " << requirementsFilePath;
            if (!checkRequirements(requirementsFilePath)) {
                qDebug() << "Requirements are not installed for Python script: " << scriptFilePath;
                continue;
            }

        }

        const QString scriptPath = scriptFileDir.filePath(json["script"].toString());
        const QString scriptName = json["name"].toString();
        const QString scriptType = json["type"].toString();

        const QString fullPyVersion = QString("%1.%2").arg(_selectedInterpreterVersion, _currentInterpreterPatchVersion);

        auto& scriptTriggerAction = _scriptTriggerActions.emplace_back(std::make_shared<PythonScript>(scriptName, mv::util::Script::getTypeEnum(scriptType), scriptPath, fullPyVersion, json, this, nullptr));

        // check if script contains input-datatypes, convert to mv::DataTypes
        if (containsMemberArray(json, "input-datatypes")) {
            const std::vector<QString> dataTypeStrings = readStringArray(json, "input-datatypes");
            mv::DataTypes dataTypes = {};

            for (const auto& dataTypeString : dataTypeStrings) {
                dataTypes.push_back(mv::DataType(dataTypeString));
            }

            scriptTriggerAction->setDataTypes(dataTypes);
        }

        numLoadedScripts++;
    }
    
    qDebug().noquote() << QString("JupyterLauncher: Loaded %1 scripts").arg(numLoadedScripts);

    setLaunchTriggersEnabled(false);
}

mv::gui::ScriptTriggerActions JupyterLauncher::getScriptTriggerActions(const mv::Datasets& datasets) const {

    // Currently, do not show scripts for multi-dataset selection
    if (datasets.count() > 1)
        return {};

    ScriptTriggerActions scriptTriggerActions;
    
    for (auto& scriptTriggerAction : _scriptTriggerActions) {
        const auto menuLocation = QString("%1/%2").arg(scriptTriggerAction->getTypeName(), scriptTriggerAction->getTitle());

        bool addScript = false;

        // Add loader only for datasets == {}
        if (datasets.count() == 0 && scriptTriggerAction->getType() == mv::util::Script::Type::Loader) {
            addScript = true;
        }
        
        if (datasets.count() > 0) {

            const auto inputDataType    = datasets.first()->getDataType();
            const auto scriptDataTypes  = scriptTriggerAction->getDataTypes();
    
            // Add script only if datasets are of same type as input-datatypes
            if (!scriptDataTypes.isEmpty() && scriptDataTypes.contains(inputDataType)) {
                addScript = true;
            }

        }

        if (addScript) {
            scriptTriggerActions << new ScriptTriggerAction(nullptr, scriptTriggerAction, menuLocation);
        }
    }

    return scriptTriggerActions;
}


/// ////////////////////// ///
/// JupyterLauncherFactory ///
/// ////////////////////// ///

JupyterLauncherFactory::JupyterLauncherFactory() :
    ViewPluginFactory(),
    _statusBarAction(nullptr),
    _statusBarPopupGroupAction(this, "Popup Group"),
    _statusBarPopupAction(this, "Popup")
{
    setIcon(QIcon(":/images/logo.svg"));
    setMaximumNumberOfInstances(1);

    for (const auto& tutorialFile : listTutorialFiles("tutorials/JupyterLauncher")) {
      if (insertMarkdownIntoJson(tutorialFile)) {

        if (auto tutorialJson = readJson(tutorialFile)) {
          mv::help().addTutorial(new util::LearningCenterTutorial(tutorialJson.value()["tutorials"].toArray().first().toObject().toVariantMap()));
        }

      }
    }
}

ViewPlugin* JupyterLauncherFactory::produce()
{
    return new JupyterLauncher(this);
}

void JupyterLauncherFactory::initialize()
{
    qDebug() << "JupyterLauncherFactory::initialize";

	ViewPluginFactory::initialize();

    // Create an instance of our GlobalSettingsAction (derived from PluginGlobalSettingsGroupAction) and assign it to the factory
    setGlobalSettingsGroupAction(new GlobalSettingsAction(this, this));

    // Configure the status bar popup action
    _statusBarPopupAction.setDefaultWidgetFlags(StringAction::Label);
    _statusBarPopupAction.setString("<b>Jupyter Launcher</b>");

    _statusBarPopupGroupAction.setShowLabels(false);
    _statusBarPopupGroupAction.setConfigurationFlag(WidgetAction::ConfigurationFlag::NoGroupBoxInPopupLayout);
    _statusBarPopupGroupAction.addAction(&_statusBarPopupAction);

    _statusBarAction = new PluginStatusBarAction(this, "Jupyter Launcher", getKind());

    // Sets the action that is shown when the status bar is clicked
    _statusBarAction->setPopupAction(&_statusBarPopupGroupAction);

    // Always show
    _statusBarAction->getConditionallyVisibleAction().setChecked(false);

    // Position to the right of the status bar action
    _statusBarAction->setIndex(-1);

    const QString jupyterPluginFolder   = QCoreApplication::applicationDirPath() + "/PluginDependencies/JupyterLauncher/bin/";
    QStringList pythonPlugins           = findLibraryFiles(jupyterPluginFolder);

	qDebug() << "jupyterPluginFolder:" << jupyterPluginFolder;
    qDebug() << "pythonPlugins:" << pythonPlugins;

    // On Linux/Mac in a conda environment we cannot switch between environments
    if constexpr (QOperatingSystemVersion::currentType() != QOperatingSystemVersion::Windows)
    {
        if (const auto [isConda, pyVersion] = isCondaEnvironmentActive(); isConda) {

            const QString pyVersionShort = extractShortVersionNumber(pyVersion).remove(".");

            QStringList filteredPythonPlugins;
            for (const QString& path : pythonPlugins) {
                const QFileInfo fileInfo(path);
                const QString fileName = fileInfo.fileName().split("_p")[0]; // Extract file name without plugin and core version

                qDebug() << "fileName:" << fileName;
                qDebug() << "pyVersion:" << pyVersionShort;

                if (fileName.contains(pyVersionShort, Qt::CaseInsensitive))
                    filteredPythonPlugins.append(path);

            }

            std::swap(pythonPlugins, filteredPythonPlugins);
        }
    }

    qDebug() << "pythonPlugins:" << pythonPlugins;

    auto getJupyterLauncherPlugin = [this]() -> JupyterLauncher* {
        const std::vector<plugin::Plugin*> openJupyterPlugins = mv::plugins().getPluginsByFactory(this);

        JupyterLauncher* plugin = nullptr;
        if (openJupyterPlugins.empty())
            plugin = mv::plugins().requestPlugin<JupyterLauncher>("Jupyter Launcher");
        else
            plugin = dynamic_cast<JupyterLauncher*>(openJupyterPlugins.front());

        return plugin;
        };

    for(const auto& pythonPlugin: pythonPlugins)
    {
        const QString pythonVersionOfPlugin = extractRegex(QFileInfo(pythonPlugin).fileName(), R"(\d+)", 0).insert(1, ".");

        qDebug() << "pythonVersionOfPlugin:" << pythonVersionOfPlugin;

        auto launchJupyterPython = new TriggerAction(this, "Start Jupyter Kernel and Lab (" + pythonVersionOfPlugin + ")");
        auto initPythonScripts   = new TriggerAction(this, "Init Python Scripts (" + pythonVersionOfPlugin + ") [BETA]");

        _launchTriggerActions.push_back(launchJupyterPython);
        _launchTriggerActions.push_back(initPythonScripts);

        // Jupyter Notebooks
        connect(launchJupyterPython, &TriggerAction::triggered, this, [this, pythonVersionOfPlugin, getJupyterLauncherPlugin]() {

            if (JupyterLauncher* plugin = getJupyterLauncherPlugin()) {
                plugin->launchJupyterKernelAndNotebook(pythonVersionOfPlugin);
            }
        });

        _statusBarAction->addMenuAction(launchJupyterPython);

        // Python Scripts
        connect(initPythonScripts, &TriggerAction::triggered, this, [this, pythonVersionOfPlugin, getJupyterLauncherPlugin]() {

            if (JupyterLauncher* plugin = getJupyterLauncherPlugin()) {
                plugin->initPythonScripts(pythonVersionOfPlugin);
            }
            });

        _statusBarAction->addMenuAction(initPythonScripts);

    }

    // Assign the status bar action so that it will appear on the main window status bar
    setStatusBarAction(_statusBarAction);
}

mv::DataTypes JupyterLauncherFactory::supportedDataTypes() const
{
    return { };
}

QUrl JupyterLauncherFactory::getReadmeMarkdownUrl() const 
{
    return { "https://raw.githubusercontent.com/ManiVaultStudio/JupyterPlugin/main/README.md" };
}

QUrl JupyterLauncherFactory::getRepositoryUrl() const
{
    return{ "https://github.com/ManiVaultStudio/JupyterPlugin" };
}
