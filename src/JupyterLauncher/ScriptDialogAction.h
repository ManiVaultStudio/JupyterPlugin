#pragma once

#include <actions/TriggerAction.h>
#include <actions/WidgetAction.h>
#include <util/Script.h>

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
    ScriptDialog(QWidget* parent, const QJsonObject& json, const QString& scriptPath, const QString& interpreterVersion, JupyterLauncher* launcher);

    void populateDialog();

private:
    void runScript();

private:
    mv::gui::TriggerAction                  _okButton;
    std::vector<mv::gui::WidgetAction*>     _argumentActions;

    QString                                 _interpreterVersion;
    QString                                 _scriptPath;
    QJsonObject                             _json;
    std::unordered_map<QString, QString>    _argumentMap;

    JupyterLauncher*                        _launcherPlugin = nullptr;
};

class PythonScript : public mv::util::Script
{
    Q_OBJECT

public:

    /**
     * Construct script with \p type and \p language
     * @param title Script title
     * @param type Script type
     * @param location Script location
     * @param languageVersion Version of the scripting language
     * @param parent Pointer to parent object (optional, default is nullptr)
     */
    explicit PythonScript(const QString& title, const Type& type, const QString& location, const QString& interpreterVersion, const QJsonObject& json, JupyterLauncher* launcher, QObject* parent = nullptr);

    /** Runs the script */
    void run() override {
        _dialog.populateDialog();
        _dialog.show();
    }

private:
    ScriptDialog _dialog;
};
