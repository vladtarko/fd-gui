#include "settingsstore.h"

#include <QDir>
#include <QFile>
#include <QMetaType>
#include <QStandardPaths>

namespace {
QString readText(const QSettings &settings, const QString &key)
{
    const QVariant value = settings.value(key, QString());
    if (value.typeId() == QMetaType::QStringList) {
        return value.toStringList().join(QLatin1Char(','));
    }
    return value.toString();
}
}

SettingsStore::SettingsStore(QObject *parent)
    : QObject(parent)
    , m_settings(QStandardPaths::writableLocation(QStandardPaths::ConfigLocation)
                     + QStringLiteral("/fd-gui/fd-gui.conf"),
                 QSettings::IniFormat)
{
    const QString configRoot = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
    const QString dir = configRoot + QStringLiteral("/fd-gui");
    QDir().mkpath(dir);

    const QString newPath = configRoot + QStringLiteral("/fd-gui/fd-gui.conf");
    const QString oldPath = configRoot + QStringLiteral("/fd-file-search/fd-file-search.conf");

    if (!QFile::exists(newPath) && QFile::exists(oldPath)) {
        QSettings oldSettings(oldPath, QSettings::IniFormat);
        m_settings.setValue(QStringLiteral("search/ignore"),
                            oldSettings.value(QStringLiteral("search/ignore")));
        m_settings.setValue(QStringLiteral("search/hidden"),
                            oldSettings.value(QStringLiteral("search/hidden"), false));
        m_settings.setValue(QStringLiteral("search/extension"),
                            oldSettings.value(QStringLiteral("search/extension")));
        m_settings.sync();
    }

    m_ignore = readText(m_settings, QStringLiteral("search/ignore"));
    m_hidden = m_settings.value(QStringLiteral("search/hidden"), false).toBool();
    m_extension = readText(m_settings, QStringLiteral("search/extension"));
}

void SettingsStore::setIgnore(const QString &value)
{
    if (m_ignore == value) {
        return;
    }

    m_ignore = value;
    m_settings.setValue(QStringLiteral("search/ignore"), value);
    m_settings.sync();
    emit ignoreChanged();
}

void SettingsStore::setHidden(bool value)
{
    if (m_hidden == value) {
        return;
    }

    m_hidden = value;
    m_settings.setValue(QStringLiteral("search/hidden"), value);
    m_settings.sync();
    emit hiddenChanged();
}

void SettingsStore::setExtension(const QString &value)
{
    if (m_extension == value) {
        return;
    }

    m_extension = value;
    m_settings.setValue(QStringLiteral("search/extension"), value);
    m_settings.sync();
    emit extensionChanged();
}
