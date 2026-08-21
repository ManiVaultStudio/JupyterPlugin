#pragma once

#include <optional>
#include <vector>

#include <QString>
#include <QJsonObject>

inline bool containsMemberString(const QJsonObject& json, const QString& entry) {
    return json.contains(entry) && json[entry].isString();
}

inline bool containsMemberArray(const QJsonObject& json, const QString& entry) {
    return json.contains(entry) && json[entry].isArray();
}

inline bool containsMemberDouble(const QJsonObject& json, const QString& entry) {
    return json.contains(entry) && json[entry].isDouble();
}

std::optional<QJsonObject> readJson(const QString& pathJson);

std::vector<QString> readStringArray(const QJsonObject& json, const QString& entry);

bool replaceJsonEntry(const QString& pathJson, const QString& arrayName, const QString& entryName, const QString& newText);

bool insertMarkdownIntoJson(const QString& pathJson);
