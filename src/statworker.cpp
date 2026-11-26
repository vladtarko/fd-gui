#include "statworker.h"

#include <QDir>
#include <QFileInfo>
#include <QMimeDatabase>
#include <QUrl>

StatWorker::StatWorker(quint64 generation, const QList<QString> &paths, QObject *parent)
    : QObject(parent)
    , m_generation(generation)
    , m_paths(paths)
{
    setAutoDelete(false);
}

void StatWorker::run()
{
    QMimeDatabase mimeDb;
    QList<ResultItem> items;
    items.reserve(m_paths.size());

    const QString home = QDir::cleanPath(QDir::homePath());

    for (const QString &path : m_paths) {
        const QFileInfo info(path);

        ResultItem item;
        item.path = path;
        item.name = info.fileName();
        item.url = QUrl::fromLocalFile(path);
        item.isDir = info.isDir();
        item.size = info.isDir() ? 0 : info.size();
        item.mtime = info.lastModified().toMSecsSinceEpoch();

        if (path == home) {
            item.displayPath = QStringLiteral("~");
        } else if (path.startsWith(home + QStringLiteral("/"))) {
            item.displayPath = QStringLiteral("~") + path.mid(home.size());
        } else {
            item.displayPath = path;
        }

        if (item.isDir) {
            item.typeName = QStringLiteral("Folder");
            item.iconName = QStringLiteral("folder");
        } else if (info.isSymLink()) {
            item.typeName = QStringLiteral("Symbolic link");
            item.iconName = QStringLiteral("symbolic-link");
        } else {
            const QMimeType mime = mimeDb.mimeTypeForFile(path, QMimeDatabase::MatchExtension);
            item.typeName = mime.comment().isEmpty() ? mime.name() : mime.comment();
            item.iconName = mime.iconName();
        }

        items.append(std::move(item));
    }

    emit resultsReady(m_generation, items);
}
