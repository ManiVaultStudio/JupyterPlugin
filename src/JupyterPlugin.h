#pragma once

#include <ViewPlugin.h>

#include <Dataset.h>
#include <widgets/DropWidget.h>

#include <PointData/PointData.h>

#include <QWidget>

#include "XeusKernel.h"

/** All plugin related classes are in the HDPS plugin namespace */
using namespace mv::plugin;

/** Drop widget used in this plugin is located in the HDPS gui namespace */
using namespace mv::gui;

/** Dataset reference used in this plugin is located in the HDPS util namespace */
using namespace mv::util;

class QLabel;

/**
 * Jupyter plugin class
 *
 * This view plugin class hosts a Xeus kernel and python interpreter.
 * On initialization the kernel is started followed by 
 *
 * To see the plugin in action, please follow the steps below:
 *
 * 1. Go to the visualization menu in HDPS
 * 2. Choose the Example view menu item, the view will be added to the layout
 *
 * @authors B. van Lew
 */
class JupyterPlugin : public ViewPlugin
{
    Q_OBJECT

public:

    /**
     * Constructor
     * @param factory Pointer to the plugin factory
     */
    JupyterPlugin(const PluginFactory* factory);

    /** Destructor */
    ~JupyterPlugin() override = default;
    
    /** This function is called by the core after the view plugin has been created */
    void init() override;

    /**
     * Invoked when a data event occurs
     * @param dataEvent Data event which occurred
     */
    void onDataEvent(mv::DatasetEvent* dataEvent);

protected:
    DropWidget*             _dropWidget;                /** Widget for drag and drop behavior */
    mv::Dataset<Points>   _points;                    /** Points smart pointer */
    QString                 _currentDatasetName;        /** Name of the current dataset */
    QLabel*                 _currentDatasetNameLabel;   /** Label that show the current dataset name */
    std::unique_ptr<XeusKernel> _xeusKernel;  /** the xeus kernel that manages the jupyter comms and the python interpreter*/
};

/**
 * Example view plugin factory class
 *
 * Note: Factory does not need to be altered (merely responsible for generating new plugins when requested)
 */
class ExampleViewPluginFactory : public ViewPluginFactory
{
    Q_INTERFACES(mv::plugin::ViewPluginFactory mv::plugin::PluginFactory)
    Q_OBJECT
    Q_PLUGIN_METADATA(IID   "nl.BioVault.JupyterPlugin"
                      FILE  "JupyterPlugin.json")

public:

    /** Default constructor */
    ExampleViewPluginFactory() {}

    /** Destructor */
    ~ExampleViewPluginFactory() override {}
    
    /** Creates an instance of the example view plugin */
    ViewPlugin* produce() override;

    /** Returns the data types that are supported by the example view plugin */
    mv::DataTypes supportedDataTypes() const override;

    /**
     * Get plugin trigger actions given \p datasets
     * @param datasets Vector of input datasets
     * @return Vector of plugin trigger actions
     */
    PluginTriggerActions getPluginTriggerActions(const mv::Datasets& datasets) const override;
};
