#include "JupyterLauncher.h"
#include "GlobalSettingsAction.h"

#include <actions/WidgetAction.h>
#include <Application.h>
#include <CoreInterface.h>
#include <PointData/PointData.h>

#include <QByteArray>
#include <QDebug>
#include <QLibrary>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QOperatingSystemVersion>
#include <QPluginLoader>
#include <QProcess>
#include <QProcessEnvironment>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QThread>

#include <exception>
#include <iostream>
#include <stdexcept>

#ifdef _WIN32
#include <windows.h>
#endif

using namespace mv;

Q_PLUGIN_METADATA(IID "nl.BioVault.JupyterLauncher")

JupyterLauncher::JupyterLauncher(const PluginFactory* factory) :
    ViewPlugin(factory),
    _points(),
    _currentDatasetName(),
    _currentDatasetNameLabel(new QLabel()),
    _serverBackgroundTask(nullptr),
    _serverPollTimer(nullptr)
{
    setObjectName("Jupyter kernel plugin launcher");
    _currentDatasetNameLabel->setAlignment(Qt::AlignCenter);
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

// Distinguish between python in a regular or conda directory
// and python in a venv
const QString JupyterLauncher::getVirtDir(const QString pydir)
{
    // check if the last 
    auto pydirInfo = QFileInfo(pydir);
    //In a venv python sits in a Script or bin dir dependant on os
#ifdef _WIN32
    auto venvParent = QString("Scripts");
#else
    auto venvParent = QString("bin");
#endif
    qDebug() << "Check if this is Script or bin: " << pydirInfo.fileName();
    // A Script/bin and a pyvenv.cfg in the dir above indicate a venv so take the parent
    if (pydirInfo.fileName() == venvParent) {
        auto virtdir = QFileInfo(pydir).absoluteDir();
        qDebug() << "Check for pyenv.cfg in " << virtdir;
        if (virtdir.exists("pyvenv.cfg")) {
            return QFileInfo(pydir).absoluteDir().absolutePath();
        }
    }
    return pydir;
}

QString JupyterLauncher::getPythonExePath()
{
    return mv::settings().getPluginGlobalSettingsGroupAction<GlobalSettingsAction>(this)->getDefaultPythonPathAction().getFilePath();
}

QString JupyterLauncher::getPythonConfigPath()
{
    return mv::settings().getPluginGlobalSettingsGroupAction<GlobalSettingsAction>(this)->getDefaultConnectionPathAction().getFilePath();
}

// Emulate the environment changes from a venv activate script
void JupyterLauncher::setPythonEnv(const QString version)
{
    // 1. Configure a process to run python on the user selected python interpreter
    QString pyInterpreter = QDir::toNativeSeparators(getPythonExePath());
    QString pyDir = QFileInfo(pyInterpreter).absolutePath();

    //auto virtdir = getVirtDir(pydir);
    //QString currentPATH = QString::fromUtf8(qgetenv("PATH"));

    //auto pathSeparator = []() -> QString {
    //    // Return the appropriate path separator based on the OS
    //    return (QOperatingSystemVersion::currentType() == QOperatingSystemVersion::Windows) ? ";" : ":";
    //    };

    //QString newPATH = pyDir + pathSeparator() + currentPATH;

    //qputenv("VIRTUAL_ENV", QDir::toNativeSeparators(virtdir).toUtf8());
    //qputenv("PATH", newPATH.toUtf8());

    qputenv("PYTHONHOME", pyDir.toUtf8());

    // PYTHONPATH is essential to picking up the modules in a venv environment
    // without it the xeusinterpreter will fail to load as the xeus_python_shell
    // will not be found.
    QString pythonPath;

#ifdef _WIN32
    pythonPath = QDir::toNativeSeparators(pyDir + "/Lib/site-packages");
#else
    pythonPath = QDir::toNativeSeparators(pyDir + "/lib/python" + version + "/site-packages");
#endif
    qputenv("PYTHONPATH", pythonPath.toUtf8());
}


// TBD merge the two runScript signatures

int JupyterLauncher::runPythonScript(const QString scriptName, QString& sout, QString& serr, const QString version, const QStringList params)
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
    QString pyInterpreter = QDir::toNativeSeparators(getPythonExePath());

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

bool JupyterLauncher::runPythonCommand(const QStringList params, const QString version)
{
    QString pyInterpreter = QDir::toNativeSeparators(getPythonExePath());

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
    if (!pythonProcess.waitForFinished()) {
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

bool JupyterLauncher::installKernel(const QString version)
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
    return runPythonCommand(QStringList({ "-m", "jupyter", "kernelspec", "install", kernelDir.absolutePath(), "--user" }), version);
}

bool JupyterLauncher::optionallyInstallMVWheel(const QString version)
{
    const QString pluginVersion = getVersion();
    QMessageBox::StandardButton reply = QMessageBox::question(
        nullptr, 
        "Python modules missing", 
        "mvstudio.kernel and mvstudio.data " + pluginVersion + " are needed in the python environment.\n Do you wish to install it now? ",
        QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
    if (reply == QMessageBox::Yes) {
        auto MVWheelPath = QCoreApplication::applicationDirPath() + "/PluginDependencies/JupyterLauncher/py/";
        MVWheelPath = QDir::toNativeSeparators(MVWheelPath);
        auto kernelWheel = MVWheelPath + "mvstudio_kernel-" + pluginVersion + "-py3-none-any.whl";
        auto dataWheel = MVWheelPath + "mvstudio_data-" + pluginVersion + "-py3-none-any.whl";
        
        qDebug() << "kernelWheel paths: " << kernelWheel;
        qDebug() << "dataWheel paths: " << dataWheel;

        if (!runPythonCommand({ "-m", "pip", "install", "numpy"}, version)) {
            qWarning() << "Installing the MVJupyterPluginManager failed. See logging for more information";
            return false;
        }

        if (!runPythonCommand({ "-m", "pip", "install", kernelWheel }, version)) {
            qWarning() << "Installing the MVJupyterPluginManager failed. See logging for more information";
            return false;
        }

        if (!runPythonCommand({ "-m", "pip", "install", dataWheel }, version)) {
            qWarning() << "Installing the MVJupyterPluginManager failed. See logging for more information";
            return false;
        }

        if (!this->installKernel(version)) {
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
    case QProcess::Starting:
    case QProcess::Running:
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
    this->_serverBackgroundTask->setFinished();
}

void JupyterLauncher::jupyterServerFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    switch (exitStatus) {
    case QProcess::NormalExit :
        qDebug() << "Jupyter server exited normally";
        break;
    default:
        qWarning() << "Jupyter server exited abnormally";
        qWarning() << "With error: " << _serverProcess.errorString();
    }
    this->_serverBackgroundTask->setFinished();
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

bool JupyterLauncher::startJupyterServerProcess(const QString version)
{
    // Return the appropriate path separator based on the OS
    auto pathSeparator = []() -> QString {
        return (QOperatingSystemVersion::currentType() == QOperatingSystemVersion::Windows) ? ";" : ":";
        };

    auto pyinterp = QFileInfo(getPythonExePath());
    auto pydir = QDir::toNativeSeparators(pyinterp.absolutePath());

    // In order to run python -m jupyter lab and access the MANIVAULT_JUPYTERPLUGIN_CONNECTION_FILE 
    // the env variable this must be set in the current process.
    // Setting it in the child QProcess does not work for reasons that are unclear.
    auto connectionPath = getPythonConfigPath();
    qputenv("MANIVAULT_JUPYTERPLUGIN_CONNECTION_FILE", QDir::toNativeSeparators(connectionPath).toUtf8());

    auto runEnvironment = QProcessEnvironment::systemEnvironment();
    runEnvironment.insert("PATH", pydir + pathSeparator() + runEnvironment.value("PATH"));
    auto configPath = QCoreApplication::applicationDirPath() + "/PluginDependencies/JupyterLauncher/py/jupyter_server_config.py";

    _serverBackgroundTask = new BackgroundTask(nullptr, "JupyterLab Server");
    _serverBackgroundTask->setProgressMode(Task::ProgressMode::Manual);
    _serverBackgroundTask->setIdle();

    _serverProcess.setProcessChannelMode(QProcess::MergedChannels);
    _serverProcess.setProcessEnvironment(runEnvironment);
    _serverProcess.setProgram(pydir + QString("/python"));
    _serverProcess.setArguments({ "-m", "jupyter", "lab", "--config", configPath});

    connect(&_serverProcess, &QProcess::stateChanged, this, &JupyterLauncher::jupyterServerStateChanged);
    connect(&_serverProcess, &QProcess::errorOccurred, this, &JupyterLauncher::jupyterServerError);

    connect(&_serverProcess, &QProcess::started, this, &JupyterLauncher::jupyterServerStarted);
    connect(&_serverProcess, &QProcess::finished, this, &JupyterLauncher::jupyterServerFinished);

    connect(Application::current(), &QCoreApplication::aboutToQuit, this, &JupyterLauncher::shutdownJupyterServer);

    _serverProcess.start();
    _serverPollTimer = new QTimer();
    _serverPollTimer->setInterval(20); // poll every 20ms

    QObject::connect(_serverPollTimer, &QTimer::timeout, [=]() { 
        logProcessOutput(); 
        });
    _serverPollTimer->start();
    return true;
}

void JupyterLauncher::logProcessOutput()
{
    if (_serverProcess.canReadLine()) {
        auto output = _serverProcess.readAll();
        _serverBackgroundTask->setProgressDescription(output);
    }

}

bool JupyterLauncher::validatePythonEnvironment() 
{
    // TBD
    return true;
}

// The pyversion should correspond to a python major.minor version
// e.g.
// "3.11"
// "3.12"
// There  must be a JupyterPlugin (a kernel provider) that matches the python version for this to work.
void JupyterLauncher::loadJupyterPythonKernel(const QString pyversion)
{
    QString serr;
    QString sout;
    // 1. Check the path to see if the correct version of mvstudio is installed
    auto exitCode = runPythonScript(":/text/check_env.py", sout, serr, pyversion, QStringList{ getVersion() });
    if (exitCode == 2) {
        qDebug() << serr << sout;
        // TODO display error message box
        return ;
    }
    if (exitCode == 1) {
        if (!this->optionallyInstallMVWheel(pyversion)) {
            return;
        }
    }
    else {
        qDebug() << "MVJupyterPluginManager is already installed";
    }

    // 2. Determine the path to the python library
    exitCode = runPythonScript(":/text/find_libpython.py", sout, serr, pyversion); 
    if (exitCode != 0) {
        qWarning() << serr << sout;
        // TODO display error message box
        return;
    }

    auto pythonLibrary = QFileInfo(sout);
    QString sharedLibFilePath = pythonLibrary.absoluteFilePath();
    QDir sharedLibDir = pythonLibrary.dir();

    // This seems cleaner but does not work
    //qDebug() << "Using python shared library at: " << sharedLibFilePath;
    //if (!QLibrary::isLibrary(sharedLibFilePath))
    //    qWarning() << "Not a library: " << sharedLibFilePath;

    //QLibrary lib(sharedLibFilePath);
    //if(lib.load())
    //    qDebug() << "Loaded python library";
    //else
    //    qDebug() << "Failed to load python library";

    qDebug() << "Using python shared library " << pythonLibrary.fileName() << " at: " << sharedLibDir.absolutePath();
#ifdef _WIN32
    SetDllDirectoryA(QString(sharedLibDir.absolutePath() + "/").toUtf8().data());
#else
    qputenv("LD_LIBRARY_PATH", QString(sharedLibDir.absolutePath() + "/").toUtf8() + ":" + qgetenv("LD_LIBRARY_PATH");
#endif

    QString jupyterPluginPath = QCoreApplication::applicationDirPath() + "/PluginDependencies/JupyterLauncher/bin/JupyterPlugin";
    QLibrary jupyterPluginLib(jupyterPluginPath);
    qDebug() << "Using python plugin at: " << jupyterPluginLib.fileName();

    QPluginLoader jupyterPluginLoader(jupyterPluginLib.fileName());

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

    QJsonObject jupyterPluginMetaData = jupyterPluginLoader.metaData().value("MetaData").toObject();
    QString pluginKind = jupyterPluginMetaData.value("name").toString();
    QString menuName = jupyterPluginMetaData.value("menuName").toString();
    QString version = jupyterPluginMetaData.value("version").toString();

    // Loading of the plugin succeeded so cast it to its original class
    _pluginFactories[pluginKind] = jupyterPluginFactory;
    _pluginFactories[pluginKind]->setKind(pluginKind);
    _pluginFactories[pluginKind]->setVersion(version);
    _pluginFactories[pluginKind]->initialize();

    auto jupyterPluginInstance = _plugins.emplace_back(jupyterPluginFactory->produce());

    // Communicate the connection file path via the child action in the JupyterPlugin
    // TODO: just create a file instead of having to point here
    auto connectionFileAction = jupyterPluginInstance->findChildByPath("Connection file");
    if (connectionFileAction != nullptr)
    {
        FilePickerAction* connectionFilePickerAction = static_cast<FilePickerAction*>(connectionFileAction);
        auto connectionPath = getPythonConfigPath();
        connectionFilePickerAction->setFilePath(connectionPath);
    }

    // Load the plugin but first set the environment to get 
    // the correct python version
    setPythonEnv(pyversion);
    jupyterPluginInstance->init();
    startJupyterServerProcess(pyversion);
    return;
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

    auto launchJupyterPython311 = new TriggerAction(this, "Start Jupyter Kernel and Lab (3.11)");
    connect(launchJupyterPython311, &TriggerAction::triggered, this, [this]() {
        auto openJupyterPlugins = mv::plugins().getPluginsByFactory(this);

        JupyterLauncher* plugin = nullptr;
        if (openJupyterPlugins.empty())
            plugin = mv::plugins().requestPlugin<JupyterLauncher>("Jupyter Launcher");
        else
            plugin = dynamic_cast<JupyterLauncher*>(openJupyterPlugins.front());

        plugin->loadJupyterPythonKernel("3.11");
    });

    _statusBarAction = new PluginStatusBarAction(this, "Jupyter Launcher", getKind());
    _statusBarAction->addMenuAction(launchJupyterPython311);

    // Sets the action that is shown when the status bar is clicked
    _statusBarAction->setPopupAction(&_statusBarPopupGroupAction);

    // Always show
    //_statusBarAction->getConditionallyVisibleAction().setChecked(false);

    // Position to the right of the status bar action
    _statusBarAction->setIndex(-1);

    // Assign the status bar action so that it will appear on the main window status bar
    setStatusBarAction(_statusBarAction);
}

QIcon JupyterLauncherFactory::getIcon(const QColor& color /*= Qt::black*/) const
{
    return QIcon(":/images/logo.svg");
}

mv::DataTypes JupyterLauncherFactory::supportedDataTypes() const
{
    return { PointType };
}

mv::gui::PluginTriggerActions JupyterLauncherFactory::getPluginTriggerActions(const mv::Datasets& datasets) const
{
    PluginTriggerActions pluginTriggerActions;

    const auto getPluginInstance = [this]() -> JupyterLauncher* {
        return dynamic_cast<JupyterLauncher*>(plugins().requestViewPlugin(getKind()));
    };

    const auto numberOfDatasets = datasets.count();

    if (numberOfDatasets >= 1 && PluginFactory::areAllDatasetsOfTheSameType(datasets, PointType)) {
        auto pluginTriggerAction = new PluginTriggerAction(const_cast<JupyterLauncherFactory*>(this), this, "JupyterPlugin", "Open Jupyter Bridge", getIcon(), [this, getPluginInstance, datasets](PluginTriggerAction& pluginTriggerAction) -> void {
            for (auto dataset : datasets)
                getPluginInstance();
        });

        pluginTriggerActions << pluginTriggerAction;
    }

    return pluginTriggerActions;
}
