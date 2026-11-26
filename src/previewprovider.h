#pragma once

#include <QObject>
#include <QRunnable>
#include <QString>
#include <QVariantMap>

class PreviewWorker : public QObject, public QRunnable {
    Q_OBJECT

public:
    PreviewWorker(int requestId, const QString &path, QObject *parent = nullptr);

    void run() override;

signals:
    void previewReady(int requestId, const QString &path, const QVariantMap &data);

private:
    int m_requestId = 0;
    QString m_path;
};

class PreviewProvider : public QObject {
    Q_OBJECT

public:
    explicit PreviewProvider(QObject *parent = nullptr);
    ~PreviewProvider() override;

    Q_INVOKABLE int request(const QString &path);

signals:
    void previewReady(int requestId, const QString &path, const QVariantMap &data);

private:
    int m_nextRequestId = 0;
};
