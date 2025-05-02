#pragma once

#include <actions/TriggerAction.h>
#include <actions/WidgetAction.h>

#include <unordered_map>
#include <vector>

#include <QDialog>
#include <QJsonObject>
#include <QString>
#include <QStringList>

class QWidget;

class ScriptDialog : public QDialog
{
    Q_OBJECT
public:
    ScriptDialog(QWidget* parent, const QJsonObject json, const QString scriptPath);

private:
    void runScript();

private:
    mv::gui::TriggerAction                  _okButton;
    std::vector<mv::gui::WidgetAction*>     _argumentActions;

    QString                                 _scriptPath;
    QStringList                             _scriptParams;
    std::unordered_map<QString, QString>    _argumentMap;
};
