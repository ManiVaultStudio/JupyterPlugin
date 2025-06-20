#include "Utils.h"

#include <QString>
#include <QFile>
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

bool convert_md_to_html(const QString& path_md_in, const QString& path_html_out) {
    if (!QFile::exists(path_md_in)) {
        qWarning() << "Input Markdown file does not exist:" << path_md_in;
        return false;
    }

    MD::Parser<MD::QStringTrait> p;

    auto doc = p.parse(path_md_in);

    if (doc->isEmpty()) {
        qWarning() << "Parsed Markdown document is empty or invalid.";
        return false;
    }

    const QString html = MD::toHtml(doc);

    return save_html_to_file(path_html_out, html);
}
