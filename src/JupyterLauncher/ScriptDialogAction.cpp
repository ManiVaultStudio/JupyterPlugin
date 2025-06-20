#include "ScriptDialogAction.h"

#include "JupyterLauncher.h"
#include "Utils.h"

#include <Dataset.h>
#include <Set.h>

#include <actions/DatasetPickerAction.h>
#include <actions/DecimalAction.h>
#include <actions/FilePickerAction.h>
#include <actions/IntegralAction.h>
#include <actions/StringAction.h>

#include <util/StyledIcon.h>

#include <algorithm>
#include <unordered_set>

#include <QJsonArray>
#include <QGridLayout>
#include <QFileDialog>
#include <QStringList>
#include <QWidget>

inline static QString insertDotAfter3(const QString& v) {
    QString temp = v;
    temp.insert(1, ".");
    return temp;
}

PythonScript::PythonScript(const QString& title, const Type& type, const QString& location, const QString& interpreterVersion, const QJsonObject& json, JupyterLauncher* launcher, QObject* parent) :
    Script(title, type, Language::Python, mv::util::Version(insertDotAfter3(interpreterVersion)), location, parent),
    _dialog(nullptr, json, location, interpreterVersion, launcher)
{

}

ScriptDialog::ScriptDialog(QWidget* parent, const QJsonObject& json, const QString& scriptPath, const QString& interpreterVersion, JupyterLauncher* launcher) :
    QDialog(parent),
    _okButton(this, "Run script"),
    _argumentActions(),
    _interpreterVersion(interpreterVersion),
    _scriptPath(scriptPath),
    _json(json),
    _argumentMap(),
    _launcherPlugin(launcher)
{
    setWindowTitle(json["name"].toString());
    setWindowIcon(mv::util::StyledIcon("gears"));

    connect(&_okButton, &mv::gui::TriggerAction::triggered, this, &QDialog::accept);
    connect(this, &QDialog::accepted, this, &ScriptDialog::runScript);
}

void ScriptDialog::populateDialog() 
{
    auto* layout = new QGridLayout(this);
    layout->setContentsMargins(10, 10, 10, 10);
    int row = 0;

    if (containsMemberString(_json, "description")) {
        QString description = _json["description"].toString();

        auto widgetAction = _argumentActions.emplace_back(new mv::gui::StringAction(this, "Description"));
        auto stringAction = static_cast<mv::gui::StringAction*>(widgetAction);
        stringAction->setDefaultWidgetFlags(mv::gui::StringAction::WidgetFlag::Label);
        stringAction->setString(description);
        layout->addWidget(widgetAction->createLabelWidget(this), ++row, 0, 1, 1);
        layout->addWidget(widgetAction->createWidget(this), row, 1, 1, -1);
    }

    if (containsMemberArray(_json, "arguments")) {
        QJsonArray arguments = _json["arguments"].toArray();

        for (const QJsonValue& argument : arguments) {
            if (!argument.isObject()) continue;

            const QJsonObject argObj = argument.toObject();
            const QString arg = argObj["argument"].toString();
            const QString name = argObj["name"].toString();
            const QString type = argObj["type"].toString();
            const bool required = argObj["required"].toBool();

            _argumentMap.insert({ arg, "" });

            if (type == "file-in") {
                auto widgetAction = _argumentActions.emplace_back(new mv::gui::FilePickerAction(this, name));
                widgetAction->setSettingsPrefix(_launcherPlugin, "Scripts/file-in", false);

                auto filePickerAction = static_cast<mv::gui::FilePickerAction*>(widgetAction);
                filePickerAction->setUseNativeFileDialog(true);
                filePickerAction->getFilePathAction().setText(name);

                layout->addWidget(widgetAction->createWidget(this), ++row, 0, 1, -1);

                connect(filePickerAction, &mv::gui::FilePickerAction::filePathChanged, this, [this, arg](const QString filePath) {
                    _argumentMap[arg] = filePath;
                    });
            }
            else if (type == "file-out") {
                QString caption = "Save as...";
                QString directoryPath = _launcherPlugin->getSetting("Scripts/file-in", "").toString();
                QString filter = "Save as...";

                if (containsMemberString(argObj, "dialog-caption")) {
                    caption = argObj["dialog-caption"].toString();
                }

                if (containsMemberString(argObj, "dialog-filter")) {
                    filter = argObj["dialog-filter"].toString();
                }

                QString outputFileName = QFileDialog::getSaveFileName(
                    nullptr,
                    caption,
                    directoryPath,
                    filter
                );

                if (!outputFileName.isNull() && !outputFileName.isEmpty()) {
                    _launcherPlugin->setSetting("Scripts/file-in", QFileInfo(outputFileName).absolutePath());
                }

                if (containsMemberString(argObj, "file-extension")) {
                    QString fileExtension = argObj["file-extension"].toString();

                    if (!outputFileName.endsWith(fileExtension, Qt::CaseInsensitive)) {
                        outputFileName += QString(".%1").arg(fileExtension);
                    }

                }

                _argumentMap[arg] = outputFileName;
            }
            else if (type == "str") {
                auto widgetAction = _argumentActions.emplace_back(new mv::gui::StringAction(this, name));
                auto stringAction = static_cast<mv::gui::StringAction*>(widgetAction);
                layout->addWidget(widgetAction->createLabelWidget(this), ++row, 0, 1, 1);
                layout->addWidget(widgetAction->createWidget(this), row, 1, 1, -1);

                connect(stringAction, &mv::gui::StringAction::stringChanged, this, [this, arg](const QString& string) {
                    _argumentMap[arg] = string;
                    });

            }
            else if (type == "float") {
                auto widgetAction = _argumentActions.emplace_back(new mv::gui::DecimalAction(this, name));
                auto decimalAction = static_cast<mv::gui::DecimalAction*>(widgetAction);

                layout->addWidget(widgetAction->createLabelWidget(this), ++row, 0, 1, 1);
                layout->addWidget(decimalAction->createWidget(this), row, 1, 1, -1);

                if (containsMemberDouble(argObj, "default")) {
                    decimalAction->setValue(argObj["default"].toDouble());
                }

                if (containsMemberDouble(argObj, "range-min")) {
                    decimalAction->setMinimum(argObj["range-min"].toDouble());
                }

                if (containsMemberDouble(argObj, "range-max")) {
                    decimalAction->setMaximum(argObj["range-max"].toDouble());
                }

                if (containsMemberDouble(argObj, "step-size")) {
                    decimalAction->setSingleStep(argObj["step-size"].toDouble());
                }

                if (containsMemberDouble(argObj, "num-decimals")) {
                    decimalAction->setNumberOfDecimals(argObj["num-decimals"].toInt());
                }

                _argumentMap[arg] = QString::number(decimalAction->getValue());

                connect(decimalAction, &mv::gui::DecimalAction::valueChanged, this, [this, arg](const float value) {
                    _argumentMap[arg] = QString::number(value);
                    });

            }
            else if (type == "int") {
                auto widgetAction = _argumentActions.emplace_back(new mv::gui::IntegralAction(this, name));
                auto integralAction = static_cast<mv::gui::IntegralAction*>(widgetAction);

                layout->addWidget(widgetAction->createLabelWidget(this), ++row, 0, 1, 1);
                layout->addWidget(integralAction->createWidget(this), row, 1, 1, -1);

                if (containsMemberDouble(argObj, "default")) {
                    integralAction->setValue(argObj["default"].toInt());
                }

                if (containsMemberDouble(argObj, "range-min")) {
                    integralAction->setMinimum(argObj["range-min"].toInt());
                }

                if (containsMemberDouble(argObj, "range-max")) {
                    integralAction->setMaximum(argObj["range-max"].toInt());
                }

                _argumentMap[arg] = QString::number(integralAction->getValue());

                connect(integralAction, &mv::gui::IntegralAction::valueChanged, this, [this, arg](const int32_t value) {
                    _argumentMap[arg] = QString::number(value);
                    });

            }
            else if (type == "mv-data-in") {
                auto widgetAction = _argumentActions.emplace_back(new mv::gui::DatasetPickerAction(this, name));
                auto datasetPickerAction = static_cast<mv::gui::DatasetPickerAction*>(widgetAction);

                std::unordered_set<QString> allowed_types;

                if (containsMemberArray(argObj, "datatypes")) {
                    const QJsonArray datatypesArray = argObj["datatypes"].toArray();

                    for (const QJsonValue& val : datatypesArray) {
                        if (val.isString()) {
                            allowed_types.insert(val.toString());
                        }
                    }
                }

                datasetPickerAction->setFilterFunction([allowed_types](const mv::Dataset<mv::DatasetImpl>& dataset) -> bool {
                    if (allowed_types.empty())
                        return true;

                    return std::any_of(allowed_types.cbegin(), allowed_types.cend(), [&dataset](const QString& type) {
                        return dataset->getRawDataKind() == type;
                        });
                    });

                layout->addWidget(widgetAction->createLabelWidget(this), ++row, 0, 1, 1);
                layout->addWidget(datasetPickerAction->createWidget(this), row, 1, 1, -1);

                connect(datasetPickerAction, &mv::gui::DatasetPickerAction::datasetPicked, this, [this, arg](const mv::Dataset<mv::DatasetImpl>& dataset) {
                    _argumentMap[arg] = dataset.getDatasetId();
                    });

            }

        }
    }

    layout->addWidget(_okButton.createWidget(this), ++row, 0, 1, -1, Qt::AlignRight);

    setLayout(layout);
}


void ScriptDialog::runScript() {
    if (!_launcherPlugin) {
        return;
    }

    QStringList scriptParams;

    for (auto& [param, value] : _argumentMap) {
        scriptParams.append(param);
        scriptParams.append(value);
    }

    _launcherPlugin->runScriptInKernel(_scriptPath, _interpreterVersion, scriptParams);
}