#include "JupyterLauncher.h"

#include "GlobalSettingsAction.h"

#include <event/Event.h>

#include <DatasetsMimeData.h>
#include <PointData/PointData.h>

#include <QDebug>
#include <QMimeData>
#include <QPluginLoader>
#include <QProcessEnvironment>
#include <QTemporaryFile>

#include <cstdlib>
#include <sstream>
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
    _settingsAction(this, "Settings Action")
{
    setObjectName("Jupyter kernel plugin launcher");
    // Align text in the center
    _currentDatasetNameLabel->setAlignment(Qt::AlignCenter);
}

JupyterLauncher::~JupyterLauncher()
{
    switch (_serverProcess.state()) {
    case QProcess::Starting:
    case QProcess::Running:
        _serverProcess.terminate();
    }
}

void JupyterLauncher::init()
{
    // Create layout
    auto layout = new QVBoxLayout();

    layout->setContentsMargins(0, 0, 0, 0);

    layout->addWidget(_currentDatasetNameLabel);

    // Apply the layout
    getWidget().setLayout(layout);

    addDockingAction(&_settingsAction);

    // Alternatively, classes which derive from hdsp::EventListener (all plugins do) can also respond to events
    _eventListener.addSupportedEventType(static_cast<std::uint32_t>(EventType::DatasetAdded));
    _eventListener.addSupportedEventType(static_cast<std::uint32_t>(EventType::DatasetDataChanged));
    _eventListener.addSupportedEventType(static_cast<std::uint32_t>(EventType::DatasetRemoved));
    _eventListener.addSupportedEventType(static_cast<std::uint32_t>(EventType::DatasetDataSelectionChanged));
    _eventListener.registerDataEventByType(PointType, std::bind(&JupyterLauncher::onDataEvent, this, std::placeholders::_1));

    //this->loadPlugin();
}

void JupyterLauncher::onDataEvent(mv::DatasetEvent* dataEvent)
{
    // Get smart pointer to dataset that changed
    const auto changedDataSet = dataEvent->getDataset();

    // Get GUI name of the dataset that changed
    const auto datasetGuiName = changedDataSet->getGuiName();

    // The data event has a type so that we know what type of data event occurred (e.g. data added, changed, removed, renamed, selection changes)
    switch (dataEvent->getType()) {

        // A points dataset was added
        case EventType::DatasetAdded:
        {
            // Cast the data event to a data added event
            const auto dataAddedEvent = static_cast<DatasetAddedEvent*>(dataEvent);

            // Get the GUI name of the added points dataset and print to the console
            qDebug() << datasetGuiName << "was added";

            break;
        }

        // Points dataset data has changed
        case EventType::DatasetDataChanged:
        {
            // Cast the data event to a data changed event
            const auto dataChangedEvent = static_cast<DatasetDataChangedEvent*>(dataEvent);

            // Get the name of the points dataset of which the data changed and print to the console
            qDebug() << datasetGuiName << "data changed";

            break;
        }

        // Points dataset data was removed
        case EventType::DatasetRemoved:
        {
            // Cast the data event to a data removed event
            const auto dataRemovedEvent = static_cast<DatasetRemovedEvent*>(dataEvent);

            // Get the name of the removed points dataset and print to the console
            qDebug() << datasetGuiName << "was removed";

            break;
        }

        // Points dataset selection has changed
        case EventType::DatasetDataSelectionChanged:
        {
            // Cast the data event to a data selection changed event
            const auto dataSelectionChangedEvent = static_cast<DatasetDataSelectionChangedEvent*>(dataEvent);

            // Get the selection set that changed
            const auto& selectionSet = changedDataSet->getSelection<Points>();

            // Print to the console
            qDebug() << datasetGuiName << "selection has changed";

            break;
        }

        default:
            break;
    }
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


// Emulate the environment changes from a venv activate script
void JupyterLauncher::setPythonEnv(const QString version)
{
    // 1. Configure a process to run python on the user selected python interpreter
    auto pyinterp = QFileInfo(_settingsAction.getPythonPathAction(version).getFilePath());
    auto pydir = QDir::toNativeSeparators(pyinterp.absolutePath());
    auto virtdir = getVirtDir(pydir);
#ifdef _WIN32
    QString sep(";"); // path separator
#else
    QString sep(":");
#endif

    auto path = QString::fromUtf8(qgetenv("PATH"));
    qputenv("VIRTUAL_ENV", QDir::toNativeSeparators(virtdir).toUtf8());
    qputenv("PATH", QString(pydir + sep + path).toUtf8());
    qputenv("PYTHONHOME", "");
    // PYTHONPATH is essential to picking up the modules in a venv environment
    // without it the xeusinterpreter will fail to load as the xeus_python_shell
    // will not be found.
#ifdef _WIN32
    qputenv("PYTHONPATH", QDir::toNativeSeparators(QString(virtdir + "/Lib/site-packages")).toUtf8());
#else
    qputenv("PYTHONPATH", QDir::toNativeSeparators(QString(virtdir + "/lib/python" + version + "/site-packages")).toUtf8());
#endif
}

void JupyterLauncher::preparePythonProcess(QProcess &process, const QString version)
{
    // 1. Configure a process to run python on the user selected python interpreter
    auto pyinterp = QFileInfo(_settingsAction.getPythonPathAction(version).getFilePath());
    auto pydir = QDir::toNativeSeparators(pyinterp.absolutePath());
#ifdef _WIN32
    QString sep(";"); // path separator
#else
    QString sep(":");
#endif
    auto runEnvironment = QProcessEnvironment::systemEnvironment();
    // set the desired python interpreter path first in the PATH environment variable
    runEnvironment.insert("PATH", pydir + sep + runEnvironment.value("PATH"));
    // execute the python check and examine the result
    process.setProcessEnvironment(runEnvironment);
}

// TBD merge the two runScript signatures

int JupyterLauncher::runPythonScript(const QString scriptName, QString& sout, QString& serr, const QString version)
{
    // 1. Prepare a python process with the python path
    QProcess pythonScriptProcess;
    this->preparePythonProcess(pythonScriptProcess, version);
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
    auto result = 0;
    auto pyinterp = QFileInfo(_settingsAction.getPythonPathAction(version).getFilePath());
    auto pydir = QDir::toNativeSeparators(pyinterp.absolutePath());
    pythonScriptProcess.start(pydir + QString("/python"), QStringList({ tempFile.fileName() }));
    if (!pythonScriptProcess.waitForStarted()) {
        result = 2;
        serr = QString("Could not run python interpreter %1 ").arg(pydir + QString("/python"));
        return result;   
    }
    if (!pythonScriptProcess.waitForFinished()) {
        result = 2;
        serr = QString("Running python interpreter %1 timed out").arg(pydir + QString("/python"));
        return result;       
    }
    
    serr = QString::fromUtf8(pythonScriptProcess.readAllStandardError());
    sout = QString::fromUtf8(pythonScriptProcess.readAllStandardOutput());
    result = pythonScriptProcess.exitCode();
    pythonScriptProcess.deleteLater();
    return result;
}

bool JupyterLauncher::runPythonScript(const QStringList params, const QString version)
{
    auto pyinterp = QFileInfo(_settingsAction.getPythonPathAction(version).getFilePath());
    auto pydir = QDir::toNativeSeparators(pyinterp.absolutePath());
    QProcess mvInstallProcess;
    this->preparePythonProcess(mvInstallProcess, version);
    mvInstallProcess.start(pydir + QString("/python"), params);
    try {
        if (!mvInstallProcess.waitForStarted()) {
            throw std::runtime_error("failed start");
        }
        if (!mvInstallProcess.waitForFinished()) {
            throw std::runtime_error("run timeout");
        }
        if (mvInstallProcess.exitStatus() != QProcess::NormalExit) {
            throw std::runtime_error("failed run");
        }
        if (mvInstallProcess.exitCode() == 1) {
            throw std::runtime_error("operation error");
        }
    }
    catch (const std::exception& e) {
        auto serr = QString::fromUtf8(mvInstallProcess.readAllStandardError());
        auto sout = QString::fromUtf8(mvInstallProcess.readAllStandardOutput());
        qWarning() << "Script failed running " << params.join(" ") << "giving: " << "\n stdout : " << sout << "\n stderr : " << serr;
        return false;
    }
    mvInstallProcess.deleteLater();
    return true;
}

bool JupyterLauncher::installKernel(const QString version)
{
    // 1. Create a marshalling directory with the correct kernel name "ManiVaultStudio"
    QTemporaryDir tdir;
    auto kernelDir = QDir(tdir.path());
    kernelDir.mkdir("ManiVaultStudio");
    // 2. Unpack the kernel files from the resources into the marshalling directory 
    QDir directory(":/kernel-files/");
    QStringList kernelList = directory.entryList(QStringList({ "*.*" }));
    for (auto a : kernelList) {
        QByteArray scriptContent;
        auto kernFile = QFile(QString(":/kernel-files/") + a);
        if (kernFile.open(QFile::ReadOnly)) {
            scriptContent = kernFile.readAll();
            kernFile.close();
        }

        QFile file(kernelDir.absolutePath() + QDir::separator() + QString("ManiVaultStudio") + QDir::separator() + a);
        qDebug() << "Kernel file: " << QFileInfo(file).absoluteFilePath();
        auto c_str = QFileInfo(file).absoluteFilePath().toStdString();
        if (file.open(QIODevice::WriteOnly)) {
            file.write(scriptContent);
            file.close();
        }
    }
    // 3. Install the ManiVaultStudio kernel for the current user 
    return runPythonScript(QStringList({ "-m", "jupyter", "kernelspec", "install",  kernelDir.absolutePath() + QDir::separator() + QString("ManiVaultStudio"), "--user" }), version);
}

bool JupyterLauncher::optionallyInstallMVWheel(const QString version)
{
    QMessageBox::StandardButton reply = QMessageBox::question(
        nullptr, 
        "mvstudio.data_hierarchy missing", 
        "mvstudio.data_hierarchy 0.8 is needed in the python environment.\n Do you wish to install it now?",
        QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
    if (reply == QMessageBox::Yes) {
        auto MVWheelPath = QCoreApplication::applicationDirPath() + "/Plugins/JupyterPlugin/mvstudio-0.8.0-py3-none-any.whl";
        auto pyinterp = QFileInfo(_settingsAction.getPythonPathAction(version).getFilePath());
        auto pydir = QDir::toNativeSeparators(pyinterp.absolutePath());
        if (!runPythonScript(QStringList({ "-m", "pip", "install", MVWheelPath.toStdString().c_str() }), version)) {
            qWarning() << "Installing the MVJupyterPluginManager failed. See logging for more information";
            return false;
        }
        if (!this->installKernel(version)) {
            qWarning() << "Installing the ManiVaultStudion Jupyter kernel failed. See logging for more information";
            return false;
        }
    } else {
        return false;
    }
    return true;
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
#ifdef _WIN32
    QString sep(";"); // path separator
#else
    QString sep(":");
#endif

    auto pyinterp = QFileInfo(_settingsAction.getPythonPathAction(version).getFilePath());
    auto pydir = QDir::toNativeSeparators(pyinterp.absolutePath());
    _serverProcess.setProcessChannelMode(QProcess::MergedChannels);
    auto connectionPath = _settingsAction.getConnectionFilePathAction(version).getFilePath();
    // In order to run python -m jupyter lab and access the MANIVAULT_JUPYTERPLUGIN_CONNECTION_FILE 
    // the env variable this must be set in the current process.
    // Setting it in the child QProcess does not work for reasons that are unclear.
    qputenv("MANIVAULT_JUPYTERPLUGIN_CONNECTION_FILE", QDir::toNativeSeparators(connectionPath).toUtf8());
    auto runEnvironment = QProcessEnvironment::systemEnvironment();
    runEnvironment.insert("PATH", pydir + sep + runEnvironment.value("PATH"));
    auto configPath = QCoreApplication::applicationDirPath() + "/Plugins/JupyterPlugin/jupyter_server_config.py";

    _serverBackgroundTask = new BackgroundTask(nullptr, "JupyterLab Server");
    _serverBackgroundTask->setProgressMode(Task::ProgressMode::Manual);
    _serverBackgroundTask->setIdle();

    _serverProcess.setProcessEnvironment(runEnvironment);
    _serverProcess.setProgram(pydir + QString("/python"));
    //_serverProcess.setArguments({ "-m", "MVJupyterPluginManager", "--config", configPath });
    _serverProcess.setArguments({ "-m", "jupyter", "lab", "--config", configPath});

    connect(&_serverProcess, &QProcess::stateChanged, this, &JupyterLauncher::jupyterServerStateChanged);
    connect(&_serverProcess, &QProcess::errorOccurred, this, &JupyterLauncher::jupyterServerError);

    connect(&_serverProcess, &QProcess::started, this, &JupyterLauncher::jupyterServerStarted);
    connect(&_serverProcess, &QProcess::finished, this, &JupyterLauncher::jupyterServerFinished);

    _serverProcess.start();
    _serverPollTimer = new QTimer();
    _serverPollTimer->setInterval(20); // poll every 20ms

    QObject::connect(_serverPollTimer, &QTimer::timeout, [=]() { logProcessOutput(); });
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
// There  must be a JupyterPlugin (a kernel provider) that matches the python vesion for this to work.
void JupyterLauncher::loadJupyterPythonKernel(const QString pyversion)
{
    auto jupyterPluginPath = QCoreApplication::applicationDirPath() + "/Plugins/JupyterPlugin/JupyterPlugin";


    // suffix might not be needed - test
#ifdef _WIN32
    jupyterPluginPath += ".dll";
#endif

#ifdef __APPLE__
    jupyterPluginPath += ".dylib";
#endif

#ifdef __linux__
    jupyterPluginPath += ".so";
#endif

#ifdef _WIN32
    QString sep(";"); // path separator
#else
    QString sep(":");
#endif

    QString serr;
    QString sout;
    // 1. Check the path to see if MVJupyterPluginManager is installed
    auto exitCode = runPythonScript(":/text/check_env.py", sout, serr, pyversion); 
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
        qDebug() << serr << sout;
        // TODO display error message box
        return;
    }
    auto sharedLib = QFileInfo(sout);
    auto sharedLibDir = sharedLib.dir();
    qDebug() << "Using python shared library at: " << sharedLibDir.absolutePath();


    QPluginLoader jupyLoader(jupyterPluginPath);

#ifdef _WIN32
    SetDllDirectoryA(QString(sharedLibDir.absolutePath() + "/").toUtf8().data());
#endif
    // Check if the plugin was loaded successfully
    if (!jupyLoader.load()) {
        qWarning() << "Failed to load plugin:" << jupyLoader.errorString();
        return;
    }
    QString pluginKind = jupyLoader.metaData().value("MetaData").toObject().value("name").toString();
    QString menuName = jupyLoader.metaData().value("MetaData").toObject().value("menuName").toString();
    QString version = jupyLoader.metaData().value("MetaData").toObject().value("version").toString();
    auto pluginFactory = dynamic_cast<PluginFactory*>(jupyLoader.instance());

    // If pluginFactory is a nullptr then loading of the plugin failed for some reason. Print the reason to output.
    if (!pluginFactory)
    {
        qWarning() << "Failed to load plugin: " << jupyterPluginPath << jupyLoader.errorString();
    }

    // Loading of the plugin succeeded so cast it to its original class
    _pluginFactories[pluginKind] = pluginFactory;
    _pluginFactories[pluginKind]->setKind(pluginKind);
    _pluginFactories[pluginKind]->setVersion(version);
    _pluginFactories[pluginKind]->initialize();

    auto pluginInstance = pluginFactory->produce();
    _plugins.push_back(std::move(std::unique_ptr<plugin::Plugin>(pluginInstance)));

    // Communicate the connection file path via the child action in the JupyterPlugin
    //auto connectionPath = _settingsAction.getConnectionFilePathAction().getFilePath();
    auto connectionPath = _settingsAction.getConnectionFilePathAction(pyversion).getFilePath();
    auto jpActions = pluginInstance->getChildren();
    for(auto action: jpActions) {
        qDebug() << action->text() << ": " << action->data();
        if (action->text() == QString("Connection file path")) {
            action->setData(connectionPath);
        }
    }
    // Load the plugin but first set the environment to get 
    // the correct python version
    setPythonEnv(pyversion);
    pluginInstance->init();
    startJupyterServerProcess(pyversion);
    return;
}

JupyterLauncherFactory::JupyterLauncherFactory() :
    ViewPluginFactory(),
    _statusBarAction(nullptr),
    _statusBarPopupGroupAction(this, "Popup Group"),
    _statusBarPopupAction(this, "Popup")
{

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


    _statusBarAction = new PluginStatusBarAction(this, "Status Bar", getKind());
    auto launchJupyterPython311 = new TriggerAction(this, "py3.11: Kernel and Jupyter Lab");
    _statusBarAction->addMenuAction(launchJupyterPython311);

    connect(launchJupyterPython311, &TriggerAction::triggered, this, [this]() {
        auto plugin = mv::plugins().requestPlugin<JupyterLauncher>("Jupyter Launcher");
        plugin->loadJupyterPythonKernel("3.11");
    });

    // Sets the action that is shown when the status bar is clicked
    _statusBarAction->setPopupAction(&_statusBarPopupGroupAction);

    // Position to the right of the status bar action
    _statusBarAction->setIndex(-1);

    // Assign the status bar action so that it will appear on the main window status bar
    setStatusBarAction(_statusBarAction);
    _statusBarAction->getConditionallyVisibleAction().setChecked(false);
}

QIcon JupyterLauncherFactory::getIcon(const QColor& color /*= Qt::black*/) const
{
    return QIcon(":/images/logo.svg");
}

mv::DataTypes JupyterLauncherFactory::supportedDataTypes() const
{
    DataTypes supportedTypes;

    // This example analysis plugin is compatible with points datasets
    supportedTypes.append(PointType);

    return supportedTypes;
}

mv::gui::PluginTriggerActions JupyterLauncherFactory::getPluginTriggerActions(const mv::Datasets& datasets) const
{
    PluginTriggerActions pluginTriggerActions;

    const auto getPluginInstance = [this]() -> JupyterLauncher* {
        return dynamic_cast<JupyterLauncher*>(plugins().requestViewPlugin(getKind()));
    };

    const auto numberOfDatasets = datasets.count();

    if (numberOfDatasets >= 1 && PluginFactory::areAllDatasetsOfTheSameType(datasets, PointType)) {
        auto pluginTriggerAction = new PluginTriggerAction(const_cast<JupyterLauncherFactory*>(this), this, "Example", "View example data", getIcon(), [this, getPluginInstance, datasets](PluginTriggerAction& pluginTriggerAction) -> void {
            for (auto dataset : datasets)
                getPluginInstance();
        });

        pluginTriggerActions << pluginTriggerAction;
    }

    return pluginTriggerActions;
}
