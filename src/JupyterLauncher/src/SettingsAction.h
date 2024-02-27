#pragma once

#include <actions/GroupAction.h>
#include <actions/DirectoryPickerAction.h>
#include <actions/FilePickerAction.h>

using namespace mv::gui;

class JupyterLauncher;

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
    
    DirectoryPickerAction& getPythonPathAction() { return _pythonPath; }
    //FilePickerAction& getConnectionFilePathAction() { return _connectionFilePath; }
    StringAction& getConnectionFilePathAction() { return _connectionFilePath; }

private:
    JupyterLauncher*    _jupyterLauncher;   
    DirectoryPickerAction _pythonPath;
    // due to issue with FilePickerAction use string for now
    //FilePickerAction _connectionFilePath;
    StringAction _connectionFilePath;
};