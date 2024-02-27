#pragma once

#include <PluginGlobalSettingsGroupAction.h>

#include <actions/DirectoryPickerAction.h>
#include <actions/FilePickerAction.h>

namespace mv {
    namespace plugin {
        class PluginFactory;
    }
}

/**
 * Global settings action class
 *
 * Action class for configuring global settings
 * 
 * This group action (once assigned to the plugin factory, see ExampleViewGLPluginFactory::initialize()) is 
 * added to the global settings panel, accessible through the file > settings menu.
 * 
 */
class GlobalSettingsAction : public mv::gui::PluginGlobalSettingsGroupAction
{
public:

    /**
     * Construct with pointer to \p parent object and \p pluginFactory
     * @param parent Pointer to parent object
     * @param pluginFactory Pointer to plugin factory
     */
    Q_INVOKABLE GlobalSettingsAction(QObject* parent, const mv::plugin::PluginFactory* pluginFactory);

public: // Action getters

    //mv::gui::FilePickerAction& getDefaultConnectionPathAction() { return _defaultConnectionPathAction; }
    mv::gui::StringAction& getDefaultConnectionPathAction() { return _defaultConnectionPathAction; }

    mv::gui::DirectoryPickerAction& getDefaultPythonPathAction() { return _defaultPythonPathAction; }

private:
    // due to some file picker bugs use string for now
    //mv::gui::FilePickerAction   _defaultConnectionPathAction;       /** Default connection file path */
    mv::gui::StringAction   _defaultConnectionPathAction;       /** Default connection file path */
    mv::gui::DirectoryPickerAction   _defaultPythonPathAction;       /** Default pathon path */
};