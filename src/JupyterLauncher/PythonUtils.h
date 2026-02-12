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
