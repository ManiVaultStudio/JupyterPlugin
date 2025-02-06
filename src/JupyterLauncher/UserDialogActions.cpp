#include "UserDialogActions.h"

#include "JupyterLauncher.h"

#include <QWidget>

LauncherDialog::LauncherDialog(QWidget* parent, JupyterLauncher* launcherLauncher) :
    QDialog(parent),
    _interpreterFileAction(this, "Python interpreter"),
    _okButton(this, "Ok"),
    _doNotShowAgainButton(this, "Do not show again"),
    _moduleInfoText(this, "Python packages"),
    _moduleInfoGroup(this, "Necessary Python modules"),
    _moduleInfoGroups(this, "Info section container"),
    _launcherLauncher(launcherLauncher)
{
    setWindowTitle(tr("Jupyter Launcher"));

    const auto pythonFilter = pythonInterpreterFilters();
    _interpreterFileAction.setNameFilters(pythonFilter);
    _interpreterFileAction.setUseNativeFileDialog(true);
    _interpreterFileAction.getFilePathAction().setText("Python interpreter");
    _interpreterFileAction.setFilePath(_launcherLauncher->getPythonInterpreterPath());

    auto [isConda, pyVersion] = isCondaEnvironmentActive();

    auto interpreterFileActionWidget =_interpreterFileAction.createWidget(this);

    if(QOperatingSystemVersion::currentType() != QOperatingSystemVersion::Windows && isConda)
    {
        const QString pythonInterpreterPath = QString::fromLocal8Bit(qgetenv("CONDA_PREFIX")) + "/bin/python3";

        _launcherLauncher->setPythonInterpreterPath(pythonInterpreterPath);
        _interpreterFileAction.setFilePath(pythonInterpreterPath);
        _interpreterFileAction.setToolTip("On UNIX systems in a conda/mamba environment you cannot switch the python interpreter");
        interpreterFileActionWidget->setDisabled(true);

        qDebug() << "JupyterLauncher: Disable python environment selection";
    }
    else
        _interpreterFileAction.setToolTip("Select the Python interpreter...");


    _doNotShowAgainButton.setToolTip("You can always change this\nback in the global settings.");

    _moduleInfoText.setDefaultWidgetFlags(mv::gui::StringAction::WidgetFlag::Label);

    _moduleInfoText.setString(
        "Communication between Python and ManiVault relies on two modules that need to be installed: \n"
        " - mvstudio_kernel (Python kernel manager for communication)\n"
        " - mvstudio_data (Python data model for passing content)\n"
        "\n"
        "These modules will be automatically installed in the provided environment alongside their dependencies:\n"
        "   comm, numpy, jupyterlab_server, jupyter_server, jupyter_client, jupyterlab, xeus-python-shell, ipython\n"
        "\n"
        "The communication packages are provided as Python wheels in\n"
        "   {ManiVault Install directory}\\PluginDependencies\\JupyterLauncher\\py\n"
        "\n"
        "Their source code is always available at: github.com/ManiVaultStudio/JupyterPlugin"
    );

    _moduleInfoGroup.addAction(&_moduleInfoText);
    _moduleInfoGroup.setShowLabels(false);

    _moduleInfoGroups.setDefaultWidgetFlags(0);
    _moduleInfoGroups.addGroupAction(&_moduleInfoGroup);

    _moduleInfoGroupsWidget = _moduleInfoGroups.createWidget(this);
    _moduleInfoGroupsWidgetMaximumHeight = _moduleInfoGroupsWidget->maximumHeight();

    auto* layout = new QGridLayout();
    layout->setContentsMargins(10, 10, 10, 10);
    int row = 0;

    QLabel* infoText = new QLabel(this);
    infoText->setText("Please provide a path to a python interpreter, i.e., the environment you want to use.");

    layout->addWidget(infoText, row, 0, 1, 5);
    layout->addWidget(interpreterFileActionWidget, ++row, 0, 1, 5);
    layout->addWidget(_moduleInfoGroupsWidget, ++row, 0, 1, 5);
    layout->addWidget(_okButton.createWidget(this), ++row, 4, 1, 1, Qt::AlignRight);
    layout->addWidget(_doNotShowAgainButton.createWidget(this), ++row, 4, 1, 1, Qt::AlignRight);

    setLayout(layout);

    connect(&_okButton, &mv::gui::TriggerAction::triggered, this, &QDialog::accept);

    connect(&_moduleInfoGroup, &mv::gui::GroupAction::collapsed, this, [this]() {
        _moduleInfoGroupsWidget->setMaximumHeight(_moduleInfoGroupsWidgetMinimumHeight);
        adjustSize();
        });

    connect(&_moduleInfoGroup, &mv::gui::GroupAction::expanded, this, [this]() {
        _moduleInfoGroupsWidget->setMaximumHeight(_moduleInfoGroupsWidgetMaximumHeight);
        adjustSize();
        });

    connect(&_interpreterFileAction, &mv::gui::FilePickerAction::filePathChanged, _launcherLauncher, &JupyterLauncher::setPythonInterpreterPath);

    _moduleInfoGroup.collapse();

    setFixedWidth(_dialogPreferredWidth);
    resize(_dialogPreferredWidth, _dialogPreferredHeight);
}
