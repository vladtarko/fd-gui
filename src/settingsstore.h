#pragma once

#include <QObject>
#include <QSettings>

class SettingsStore : public QObject {
    Q_OBJECT

    Q_PROPERTY(QString ignore READ ignore WRITE setIgnore NOTIFY ignoreChanged)
    Q_PROPERTY(bool hidden READ hidden WRITE setHidden NOTIFY hiddenChanged)
    Q_PROPERTY(QString extension READ extension WRITE setExtension NOTIFY extensionChanged)

public:
    explicit SettingsStore(QObject *parent = nullptr);

    QString ignore() const { return m_ignore; }
    bool hidden() const { return m_hidden; }
    QString extension() const { return m_extension; }

public slots:
    void setIgnore(const QString &value);
    void setHidden(bool value);
    void setExtension(const QString &value);

signals:
    void ignoreChanged();
    void hiddenChanged();
    void extensionChanged();

private:
    QSettings m_settings;
    QString m_ignore;
    bool m_hidden = false;
    QString m_extension;
};
