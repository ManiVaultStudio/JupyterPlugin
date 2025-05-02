#pragma once

#include <actions/TriggerAction.h>
#include <actions/WidgetAction.h>

#include <vector>

#include <QDialog>
#include <QJsonObject>

class QWidget;

class ScriptDialog : public QDialog
{
    Q_OBJECT
public:
    ScriptDialog(QWidget* parent, const QJsonObject json);

private:
    mv::gui::TriggerAction              _okButton;
    std::vector<mv::gui::WidgetAction*> _argumentActions;
};
