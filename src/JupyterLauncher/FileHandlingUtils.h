#pragma once

#include <vector>

#include <QFileInfo>
#include <QJsonObject>
#include <QString>

bool convert_md_to_html(const QString& path_md_in, const QString& path_md_out);

QString convert_md_to_html(const QString& path_md_in);

std::vector<QString> list_tutorial_files(const QString& subDir);

bool loadDynamicLibrary(const QFileInfo& pythonLibrary);

QStringList findLibraryFiles(const QString& folderPath);
