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

    this->loadPlugin();
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

void JupyterLauncher::preparePythonProcess(QProcess &process)
{
    // 1. Configure a process to run python on the user selected python interpreter
    auto pydir = _settingsAction.getPythonPathAction().getDirectory();
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

int JupyterLauncher::runPythonScript(const QString scriptName, QString& sout, QString& serr)
{
    // 1. Prepare a python process with the python path
    QProcess pythonScriptProcess;
    this->preparePythonProcess(pythonScriptProcess);
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
    auto pydir = _settingsAction.getPythonPathAction().getDirectory(); 
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

bool JupyterLauncher::runPythonScript(const QStringList params)
{
    auto pydir = _settingsAction.getPythonPathAction().getDirectory();
    QProcess mvInstallProcess;
    this->preparePythonProcess(mvInstallProcess);
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

bool JupyterLauncher::installKernel()
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
    return runPythonScript(QStringList({ "-m", "jupyter", "kernelspec", "install",  kernelDir.absolutePath() + QDir::separator() + QString("ManiVaultStudio"), "--user" }));
}

bool JupyterLauncher::optionallyInstallMVWheel()
{
    QMessageBox::StandardButton reply = QMessageBox::question(
        nullptr, 
        "MVJupyterPluginManager missing", 
        "MVJupyterPluginManager 0.4.5 is needed in the python environment.\n Do you wish to install it now?",
        QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
    if (reply == QMessageBox::Yes) {
        auto MVWheelPath = QCoreApplication::applicationDirPath() + "/Plugins/JupyterPlugin/mvjupyterpluginmanager-0.4.5-py3-none-any.whl";
        auto pydir = _settingsAction.getPythonPathAction().getDirectory();
        if (!runPythonScript(QStringList({ "-m", "pip", "install", MVWheelPath.toStdString().c_str() }))) {
            qWarning() << "Installing the MVJupyterPluginManager failed. See logging for more information";
            return false;
        }
        if (!this->installKernel()) {
            qWarning() << "Installing the ManiVaultStudion Jupyter kernel failed. See logging for more information";
            return false;
        }
    } else {
        return false;
    }
    return true;
}

bool JupyterLauncher::startJupyterServerProcess()
{
#ifdef _WIN32
    QString sep(";"); // path separator
#else
    QString sep(":");
#endif
    auto pydir = _settingsAction.getPythonPathAction().getDirectory();
    _serverProcess.setProcessChannelMode(QProcess::MergedChannels);
    auto connectionPath = _settingsAction.getConnectionFilePathAction().getString();
    // In order to run python -m jupyter lab and access the MANIVAULT_JUPYTERPLUGIN_CONNECTION_FILE 
    // the env variable this must be set in the current process.
    // Setting it in the child QProcess does not work for reasons that are unclear.
    qputenv("MANIVAULT_JUPYTERPLUGIN_CONNECTION_FILE", QDir::toNativeSeparators(connectionPath).toUtf8());
    auto runEnvironment = QProcessEnvironment::systemEnvironment();
    // runEnvironment.insert("MANIVAULT_JUPYTERPLUGIN_CONNECTION_FILE", QDir::toNativeSeparators(connectionPath));
    runEnvironment.insert("PATH", pydir + sep + runEnvironment.value("PATH"));
    _serverProcess.setProcessEnvironment(runEnvironment);
    auto started = _serverProcess.startDetached(pydir + QString("/python"), { "-m", "jupyter", "lab"});
    if (!started) {
        qWarning() << "Failed to start the JupyterLab Server";
        return false;
    }
    _serverPollTimer = new QTimer();
    _serverPollTimer->setInterval(20); // poll every 20ms
    _serverBackgroundTask = new BackgroundTask(nullptr, "JupyterLab Server");
    _serverBackgroundTask->setProgressMode(Task::ProgressMode::Manual);
    _serverBackgroundTask->setProgress(0);
    QObject::connect(_serverPollTimer, &QTimer::timeout, [=]() { reportProcessState(); });
    _serverPollTimer->start();
    return true;
}

void JupyterLauncher::reportProcessState()
{
    switch (_serverProcess.state()) {
        case QProcess::NotRunning:
            //this->_serverBackgroundTask->setFinished();
            break;
        case QProcess::Starting:
            this->_serverBackgroundTask->setIdle();
            break;
        case QProcess::Running:
            this->_serverBackgroundTask->setRunning();
            this->_serverBackgroundTask->setProgress(0.1f);
            break;
        default:
            this->_serverBackgroundTask->setUndefined();
    }
    if (_serverProcess.canReadLine()) {
        auto output = _serverProcess.readAll();
        _serverBackgroundTask->setProgressDescription(output);
    }

}

bool JupyterLauncher::validatePythonEnvironment() 
{
    auto pydir = _settingsAction.getPythonPathAction().getDirectory();
    return true;
}

bool JupyterLauncher::loadPlugin()
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
    auto exitCode = runPythonScript(":/text/check_env.py", sout, serr); 
    if (exitCode == 2) {
        qDebug() << serr << sout;
        // TODO display error message box
        return false;
    }
    if (exitCode == 1) {
        if (!this->optionallyInstallMVWheel()) {
            return false;
        }
    }
    else {
        qDebug() << "MVJupyterPluginManager is already installed";
    }

    // 2. Determine the path to the python library
    exitCode = runPythonScript(":/text/find_libpython.py", sout, serr); 
    if (exitCode != 0) {
        qDebug() << serr << sout;
        // TODO display error message box
        return false;
    }
    auto sharedLib = QFileInfo(sout);
    auto sharedLibDir = sharedLib.dir();
    qDebug() << "Using python shared library at: " << sharedLibDir.absolutePath();

    // Load the plugin
    QPluginLoader jupyLoader(jupyterPluginPath);

#ifdef _WIN32
    SetDllDirectoryA(sharedLibDir.absolutePath().toUtf8().data());
#endif
    // Check if the plugin was loaded successfully
    if (!jupyLoader.load()) {
        qWarning() << "Failed to load plugin:" << jupyLoader.errorString();
        return false;
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
    auto connectionPath = _settingsAction.getConnectionFilePathAction().getString();
    auto jpActions = pluginInstance->getChildren();
    for(auto action: jpActions) {
        qDebug() << action->text() << ": " << action->data();
        if (action->text() == QString("Connection file path")) {
            action->setData(connectionPath);
        }
    }
    pluginInstance->init();
    startJupyterServerProcess();
    return true;
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
    _statusBarPopupAction.setString("<b>Launch Jupyter Python Kernel for Python 3.11</b>");

    _statusBarPopupGroupAction.setShowLabels(false);
    _statusBarPopupGroupAction.setConfigurationFlag(WidgetAction::ConfigurationFlag::NoGroupBoxInPopupLayout);
    _statusBarPopupGroupAction.addAction(&_statusBarPopupAction);
    /*_statusBarPopupGroupAction.setWidgetConfigurationFunction([](WidgetAction* action, QWidget* widget) -> void {
        auto label = widget->findChild<QLabel*>("Label");

        Q_ASSERT(label != nullptr);

        if (label == nullptr)
            return;

        label->setOpenExternalLinks(true);
    });*/

    // 

    _statusBarAction = new PluginStatusBarAction(this, "Status Bar", getKind());

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
