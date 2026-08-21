#pragma once

#include <actions/DirectoryPickerAction.h>
#include <actions/FilePickerAction.h>
#include <actions/GroupAction.h>
#include <actions/GroupsAction.h>
#include <actions/StringAction.h>
#include <actions/ToggleAction.h>
#include <actions/TriggerAction.h>

#include <QDialog>

class JupyterLauncher;
class QWidget;

class LauncherDialog : public QDialog
{
    Q_OBJECT
public:
    LauncherDialog(QWidget* parent, JupyterLauncher* launcherPlugin);

    mv::gui::ToggleAction& getDoNotShowAgainButton() { return _doNotShowAgainButton; }

    int getMode() const {
        return _mode;
    }

    void setMode(const int m) {
        _mode = m;
    }

private:
    mv::gui::FilePickerAction   _interpreterFileAction;
    mv::gui::DirectoryPickerAction   _kernelWorkingDirectoryAction;
    mv::gui::TriggerAction      _okButton;
    mv::gui::ToggleAction       _doNotShowAgainButton;
    mv::gui::StringAction       _moduleInfoText;
    mv::gui::GroupAction        _moduleInfoGroup;
    mv::gui::GroupsAction       _moduleInfoGroups;

    QWidget*                    _moduleInfoGroupsWidget = nullptr;
    const int                   _moduleInfoGroupsWidgetMinimumHeight = 32;  // this is a suboptimal setup but helps hide the text field
    int                         _moduleInfoGroupsWidgetMaximumHeight = -1;  // set in constructor based on content

    const int                   _dialogPreferredWidth = 700;

    int                         _mode = 0;  // 0 calls notebook, 1 calls init scripts

    JupyterLauncher*            _launcherPlugin = nullptr;
};
