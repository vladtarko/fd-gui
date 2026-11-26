#include "fdsearch.h"

#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QRegularExpression>
#include <QSet>
#include <QThreadPool>

#include "statworker.h"

FdSearch::FdSearch(ResultModel *model, QObject *parent)
    : QObject(parent)
    , m_model(model)
{
    m_debounce.setSingleShot(true);
    m_debounce.setInterval(120);

    connect(&m_debounce, &QTimer::timeout, this, &FdSearch::startSearch);
    connect(&m_process, &QProcess::readyRead, this, &FdSearch::onReadyRead);
    connect(&m_process, &QProcess::finished, this, &FdSearch::onFinished);
    connect(&m_process, &QProcess::errorOccurred, this, &FdSearch::onProcessError);
}

FdSearch::~FdSearch()
{
    killProcess();
    QThreadPool::globalInstance()->waitForDone();
}

void FdSearch::search(const QString &pattern, const QString &extension,
                      const QString &ignore, bool hidden)
{
    m_terms = parseTerms(pattern);
    m_extensions = parseExtensions(extension);
    m_excludes = parseIgnore(ignore);
    m_hidden = hidden;

    ++m_generation;
    killProcess();

    m_buffer.clear();
    m_pendingPaths.clear();
    m_model->clear();

    m_scanning = true;
    emit scanningChanged(true);

    m_debounce.start();
}

void FdSearch::cancel()
{
    ++m_generation;
    m_debounce.stop();
    killProcess();

    m_buffer.clear();
    m_pendingPaths.clear();

    m_scanning = false;
    emit scanningChanged(false);
}

void FdSearch::startSearch()
{
    if (!m_scanning) {
        return;
    }

    m_buffer.clear();
    m_pendingPaths.clear();

    QStringList args;
    args << QStringLiteral("--color") << QStringLiteral("never");
    args << QStringLiteral("--hyperlink=never");
    args << QStringLiteral("--print0");
    args << QStringLiteral("-F");

    if (m_terms.isEmpty() && m_extensions.isEmpty()) {
        args << QStringLiteral("--type") << QStringLiteral("directory");
        args << QStringLiteral("--max-depth") << QStringLiteral("1");
    }

    if (m_hidden) {
        args << QStringLiteral("--hidden");
    }

    for (const QString &exclude : m_excludes) {
        args << QStringLiteral("-E") << exclude;
    }

    for (const QString &extension : m_extensions) {
        args << QStringLiteral("-e") << extension;
    }

    for (int i = 1; i < m_terms.size(); ++i) {
        args << QStringLiteral("--and") << m_terms[i];
    }

    args << QStringLiteral("--");
    if (!m_terms.isEmpty()) {
        args << m_terms.first();
    }

    m_process.setProgram(QStringLiteral("fd"));
    m_process.setArguments(args);
    m_process.setProcessChannelMode(QProcess::SeparateChannels);
    m_activeGeneration = m_generation;
    m_process.start();
}

void FdSearch::onReadyRead()
{
    if (m_activeGeneration != m_generation) {
        return;
    }

    m_buffer += m_process.readAll();

    int separator = m_buffer.indexOf('\0');
    while (separator >= 0) {
        const QByteArray bytes = m_buffer.left(separator);
        m_buffer.remove(0, separator + 1);

        if (!bytes.isEmpty()) {
            QString path = QString::fromUtf8(bytes);
            if (!QDir::isAbsolutePath(path)) {
                path = QDir::cleanPath(QDir::current().absoluteFilePath(path));
            }
            m_pendingPaths.append(path);
        }

        if (m_pendingPaths.size() >= 128) {
            dispatchBatch();
        }

        separator = m_buffer.indexOf('\0');
    }
}

void FdSearch::onFinished(int exitCode, QProcess::ExitStatus status)
{
    Q_UNUSED(status)

    if (m_activeGeneration == 0 || m_activeGeneration != m_generation) {
        return;
    }

    onReadyRead();
    flushBatch();

    if (m_scanning) {
        m_scanning = false;
        emit scanningChanged(false);
    }

    const QByteArray stderrOutput = m_process.readAllStandardError().trimmed();
    if (exitCode != 0 && !stderrOutput.isEmpty()) {
        emit searchError(QString::fromUtf8(stderrOutput));
    }
}

void FdSearch::onResultsReady(quint64 generation, const QList<ResultItem> &items)
{
    if (generation != m_generation) {
        return;
    }

    m_model->addBatch(items);
}

void FdSearch::onProcessError(QProcess::ProcessError error)
{
    if (m_activeGeneration != m_generation) {
        return;
    }

    if (error == QProcess::FailedToStart) {
        emit searchError(m_process.errorString());
        m_scanning = false;
        emit scanningChanged(false);
    }
}

void FdSearch::killProcess()
{
    m_activeGeneration = 0;

    if (m_process.state() != QProcess::NotRunning) {
        m_process.kill();
        m_process.waitForFinished(1000);
    }
}

void FdSearch::flushBatch()
{
    dispatchBatch();
}

void FdSearch::dispatchBatch()
{
    if (m_pendingPaths.isEmpty()) {
        return;
    }

    const quint64 generation = m_generation;
    auto *worker = new StatWorker(generation, m_pendingPaths, this);

    connect(worker, &StatWorker::resultsReady, this, &FdSearch::onResultsReady,
            Qt::QueuedConnection);

    m_pendingPaths.clear();
    QThreadPool::globalInstance()->start(worker);
}

QStringList FdSearch::parseTerms(const QString &pattern) const
{
    return pattern.split(QRegularExpression(QStringLiteral("\\s+")), Qt::SkipEmptyParts);
}

QStringList FdSearch::parseExtensions(const QString &extension) const
{
    QStringList result;
    QSet<QString> seen;

    const QStringList parts = extension.split(QRegularExpression(QStringLiteral("[,\\s]+")),
                                              Qt::SkipEmptyParts);
    for (QString part : parts) {
        part = part.trimmed();
        while (part.startsWith(QLatin1Char('.'))) {
            part.remove(0, 1);
        }

        if (!part.isEmpty() && !seen.contains(part)) {
            seen.insert(part);
            result.append(part);
        }
    }

    return result;
}

QStringList FdSearch::parseIgnore(const QString &ignore) const
{
    QStringList result;

    const QString home = QDir::cleanPath(QDir::homePath());
    const QDir currentDir = QDir::current();

    const QStringList parts = ignore.split(QRegularExpression(QStringLiteral("[,\\r\\n]+")),
                                           Qt::SkipEmptyParts);
    for (QString pattern : parts) {
        pattern = pattern.trimmed();
        if (pattern.isEmpty() || pattern.startsWith(QLatin1Char('#'))) {
            continue;
        }

        if (pattern == QLatin1String("~")) {
            pattern = home;
        } else if (pattern.startsWith(QLatin1String("~/"))) {
            pattern = home + pattern.mid(1);
        }

        if (QDir::isAbsolutePath(pattern)) {
            const QString cleaned = QDir::cleanPath(pattern);
            const QString relative = currentDir.relativeFilePath(cleaned);

            if (!relative.isEmpty() && relative != QLatin1String(".")
                && !relative.startsWith(QLatin1String(".."))) {
                pattern = relative;
            } else {
                pattern = QFileInfo(cleaned).fileName();
            }
        }

        if (!pattern.isEmpty()) {
            result.append(pattern);
        }
    }

    return result;
}
