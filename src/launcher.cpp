#include "launcher.h"

#include <QDesktopServices>
#include <QDir>

Launcher::Launcher(QObject *parent)
    : QObject(parent)
{
}

bool Launcher::open(const QUrl &url) const
{
    return QDesktopServices::openUrl(url);
}

QUrl Launcher::fileUrl(const QString &path) const
{
    return QUrl::fromLocalFile(path);
}

QString Launcher::displayPath(const QString &path) const
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
