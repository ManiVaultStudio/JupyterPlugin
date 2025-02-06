#include "JupyterLauncher.h"

#include "GlobalSettingsAction.h"

#include <Application.h>
#include <CoreInterface.h>

#include <actions/WidgetAction.h>

#include <QByteArray>
#include <QDebug>
#include <QLibrary>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QOperatingSystemVersion>
#include <QPluginLoader>
#include <QProcess>
#include <QProcessEnvironment>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QThread>
#include <QUrl>

#include <exception>
#include <iostream>
#include <stdexcept>

#ifdef _WIN32
#include <windows.h>
#endif

using namespace mv;

Q_PLUGIN_METADATA(IID "studio.manivault.JupyterLauncher")

static inline bool loadDynamicLibray(const QFileInfo& pythonLibrary)
{
#ifdef WIN32
    // Adds a directory to the search path used to locate DLLs for the application.
    return SetDllDirectoryA(QString(pythonLibrary.dir().absolutePath() + "/").toUtf8().data());
#else
    // This approach seems cleaner but does not work on Windows
    QString sharedLibFilePath   = pythonLibrary.absoluteFilePath();

    qDebug() << "Using python shared library at: " << sharedLibFilePath;
    if (!QLibrary::isLibrary(sharedLibFilePath))
        qWarning() << "Not a library: " << sharedLibFilePath;

    QLibrary lib(sharedLibFilePath);
    return lib.load();
#endif
}

static QStringList findLibraryFiles(const QString &folderPath) {
    QStringList libraryFiles;
    QDirIterator it(folderPath, QDir::Files);

    while (it.hasNext()) {
        QString filePath = it.next();
        qDebug() << "current: " << filePath;
        if (QFileInfo(filePath).fileName().contains("JupyterPlugin3") && QLibrary::isLibrary(filePath)) {
            libraryFiles.append(filePath);
        }
    }

    return libraryFiles;
}

static QString extractSingleNumber(const QString &input) {
    QRegularExpression regex(R"(\d+)");
    QRegularExpressionMatch match = regex.match(input);

    if (match.hasMatch()) {
        return match.captured(0); // Return the matched number
    }

    return QString();
}

static QString extractVersionNumber(const QString &input) {
    QRegularExpression regex(R"(Python\s+(\d+\.\d+))");
    QRegularExpressionMatch match = regex.match(input);

    if (match.hasMatch()) {
        return match.captured(1); // Return the matched number
    }

    return QString();
}

static QString getPythonVersion(const QString& pythonInterpreterPath)
{
    QProcess process;
    process.setProgram(pythonInterpreterPath);
    process.setArguments({"--version"});
    process.start();
    
    if (!process.waitForFinished(3000)) {  // Timeout after 3 seconds
        qWarning() << "Failed to get Python version";
        return "";
    }

    QString output = process.readAllStandardOutput().trimmed();
    if (output.isEmpty()) {
        output = process.readAllStandardError().trimmed();  // Some Python versions print to stderr
    }

    QString givenInterpreterVersion = extractVersionNumber(output);

    return givenInterpreterVersion;
}

// =============================================================================
// JupyterLauncher
// =============================================================================

JupyterLauncher::JupyterLauncher(const PluginFactory* factory) :
    ViewPlugin(factory),
    _connectionFilePath(QDir::toNativeSeparators(QCoreApplication::applicationDirPath() + "/PluginDependencies/JupyterLauncher/py/connection.json")),
    _selectedInterpreterVersion(""),
    _jupyterPluginFolder(QCoreApplication::applicationDirPath() + "/PluginDependencies/JupyterLauncher/bin/"),
    _serverProcess(this),
    _launcherDialog(std::make_unique<LauncherDialog>(nullptr, this))
{
    setObjectName("Jupyter kernel plugin launcher");

    QFile jsonFile(_connectionFilePath);
    if (jsonFile.open(QIODevice::WriteOnly))
        jsonFile.close();
    else 
        qWarning() << "JupyterLauncher: Could not create connection file at " << _connectionFilePath;

    connect(_launcherDialog.get(), &LauncherDialog::accepted, this, &JupyterLauncher::launchJupyterKernelAndNotebookImpl);
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

std::pair<bool, QString> JupyterLauncher::getPythonHomePath(const QString& pyInterpreterPath)
{
    // check if the last 
    auto pyInterpreterInfo      = QFileInfo(pyInterpreterPath);
    QDir pyInterpreterParent    = pyInterpreterInfo.dir();
    QString pyHomePath          = pyInterpreterParent.absolutePath();
    bool isVenv                 = false;

    //In a venv python sits in a Script or bin dir dependent on os
    QString venvParent = "";
    if (QOperatingSystemVersion::currentType() == QOperatingSystemVersion::Windows)
        venvParent = "Scripts";
    else
        venvParent = "bin";

    // A Script/bin and a pyvenv.cfg in the dir above indicate a venv so take the parent
    if (pyInterpreterParent.dirName() == venvParent)
    {
        pyInterpreterParent.cdUp();
        if (pyInterpreterParent.exists("pyvenv.cfg"))
        {
            pyHomePath = pyInterpreterParent.absolutePath();
            isVenv = true;
        }
    }

    if (pyHomePath.endsWith("bin"))
        pyHomePath = pyHomePath.chopped(3); // Removes the last 3 characters ("bin")

    return { isVenv, QDir::toNativeSeparators(pyHomePath) };
}

bool JupyterLauncher::checkPythonVersion()
{
    QString givenInterpreterVersion = getPythonVersion(getPythonInterpreterPath());

    bool match = _selectedInterpreterVersion == givenInterpreterVersion;

    if(!match)
    {
        qDebug() << "Version of Python interpreter: " << givenInterpreterVersion;
        qDebug() << "Version selected in launcher:  " << _selectedInterpreterVersion;
    }

    return match;
}


QString JupyterLauncher::getPythonInterpreterPath()
{
    return mv::settings().getPluginGlobalSettingsGroupAction<GlobalSettingsAction>(this)->getDefaultPythonPathAction().getFilePath();
}

void JupyterLauncher::setPythonInterpreterPath(const QString& p)
{
    mv::settings().getPluginGlobalSettingsGroupAction<GlobalSettingsAction>(this)->getDefaultPythonPathAction().setFilePath(p);
}

bool JupyterLauncher::getShowInterpreterPathDialog()
{
    return !mv::settings().getPluginGlobalSettingsGroupAction<GlobalSettingsAction>(this)->getDoNotShowAgainButton().isChecked();
}

// Emulate the environment changes from a venv activate script
// https://docs.python.org/3/library/venv.html
// https://docs.python.org/3/using/cmdline.html#envvar-PYTHONHOME
// https://docs.python.org/3/using/cmdline.html#envvar-PYTHONPATH
void JupyterLauncher::setPythonEnv()
{
    QString pyInterpreter = QDir::toNativeSeparators(getPythonInterpreterPath());

    auto [isVenv, pythonHomePath] = getPythonHomePath(pyInterpreter);
    QString pythonPath = pythonHomePath;
    
    if (isVenv) // contains "pyvenv.cfg"
        qputenv("VIRTUAL_ENV", pythonHomePath.toUtf8());
    else  // contains python interpreter executable
        qputenv("PYTHONHOME", pythonHomePath.toUtf8());

    if(QOperatingSystemVersion::currentType() == QOperatingSystemVersion::Windows)
        pythonPath += "/Lib/site-packages";
    else
    {
        if (pythonPath.endsWith("bin"))
            pythonPath = pythonPath.chopped(3); // Removes the last 3 characters ("bin")

        if (pythonPath.endsWith("/"))
            pythonPath = pythonPath.chopped(1);

        QString pythonVersion = "python" + QString(_selectedInterpreterVersion);

        // PREFIX is something like -> /home/USER/miniconda3/envs/ENV_NAME
        // pythonPath -> "PREFIX/lib:PREFIX/lib/python3.11:PREFIX/lib/python3.11/site-packages"
        pythonPath = pythonPath + "/lib" + ":" + 
                     pythonPath + "/lib/" + pythonVersion + ":" + 
                     pythonPath + "/lib/" + pythonVersion + "/site-packages";
    }

    // Path to folder with installed packages
    // PYTHONPATH is essential for xeusinterpreter to load as the xeus_python_shell
    qputenv("PYTHONPATH", QDir::toNativeSeparators(pythonPath).toUtf8());

    // In order to run python -m jupyter lab and access the MANIVAULT_JUPYTERPLUGIN_CONNECTION_FILE 
    // the env variable must be set in the current process.
    // Setting it in the child QProcess does not work for reasons that are unclear.
    qputenv("MANIVAULT_JUPYTERPLUGIN_CONNECTION_FILE", _connectionFilePath.toUtf8());
}

std::pair<bool, QString> isCondaEnvironmentActive()
{
    if (!qEnvironmentVariableIsSet("CONDA_PREFIX"))
        return {false, ""};

    const QString condaPrefix = QString::fromLocal8Bit(qgetenv("CONDA_PREFIX"));

    QString pythonInterpreterPath = "";

    if(QOperatingSystemVersion::currentType() == QOperatingSystemVersion::Windows)
        pythonInterpreterPath += "/Scripts/python.exe";
    else // Linux/macOS
        pythonInterpreterPath += "/bin/python3";

    QString givenInterpreterVersion = getPythonVersion(pythonInterpreterPath);

    if(givenInterpreterVersion.isEmpty())
        return {false, ""};

    return {true, givenInterpreterVersion.trimmed()};
}

// TBD merge the two runScript signatures
int JupyterLauncher::runPythonScript(const QString& scriptName, QString& sout, QString& serr, const QStringList& params)
{
    // 1. Prepare a python process with the python path
    auto scriptFile =  QFile(scriptName);

    // 2. Place the script in a temporary file
    QByteArray scriptContent;
    if (scriptFile.open(QFile::ReadOnly)) {
        scriptContent = scriptFile.readAll();
        scriptFile.close();
    }

    QTemporaryFile tempFile;
    if (tempFile.open()) {
        tempFile.write(scriptContent);
        tempFile.close();
    }

    // 3. Run the script synchronously
    QString pyInterpreter = QDir::toNativeSeparators(getPythonInterpreterPath());

    qDebug() << "pyInterpreter: " << pyInterpreter;
    qDebug() << "Script: " << scriptName;
    qDebug() << "Running: " << tempFile.fileName();

    QProcess pythonProcess;

    auto printOut = [](const QString& output) {
        for (const QString& outline : output.split("\n"))
            if (!outline.isEmpty())
                qDebug() << outline;
        };

    QObject::connect(&pythonProcess, &QProcess::readyReadStandardOutput, [printOut, &pythonProcess, &sout]() {
        QString output = QString::fromUtf8(pythonProcess.readAllStandardOutput()).replace("\r\n", "\n");
        sout = output;
        printOut(output);
        });

    QObject::connect(&pythonProcess, &QProcess::readyReadStandardError, [printOut, &pythonProcess, &serr]() {
        QString errorOutput = QString::fromUtf8(pythonProcess.readAllStandardError()).replace("\r\n", "\n");
        serr = errorOutput;
        printOut(errorOutput);
        });

    pythonProcess.start(pyInterpreter, QStringList({ tempFile.fileName() }) + params);

    int result = 0;

    if (!pythonProcess.waitForStarted()) {
        result = 2;
        serr = QString("Could not run python interpreter %1 ").arg(pyInterpreter);
    }
    if (!pythonProcess.waitForFinished()) {
        result = 2;
        serr = QString("Running python interpreter %1 timed out").arg(pyInterpreter);
    }
    else
        result = pythonProcess.exitCode();

    if (result > 0)
        qWarning() << "Failed running script.";

    pythonProcess.deleteLater();

    return result;
}

bool JupyterLauncher::runPythonCommand(const QStringList& params)
{
    QString pyInterpreter = QDir::toNativeSeparators(getPythonInterpreterPath());

    qDebug() << "pyInterpreter: " << pyInterpreter;
    qDebug() << "Command " << params.join(" ");

    QProcess pythonProcess;

    auto printOut = [](const QString& output) {
        for (const QString& outline : output.split("\n"))
            if (!outline.isEmpty())
                qDebug() << outline;
        };

    QObject::connect(&pythonProcess, &QProcess::readyReadStandardOutput, [printOut, &pythonProcess]() {
        QString output = QString::fromUtf8(pythonProcess.readAllStandardOutput()).replace("\r\n", "\n");
        printOut(output);
        });

    QObject::connect(&pythonProcess, &QProcess::readyReadStandardError, [printOut, &pythonProcess]() {
        QString errorOutput = QString::fromUtf8(pythonProcess.readAllStandardError()).replace("\r\n", "\n");
        printOut(errorOutput);
        });

    bool succ = true;
    pythonProcess.start(pyInterpreter, params);

    if (!pythonProcess.waitForStarted()) {
        qWarning() << "failed start";
        succ = false;
    }
    if (!pythonProcess.waitForFinished(/* int msecs */ 60'000 )) {
        qWarning() << "failed timeout";
        succ = false;
    }
    if (pythonProcess.exitStatus() != QProcess::NormalExit) {
        qWarning() << "failed run";
        succ = false;
    }
    if (pythonProcess.exitCode() == 1) {
        qWarning() << "operation error";
        succ = false;
    }

    if (!succ)
        qWarning() << "Failed running command.";

    pythonProcess.deleteLater();

    return succ;
}

bool JupyterLauncher::installKernel()
{
    // 1. Create a marshalling directory with the correct kernel name "ManiVaultStudio"
    QTemporaryDir tdir;
    auto kernelDir = QDir(tdir.path());
    kernelDir.mkdir("ManiVaultStudio");
    kernelDir.cd("ManiVaultStudio");

    // 2. Unpack the kernel files from the resources into the marshalling directory 
    QDir directory(":/kernel-files/");
    QStringList kernelList = directory.entryList(QStringList({ "*.*" }));
    for (const QString& a : kernelList) {
        QByteArray scriptContent;
        auto kernFile = QFile(QString(":/kernel-files/") + a);
        if (kernFile.open(QFile::ReadOnly)) {
            scriptContent = kernFile.readAll();
            kernFile.close();
        }

        QString newFilePath = kernelDir.absoluteFilePath(a);
        QFile newFile(newFilePath);
        qDebug() << "Kernel file: " << newFilePath;
        if (newFile.open(QIODevice::WriteOnly)) {
            newFile.write(scriptContent);
            newFile.close();
        }
    }

    // 3. Install the ManiVaultStudio kernel for the current user 
    return runPythonCommand(QStringList({ "-m", "jupyter", "kernelspec", "install", kernelDir.absolutePath(), "--user" }));
}

bool JupyterLauncher::optionallyInstallMVWheel()
{
    const QString pluginVersion = QString::fromStdString(getVersion().getVersionString());
    QMessageBox::StandardButton reply = QMessageBox::question(
        nullptr, 
        "Python modules missing", 
        "mvstudio.kernel " + pluginVersion + " and mvstudio.data " + pluginVersion + " are required for passing data between Python and ManiVault.\nDo you wish to install them now?",
        QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
    if (reply == QMessageBox::Yes) {
        auto MVWheelPath = QCoreApplication::applicationDirPath() + "/PluginDependencies/JupyterLauncher/py/";
        MVWheelPath = QDir::toNativeSeparators(MVWheelPath);
        auto kernelWheel = MVWheelPath + "mvstudio_kernel-" + pluginVersion + "-py3-none-any.whl";
        auto dataWheel = MVWheelPath + "mvstudio_data-" + pluginVersion + "-py3-none-any.whl";
        
        qDebug() << "kernelWheel paths: " << kernelWheel;

        if (!runPythonCommand({ "-m", "pip", "install", kernelWheel })) {
            qWarning() << "Installing the mvstudio_kernel package failed. See logging for more information";
            return false;
        }

        qDebug() << "dataWheel paths: " << dataWheel;

        if (!runPythonCommand({ "-m", "pip", "install", dataWheel })) {
            qWarning() << "Installing the mvstudio_data package failed. See logging for more information";
            return false;
        }

        if (!installKernel()) {
            qWarning() << "Installing the ManiVaultStudio Jupyter kernel failed. See logging for more information";
            return false;
        }
    } else {
        return false;
    }
    return true;
}

void JupyterLauncher::shutdownJupyterServer()
{
    switch (_serverProcess.state()) {
    case QProcess::Starting: [[fallthrough]];
    case QProcess::Running:
    {
        // Use the shutdown api with our fixed Authorization token to close the server
        auto url = QUrl("http://127.0.0.1:8888/api/shutdown");
        auto request = QNetworkRequest(url);
        request.setRawHeader("Authorization", "token manivaultstudio_jupyterkerneluser");
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
 
        QNetworkAccessManager manager;
        auto reply = QSharedPointer<QNetworkReply>(manager.post(request, QByteArray("")));
        
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

void JupyterLauncher::jupyterServerError(QProcess::ProcessError error)
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
        qWarning() << "Timedout";
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

void JupyterLauncher::jupyterServerFinished(int exitCode, QProcess::ExitStatus exitStatus)
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

void JupyterLauncher::jupyterServerStateChanged(QProcess::ProcessState newState)
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

void JupyterLauncher::jupyterServerStarted()
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

    QObject::connect(_serverPollTimer, &QTimer::timeout, [=]() { 
        logProcessOutput(); 
        });

    _serverPollTimer->start();
}

void JupyterLauncher::logProcessOutput()
{
    if (!_serverProcess.canReadLine())
        return; 

    auto output = _serverProcess.readAll();
    _serverBackgroundTask->setProgressDescription(output);

}

void JupyterLauncher::launchJupyterKernelAndNotebook(const QString& version)
{
    _selectedInterpreterVersion = version;

    if (getShowInterpreterPathDialog())
        _launcherDialog->show();                // by default ask user for python path
    else
        launchJupyterKernelAndNotebookImpl();   // open notebook immediately if user has set do-not-show-dialog option

}

void JupyterLauncher::launchJupyterKernelAndNotebookImpl()
{
    // 0. Check if the user set a python interpreter path
    if (getPythonInterpreterPath() == "")
    {
        [[maybe_unused]] QMessageBox::StandardButton reply = QMessageBox::information(
            nullptr,
            "No Python interpreter",
            "Please provide a path to a python interpreter in the plugin settings.\n Go to File -> Settings -> Plugin: Jupyter Launcher -> Python interpreter");
        return;
    }

    if(!checkPythonVersion())
    {
        qDebug() << "The given python interpreter does not match the selected python version";
        return;
    }

    QString serr;
    QString sout;

    // 1. Check the path to see if the correct version of mvstudio is installed
    QString pluginVersion = QString::fromStdString(getVersion().getVersionString());
    auto exitCode = runPythonScript(":/text/check_env.py", sout, serr, QStringList{ pluginVersion });

    if (exitCode > 0)
    {
        qDebug() << "JupyterLauncher::launchJupyterKernelAndNotebookImpl: Checking environment failed";
        qDebug() << sout;
        qDebug() << serr;

        // error like time out or other failed connection
        if (exitCode >= 2)
            return;

        // we might just miss the communication modules
        if (exitCode == 1)
        {
            bool couldInstallModules = optionallyInstallMVWheel();

            if(!couldInstallModules)
            {
                qDebug() << "Could not install ManiVault JupyterPythonKernel modules";
                return;
            }
        }
    }

    // 2. Determine the path to the python library
    exitCode = runPythonScript(":/text/find_libpython.py", sout, serr);
    if (exitCode != 0) {
        qDebug() << "JupyterLauncher::launchJupyterKernelAndNotebookImpl: Finding python library failed";
        qDebug() << sout;
        qDebug() << serr;
        return;
    }

    QFileInfo pythonLibrary = QFileInfo(sout);

    setPythonEnv();

    qDebug() << "Using python communication plugin library " << pythonLibrary.fileName() << " at: " << pythonLibrary.dir().absolutePath();
    if (!loadDynamicLibray(pythonLibrary))
        qWarning() << "Failed to load/locate python communication plugin";

    QString jupyterPluginPath           = QCoreApplication::applicationDirPath() + "/PluginDependencies/JupyterLauncher/bin/JupyterPlugin" + _selectedInterpreterVersion.remove(".");
    QLibrary jupyterPluginLib           = QLibrary(jupyterPluginPath);
    QPluginLoader jupyterPluginLoader   = QPluginLoader(jupyterPluginLib.fileName());

    qDebug() << "Using python plugin at: " << jupyterPluginLib.fileName();

    // Check if the plugin was loaded successfully
    if (!jupyterPluginLoader.load()) {
        qWarning() << "Failed to load plugin:" << jupyterPluginLoader.errorString();
        return;
    }

    // If pluginFactory is a nullptr then loading of the plugin failed for some reason. Print the reason to output.
    auto jupyterPluginFactory = dynamic_cast<PluginFactory*>(jupyterPluginLoader.instance());
    if (!jupyterPluginFactory)
    {
        qWarning() << "Failed to load plugin: " << jupyterPluginPath << jupyterPluginLoader.errorString();
        return;
    }

    // Create the jupyter plugin
    mv::plugin::Plugin* jupyterPluginInstance = jupyterPluginFactory->produce();

    // Communicate the connection file path via the child action in the JupyterPlugin
    auto connectionFileAction = jupyterPluginInstance->findChildByPath("Connection file");
    if (connectionFileAction != nullptr)
    {
        FilePickerAction* connectionFilePickerAction = static_cast<FilePickerAction*>(connectionFileAction);
        connectionFilePickerAction->setFilePath(_connectionFilePath);
    }

    // Load the plugin but first set the environment to get 
    // the correct python version
    jupyterPluginInstance->init();
    startJupyterServerProcess();
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
    setMaximumNumberOfInstances(1);
}

ViewPlugin* JupyterLauncherFactory::produce()
{
    return new JupyterLauncher(this);
}

void JupyterLauncherFactory::initialize()
{
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

    auto [isConda, pyVersion] = isCondaEnvironmentActive();

    qDebug() << "JupyterLauncherFactory::initialize";

    QString  jupyterPluginFolder = QCoreApplication::applicationDirPath() + "/PluginDependencies/JupyterLauncher/bin/";
    qDebug() << "jupyterPluginFolder:" << jupyterPluginFolder;

    QStringList pythonPlugins = findLibraryFiles(jupyterPluginFolder);

    qDebug() << "pythonPlugins:" << pythonPlugins;

    // On Linux/Mac in a conda environment we cannot switch between environments
    if(QOperatingSystemVersion::currentType() != QOperatingSystemVersion::Windows && isConda)
    {
        QStringList filteredPythonPlugins;
        for (const QString &path : pythonPlugins) {
            QFileInfo fileInfo(path);
            QString fileName = fileInfo.fileName(); // Extract file name

            qDebug() << "fileName:" << fileName;
            qDebug() << "pyVersion:" << pyVersion.remove(".");

            if (fileName.contains(pyVersion.remove("."), Qt::CaseInsensitive))
                filteredPythonPlugins.append(path);

        }

        pythonPlugins = filteredPythonPlugins;
    }

    qDebug() << "pythonPlugins:" << pythonPlugins;

    for(const auto& pythonPlugin: pythonPlugins)
    {
        QString pythonVersionOfPlugin = extractSingleNumber(QFileInfo(pythonPlugin).fileName()).insert(1, ".");

        qDebug() << "pythonVersionOfPlugin:" << pythonVersionOfPlugin;

        auto launchJupyterPython = new TriggerAction(this, "Start Jupyter Kernel and Lab (" + pythonVersionOfPlugin + ")");
        connect(launchJupyterPython, &TriggerAction::triggered, this, [this, pythonVersionOfPlugin]() {
            auto openJupyterPlugins = mv::plugins().getPluginsByFactory(this);

            JupyterLauncher* plugin = nullptr;
            if (openJupyterPlugins.empty())
                plugin = mv::plugins().requestPlugin<JupyterLauncher>("Jupyter Launcher");
            else
                plugin = dynamic_cast<JupyterLauncher*>(openJupyterPlugins.front());

            plugin->launchJupyterKernelAndNotebook(pythonVersionOfPlugin);
        });

        _statusBarAction->addMenuAction(launchJupyterPython);
    }

    // Assign the status bar action so that it will appear on the main window status bar
    setStatusBarAction(_statusBarAction);
}

QIcon JupyterLauncherFactory::getIcon(const QColor& color /*= Qt::black*/) const
{
    return QIcon(":/images/logo.svg");
}

mv::DataTypes JupyterLauncherFactory::supportedDataTypes() const
{
    return { };
}
