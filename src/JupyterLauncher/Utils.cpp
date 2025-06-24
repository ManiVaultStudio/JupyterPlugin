#include "Utils.h"

#include <optional>
#include <vector>

#include <QCoreApplication>
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

    MD::Parser<MD::QStringTrait> parser;
    auto doc = parser.parse(path_md_in);

    if (doc->isEmpty()) {
        qWarning() << "Parsed Markdown document is empty or invalid.";
        return {};
    }

    return MD::toHtml(doc, 
        /*wrapInBodyTag*/ false, 
        /*hrefForRefBackImage*/ {},
        /*wrapInArticle*/ false);
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

        if (!containsMemberString(itemObj, entry_name)) {
            qWarning() << "Failed to handle json: Does not include entry named " << entry_name;
            return false;
        }

        itemObj[entry_name] = new_text;     // Replace PLACEHOLDER
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

static QString generateIdFromText(const QString& text) {
    // 1. Convert to lowercase
    QString id = text.toLower();

    // 2. Remove any HTML tags that might be in the header text
    id.remove(QRegularExpression("<[^>]*>"));

    // 3. Replace whitespace sequences with a single hyphen
    id = id.replace(QRegularExpression(R"(\s+)"), "-");

    // 4. Remove all characters that are not lowercase letters, numbers, or hyphens
    id = id.remove(QRegularExpression(R"([^a-z0-9\-])"));

    // 5. Collapse multiple hyphens into one (e.g., "a---b" -> "a-b")
    id = id.replace(QRegularExpression(R"(-{2,})"), "-");

    // 6. Remove leading or trailing hyphens
    id = id.remove(QRegularExpression(R"(^-|-$)"));

    return id;
}

static QString replaceHeaderIdsWithText(const QString& htmlInput) {
    QString result;
    int lastPos = 0;

    // This regex captures the essential parts of a header tag:
    // 1: The header level (1-6)
    // 2: The existing attributes (like class="...")
    // 3: The header's text content
    QRegularExpression headerRegex(R"(<h([1-6])([^>]*)>(.*?)<\/h\1>)");
    // Use the DotMatchesEverythingOption to correctly handle multi-line headers
    headerRegex.setPatternOptions(QRegularExpression::DotMatchesEverythingOption);

    QRegularExpressionMatchIterator it = headerRegex.globalMatch(htmlInput);

    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();

        // Append the content between the previous header and this one
        result.append(htmlInput.mid(lastPos, match.capturedStart() - lastPos));

        QString level = match.captured(1);
        QString attributes = match.captured(2);
        QString content = match.captured(3);

        // Generate the new ID from the header's text content
        QString newId = generateIdFromText(content);

        // Remove any old 'id' attribute from the attributes string
        attributes.remove(QRegularExpression(R"(\s+id="[^"]*")"));

        // Build the new header tag
        result.append(QString("<h%1").arg(level)); // <h2
        result.append(attributes);                // class="foo"
        if (!newId.isEmpty()) {
            result.append(QString(" id=\"%1\"").arg(newId)); // id="my-header"
        }
        result.append(">");                       // >
        result.append(content);                   // My Header
        result.append(QString("</h%1>").arg(level)); // </h2>

        // Update our position in the string
        lastPos = match.capturedEnd();
    }

    // Append any remaining text after the last header
    result.append(htmlInput.mid(lastPos));

    return result;
}

bool insert_md_into_json(const QString& path_json) {

    if (!path_json.endsWith(".json", Qt::CaseInsensitive)) {
        return false;
    }

    QFileInfo path_json_info(path_json);
    QString path_md = path_json_info.path() + "/" + path_json_info.completeBaseName() + ".md";
    QString path_assets = path_json_info.path() + "/assets";

    QString html = convert_md_to_html(path_md);

    // replace filename in ids
    html = replaceHeaderIdsWithText(html);

    // ensure all local assets are prefixed correctly
    QRegularExpression re(R"(<img\s+src\s*=\s*["'](?!file://)([^"']+)["'])", QRegularExpression::CaseInsensitiveOption);
    html.replace(re, R"(<img src=")" + QStringLiteral("file://") + R"(\1")");

    // replace and remove some characters to insert the entire html as a single line entry in json
    html.replace('"', '\"');
    html.remove('\n');
    html.remove('\r');

    if (!replace_json_entry(path_json, "tutorials", "url", QUrl::fromLocalFile(path_assets).toString()))
        return false;

    if (!replace_json_entry(path_json, "tutorials", "fullpost", html))
        return false;

    return true;
}

std::vector<QString> list_tutorial_files() {
    const QDir applicationDir(QCoreApplication::applicationDirPath());

    // Navigate to "tutorials" subdirectory
    QDir tutorialsDir(applicationDir.filePath("tutorials/JupyterLauncher"));

    if (!tutorialsDir.exists()) {
        qWarning() << "Tutorials folder does not exist:" << tutorialsDir.absolutePath();
        return {};
    }

    // Set filters: only JSON files
    QStringList jsonFiles = tutorialsDir.entryList(QStringList() << "*.json", QDir::Files | QDir::NoSymLinks);
    
    std::vector<QString> result;
    result.reserve(jsonFiles.size());

    for (const QString& f : jsonFiles) {
        result.push_back(tutorialsDir.filePath(f));
    }

    return result;
}