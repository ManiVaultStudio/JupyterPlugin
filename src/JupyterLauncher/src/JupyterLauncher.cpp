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

    // Load the plugin
    QPluginLoader jupyLoader(jupyterPluginPath);
    auto pydir = _settingsAction.getPythonPathAction().getDirectory();
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

ViewPlugin* JupyterLauncherFactory::produce()
{
    return new JupyterLauncher(this);
}

void JupyterLauncherFactory::initialize()
{
    ViewPluginFactory::initialize();

    // Create an instance of our GlobalSettingsAction (derived from PluginGlobalSettingsGroupAction) and assign it to the factory
    setGlobalSettingsGroupAction(new GlobalSettingsAction(this, this));
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
