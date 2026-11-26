#pragma once

#include <QColor>
#include <QFileSystemWatcher>
#include <QObject>
#include <QTimer>
#include <QVariantMap>

class DmsTheme : public QObject {
    Q_OBJECT

    Q_PROPERTY(QString mode READ mode NOTIFY paletteChanged)
    Q_PROPERTY(bool dark READ dark NOTIFY paletteChanged)
    Q_PROPERTY(QVariantMap colors READ colors NOTIFY paletteChanged)

public:
    explicit DmsTheme(QObject *parent = nullptr);

    QString mode() const { return m_mode; }
    bool dark() const { return m_mode != QStringLiteral("light"); }
    QVariantMap colors() const { return m_colors; }

    Q_INVOKABLE QColor color(const QString &role, const QColor &fallback = QColor()) const;

public slots:
    void reload();

signals:
    void paletteChanged();

private:
    QString m_path;
    QString m_mode = QStringLiteral("dark");
    QVariantMap m_colors;
    QFileSystemWatcher m_watcher;
    QTimer m_debounce;
};
