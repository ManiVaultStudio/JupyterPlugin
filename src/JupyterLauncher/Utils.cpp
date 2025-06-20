#include "Utils.h"

#include <optional>
#include <vector>

#include <QString>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTextStream>
#include <QDebug>

#define MD4QT_QT_SUPPORT
#include <md4qt/traits.h>
#include <md4qt/parser.h>
#include <md4qt/html.h>

static bool save_html_to_file(const QString& filePath, const QString& htmlContent) {
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "Failed to open file for writing:" << file.errorString();
        return false;
    }

    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);
    out << htmlContent;
    file.close();

    return true;
}

static std::optional<QString> _convert_md_to_html_impl(const QString& path_md_in) {
    if (!QFile::exists(path_md_in)) {
        qWarning() << "Input Markdown file does not exist:" << path_md_in;
        return {};
    }

    MD::Parser<MD::QStringTrait> p;

    auto doc = p.parse(path_md_in);

    if (doc->isEmpty()) {
        qWarning() << "Parsed Markdown document is empty or invalid.";
        return {};
    }

    const QString html = MD::toHtml(doc, /*wrapInBodyTag*/ false, {}, /*wrapInArticle*/ false);
    return html;
}

bool convert_md_to_html(const QString& path_md_in, const QString& path_html_out) {

    auto html = _convert_md_to_html_impl(path_md_in);

    if (!html.has_value()) {
        qWarning() << "Could not convert html to Markdown";
        return false;
    }
    
    return save_html_to_file(path_html_out, html.value());
}

QString convert_md_to_html(const QString& path_md_in) {
    auto html = _convert_md_to_html_impl(path_md_in);

    if (!html.has_value()) {
        qWarning() << "Could not convert html to Markdown";
        return "";
    }

    return html.value();
}

bool replace_json_entry(const QString& path_json, const QString& array_name, const QString& entry_name, const QString& new_text) {
    QFile file(path_json);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Failed to open file for reading:" << file.errorString();
        return false;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        qWarning() << "Failed to parse JSON:" << parseError.errorString();
        return false;
    }

    QJsonObject rootObj = doc.object();

    if (!containsMemberArray(rootObj, array_name)) {
        qWarning() << "Failed to handle json: Does not include array named " << array_name;
        return false;
    }

    QJsonArray jArray = rootObj.value(array_name).toArray();

    if (!jArray.isEmpty() && jArray[0].isObject()) {
        QJsonObject itemObj = jArray[0].toObject();
        itemObj["entry_name"] = new_text;   // Replace PLACEHOLDER
        jArray[0] = itemObj;                // Update array
        rootObj[array_name] = jArray;       // Update root
    }
    else {
        qWarning() << "JSON format is unexpected.";
        return false;
    }

    // Write back to disk
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "Failed to open file for writing:" << file.errorString();
        return false;
    }

    QJsonDocument outDoc(rootObj);
    file.write(outDoc.toJson(QJsonDocument::Indented));
    file.close();

    return true;
}

std::vector<QString> readStringArray(const QJsonObject& json, const QString& entry) {
    if (!containsMemberArray(json, entry))
        return {};

    std::vector<QString> res;

    const QJsonArray array = json[entry].toArray();
    for (const QJsonValue& val : array) {
        if (val.isString()) {
            res.push_back(val.toString());
        }
    }

    return res;
}
