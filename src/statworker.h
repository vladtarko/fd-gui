#pragma once

#include <QList>
#include <QObject>
#include <QRunnable>
#include <QString>

#include "resultmodel.h"

class StatWorker : public QObject, public QRunnable {
    Q_OBJECT

public:
    StatWorker(quint64 generation, const QList<QString> &paths, QObject *parent = nullptr);

    void run() override;

signals:
    void resultsReady(quint64 generation, const QList<ResultItem> &items);

private:
    quint64 m_generation = 0;
    QList<QString> m_paths;
};
