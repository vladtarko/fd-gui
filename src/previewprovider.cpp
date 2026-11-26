#include "previewprovider.h"

#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QLocale>
#include <QMimeDatabase>
#include <QProcess>
#include <QThreadPool>

namespace {
constexpr qint64 MaxPreviewBytes = 256 * 1024;
constexpr qint64 MaxErrorBytes = 16 * 1024;
constexpr qint64 ConverterTimeoutMs = 5000;
constexpr int PdfPreviewPages = 8;

struct ConversionResult {
    QByteArray output;
    QString error;
    bool ok = false;
    bool truncated = false;
};

QString formatSize(qint64 bytes)
{
    if (bytes < 1024) {
        return QStringLiteral("%1 B").arg(bytes);
    }

    const qreal value = static_cast<qreal>(bytes);
    if (bytes < 1024LL * 1024) {
        return QStringLiteral("%1 KiB").arg(QString::number(value / 1024.0, 'f', 1));
    }
    if (bytes < 1024LL * 1024 * 1024) {
        return QStringLiteral("%1 MiB").arg(QString::number(value / (1024.0 * 1024.0), 'f', 1));
    }
    return QStringLiteral("%1 GiB").arg(QString::number(value / (1024.0 * 1024.0 * 1024.0), 'f', 1));
}

QString shortPath(const QString &path)
{
    const QString home = QDir::cleanPath(QDir::homePath());

    if (path == home) {
        return QStringLiteral("~");
    }
    if (path.startsWith(home + QStringLiteral("/"))) {
        return QStringLiteral("~") + path.mid(home.size());
    }
    return path;
}

bool isTextMime(const QString &name)
{
    if (name.startsWith(QStringLiteral("text/"))) {
        return true;
    }

    static const QStringList extra = {
        QStringLiteral("application/json"),
        QStringLiteral("application/xml"),
        QStringLiteral("application/x-yaml"),
        QStringLiteral("application/yaml"),
        QStringLiteral("application/javascript"),
        QStringLiteral("application/x-shellscript"),
        QStringLiteral("application/x-desktop")
    };

    return extra.contains(name);
}

bool isMarkdownFile(const QString &mimeName, const QString &path)
{
    if (mimeName == QStringLiteral("text/markdown")) {
        return true;
    }

    static const QStringList suffixes = {
        QStringLiteral("md"), QStringLiteral("markdown"), QStringLiteral("mdown"),
        QStringLiteral("mkd"), QStringLiteral("mkdn")
    };
    return suffixes.contains(QFileInfo(path).suffix().toLower());
}

bool isDocxFile(const QString &mimeName, const QString &path)
{
    return mimeName == QStringLiteral("application/vnd.openxmlformats-officedocument.wordprocessingml.document")
        || QFileInfo(path).suffix().compare(QStringLiteral("docx"), Qt::CaseInsensitive) == 0;
}

ConversionResult runConverter(const QString &program, const QStringList &arguments)
{
    QProcess process;
    process.setProgram(program);
    process.setArguments(arguments);
    process.setProcessChannelMode(QProcess::SeparateChannels);
    process.start();

    if (!process.waitForStarted(1000)) {
        return {{}, process.errorString(), false, false};
    }

    QByteArray output;
    QByteArray errorOutput;
    QElapsedTimer timer;
    timer.start();
    bool hitLimit = false;

    while (process.state() != QProcess::NotRunning) {
        if (timer.elapsed() > ConverterTimeoutMs) {
            process.kill();
            process.waitForFinished(1000);
            return {{}, QStringLiteral("%1 timed out").arg(program), false, false};
        }

        process.waitForReadyRead(100);
        output += process.readAllStandardOutput();
        errorOutput += process.readAllStandardError();

        if (errorOutput.size() > MaxErrorBytes) {
            errorOutput.truncate(MaxErrorBytes);
        }

        if (output.size() > MaxPreviewBytes) {
            output.truncate(MaxPreviewBytes);
            hitLimit = true;
            process.kill();
            process.waitForFinished(1000);
            break;
        }
    }

    output += process.readAllStandardOutput();
    errorOutput += process.readAllStandardError();

    if (output.size() > MaxPreviewBytes) {
        output.truncate(MaxPreviewBytes);
        hitLimit = true;
    }

    if (!hitLimit && process.exitStatus() != QProcess::NormalExit) {
        return {{}, QStringLiteral("%1 crashed").arg(program), false, false};
    }

    if (!hitLimit && process.exitCode() != 0) {
        QString error = QString::fromUtf8(errorOutput).trimmed();
        if (error.isEmpty()) {
            error = QStringLiteral("%1 exited with code %2").arg(program).arg(process.exitCode());
        }
        return {{}, error, false, false};
    }

    return {output, QString(), true, hitLimit};
}

void readTextPreview(const QString &path, QVariantMap &data, const QString &kind)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        data.insert(QStringLiteral("kind"), QStringLiteral("unreadable"));
        data.insert(QStringLiteral("error"), file.errorString());
        return;
    }

    const QByteArray bytes = file.read(MaxPreviewBytes);
    data.insert(QStringLiteral("truncated"), file.size() > bytes.size());

    if (bytes.contains('\0')) {
        data.insert(QStringLiteral("kind"), QStringLiteral("binary"));
    } else {
        data.insert(QStringLiteral("kind"), kind);
        data.insert(QStringLiteral("content"), QString::fromUtf8(bytes));
    }
}
}

PreviewWorker::PreviewWorker(int requestId, const QString &path, QObject *parent)
    : QObject(parent)
    , m_requestId(requestId)
    , m_path(path)
{
    setAutoDelete(false);
}

void PreviewWorker::run()
{
    QVariantMap data;
    const QFileInfo info(m_path);

    data.insert(QStringLiteral("name"), info.fileName());
    data.insert(QStringLiteral("path"), m_path);
    data.insert(QStringLiteral("displayPath"), shortPath(m_path));
    data.insert(QStringLiteral("isDir"), info.isDir());
    data.insert(QStringLiteral("size"), info.isDir() ? 0 : info.size());
    data.insert(QStringLiteral("sizeText"), info.isDir() ? QString() : formatSize(info.size()));
    data.insert(QStringLiteral("modified"),
                QLocale::system().toString(info.lastModified(), QLocale::ShortFormat));

    QMimeDatabase mimeDb;
    const QMimeType mime = mimeDb.mimeTypeForFile(m_path, QMimeDatabase::MatchExtension);
    const QString mimeName = mime.name();
    data.insert(QStringLiteral("mime"), mimeName);
    data.insert(QStringLiteral("typeName"),
                info.isDir() ? QStringLiteral("Folder")
                             : (mime.comment().isEmpty() ? mimeName : mime.comment()));

    if (!info.exists()) {
        data.insert(QStringLiteral("kind"), QStringLiteral("missing"));
    } else if (info.isDir()) {
        QStringList names = QDir(m_path).entryList(
            QDir::AllEntries | QDir::NoDotAndDotDot | QDir::System,
            QDir::Name | QDir::DirsFirst);
        if (names.size() > 60) {
            names = names.mid(0, 60);
            names.append(QStringLiteral("..."));
        }
        data.insert(QStringLiteral("kind"), QStringLiteral("dir"));
        data.insert(QStringLiteral("entries"), names.join(QLatin1Char('\n')));
    } else if (mimeName.startsWith(QStringLiteral("image/"))) {
        data.insert(QStringLiteral("kind"), QStringLiteral("image"));
    } else if (mimeName == QStringLiteral("application/pdf")) {
        ConversionResult converted = runConverter(
            QStringLiteral("pdftotext"),
            {QStringLiteral("-f"), QStringLiteral("1"), QStringLiteral("-l"),
             QString::number(PdfPreviewPages), QStringLiteral("-nopgbrk"), m_path,
             QStringLiteral("-")});

        if (!converted.ok) {
            const QString firstError = converted.error;
            converted = runConverter(
                QStringLiteral("pdf2txt"),
                {QStringLiteral("--maxpages"), QString::number(PdfPreviewPages),
                 QStringLiteral("--outfile"), QStringLiteral("-"), m_path});
            if (!converted.ok) {
                converted.error = firstError + QLatin1Char('\n') + converted.error;
            }
        }

        if (converted.ok) {
            data.insert(QStringLiteral("kind"), QStringLiteral("text"));
            data.insert(QStringLiteral("content"), QString::fromUtf8(converted.output));
            data.insert(QStringLiteral("truncated"), converted.truncated);
            data.insert(QStringLiteral("note"),
                        QStringLiteral("PDF text preview — up to %1 pages").arg(PdfPreviewPages));
        } else {
            data.insert(QStringLiteral("kind"), QStringLiteral("unreadable"));
            data.insert(QStringLiteral("error"), converted.error);
        }
    } else if (isDocxFile(mimeName, m_path)) {
        ConversionResult converted = runConverter(
            QStringLiteral("pandoc"),
            {QStringLiteral("--from=docx"), QStringLiteral("--to=gfm"),
             QStringLiteral("--wrap=none"), m_path});

        if (converted.ok) {
            data.insert(QStringLiteral("kind"), QStringLiteral("markdown"));
            data.insert(QStringLiteral("content"), QString::fromUtf8(converted.output));
            data.insert(QStringLiteral("truncated"), converted.truncated);
        } else {
            data.insert(QStringLiteral("kind"), QStringLiteral("unreadable"));
            data.insert(QStringLiteral("error"), converted.error);
        }
    } else if (isMarkdownFile(mimeName, m_path)) {
        readTextPreview(m_path, data, QStringLiteral("markdown"));
    } else if (isTextMime(mimeName)) {
        readTextPreview(m_path, data, QStringLiteral("text"));
    } else {
        data.insert(QStringLiteral("kind"), QStringLiteral("binary"));
    }

    emit previewReady(m_requestId, m_path, data);
}

PreviewProvider::PreviewProvider(QObject *parent)
    : QObject(parent)
{
}

PreviewProvider::~PreviewProvider()
{
    QThreadPool::globalInstance()->waitForDone();
}

int PreviewProvider::request(const QString &path)
{
    if (path.isEmpty()) {
        return 0;
    }

    const int requestId = ++m_nextRequestId;
    auto *worker = new PreviewWorker(requestId, path, this);

    connect(worker, &PreviewWorker::previewReady, this,
            [this, worker](int id, const QString &path, const QVariantMap &data) {
                worker->deleteLater();
                emit previewReady(id, path, data);
            });

    QThreadPool::globalInstance()->start(worker);
    return requestId;
}
