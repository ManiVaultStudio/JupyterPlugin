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

std::optional<QJsonObject> readJSON(const QString& path_json);

std::vector<QString> readStringArray(const QJsonObject& json, const QString& entry);

bool replace_json_entry(const QString& path_json, const QString& array_name, const QString& entry_name, const QString& new_text);

bool insert_md_into_json(const QString& path_json);
