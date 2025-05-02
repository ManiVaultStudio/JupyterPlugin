#include "ScriptDialogAction.h"

#include "JupyterLauncher.h"

#include <actions/FilePickerAction.h>
#include <actions/StringAction.h>
#include <actions/ToggleAction.h>
#include <actions/VerticalGroupAction.h>

#include <util/StyledIcon.h>

#include <QJsonArray>
#include <QVBoxLayout>
#include <QWidget>

ScriptDialog::ScriptDialog(QWidget* parent, const QJsonObject json, const QString scriptPath) :
    QDialog(parent),
    _okButton(this, "Run script"),
    _argumentActions(),
    _scriptPath(scriptPath),
    _scriptParams(),
    _argumentMap()
{
    setWindowTitle(json["name"].toString());
    setWindowIcon(mv::util::StyledIcon("gears"));

    auto* layout = new QGridLayout(this);
    layout->setContentsMargins(10, 10, 10, 10);
    int row = 0;

    if (json.contains("arguments") && json["arguments"].isArray()) {
        QJsonArray arguments = json["arguments"].toArray();

        for (const QJsonValue& argument : arguments) {
            if (!argument.isObject()) continue;

            const QJsonObject argObj = argument.toObject();
            const QString arg        = argObj["argument"].toString();
            const QString name       = argObj["name"].toString();
            const QString type       = argObj["type"].toString();
            const bool required      = argObj["required"].toBool();

            _argumentMap.insert({ arg, "" });

            if (type == "file-in") {
                auto widgetAction = _argumentActions.emplace_back(new mv::gui::FilePickerAction(this, name));
                auto filePickerAction = static_cast<mv::gui::FilePickerAction*>(widgetAction);
                filePickerAction->setUseNativeFileDialog(true);
                filePickerAction->getFilePathAction().setText(name);
                layout->addWidget(widgetAction->createWidget(this), ++row, 0, 1, -1);

                connect(filePickerAction, &mv::gui::FilePickerAction::filePathChanged, this, [this, arg](const QString filePath) {
                    _argumentMap[arg] = filePath;
                    });
            }
            else if (type == "file-out") {
                // TODO
            }
            else if (type == "str") {
                auto widgetAction = _argumentActions.emplace_back(new mv::gui::StringAction(this, name));
                auto stringAction = static_cast<mv::gui::StringAction*>(widgetAction);
                layout->addWidget(widgetAction->createLabelWidget(this), ++row, 0, 1, 1);
                layout->addWidget(widgetAction->createWidget(this), row, 1, 1, -1);

                connect(stringAction, &mv::gui::StringAction::stringChanged, this, [this, arg](const QString string) {
                    _argumentMap[arg] = string;
                    });

            }
            else if (type == "num") {
                // TODO
            }

        }
    }

    layout->addWidget(_okButton.createWidget(this), ++row, 0, 1, -1, Qt::AlignRight);

    setLayout(layout);

    connect(&_okButton, &mv::gui::TriggerAction::triggered, this, &QDialog::accept);
    connect(this, &QDialog::accepted, this, &ScriptDialog::runScript);
}

void ScriptDialog::runScript() {
    _scriptParams.clear();

    for (auto& [param, value] : _argumentMap) {
        _scriptParams.append(param);
        _scriptParams.append(value);
    }

    QString serr = {};
    QString sout = {};

    int exitCode = JupyterLauncher::runPythonScript(_scriptPath, sout, serr, _scriptParams);

    if (exitCode != 0) {
        qWarning() << sout;
        qWarning() << serr;
    }

}