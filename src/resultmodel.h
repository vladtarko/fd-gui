#pragma once

#include <QAbstractListModel>
#include <QCollator>
#include <QUrl>

#include <vector>

struct ResultItem {
    QString path;
    QString name;
    QString displayPath;
    QUrl url;
    bool isDir = false;
    qint64 size = 0;
    qint64 mtime = 0;
    QString typeName;
    QString iconName;
};

Q_DECLARE_METATYPE(ResultItem)
Q_DECLARE_METATYPE(QList<ResultItem>)

class ResultModel : public QAbstractListModel {
    Q_OBJECT

public:
    enum Role {
        PathRole = Qt::UserRole + 1,
        NameRole,
        DisplayPathRole,
        UrlRole,
        IsDirRole,
        SizeRole,
        MTimeRole,
        TypeNameRole,
        IconNameRole
    };
    Q_ENUM(Role)

    enum class SortKey { Time, Name, Size, TypeName };
    Q_ENUM(SortKey)

    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
    Q_PROPERTY(SortKey sortKey READ sortKey WRITE setSortKeyEnum NOTIFY sortKeyChanged)
    Q_PROPERTY(bool foldersFirst READ foldersFirst WRITE setFoldersFirst NOTIFY foldersFirstChanged)

    explicit ResultModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    SortKey sortKey() const { return m_sortKey; }
    bool foldersFirst() const { return m_foldersFirst; }

    Q_INVOKABLE QString pathAt(int row) const;
    Q_INVOKABLE QUrl urlAt(int row) const;
    Q_INVOKABLE int rowForPath(const QString &path) const;

public slots:
    void clear();
    void addBatch(const QList<ResultItem> &items);
    void setSortKey(int key);
    void setSortKeyEnum(SortKey key);
    void setFoldersFirst(bool enabled);

signals:
    void countChanged();
    void sortKeyChanged();
    void foldersFirstChanged();

private:
    bool less(const ResultItem &a, const ResultItem &b) const;
    void resort();

    std::vector<ResultItem> m_items;
    SortKey m_sortKey = SortKey::Time;
    bool m_foldersFirst = true;
    QCollator m_collator;
};
