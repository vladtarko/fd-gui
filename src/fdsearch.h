#pragma once

#include <QByteArray>
#include <QObject>
#include <QProcess>
#include <QString>
#include <QTimer>

#include "resultmodel.h"

class FdSearch : public QObject {
    Q_OBJECT

    Q_PROPERTY(bool scanning READ scanning NOTIFY scanningChanged)

public:
    explicit FdSearch(ResultModel *model, QObject *parent = nullptr);
    ~FdSearch() override;

    bool scanning() const { return m_scanning; }

    Q_INVOKABLE void search(const QString &pattern, const QString &extension,
                            const QString &ignore, bool hidden);
    Q_INVOKABLE void cancel();

private slots:
    void startSearch();
    void onReadyRead();
    void onFinished(int exitCode, QProcess::ExitStatus status);
    void onResultsReady(quint64 generation, const QList<ResultItem> &items);
    void onProcessError(QProcess::ProcessError error);

signals:
    void scanningChanged(bool scanning);
    void searchError(const QString &message);

private:
    void killProcess();
    void flushBatch();
    void dispatchBatch();
    QStringList parseTerms(const QString &pattern) const;
    QStringList parseExtensions(const QString &extension) const;
    QStringList parseIgnore(const QString &ignore) const;

    ResultModel *m_model = nullptr;
    QProcess m_process;
    QTimer m_debounce;
    QByteArray m_buffer;
    QList<QString> m_pendingPaths;
    QStringList m_terms;
    QStringList m_extensions;
    QStringList m_excludes;
    bool m_hidden = false;
    quint64 m_generation = 0;
    quint64 m_activeGeneration = 0;
    bool m_scanning = false;
};
