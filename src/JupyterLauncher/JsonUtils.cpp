#include "JsonUtils.h"

#include "FileHandlingUtils.h"

#include <optional>
#include <vector>

#include <QCoreApplication>
#include <QString>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <QFile>
#include <QFileInfo>

std::optional<QJsonObject> readJson(const QString& pathJson) {
    QFile file(pathJson);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Failed to open file for reading:" << file.errorString();
        return std::nullopt;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        qWarning() << "Failed to parse JSON:" << parseError.errorString();
        return std::nullopt;
    }

    return doc.object();
}

bool replaceJsonEntry(const QString& pathJson, const QString& arrayName, const QString& entryName, const QString& newText) {
    QFile file(pathJson);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Failed to open file for reading:" << file.errorString();
        return false;
    }

    const QByteArray data = file.readAll();
    file.close();

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        qWarning() << "Failed to parse JSON:" << parseError.errorString();
        return false;
    }

    QJsonObject rootObj = doc.object();

    if (!containsMemberArray(rootObj, arrayName)) {
        qWarning() << "Failed to handle json: Does not include array named " << arrayName;
        return false;
    }

    QJsonArray jArray = rootObj.value(arrayName).toArray();

    if (!jArray.isEmpty() && jArray[0].isObject()) {
        QJsonObject itemObj = jArray[0].toObject();

        if (!containsMemberString(itemObj, entryName)) {
            qWarning() << "Failed to handle json: Does not include entry named " << entryName;
            return false;
        }

        itemObj[entryName] = newText;     // Replace PLACEHOLDER
        jArray[0] = itemObj;                // Update array
        rootObj[arrayName] = jArray;       // Update root
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

    const QJsonDocument outDoc(rootObj);
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

namespace
{
    QString generateIdFromText(const QString& text) {
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

    QString replaceHeaderIdsWithText(const QString& htmlInput) {
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
            lastPos = static_cast<int>(match.capturedEnd());
        }

        // Append any remaining text after the last header
        result.append(htmlInput.mid(lastPos));

        return result;
    }
}

bool insertMarkdownIntoJson(const QString& pathJson) {

    if (!pathJson.endsWith(".json", Qt::CaseInsensitive)) {
        return false;
    }

    const auto pathJsonInfo = QFileInfo (pathJson);
    const QString pathMarkdown = pathJsonInfo.path() + "/" + pathJsonInfo.completeBaseName() + ".md";
    const QString pathAssets = pathJsonInfo.path() + "/assets";

    QString html = convertMarkdownToHtml(pathMarkdown);

    // replace filename in ids
    html = replaceHeaderIdsWithText(html);

    // ensure all local assets are prefixed correctly
    const QRegularExpression re(R"(<img\s+src\s*=\s*["'](?!file://)([^"']+)["'])", QRegularExpression::CaseInsensitiveOption);
    html.replace(re, R"(<img src=")" + QStringLiteral("file://") + R"(\1")");

    // replace and remove some characters to insert the entire html as a single line entry in json
    html.replace('"', '\"');
    html.remove('\n');
    html.remove('\r');

    if (!replaceJsonEntry(pathJson, "tutorials", "url", QUrl::fromLocalFile(pathAssets).toString()))
        return false;

    if (!replaceJsonEntry(pathJson, "tutorials", "fullpost", html))
        return false;

    return true;
}
