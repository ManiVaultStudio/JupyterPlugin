#pragma once

#include <QString>

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

// Helps to distinguish between python in a regular or conda directory and python in a venv
std::pair<bool, QString> getPythonHomePath(const QString& pyInterpreterPath);

void setPythonEnv(const QString& interpreterPath, const QString& interpreterVersion, const QString& connectionFilePath);

std::pair<bool, QString> isCondaEnvironmentActive();

QString createKernelDir();
