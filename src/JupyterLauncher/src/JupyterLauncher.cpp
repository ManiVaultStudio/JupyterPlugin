#include "JupyterLauncher.h"

#include "GlobalSettingsAction.h"

#include <event/Event.h>

#include <DatasetsMimeData.h>
#include <PointData/PointData.h>

#include <QDebug>
#include <QMimeData>
#include <QPluginLoader>
#include <QProcessEnvironment>

#include <cstdlib>
#include <sstream>

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

/*
    T.B.D. 
    QString text = "exit(1)";
    // check the path to see if MVJupyterPluginManager is installed
    auto checkScript =  QFile(":/text/check_env.py");
    if (checkScript.open(QFile::ReadOnly)) {
        text = checkScript.readAll();
        checkScript.close();
    } else {
        qDebug() << "Unable to get the environment test script for the JupyterLauncher";
    }
#ifdef _WIN32
    QString sep(";"); // path separator
#else
    QString sep(":");
#endif

    auto pydir = _settingsAction.getPythonPathAction().getDirectory();

    QProcess pythonCheckProcess;
    auto runEnvironment = QProcessEnvironment::systemEnvironment();
    // set the desired python interpreter path first in the PATH environment variable
    runEnvironment.insert("PATH", pydir + sep + runEnvironment.value("PATH"));
    // execute the python check and examine the result
    pythonCheckProcess.setProcessEnvironment(runEnvironment);
    pythonCheckProcess.start("python", {"-c", text.toStdString().c_str()});
    QObject::connect(pythonCheckProcess, &QProcess::finished, pythonCheckProcess, [pythonCheckProcess](int exitCode, QProcess::ExitStatus exitStatus){
        if (exitStatus != QProcess::ExitStatus::NormalExit || exitCode != EXIT_SUCCESS){
            WARN << "Paste failed. xdotool installed?";
            QMessageBox::warning(nullptr, "Error", "Paste failed. xdotool installed?");
        }
        pythonCheckProcess.deleteLater();
    });
*/
    // Load the plugin
    QPluginLoader jupyLoader(jupyterPluginPath);

    qInfo() << "Current path" << getenv("PATH");
#ifdef _WIN32
    SetDllDirectoryA(pydir.toUtf8().data());
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
    _statusBarPopupAction.setString(
        "<p><b>Launch Jupyter Python Kernel</b></p>"
        "<p>This launches a python 3.11 Jupyter kernel that can be used to access ManiVaultStudio data.</p>"
        "<p>The kernel provides users with a path to process ManiVault data "
        "in python or use standard libraries. "
        "Using python it may be possible to load or save data from unsupported or non-standard file formats.</p>"
        "<p>For more details see the documentation for the MVData python module and the "
        "example Jupyter notebook MVDataExample.ipynb</p>");
    _statusBarPopupAction.setPopupSizeHint(QSize(200, 10));

    _statusBarPopupGroupAction.setShowLabels(false);
    _statusBarPopupGroupAction.setConfigurationFlag(WidgetAction::ConfigurationFlag::NoGroupBoxInPopupLayout);
    _statusBarPopupGroupAction.addAction(&_statusBarPopupAction);
    _statusBarPopupGroupAction.setWidgetConfigurationFunction([](WidgetAction* action, QWidget* widget) -> void {
        auto label = widget->findChild<QLabel*>("Label");

        Q_ASSERT(label != nullptr);

        if (label == nullptr)
            return;

        label->setOpenExternalLinks(true);
    });

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
