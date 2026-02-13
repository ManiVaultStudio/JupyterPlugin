#pragma once

#include <vector>

#include <QFileInfo>
#include <QJsonObject>
#include <QString>

bool convertMarkdownToHtml(const QString& pathInMarkdown, const QString& pathOutHtml);

QString convertMarkdownToHtml(const QString& pathInMarkdown);

std::vector<QString> listTutorialFiles(const QString& subDir);

bool loadDynamicLibrary(const QFileInfo& pythonLibrary);

QStringList findLibraryFiles(const QString& folderPath);
