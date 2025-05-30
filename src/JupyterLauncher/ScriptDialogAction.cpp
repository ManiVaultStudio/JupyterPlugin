#include "ScriptDialogAction.h"

#include "JupyterLauncher.h"

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

ScriptDialog::ScriptDialog(QWidget* parent, const QJsonObject json, const QString scriptPath, const QString interpreterVersion, JupyterLauncher* launcher) :
    QDialog(parent),
    _okButton(this, "Run script"),
    _argumentActions(),
    _interpreterVersion(interpreterVersion),
    _scriptPath(scriptPath),
    _argumentMap(),
    _launcherPlugin(launcher)
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

                if (argObj.contains("dialog-caption")) {
                    caption = argObj["dialog-caption"].toString();
                }

                if (argObj.contains("dialog-filter")) {
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

                if (argObj.contains("file-extension")) {
                    QString fileExtention = argObj["file-extension"].toString();

                    if (!outputFileName.endsWith(fileExtention, Qt::CaseInsensitive)) {
                        outputFileName += QString(".%1").arg(fileExtention);
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
                auto widgetAction   = _argumentActions.emplace_back(new mv::gui::DecimalAction(this, name));
                auto decimalAction  = static_cast<mv::gui::DecimalAction*>(widgetAction);

                layout->addWidget(widgetAction->createLabelWidget(this), ++row, 0, 1, 1);
                layout->addWidget(decimalAction->createWidget(this), row, 1, 1, -1);

                if (argObj.contains("default")) {
                    decimalAction->setValue(argObj["default"].toDouble());
                }

                if (argObj.contains("range-min")) {
                    decimalAction->setMinimum(argObj["range-min"].toDouble());
                }

                if (argObj.contains("range-max")) {
                    decimalAction->setMaximum(argObj["range-max"].toDouble());
                }

                if (argObj.contains("step-size")) {
                    decimalAction->setSingleStep(argObj["step-size"].toDouble());
                }

                if (argObj.contains("num-decimals")) {
                    decimalAction->setNumberOfDecimals(argObj["step-size"].toInt());
                }

                _argumentMap[arg] = QString::number(decimalAction->getValue());

                connect(decimalAction, &mv::gui::DecimalAction::valueChanged, this, [this, arg](const float value) {
                    _argumentMap[arg] = QString::number(value);
                    });

            }
            else if (type == "int") {
                auto widgetAction   = _argumentActions.emplace_back(new mv::gui::IntegralAction(this, name));
                auto integralAction = static_cast<mv::gui::IntegralAction*>(widgetAction);

                layout->addWidget(widgetAction->createLabelWidget(this), ++row, 0, 1, 1);
                layout->addWidget(integralAction->createWidget(this), row, 1, 1, -1);

                if (argObj.contains("default")) {
                    integralAction->setValue(argObj["default"].toInt());
                }

                if (argObj.contains("range-min")) {
                    integralAction->setMinimum(argObj["range-min"].toInt());
                }

                if (argObj.contains("range-max")) {
                    integralAction->setMaximum(argObj["range-max"].toInt());
                }

                _argumentMap[arg] = QString::number(integralAction->getValue());

                connect(integralAction, &mv::gui::IntegralAction::valueChanged, this, [this, arg](const int32_t value) {
                    _argumentMap[arg] = QString::number(value);
                    });

            }
            else if (type == "mv-data-in") {
                auto widgetAction           = _argumentActions.emplace_back(new mv::gui::DatasetPickerAction(this, name));
                auto datasetPickerAction    = static_cast<mv::gui::DatasetPickerAction*>(widgetAction);

                std::unordered_set<QString> allowed_types;

                if (argObj.contains("datatypes")) {
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

    connect(&_okButton, &mv::gui::TriggerAction::triggered, this, &QDialog::accept);
    connect(this, &QDialog::accepted, this, &ScriptDialog::runScript);
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