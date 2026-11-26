#pragma once

#include <QObject>
#include <QUrl>

class Launcher : public QObject {
    Q_OBJECT

public:
    explicit Launcher(QObject *parent = nullptr);

    Q_INVOKABLE bool open(const QUrl &url) const;
    Q_INVOKABLE QUrl fileUrl(const QString &path) const;
    Q_INVOKABLE QString displayPath(const QString &path) const;
};
