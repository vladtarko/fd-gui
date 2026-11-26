#include <QtTest>
#include <QStandardPaths>

#include "resultmodel.h"
#include "settingsstore.h"

namespace {
ResultItem makeItem(const QString &path, bool isDir, qint64 mtime)
{
    ResultItem item;
    item.path = path;
    item.name = path.mid(path.lastIndexOf(QLatin1Char('/')) + 1);
    item.displayPath = path;
    item.url = QUrl::fromLocalFile(path);
    item.isDir = isDir;
    item.size = isDir ? 0 : 100;
    item.mtime = mtime;
    item.typeName = isDir ? QStringLiteral("Folder") : QStringLiteral("Text file");
    return item;
}
}

class BackendTest : public QObject {
    Q_OBJECT

private slots:
    void sorting();
    void settings();
};

void BackendTest::sorting()
{
    ResultModel model;

    model.setSortKey(static_cast<int>(ResultModel::SortKey::Name));
    model.addBatch({
        makeItem(QStringLiteral("/x/b.txt"), false, 400),
        makeItem(QStringLiteral("/x/zdir"), true, 200),
        makeItem(QStringLiteral("/x/a.txt"), false, 300),
        makeItem(QStringLiteral("/x/adir"), true, 100)
    });

    QCOMPARE(model.rowCount(), 4);
    QCOMPARE(model.pathAt(0), QStringLiteral("/x/adir"));
    QCOMPARE(model.pathAt(1), QStringLiteral("/x/zdir"));
    QCOMPARE(model.pathAt(2), QStringLiteral("/x/a.txt"));
    QCOMPARE(model.pathAt(3), QStringLiteral("/x/b.txt"));

    model.setSortKey(static_cast<int>(ResultModel::SortKey::Time));
    QCOMPARE(model.pathAt(0), QStringLiteral("/x/zdir"));
    QCOMPARE(model.pathAt(1), QStringLiteral("/x/adir"));
    QCOMPARE(model.pathAt(2), QStringLiteral("/x/b.txt"));
    QCOMPARE(model.pathAt(3), QStringLiteral("/x/a.txt"));

    model.setFoldersFirst(false);
    QCOMPARE(model.pathAt(0), QStringLiteral("/x/b.txt"));
    QCOMPARE(model.pathAt(1), QStringLiteral("/x/a.txt"));
    QCOMPARE(model.pathAt(2), QStringLiteral("/x/zdir"));
    QCOMPARE(model.pathAt(3), QStringLiteral("/x/adir"));
}

void BackendTest::settings()
{
    QStandardPaths::setTestModeEnabled(true);

    {
        SettingsStore store;
        store.setIgnore(QStringLiteral("node_modules"));
        store.setHidden(true);
        store.setExtension(QStringLiteral("pdf,md"));
    }

    SettingsStore restored;
    QCOMPARE(restored.ignore(), QStringLiteral("node_modules"));
    QCOMPARE(restored.hidden(), true);
    QCOMPARE(restored.extension(), QStringLiteral("pdf,md"));
}

QTEST_MAIN(BackendTest)

#include "backendtest.moc"
