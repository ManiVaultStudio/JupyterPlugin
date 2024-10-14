#pragma once

#include <actions/GroupAction.h>
#include <actions/DirectoryPickerAction.h>
#include <actions/FilePickerAction.h>

#include <QtLogging>

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
    
    FilePickerAction& getPythonPathAction(const QString& pyversion) {
        if (pyversion == "3.11") {
            return _python311Path;
        }
        if (pyversion == "3.12") {
            return _python312Path;
        }

        qWarning() << "getPythonPathAction: Only 3.11 and 3.12 are supported but got " << pyversion << ". Defaulting to 3.12";
        return _python312Path;
    }
    FilePickerAction& getConnectionFilePathAction(const QString& pyversion) {
        if (pyversion == "3.11") {
            return _connection311FilePath;
        }
        if (pyversion == "3.12") {
            return _connection312FilePath;
        }

        qWarning() << "getConnectionFilePathAction: Only 3.11 and 3.12 are supported but got " << pyversion << ". Defaulting to 3.12";
        return _connection312FilePath;
    }

    void enableVersion(const QString& pyversion);

private:
    JupyterLauncher*    _jupyterLauncher;   
    FilePickerAction _python311Path;
    FilePickerAction _connection311FilePath;
    FilePickerAction _python312Path;
    FilePickerAction _connection312FilePath;
    // due to issue with FilePickerAction use string for now
    //FilePickerAction _connectionFilePath;

};