#pragma once

#include <actions/TriggerAction.h>
#include <actions/WidgetAction.h>

#include <unordered_map>
#include <vector>

#include <QDialog>
#include <QJsonObject>
#include <QString>

class QWidget;
class JupyterLauncher;

class ScriptDialog : public QDialog
{
    Q_OBJECT
public:
    ScriptDialog(QWidget* parent, const QJsonObject json, const QString scriptPath, const QString interpreterVersion, JupyterLauncher* launcher);

private:
    void runScript();

private:
    mv::gui::TriggerAction                  _okButton;
    std::vector<mv::gui::WidgetAction*>     _argumentActions;

    QString                                 _interpreterVersion;
    QString                                 _scriptPath;
    std::unordered_map<QString, QString>    _argumentMap;

    JupyterLauncher*                        _launcherPlugin = nullptr;
};
