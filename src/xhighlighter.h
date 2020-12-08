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


#ifndef _xHighligher_h_
#define _xHighligher_h_ 1

#include <QObject>
#include <QTextCharFormat>
#include <QTextLayout>
#include <QRegularExpression>

#include "xvaluelistmodel.h"

enum {
    highlighterViewColumnVisible = 0,
    highlighterViewColumnColor   = 1,
    highlighterViewColumnData    = 2,
};

enum HighlighterType {
    PositionHighlighter = 0,
    PatternHighlighter  = 1,
    RegExtHighlighter   = 2
};

class   highlighterItem {
public:
    HighlighterType     type = PatternHighlighter;
    int                 from = -1;
    int                 to   = -1;
    QTextCharFormat     format;
    QStringMatcher      matcher;
    QRegularExpression  regexp;
    bool                isActive = true;

    bool operator == (const highlighterItem & item) const {
        if (type != item.type)
            return false;

        if (type == PatternHighlighter)
            return (matcher.pattern() == item.matcher.pattern());

        if (type == RegExtHighlighter)
            return (regexp.pattern() == item.regexp.pattern());

        if (type == PositionHighlighter)
            return ((from == item.from) && (to == item.to)); 

        return false;
    }

    QString     pattern() const {
        if (type == PatternHighlighter)
            return matcher.pattern();
        if (type == RegExtHighlighter)
            return regexp.pattern();
        if (type == PositionHighlighter)
            return QString("%1-%2").arg(qMin(from, to)).arg(qMax( from, to));

        return QString();
    }
};

class xHighlighter: public QObject {
	Q_OBJECT

public:
    xHighlighter(QObject * pParent = nullptr);
	~xHighlighter();

    void appendHighlight(const highlighterItem & item);
    void appendHighlight(const QStringMatcher & match, const QTextCharFormat & fmt);
    void appendHighlight(const QRegularExpression & match, const QTextCharFormat & fmt);
    void appendHighlight(int columnStart, int columnEnd, const QTextCharFormat & fmt);
   
    void setHighligherVisibility(const highlighterItem & item, bool bVisible);
    void toggleHighligherVisibility(const highlighterItem & item);
    void removeHighlighter(const highlighterItem & item);
    void removeAll();

    void setHighligherColor(const highlighterItem & item, const QColor & color);
   

    QVector<QTextLayout::FormatRange>  highlight(const QString & str) const;

    xValueCollection<highlighterItem> * highlighters() const {
        return m_highlighters;
    }

    QVector<QTextLayout::FormatRange>   highlight(const QString & str, const highlighterItem & item) const;


signals:

    void       highlightRulesChanged();

protected:

    xValueCollection<highlighterItem> *  m_highlighters = nullptr;
};
#endif