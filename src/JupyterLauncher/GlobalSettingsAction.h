#pragma once

#include <PluginGlobalSettingsGroupAction.h>

#include <actions/FilePickerAction.h>
#include <actions/ToggleAction.h>

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
    mv::gui::FilePickerAction& getDefaultPythonPathAction() { return _defaultPythonPathAction; }
    mv::gui::ToggleAction& getDoNotShowAgainButton() { return _doNotShowAgainButton; }

private:
    mv::gui::FilePickerAction   _defaultPythonPathAction;       /** Default python path */
    mv::gui::ToggleAction       _doNotShowAgainButton;          /** Whether to show the interpreter path picker on start */
};