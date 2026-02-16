#pragma once

#include <QString>
#include <QStringList>
#include <QOperatingSystemVersion>

QString extractRegex(const QString& input, const QString& pattern, int group = 1);

inline QString extractVersionNumber(const QString& input) {
    return extractRegex(input, R"(Python\s+(\d+\.\d+.\d+))");
}

inline QString extractShortVersionNumber(const QString& input) {
    return extractRegex(input, R"(^(\d+\.\d+))");
}

inline QString extractPatchVersionNumber(const QString& input) {
    return extractRegex(input, R"(^\d+\.\d+\.(\d+)$)");
}

QString getPythonVersion(const QString& pythonInterpreterPath);

inline QStringList pythonInterpreterFilters()
{
    QStringList pythonFilter = {};
    if (QOperatingSystemVersion::currentType() == QOperatingSystemVersion::Windows)
        pythonFilter = { "Python interpreter (python*.exe)" };
    else
        pythonFilter = { "Python interpreter (python*)" };

    return pythonFilter;
}

struct PythonExecutionReturn
{
    int result = 0;
    QString out = {};
    QString err = {};
};

// Helps to distinguish between python in a regular or conda directory and python in a venv
std::pair<bool, QString> getPythonHomePath(const QString& pyInterpreterPath);

void setPythonEnv(const QString& interpreterPath, const QString& interpreterVersion, const QString& connectionFilePath);

std::pair<bool, QString> isCondaEnvironmentActive();

QString createKernelDir();

PythonExecutionReturn runPythonCommand(const QStringList& params, const QString& pythonInterpreterPath, const bool verbose = false, const int waitForFinishedMSecs = 180'000);

PythonExecutionReturn runPythonScript(const QString& scriptName, const QString& pythonInterpreterPath, const QStringList& params = {}, const bool verbose = false);

