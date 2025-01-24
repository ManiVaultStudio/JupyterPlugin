#pragma once

#include <actions/FilePickerAction.h>
#include <actions/ToggleAction.h>
#include <actions/TriggerAction.h>

#include <QDialog>
#include <QObject>
#include <QWidget>

class JupyterLauncher;

class LauncherDialog : public QDialog
{
    Q_OBJECT
public:
    LauncherDialog(QWidget* parent, JupyterLauncher* launcherPlugin);

    mv::gui::ToggleAction& getDoNotShowAgainButton() { return _doNotShowAgainButton; }

private:
    mv::gui::FilePickerAction   _interpreterFileAction;
    mv::gui::TriggerAction      _okButton;
    mv::gui::ToggleAction       _doNotShowAgainButton;

    JupyterLauncher*            _launcherLauncher;
};
