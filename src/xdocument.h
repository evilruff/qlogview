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

#ifndef _xDocument_h_
#define _xDocument_h_ 1

#include <QObject>
#include <QFile>
#include <QRegularExpression>
#include <QCache>

#include "xvaluelistmodel.h"

class xFileProcessor;

enum {
    findResultColumnLineNumber = 0,
    findResultColumnText = 1
};

enum {
    filterColumnChecked = 0,
    filterColumnText    = 1
};

struct lineData {
    quint64     position  = 0;
    int         length    = 0;
    bool    operator  == (const lineData & other) const {
        return ((position == other.position) && (length == other.length));
    }
  
    bool    operator  != (const lineData & other) const {
        return !(other == *this);
    }
};
typedef QVector<lineData>   linesData;

typedef struct {
    quint64     position    = 0;
    int         lineNumber  = 0;
    int         matchLength = 0;
    QString     line;
} searchResult;
typedef QVector<searchResult>   searchResults;

enum SearchRequestType {
    Undef            = 0,
    PatternSearch    = 1,
    RegExpSearch     = 2    
};

struct searchRequestItem {   
    SearchRequestType   type  = Undef;
    QStringMatcher      matcher;
    QRegularExpression  regexp;

    bool    operator ==(const searchRequestItem & other) const {
        if (type != other.type)
            return false;

        switch (type) {
            case PatternSearch:
                return ((matcher.pattern() == other.matcher.pattern()) && (matcher.caseSensitivity() == other.matcher.caseSensitivity()));
            case RegExpSearch:
                return (regexp.pattern() == other.regexp.pattern());
        }

        return false;
    }

    static  searchRequestItem       patternSearch(const QString & pattern) { 
        searchRequestItem   item;
        item.type           = PatternSearch;
        item.matcher        = QStringMatcher(pattern);
        return item;
    };

    static  searchRequestItem       regexpSearch(const QString & pattern) { 
        searchRequestItem   item;
        item.type           = RegExpSearch;
        item.regexp         = QRegularExpression(pattern);
        return item;
    };

    bool    isValid() const { return type != Undef; };
};

struct filterRule {
    searchRequestItem   filter;
    bool                isActive = false;

    bool    operator == (const filterRule & other) const {
        return filter == other.filter;
    }

    bool    isValid() const { return filter.isValid(); };
};
typedef QVector<filterRule>   filterRules;

typedef struct {
    QMap<int, int>       forwardIndex;
    QMap<int, int>       reverseIndex;
} documentIndex;

Q_DECLARE_METATYPE(lineData);
Q_DECLARE_METATYPE(linesData);

Q_DECLARE_METATYPE(searchResult);
Q_DECLARE_METATYPE(searchResults);

Q_DECLARE_METATYPE(searchRequestItem);

Q_DECLARE_METATYPE(filterRule);
Q_DECLARE_METATYPE(filterRules);

Q_DECLARE_METATYPE(documentIndex);

class xDocument: public QObject {
	Q_OBJECT

public:

	xDocument(QObject * pParent = nullptr);
	~xDocument();
    
    void        setFilePath(const QString & name);
    QString     filePath() const;
    QString     fileName() const;
       
    void                invalidate();

    int                 logicalLinesCount() const;
    quint64             logicalLineStart(int lineNumber, bool * bOk = nullptr) const;
    quint64             logicalLineEnd(int lineNumber, bool * bOk = nullptr) const;
    int                 logicalLineByPosition(quint64 pos) const;

    QByteArray          logicalLine(int lineNumber);
    QVector<QByteArray> logicalLines(int fromLine, int toLine);
    QByteArray          logicalLinesAsText(int fromLine, int toLine);

    int                 logicalToSourceLineNumber(int lineNumber) const;
    int                 sourceToLogicalLineNumber(int lineNumber) const;

    quint64             logicalLinePosition(int lineNumber) const;
    int                 logicalLinesBetweenPositions(quint64 start, quint64 stop) const;

    QByteArray          text(quint64 from, quint64 to);
             
    QAbstractTableModel *   findResults() const { return m_findResultsModel; };
    QAbstractTableModel *   filters() const { return m_filtersModel; };
       

    void                search(const searchRequestItem &  requestItem, const QByteArray & encoding, bool bStore, quint64 startPosition = 0, int maxOccurencies = 500);
    void                filter(const filterRules & rules, const QByteArray & encoding, bool bSetActive = true);

    void                setFilterRulesEnabled(bool bEnabled);
    bool                isFilterRulesEnabled() const;
    void                appendFilterRule(const searchRequestItem & item, bool bSetActive = false);
    void                removeFilterRule(const searchRequestItem & item);

    void                resetFilter();
    
    filterRule          toggleFilterRuleVisibility(const filterRule & rule);
    filterRule          setFilterRuleEnabled(const filterRule & rule, bool bEnabled);

    int                 currentOperationProgress() const;

    void                setAutoRefresh(bool b);
    bool                autoRefresh() const;
    
signals:

    void        layoutChanged();
    void        progressChanged(int);
    void        message(const QString & message, int timeout = 0);
    
public slots:

    void        onIndexDataReady(linesData index , bool bCompleted);
    void        onFilterDataReady(documentIndex data, bool bCompleted);
    void        onSearchResultsReady(searchResults results, bool bCompleted);

protected:

    void        initModels();


protected:

    int                     m_blockSize     = 1000000;
    int                     m_notifyPerLine = 1000;
    int                     m_lineCacheSize = 500;

    QVector<lineData>       m_fileIndex; 

    bool                    m_bFilterActive = false;
    documentIndex           m_filterIndex;
    
    QString                 m_filePath;
    QFile                   m_file;

    QCache<int, QByteArray> m_lineCache;

    xValueCollection<searchResult>       * m_findResultsModel   = nullptr;
    xValueCollection<filterRule>         * m_filtersModel       = nullptr;
  
    xFileProcessor      *   m_fileProcessor                 = nullptr;    
};

#endif