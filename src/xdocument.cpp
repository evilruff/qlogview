/**
 *  Copyright 2020 by Yuri Alexandrov <evilruff@gmail.com>
 *
 * This file is part of some open source application.
 *
 * Some open source application is free software: you can redistribute
 * it and/or modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * Some open source application is distributed in the hope that it will
 * be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with QLogView.  If not, see <http://www.gnu.org/licenses/>.
 *
 * @license GPL-3.0+ <http://spdx.org/licenses/GPL-3.0+>
 */

#include <QThread>
#include <QMetaMethod>
#include <QFileInfo>

#include "xdocument.h"
#include "xfileprocessor.h"
#include "xlog.h"

xDocument::xDocument(QObject * pParent):
    QObject(pParent) {
    qCDebug(logicDocument) << "xDocument: created";

    m_findResultsModel = new xValueCollection<searchResult>(this);
    m_filtersModel = new xValueCollection<filterRule>(this);

    m_lineCache.setMaxCost(m_lineCacheSize);
    m_fileProcessor = new xFileProcessor();
    m_fileProcessor->activate();
    
    connect(m_fileProcessor, &xFileProcessor::progressChanged, this, &xDocument::progressChanged); 
    connect(m_fileProcessor, &xFileProcessor::indexDataReady, this, &xDocument::onIndexDataReady, Qt::QueuedConnection);
    connect(m_fileProcessor, &xFileProcessor::filterDataReady, this, &xDocument::onFilterDataReady, Qt::QueuedConnection);
    connect(m_fileProcessor, &xFileProcessor::searchResultsReady, this, &xDocument::onSearchResultsReady, Qt::QueuedConnection);

    connect(this, &xDocument::layoutChanged, [this]() {
        m_findResultsModel->layoutChanged();
    });
    
    initModels();
}

xDocument::~xDocument() {
    m_fileProcessor->shutdown();    
    qCDebug(logicDocument) << "xDocument: destroyed";
}

void xDocument::setFilePath(const QString & name) {
    m_filePath = name;
}

QString xDocument::filePath() const {
    return m_filePath;
}

quint64             xDocument::logicalLineStart(int lineNumber, bool * bOk) const {
    lineNumber = logicalToSourceLineNumber(lineNumber);

    if ((lineNumber < 0) || (lineNumber > m_fileIndex.size() - 1)) {
        if (bOk) *bOk = false;
        return 0;
    }

    if (bOk) *bOk = true;

    return m_fileIndex[lineNumber].position;
}

quint64             xDocument::logicalLineEnd(int lineNumber, bool * bOk) const {
    lineNumber = logicalToSourceLineNumber(lineNumber);

    if ((lineNumber < 0) || (lineNumber > m_fileIndex.size() - 1)) {
        if (bOk) *bOk = false;
        return 0;
    }

    if (bOk) *bOk = true;

    return m_fileIndex[lineNumber].position + m_fileIndex[lineNumber].length - 1;
}

QByteArray     xDocument::logicalLine(int lineNumber) {
    lineNumber = logicalToSourceLineNumber(lineNumber);

    if ((lineNumber < 0) || (lineNumber > m_fileIndex.size() - 1))
        return QByteArray();

    if (m_lineCache.contains(lineNumber))
        return *m_lineCache[lineNumber];

    quint64 readFrom = m_fileIndex[lineNumber].position;
    quint64 readCount = m_fileIndex[lineNumber].length;

    m_file.seek(readFrom);
    QByteArray * dataReaded = new QByteArray();
    *dataReaded = m_file.read(readCount);
    m_lineCache.insert(lineNumber, dataReaded);

    return *dataReaded;
}

QByteArray           xDocument::text(quint64 from, quint64 to) {
    m_file.seek(from);
    QByteArray dataReaded = m_file.read(to-from);

    return dataReaded;
}

quint64 xDocument::logicalLinePosition(int lineNumber) const {
    lineNumber = logicalToSourceLineNumber(lineNumber);

    if ((lineNumber < 0) || (lineNumber >= m_fileIndex.size()))
        return 0;

     return m_fileIndex[lineNumber].position;
}

QByteArray          xDocument::logicalLinesAsText(int fromLine, int toLine) {
    QByteArray   result;

    fromLine = fromLine < 0 ? 0 : fromLine;
    fromLine = fromLine > (logicalLinesCount() - 1) ? logicalLinesCount() - 1 : fromLine;

    toLine = toLine < 0 ? 0 : toLine;
    toLine = toLine > (logicalLinesCount() - 1) ? logicalLinesCount() - 1 : toLine;

    for (int i = fromLine; i <= toLine; i++) {
        result.append(logicalLine(i));
    }

    return result;
}

QVector<QByteArray> xDocument::logicalLines(int from, int to) {
    QVector<QByteArray>     result;

    from = from < 0 ? 0 : from;
    from = from > (logicalLinesCount() - 1) ? logicalLinesCount() - 1 : from;

    to = to < 0 ? 0 : to;
    to = to > (logicalLinesCount() - 1) ? logicalLinesCount() - 1 : to;

    for (int i = from; i <= to; i++) {
        result << logicalLine(i);
    }

    return result;
}

int                 xDocument::logicalLineByPosition(quint64 pos) const {

    QVector<lineData>::const_iterator   it = std::upper_bound(m_fileIndex.begin(), m_fileIndex.end(), pos, [](quint64 pos, const lineData & item) {
        return pos < (item.position + item.length);
    });

    if (it == m_fileIndex.end())
        return -1;

    int nLineIndex = std::distance(m_fileIndex.begin(), it) - 1;
    if (nLineIndex < 0) nLineIndex = 0;

    if (m_bFilterActive) {
        QMap<int, int>::const_iterator  indexIterator = m_filterIndex.forwardIndex.find(nLineIndex);
        if (indexIterator == m_filterIndex.forwardIndex.end()) {
            return -1;
        }

        nLineIndex = indexIterator.value();
    }

    return nLineIndex;
}

int                 xDocument::logicalLinesBetweenPositions(quint64 start, quint64 stop) const {
    int lineStart = logicalLineByPosition(start);
    int lineEnd   = logicalLineByPosition(stop);

    if (lineStart == -1 || lineEnd == -1)
        return -1;

    return qAbs(lineStart - lineEnd)+1;
};

void        xDocument::invalidate() {
    emit    message(tr("Loading file..."));

    emit    layoutChanged();

    if (m_file.isOpen()) {
        m_file.close();
    }

    m_fileProcessor->interrupt();
    m_file.setFileName(m_filePath);
    m_file.open(QIODevice::ReadOnly);
    setFilterRulesEnabled(false);
    m_filterIndex = documentIndex();
    m_fileIndex.clear();

    static int          methodIndex = -1;
    static QMetaMethod  method;

    if (methodIndex == -1) {
        methodIndex = m_fileProcessor->metaObject()->indexOfMethod("createIndex(QString,int,int)");
        method = m_fileProcessor->metaObject()->method(methodIndex);
    }

    method.invoke(m_fileProcessor, Qt::QueuedConnection,
        Q_ARG(QString, m_filePath),
        Q_ARG(int, m_notifyPerLine),
        Q_ARG(int, m_blockSize));
}

int xDocument::logicalLinesCount() const
{
    return m_bFilterActive ? m_filterIndex.forwardIndex.size() : m_fileIndex.size();
}

void                xDocument::search(const searchRequestItem &  requestItem, const QByteArray & encoding, bool bStore, quint64 startPosition, int maxOccurencies) {
    if (m_fileProcessor->isBusy())
        return;

    emit message(tr("Searching..."));

    m_findResultsModel->clear();
    if (bStore) {
        m_filtersModel->appendItem(filterRule{ requestItem, false });
    }

    static int          methodIndex = -1;
    static QMetaMethod  method;

    if (methodIndex == -1) {
        methodIndex = m_fileProcessor->metaObject()->indexOfMethod("searchData(QString,QByteArray,searchRequestItem,quint64,int,int,int)");
        method = m_fileProcessor->metaObject()->method(methodIndex);
    }

    method.invoke(m_fileProcessor, Qt::QueuedConnection,
        Q_ARG(QString, m_filePath),
        Q_ARG(QByteArray, encoding),
        Q_ARG(searchRequestItem, requestItem),
        Q_ARG(quint64, startPosition),
        Q_ARG(int, maxOccurencies),
        Q_ARG(int, m_notifyPerLine),
        Q_ARG(int, m_blockSize));
}

void                xDocument::filter(const filterRules & rules, const QByteArray & encoding, bool bSetActive) {
    if (m_fileProcessor->isBusy()) {
        m_fileProcessor->interrupt();
    }

    resetFilter();

    emit message(tr("Applying selected filter..."));
    
    static int          methodIndex = -1;
    static QMetaMethod  method;

    if (methodIndex == -1) {
        methodIndex = m_fileProcessor->metaObject()->indexOfMethod("createFilter(QString,QByteArray,filterRules,int,int)");
        method = m_fileProcessor->metaObject()->method(methodIndex);
    }

    method.invoke(m_fileProcessor, Qt::QueuedConnection,
        Q_ARG(QString, m_filePath),
        Q_ARG(QByteArray, encoding),
        Q_ARG(filterRules, rules),
        Q_ARG(int, m_notifyPerLine),
        Q_ARG(int, m_blockSize));

    setFilterRulesEnabled(bSetActive);
}

void                xDocument::setAutoRefresh(bool b) {

    if (b == autoRefresh())
        return;

    if (b) {
        emit message(tr("Auto refresh enabled"));

        static int          methodIndex = -1;
        static QMetaMethod  method;

        if (methodIndex == -1) {
            methodIndex = m_fileProcessor->metaObject()->indexOfMethod("enabledWatch(QString,lineData,int)");
            method = m_fileProcessor->metaObject()->method(methodIndex);
        }

        method.invoke(m_fileProcessor, Qt::QueuedConnection,
            Q_ARG(QString, m_filePath),
            Q_ARG(lineData, (m_fileIndex.size() ? m_fileIndex.last() : lineData{0,0})),
            Q_ARG(int, 1000));
    }
    else {
        emit message(tr("Auto refresh disabled"));

        static int          methodIndex = -1;
        static QMetaMethod  method;

        if (methodIndex == -1) {
            methodIndex = m_fileProcessor->metaObject()->indexOfMethod("disableWatch()");
            method = m_fileProcessor->metaObject()->method(methodIndex);
        }

        method.invoke(m_fileProcessor, Qt::QueuedConnection);
    }
}

void                xDocument::resetFilter() {
    m_bFilterActive = false;
    m_filterIndex.forwardIndex.clear();
    m_filterIndex.reverseIndex.clear();
    emit layoutChanged();
}

void                xDocument::setFilterRulesEnabled(bool bEnabled) {
    if (m_bFilterActive != bEnabled) {
        m_bFilterActive = bEnabled;
        emit layoutChanged();
    }
}

void        xDocument::onSearchResultsReady(searchResults results, bool bCompleted) {
    m_findResultsModel->appendItems(results);    
    if (bCompleted) {
        emit message(tr("Search completed, %1 results found").arg(m_findResultsModel->rowCount()));
    }
}

void        xDocument::onFilterDataReady(documentIndex  data, bool bCompleted) {  
    {
        QMapIterator<int, int>   it(data.forwardIndex);
        while (it.hasNext()) { it.next();  m_filterIndex.forwardIndex[it.key()] = it.value(); };
    }
    {
        QMapIterator<int, int>   it(data.reverseIndex);
        while (it.hasNext()) { it.next(); m_filterIndex.reverseIndex[it.key()] = it.value(); };
    }

    if (bCompleted) {
        emit message(tr("Filter ready, %1 lines found").arg(m_filterIndex.forwardIndex.size()));
    }

    emit layoutChanged();
}

void        xDocument::onIndexDataReady(linesData data, bool bCompleted) {
 
    if (data.size() && m_fileIndex.size() && data.first() == m_fileIndex.last()) {
        data.removeFirst();
    }

    if (data.size()) {
        if (m_fileIndex.size()) {

            if  (data.first().position <= m_fileIndex.last().position) {
                quint64 lookupIndex = data.first().position;
                QVector<lineData>::reverse_iterator it = std::find_if(m_fileIndex.rbegin(), m_fileIndex.rend(), [lookupIndex](const lineData & item)->bool {
                    return item.position >= lookupIndex;
                });

                int nRemoveFromLine = std::distance(m_fileIndex.begin(), (it + 1).base());
                int nRemoveToLine = m_fileIndex.size() - 1;
                for (int i = nRemoveFromLine; i <= nRemoveToLine; i++) {
                    m_lineCache.remove(i);
                }

                m_fileIndex.erase((it+1).base(), m_fileIndex.end());                
            }
        }
         
        m_fileIndex.append(data);        
        emit layoutChanged();

        if (bCompleted) {
            emit message(tr("Document ready"), 3000);
        }
    }
}

int         xDocument::currentOperationProgress() const {
    return m_fileProcessor->currentProgress();
}

QString     xDocument::fileName() const {
    QFileInfo fi(m_filePath);
    return fi.fileName();
}

bool                xDocument::autoRefresh() const {
    return m_fileProcessor->isWatchEnabled();
};

filterRule          xDocument::setFilterRuleEnabled(const filterRule & rule, bool bEnabled) {
    int nIndex = m_filtersModel->indexOf(rule);
    if (nIndex >= 0) {
        filterRule currentItem = m_filtersModel->itemAt(nIndex);
        if (currentItem.isActive != bEnabled) {
            currentItem.isActive = bEnabled;
            m_filtersModel->setItem(nIndex, currentItem);
            return currentItem;
        }
    }

    return filterRule();
}

filterRule                xDocument::toggleFilterRuleVisibility(const filterRule & rule) {
    int nIndex = m_filtersModel->indexOf(rule);
    if (nIndex >= 0) {
        filterRule currentItem = m_filtersModel->itemAt(nIndex);
        currentItem.isActive = !currentItem.isActive;
        m_filtersModel->setItem(nIndex, currentItem);
        return currentItem;
    }
    return filterRule();
}

int                 xDocument::logicalToSourceLineNumber(int lineNumber) const {
    if (!m_bFilterActive)
        return lineNumber;

    QMap<int,int>::const_iterator it = m_filterIndex.reverseIndex.find(lineNumber);

    return (it == m_filterIndex.reverseIndex.end() ? -1 : it.value());
}

int                 xDocument::sourceToLogicalLineNumber(int lineNumber) const {
    if (!m_bFilterActive)
        return lineNumber;

    QMap<int, int>::const_iterator it = m_filterIndex.forwardIndex.find(lineNumber);

    return (it == m_filterIndex.forwardIndex.end() ? -1 : it.value());
}

bool                xDocument::isFilterRulesEnabled() const {
    return m_bFilterActive;
}

void         xDocument::initModels() {
    m_findResultsModel->setColumntCount(2);
    m_findResultsModel->setDataCallback([this](int row, int column, int /* role */) -> QVariant {
        const searchResult & item = m_findResultsModel->itemAt(row);
        switch (column) {
        case findResultColumnLineNumber:
            return item.lineNumber + 1;
        case findResultColumnText:
            return item.line.trimmed();
        }
        return QVariant();
    }, Qt::DisplayRole);

    m_findResultsModel->setHeaderCallback([this](int section, Qt::Orientation orientation, int /*role = Qt::DisplayRole*/) -> QVariant {
        if (orientation != Qt::Horizontal)
            return QVariant();

        switch (section) {
        case findResultColumnLineNumber:
            return QString(tr("Line"));
        case findResultColumnText:
            return QString(tr("Text"));
        }

        return QVariant();
    }, Qt::DisplayRole);

    m_findResultsModel->setFlagsCallback([this](int row, int /*column*/, Qt::ItemFlags defaultFlags) -> Qt::ItemFlags {
        const searchResult & item = m_findResultsModel->itemAt(row);
        if (logicalToSourceLineNumber(item.lineNumber) == -1) {
            return (defaultFlags & ~(Qt::ItemIsEnabled));
        }

        return defaultFlags;
    });

    m_filtersModel->setColumntCount(2);
    m_filtersModel->setDataCallback([this](int row, int column, int /*role*/) -> QVariant {
        const filterRule & filterItem = m_filtersModel->itemAt(row);
        switch (column) {
        case filterColumnText:
            return filterItem.filter.type == PatternSearch ? filterItem.filter.matcher.pattern() : filterItem.filter.regexp.pattern();
        }

        return QVariant();
    }, Qt::DisplayRole);

    m_filtersModel->setDataCallback([this](int row, int column, int /* role */) -> QVariant {
        const filterRule & filterItem = m_filtersModel->itemAt(row);
        if (column == filterColumnChecked) {
            return filterItem.isActive;
        }

        return QVariant();
    }, Qt::CheckStateRole);

    m_filtersModel->setFlagsCallback([this](int /*row*/, int column, Qt::ItemFlags defaultFlags) -> Qt::ItemFlags {

        if (column == filterColumnChecked) {
            return (defaultFlags | Qt::ItemIsUserCheckable);
        }

        return defaultFlags;
    });

    m_filtersModel->setHeaderCallback([this](int section, Qt::Orientation orientation, int /*role = Qt::DisplayRole*/) -> QVariant {
        if (orientation != Qt::Horizontal)
            return QVariant();

        switch (section) {

        case filterColumnChecked:
            return QString(tr("Enabled"));

        case filterColumnText:
            return QString(tr("Pattern"));
        }

        return QVariant();
    }, Qt::DisplayRole);

}

void                xDocument::appendFilterRule(const searchRequestItem & item, bool bSetActive) {
    filterRule rule;
    rule.filter     = item;
    rule.isActive   = bSetActive;
    
    if (!m_filtersModel->contains(rule)) {
        m_filtersModel->appendItem(rule);
    }
}

void                xDocument::removeFilterRule(const searchRequestItem & item) {
    filterRule rule;
    rule.filter = item;

    m_filtersModel->removeItem(rule);
}
