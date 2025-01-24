#include "UserDialogActions.h"

#include "JupyterLauncher.h"

LauncherDialog::LauncherDialog(QWidget* parent, JupyterLauncher* launcherLauncher) :
    QDialog(parent),
    _interpreterFileAction(this, "Python interpreter"),
    _okButton(this, "Ok"),
    _doNotShowAgainButton(this, "Do not show again"),
    _launcherLauncher(launcherLauncher)
{
    setWindowTitle(tr("Jupyter Launcher Dialog"));
    resize(600, 100);

    const auto pythonFilter = pythonInterpreterFilters();
    _interpreterFileAction.setNameFilters(pythonFilter);
    _interpreterFileAction.setUseNativeFileDialog(true);
    _interpreterFileAction.getFilePathAction().setText("Python interpreter");
    _interpreterFileAction.setFilePath(_launcherLauncher->getPythonInterpreterPath());

    auto* layout = new QGridLayout();
    layout->setContentsMargins(10, 10, 10, 10);

    QLabel* infoText = new QLabel(this);
    infoText->setText("Please provide a path to a python interpreter, i.e., the environment you want to use.");

    layout->addWidget(infoText, 0, 0, 1, 5);
    layout->addWidget(_interpreterFileAction.createWidget(this), 1, 0, 1, 5);
    layout->addWidget(_okButton.createWidget(this), 2, 4, 1, 1, Qt::AlignRight);
    layout->addWidget(_doNotShowAgainButton.createWidget(this), 3, 4, 1, 1, Qt::AlignRight);

    setLayout(layout);

    connect(&_okButton, &mv::gui::TriggerAction::triggered, this, &QDialog::accept);
}
