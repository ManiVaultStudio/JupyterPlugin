#include "PythonUtils.h"

#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QOperatingSystemVersion>
#include <QRegularExpression>
#include <QTemporaryDir>
#include <QTemporaryFile>

QString extractRegex(const QString& input, const QString& pattern, int group) {
    const QRegularExpression regex(pattern);
    const QRegularExpressionMatch match = regex.match(input);

    if (match.hasMatch()) {
        return match.captured(group); // Return the matched number
    }

    return "";
}

QString getPythonVersion(const QString& pythonInterpreterPath)
{
    QProcess process;
    process.setProgram(pythonInterpreterPath);
    process.setArguments({ "--version" });
    process.start();

    if (!process.waitForFinished(3000)) {  // Timeout after 3 seconds
        qWarning() << "Failed to get Python version";
        return "";
    }

    QString output = process.readAllStandardOutput().trimmed();
    if (output.isEmpty()) {
        output = process.readAllStandardError().trimmed();  // Some Python versions print to stderr
    }

    return extractVersionNumber(output);
}

std::pair<bool, QString> getPythonHomePath(const QString& pyInterpreterPath)
{
    // check if the last 
    const auto pyInterpreterInfo = QFileInfo(QDir::toNativeSeparators(pyInterpreterPath));
    QDir pyInterpreterParent     = pyInterpreterInfo.dir();
    QString pyHomePath           = pyInterpreterParent.absolutePath();
    bool isVenv                  = false;

    //In a venv python sits in a Script or bin dir dependent on os
    QString venvParent = "";
    if constexpr (QOperatingSystemVersion::currentType() == QOperatingSystemVersion::Windows)
        venvParent = "Scripts";
    else
        venvParent = "bin";

    // A Script/bin and a pyvenv.cfg in the dir above indicate a venv so take the parent
    if (pyInterpreterParent.dirName() == venvParent)
    {
        pyInterpreterParent.cdUp();
        if (pyInterpreterParent.exists("pyvenv.cfg"))
        {
            pyHomePath = pyInterpreterParent.absolutePath();
            isVenv = true;
        }
    }

    if (pyHomePath.endsWith("bin"))
        pyHomePath = pyHomePath.chopped(3); // Removes the last 3 characters ("bin")

    return { isVenv, QDir::toNativeSeparators(pyHomePath) };
}

// Emulate the environment changes from a venv activate script
// https://docs.python.org/3/library/venv.html
// https://docs.python.org/3/using/cmdline.html#envvar-PYTHONHOME
// https://docs.python.org/3/using/cmdline.html#envvar-PYTHONPATH
void setPythonEnv(const QString& interpreterPath, const QString& interpreterVersion, const QString& connectionFilePath)
{
    auto [isVenv, pythonPath] = getPythonHomePath(interpreterPath);

    if (isVenv) // contains "pyvenv.cfg"
        qputenv("VIRTUAL_ENV", pythonPath.toUtf8());
    else  // contains python interpreter executable
        qputenv("PYTHONHOME", pythonPath.toUtf8());

    if constexpr (QOperatingSystemVersion::currentType() == QOperatingSystemVersion::Windows) {
        pythonPath += "/Lib/site-packages";
    }
    else {
        if (pythonPath.endsWith("bin"))
            pythonPath = pythonPath.chopped(3); // Removes the last 3 characters ("bin")

        if (pythonPath.endsWith("/"))
            pythonPath = pythonPath.chopped(1);

        QString pythonVersion = "python" + QString(interpreterVersion);

        // PREFIX is something like -> /home/USER/miniconda3/envs/ENV_NAME
        // pythonPath -> "PREFIX/lib:PREFIX/lib/python3.11:PREFIX/lib/python3.11/site-packages"
        pythonPath = pythonPath + "/lib" + ":" +
            pythonPath + "/lib/" + pythonVersion + ":" +
            pythonPath + "/lib/" + pythonVersion + "/site-packages";
    }

    // Path to folder with installed packages
    // PYTHONPATH is essential for xeus interpreter to load as the xeus_python_shell
    qputenv("PYTHONPATH", QDir::toNativeSeparators(pythonPath).toUtf8());

    // In order to run python -m jupyter lab and access the MANIVAULT_JUPYTERPLUGIN_CONNECTION_FILE 
    // the env variable must be set in the current process.
    // Setting it in the child QProcess does not work for reasons that are unclear.
    qputenv("MANIVAULT_JUPYTERPLUGIN_CONNECTION_FILE", connectionFilePath.toUtf8());
}

std::pair<bool, QString> isCondaEnvironmentActive()
{
    if (!qEnvironmentVariableIsSet("CONDA_PREFIX"))
        return { false, "" };

    QString pythonInterpreterPath = QString::fromLocal8Bit(qgetenv("CONDA_PREFIX"));

    if constexpr (QOperatingSystemVersion::currentType() == QOperatingSystemVersion::Windows)
        pythonInterpreterPath += "/Scripts/python.exe";
    else // Linux/macOS
        pythonInterpreterPath += "/bin/python3";

    const QString givenInterpreterVersion = getPythonVersion(pythonInterpreterPath);

    if (givenInterpreterVersion.isEmpty())
        return { false, "" };

    return { true, givenInterpreterVersion.trimmed() };
}

QTemporaryDir createKernelDir()
{
    // 1. Create a marshalling directory with the correct kernel name "ManiVaultStudio"
    const QString templatePath = QDir::temp().filePath("ManiVaultStudio");
    QTemporaryDir kernelDir(templatePath);

    // 2. Unpack the kernel files from the resources into the marshalling directory 
    const QDir resourceDir(":/kernel-files/");
    const QStringList kernelList = resourceDir.entryList(QDir::Files);

    for (const QString& filename : kernelList) {
        const QString sourcePath = resourceDir.absoluteFilePath(filename);
        const QString destPath = kernelDir.filePath(filename);

        if (QFile::copy(sourcePath, destPath)) {
            QFile::setPermissions(destPath, QFile::ReadOwner | QFile::WriteOwner);
        }
        else {
            qWarning() << "Failed to copy kernel file:" << filename;
        }
    }

    return kernelDir;
}

PythonExecutionReturn runPythonCommand(const QStringList& params, const QString& pythonInterpreterPath, const bool verbose, const int waitForFinishedMSecs)
{
    if (verbose) {
        qDebug() << "Interpreter: " << pythonInterpreterPath;
        qDebug() << "Command: " << params.join(" ");
    }

    QProcess pythonProcess;
    PythonExecutionReturn pythonReturn = {};

    auto printOut = [verbose](const QString& output) {
        if (!verbose)
            return;

        for (const QString& outline : output.split("\n"))
            if (!outline.isEmpty())
                qDebug() << outline;
        };

    QObject::connect(&pythonProcess, &QProcess::readyReadStandardOutput, [printOut, &pythonProcess, &pythonReturn]() {
        pythonReturn.out = QString::fromUtf8(pythonProcess.readAllStandardOutput()).replace("\r\n", "\n");
        printOut(pythonReturn.out);
        });

    QObject::connect(&pythonProcess, &QProcess::readyReadStandardError, [printOut, &pythonProcess, &pythonReturn]() {
        pythonReturn.err = QString::fromUtf8(pythonProcess.readAllStandardError()).replace("\r\n", "\n");
        printOut(pythonReturn.err);
        });

    pythonProcess.start(pythonInterpreterPath, params);

    if (!pythonProcess.waitForStarted()) {
        pythonReturn.result = 2;
        pythonReturn.err = QString("Could not run python interpreter %1 ").arg(pythonInterpreterPath);
    }

    if (!pythonProcess.waitForFinished(waitForFinishedMSecs)) { // three minutes, as dependency installation might take its time...
        pythonReturn.result = 2;
        pythonReturn.err = QString("Running python interpreter %1 timed out").arg(pythonInterpreterPath);
    }

    if (pythonReturn.result != 2)
        pythonReturn.result = pythonProcess.exitCode();

    if (pythonReturn.result != QProcess::NormalExit)
        qWarning() << "Failed running script.";

    pythonProcess.deleteLater();

    return pythonReturn;
}

PythonExecutionReturn runPythonScript(const QString& scriptName, const QString& pythonInterpreterPath, const QStringList& params, const bool verbose)
{
    if (!verbose)
    {
        qDebug() << "runPythonScript:: Interpreter: " << pythonInterpreterPath;
        qDebug() << "runPythonScript:: Script: " << scriptName;
    }

    // 1. Prepare a python process with the python path
    auto scriptFile = QFile(scriptName);

    // 2. Place the script in a temporary file
    QByteArray scriptContent;
    if (scriptFile.open(QFile::ReadOnly)) {
        scriptContent = scriptFile.readAll();
        scriptFile.close();
    }

    QTemporaryFile tempFile;
    if (tempFile.open()) {
        tempFile.write(scriptContent);
        tempFile.close();
    }

    // 3. Run the script synchronously
    const auto pythonParams = QStringList({ tempFile.fileName() }) + params;

    return runPythonCommand(pythonParams, pythonInterpreterPath, false);
}
