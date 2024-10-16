#include "JupyterPlugin.h"

#include <QDebug>
#include <QProcess>
#include <QDir>

#include "XeusKernel.h"
#include "jupyterplugin_export.h"

Q_PLUGIN_METADATA(IID "nl.BioVault.JupyterPlugin")


using namespace mv;


class JupyterPlugin::PrivateKernel {
public:
    PrivateKernel() 
    {
        _xeusKernel = std::make_unique<XeusKernel>();
    }

    void startKernel(const QString& connection_path) 
    {
        _xeusKernel->startKernel(connection_path);
    }

public:
    std::unique_ptr<XeusKernel> _xeusKernel;  /** the xeus kernel that manages the jupyter comms and the python interpreter*/
};

JupyterPlugin::JupyterPlugin(const PluginFactory* factory) :
    ViewPlugin(factory),
    _dropWidget(nullptr),
    _points(),
    _currentDatasetName(),
    _currentDatasetNameLabel(new QLabel()),
    _settingsAction(this, "JupyterPlugin Settings"),
    pKernel(new PrivateKernel())
{
    // This line is mandatory if drag and drop behavior is required
    _currentDatasetNameLabel->setAcceptDrops(true);

    // Align text in the center
    _currentDatasetNameLabel->setAlignment(Qt::AlignCenter);
}

JupyterPlugin::~JupyterPlugin() = default;

void JupyterPlugin::setConnectionPath(const QString& connection_path)
{
    this->_connectionPath = connection_path;
}

void JupyterPlugin::init()
{
    // Create layout
    auto layout = new QVBoxLayout();

    layout->setContentsMargins(0, 0, 0, 0);

    layout->addWidget(_currentDatasetNameLabel);

    // Apply the layout
    getWidget().setLayout(layout);

    // Respond when the name of the dataset in the dataset reference changes
    connect(&_points, &Dataset<Points>::guiNameChanged, this, [this]() {

        auto newDatasetName = _points->getGuiName();

        // Update the current dataset name label
        _currentDatasetNameLabel->setText(QString("Current points dataset: %1").arg(newDatasetName));

    });

    auto jupyter_configFilepath = std::string("TODO: Add config file path here from action");
    
    // Manual start
    pKernel->startKernel(_settingsAction.getConnectionFilePathAction().getFilePath());
}

ViewPlugin* JupyterPluginFactory::produce()
{
    return new JupyterPlugin(this);
}

mv::DataTypes JupyterPluginFactory::supportedDataTypes() const
{
    return { PointType };
}

mv::gui::PluginTriggerActions JupyterPluginFactory::getPluginTriggerActions(const mv::Datasets& datasets) const
{
    PluginTriggerActions pluginTriggerActions;

    const auto getPluginInstance = [this]() -> JupyterPlugin* {
        return dynamic_cast<JupyterPlugin*>(plugins().requestViewPlugin(getKind()));
    };

    const auto numberOfDatasets = datasets.count();

    if (numberOfDatasets >= 1 && PluginFactory::areAllDatasetsOfTheSameType(datasets, PointType)) {
        auto pluginTriggerAction = new PluginTriggerAction(const_cast<JupyterPluginFactory*>(this), this, "Jupyter Plugin", "Jupyter bridge", getIcon(), [this, getPluginInstance, datasets](PluginTriggerAction& pluginTriggerAction) -> void {
            for (auto dataset : datasets)
                getPluginInstance();
        });

        pluginTriggerActions << pluginTriggerAction;
    }

    return pluginTriggerActions;
}
