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

struct PythonScriptContent
{
    PythonScriptContent() = default;
    PythonScriptContent(const QString& scriptPathIn, const QJsonObject& scriptTextIn) : scriptPath(scriptPathIn), scriptText(scriptTextIn) {}
    QString scriptPath = {};
    QJsonObject scriptText = {};
};

class ScriptDialog : public QDialog
{
    Q_OBJECT
public:
    ScriptDialog(QWidget* parent, PythonScriptContent* scriptContent, JupyterLauncher* launcher);

    void populateDialog();

private:
    void runScript();

private:
    mv::gui::TriggerAction                  _okButton;
    QWidget*                                _okButtonWidget = nullptr;
    std::vector<mv::gui::WidgetAction*>     _argumentActions = {};
    std::unordered_map<QString, QString>    _argumentMap = {};

    PythonScriptContent*                    _scriptContent = nullptr;
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
     * @param scriptPath Script location
     * @param scriptText Script content
     * @param interpreterVersion Version of the scripting language
     * @param launcher pointer to Launcher plugin
     * @param parent Pointer to parent object (optional, default is nullptr)
     */
    explicit PythonScript(const QString& title, const Type& type, const QString& interpreterVersion, const QString& scriptPath, const QJsonObject& scriptText, JupyterLauncher* launcher, QObject* parent = nullptr);

    /** Runs the script */
    void run() override {
        auto dialog = new ScriptDialog(nullptr, &_scriptContent, _launcherPlugin);
        dialog->setAttribute(Qt::WA_DeleteOnClose);
        dialog->populateDialog();
        dialog->show();
    }

private:
    PythonScriptContent _scriptContent = {};
    JupyterLauncher*    _launcherPlugin = nullptr;
};
