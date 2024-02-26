#pragma once

#include <actions/GroupAction.h>
#include <actions/FilePickerAction.h>

using namespace mv::gui;

class JupyterPlugin;

/**
 * Settings action class
 *
 * Action class for configuring settings
 */
class SettingsAction : public GroupAction
{
public:

    /**
     * Construct with \p parent object and \p title
     * @param parent Pointer to parent object
     * @param title Title
     */
    Q_INVOKABLE SettingsAction(QObject* parent, const QString& title);

public: // Action getters
    
    FilePickerAction& getConnectionFilePathAction() { return _connectionFilePath; }

private:
    JupyterPlugin*    _jupyterPlugin;       /** Pointer to Example OpenGL Viewer Plugin */
    FilePickerAction _connectionFilePath;
};