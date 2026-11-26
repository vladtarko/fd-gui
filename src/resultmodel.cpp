#include "resultmodel.h"

#include <algorithm>

ResultModel::ResultModel(QObject *parent)
    : QAbstractListModel(parent)
{
    m_collator.setCaseSensitivity(Qt::CaseInsensitive);
    m_collator.setNumericMode(true);
}

int ResultModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : static_cast<int>(m_items.size());
}

QVariant ResultModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= static_cast<int>(m_items.size())) {
        return {};
    }

    const ResultItem &item = m_items[static_cast<size_t>(index.row())];

    switch (role) {
    case Qt::DisplayRole:
    case NameRole:
        return item.name;
    case PathRole:
        return item.path;
    case DisplayPathRole:
        return item.displayPath;
    case UrlRole:
        return item.url;
    case IsDirRole:
        return item.isDir;
    case SizeRole:
        return item.size;
    case MTimeRole:
        return item.mtime;
    case TypeNameRole:
        return item.typeName;
    case IconNameRole:
        return item.iconName;
    default:
        return {};
    }
}

QHash<int, QByteArray> ResultModel::roleNames() const
{
    return {
        {PathRole, "path"},
        {NameRole, "name"},
        {DisplayPathRole, "displayPath"},
        {UrlRole, "url"},
        {IsDirRole, "isDir"},
        {SizeRole, "size"},
        {MTimeRole, "mtime"},
        {TypeNameRole, "typeName"},
        {IconNameRole, "iconName"}
    };
}

QString ResultModel::pathAt(int row) const
{
    if (row < 0 || row >= static_cast<int>(m_items.size())) {
        return {};
    }
    return m_items[static_cast<size_t>(row)].path;
}

QUrl ResultModel::urlAt(int row) const
{
    if (row < 0 || row >= static_cast<int>(m_items.size())) {
        return {};
    }
    return m_items[static_cast<size_t>(row)].url;
}

int ResultModel::rowForPath(const QString &path) const
{
    for (size_t i = 0; i < m_items.size(); ++i) {
        if (m_items[i].path == path) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

void ResultModel::clear()
{
    if (m_items.empty()) {
        return;
    }

    beginResetModel();
    m_items.clear();
    endResetModel();
    emit countChanged();
}

void ResultModel::addBatch(const QList<ResultItem> &items)
{
    if (items.isEmpty()) {
        return;
    }

    const int first = static_cast<int>(m_items.size());
    const int last = first + items.size() - 1;

    beginInsertRows(QModelIndex(), first, last);
    m_items.insert(m_items.end(), items.cbegin(), items.cend());
    endInsertRows();

    resort();
    emit countChanged();
}

void ResultModel::setSortKey(int key)
{
    const auto next = static_cast<SortKey>(std::clamp(key, 0, 3));
    if (next == m_sortKey) {
        return;
    }

    m_sortKey = next;
    emit sortKeyChanged();
    resort();
}

void ResultModel::setSortKeyEnum(SortKey key)
{
    setSortKey(static_cast<int>(key));
}

void ResultModel::setFoldersFirst(bool enabled)
{
    if (enabled == m_foldersFirst) {
        return;
    }

    m_foldersFirst = enabled;
    emit foldersFirstChanged();
    resort();
}

bool ResultModel::less(const ResultItem &a, const ResultItem &b) const
{
    if (m_foldersFirst && a.isDir != b.isDir) {
        return a.isDir;
    }

    switch (m_sortKey) {
    case SortKey::Time:
        if (a.mtime != b.mtime) {
            return a.mtime > b.mtime;
        }
        break;
    case SortKey::Name: {
        const int cmp = m_collator.compare(a.name, b.name);
        if (cmp != 0) {
            return cmp < 0;
        }
        break;
    }
    case SortKey::Size:
        if (a.size != b.size) {
            return a.size > b.size;
        }
        break;
    case SortKey::TypeName: {
        const int cmp = m_collator.compare(a.typeName, b.typeName);
        if (cmp != 0) {
            return cmp < 0;
        }
        break;
    }
    }

    return m_collator.compare(a.path, b.path) < 0;
}

void ResultModel::resort()
{
    if (m_items.size() < 2) {
        return;
    }

    layoutAboutToBeChanged();
    std::stable_sort(m_items.begin(), m_items.end(), [this](const ResultItem &a, const ResultItem &b) {
        return less(a, b);
    });
    layoutChanged();
}
