#include "sync/application/to_delete_batches.hpp"
#include <QSet>
#include <algorithm>

namespace ydisquette {
namespace sync {

ToDeleteBatches computeToDeleteBatches(const QStringList& toDeletePaths) {
    ToDeleteBatches out;
    if (toDeletePaths.isEmpty()) return out;
    QSet<QString> folderPrefixes;
    for (const QString& rel : toDeletePaths) {
        int slash = rel.indexOf(QLatin1Char('/'));
        if (slash >= 0) {
            folderPrefixes.insert(rel.left(slash));
            QString dir = rel.left(rel.lastIndexOf(QLatin1Char('/')));
            if (!dir.isEmpty()) folderPrefixes.insert(dir);
        }
    }
    for (const QString& rel : toDeletePaths) {
        if (rel.indexOf(QLatin1Char('/')) < 0 && !folderPrefixes.contains(rel))
            out.rootFiles.append(rel);
    }
    QList<QString> sortedPrefixes = folderPrefixes.values();
    std::sort(sortedPrefixes.begin(), sortedPrefixes.end(),
              [](const QString& a, const QString& b) { return a.size() < b.size(); });
    for (const QString& p : sortedPrefixes) {
        bool hasParent = false;
        for (const QString& m : out.minimalFolders) {
            if (p == m || p.startsWith(m + QLatin1Char('/'))) {
                hasParent = true;
                break;
            }
        }
        if (!hasParent) {
            for (auto it = out.minimalFolders.begin(); it != out.minimalFolders.end(); ) {
                if (it->startsWith(p + QLatin1Char('/')) || *it == p)
                    it = out.minimalFolders.erase(it);
                else
                    ++it;
            }
            out.minimalFolders.append(p);
        }
    }
    return out;
}

}  // namespace sync
}  // namespace ydisquette
