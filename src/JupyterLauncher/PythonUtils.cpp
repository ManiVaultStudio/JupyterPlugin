#include "PythonUtils.h"

#include <QProcess>
#include <QRegularExpression>

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