#include "dmstheme.h"

#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

DmsTheme::DmsTheme(QObject *parent)
    : QObject(parent)
    , m_path(QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation)
             + QStringLiteral("/dms/dms-colors.json"))
{
    m_debounce.setSingleShot(true);
    m_debounce.setInterval(100);

    connect(&m_watcher, &QFileSystemWatcher::fileChanged, this, [this]() {
        m_debounce.start();
    });
    connect(&m_watcher, &QFileSystemWatcher::directoryChanged, this, [this]() {
        m_debounce.start();
    });
    connect(&m_debounce, &QTimer::timeout, this, &DmsTheme::reload);

    reload();
}

QColor DmsTheme::color(const QString &role, const QColor &fallback) const
{
    const QVariant value = m_colors.value(role);
    if (value.isValid() && value.canConvert<QColor>()) {
        return value.value<QColor>();
    }

    return fallback;
}

void DmsTheme::reload()
{
    QFile file(m_path);
    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }

    const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
    if (!document.isObject()) {
        return;
    }

    const QJsonObject root = document.object();
    const QString mode = root.value(QStringLiteral("mode")).toString(QStringLiteral("dark"));
    const QJsonObject roles = root.value(QStringLiteral("colors")).toObject()
                                  .value(mode).toObject();

    if (roles.isEmpty()) {
        return;
    }

    QVariantMap parsed;
    for (auto it = roles.constBegin(); it != roles.constEnd(); ++it) {
        const QString value = it.value().toString();
        if (value.isEmpty()) {
            continue;
        }

        const QColor color(value);
        if (!color.isValid()) {
            continue;
        }

        const QString key = it.key();
        parsed.insert(key, color);

        QString normalized = key;
        normalized.replace(QLatin1Char('-'), QLatin1Char('_'));
        if (normalized != key) {
            parsed.insert(normalized, color);
        }
    }

    if (parsed.isEmpty()) {
        return;
    }

    m_mode = mode;
    m_colors = parsed;

    const QString dir = QFileInfo(m_path).absolutePath();
    if (!m_watcher.directories().contains(dir)) {
        m_watcher.addPath(dir);
    }
    if (!m_watcher.files().contains(m_path)) {
        m_watcher.addPath(m_path);
    }

    emit paletteChanged();
}
