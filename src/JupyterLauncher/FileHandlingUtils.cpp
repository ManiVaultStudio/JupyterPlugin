#include "FileHandlingUtils.h"

#include <optional>
#include <vector>

#include <QCoreApplication>
#include <QString>
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QDirIterator>
#include <QLibrary>

#ifdef WIN32
#define NOMINMAX
#include <windows.h>
#endif

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
        /*hrefForRefBackImage*/{},
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

std::vector<QString> list_tutorial_files(const QString& path_json) {
    const QDir applicationDir(QCoreApplication::applicationDirPath());

    // Navigate to "tutorials" subdirectory
    QDir tutorialsDir(applicationDir.filePath(path_json));

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

bool loadDynamicLibrary(const QFileInfo& pythonLibrary)
{
#ifdef WIN32
    // Adds a directory to the search path used to locate DLLs for the application.
    return SetDllDirectoryA(QString(pythonLibrary.dir().absolutePath() + "/").toUtf8().data());
#else
    // This approach seems cleaner but does not work on Windows
    QString sharedLibFilePath = pythonLibrary.absoluteFilePath();

    qDebug() << "Using python shared library at: " << sharedLibFilePath;
    if (!QLibrary::isLibrary(sharedLibFilePath))
        qWarning() << "Not a library: " << sharedLibFilePath;

    QLibrary lib(sharedLibFilePath);
    return lib.load();
#endif
}

QStringList findLibraryFiles(const QString& folderPath) {
    QStringList libraryFiles;
    QDirIterator it(folderPath, QDir::Files);

    while (it.hasNext()) {
        QString filePath = it.next();
        qDebug() << "current: " << filePath;
        if (QFileInfo(filePath).fileName().contains("JupyterPlugin3") && QLibrary::isLibrary(filePath)) {
            libraryFiles.append(filePath);
        }
    }

    return libraryFiles;
}