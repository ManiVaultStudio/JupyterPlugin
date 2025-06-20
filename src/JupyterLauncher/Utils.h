#pragma once

#include <vector>

#include <QString>
#include <QJsonObject>

bool convert_md_to_html(const QString& path_md_in, const QString& path_md_out);

QString convert_md_to_html(const QString& path_md_in);

bool replace_json_entry(const QString& path_json, const QString& array_name, const QString& entry_name, const QString& new_text);

static inline bool containsMemberString(const QJsonObject& json, const QString& entry) {
    return json.contains(entry) && json[entry].isString();
}

static inline bool containsMemberArray(const QJsonObject& json, const QString& entry) {
    return json.contains(entry) && json[entry].isArray();
}

static inline bool containsMemberDouble(const QJsonObject& json, const QString& entry) {
    return json.contains(entry) && json[entry].isDouble();
}

std::vector<QString> readStringArray(const QJsonObject& json, const QString& entry);
