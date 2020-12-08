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

#include <QTextLayout>

#include "xhighlighter.h"

xHighlighter::xHighlighter(QObject * pParent):
    QObject(pParent) {

    m_highlighters = new xValueCollection<highlighterItem>(this);
    m_highlighters->setColumntCount(3);
    m_highlighters->setDataCallback([this](int row, int column, int /* role */) -> QVariant {
        const highlighterItem & item = m_highlighters->itemAt(row);      
            switch (column) {            
            case highlighterViewColumnData:
                    return item.pattern();
            }
      
        return QVariant();
    }, Qt::DisplayRole);

    m_highlighters->setDataCallback([this](int row, int column, int /* role */) -> QVariant {
        const highlighterItem & item = m_highlighters->itemAt(row);
        switch (column) {
        case highlighterViewColumnData:
            return QColor(item.type == PatternHighlighter ? Qt::black : (item.type == RegExtHighlighter ? Qt::blue : Qt::darkGreen));
        }

        return QVariant();
    }, Qt::ForegroundRole);

    m_highlighters->setDataCallback([this](int row, int column, int /* role */) -> QVariant {
        const highlighterItem & item = m_highlighters->itemAt(row);
        if (column == highlighterViewColumnVisible) {
            return item.isActive;
        }

        return QVariant();
    }, Qt::CheckStateRole);

    m_highlighters->setDataCallback([this](int /*row*/, int column, int /* role */) -> QVariant {      
        switch (column) {
        case highlighterViewColumnVisible:
            return (int)(Qt::AlignHCenter | Qt::AlignVCenter);

        case highlighterViewColumnColor:
            return (int)(Qt::AlignHCenter | Qt::AlignVCenter);
        }
        return Qt::AlignLeft;
    }, Qt::TextAlignmentRole);
   
    m_highlighters->setDataCallback([this](int row, int column, int /*role*/) -> QVariant {
        const highlighterItem & item = m_highlighters->itemAt(row);

        switch (column) {
            case highlighterViewColumnColor:
                QPixmap pixmap(20, 20);
                pixmap.fill(item.format.foreground().color());
                return pixmap;
        }

        return QVariant();
    }, Qt::DecorationRole);

    m_highlighters->setFlagsCallback([this](int /*row*/, int column, Qt::ItemFlags defaultFlags) -> Qt::ItemFlags {
     
        if (column == highlighterViewColumnVisible) {
            return (defaultFlags | Qt::ItemIsUserCheckable);
        }

        return defaultFlags;
    });

    m_highlighters->setHeaderCallback([this](int section, Qt::Orientation orientation, int /*role = Qt::DisplayRole*/) -> QVariant {
        if (orientation != Qt::Horizontal)
            return QVariant();

        switch (section) {

        case highlighterViewColumnVisible:
            return QString(tr("Enabled"));

        case highlighterViewColumnColor:
            return QString(tr("Color"));

        case highlighterViewColumnData:
            return QString(tr("Search pattern"));
        }

        return QVariant();
    }, Qt::DisplayRole);    
}

xHighlighter::~xHighlighter() {

}

QVector<QTextLayout::FormatRange>  xHighlighter::highlight(const QString & str) const {
    QVector<QTextLayout::FormatRange> result;

    QVectorIterator<highlighterItem>      it(m_highlighters->items());
    while (it.hasNext()) {
        const highlighterItem & item = it.next();
        if (item.isActive) {
            result << highlight(str, item);
        }        
    }

    return result;
}

void xHighlighter::appendHighlight(const highlighterItem & item) {
    if (std::find(m_highlighters->items().begin(), m_highlighters->items().end(), item) == m_highlighters->items().end()) {
        m_highlighters->appendItem(item);
        emit highlightRulesChanged();
    }
}

void  xHighlighter::appendHighlight(const QStringMatcher & match, const QTextCharFormat & fmt) {
    highlighterItem item;
    item.type   = PatternHighlighter;
    item.format = fmt;
    item.matcher = match;

    if (std::find(m_highlighters->items().begin(), m_highlighters->items().end(), item) == m_highlighters->items().end()) {
        m_highlighters->appendItem(item);
        emit highlightRulesChanged();
    }
}

void  xHighlighter::appendHighlight(int columnStart, int columnEnd, const QTextCharFormat & fmt) {
    highlighterItem item;

    item.type   = PositionHighlighter;
    item.format = fmt;
    item.from   = columnStart;
    item.to     = columnEnd;

    if (std::find(m_highlighters->items().begin(), m_highlighters->items().end(), item) == m_highlighters->items().end()) {
        m_highlighters->appendItem(item);
        emit highlightRulesChanged();
    }
}

void xHighlighter::setHighligherVisibility(const highlighterItem & item, bool bVisible) {
    int nIndex = m_highlighters->indexOf(item);
    highlighterItem currentItem = m_highlighters->itemAt(nIndex);
    if (currentItem.isActive != bVisible) {
        currentItem.isActive = bVisible;
        m_highlighters->setItem(nIndex, currentItem);
        emit highlightRulesChanged();
    }
}

void xHighlighter::removeHighlighter(const highlighterItem & item) {
    if (m_highlighters->removeItem(item)) {
        emit highlightRulesChanged();
    }
}

void xHighlighter::toggleHighligherVisibility(const highlighterItem & item) {
    int nIndex = m_highlighters->indexOf(item);
    if (nIndex >= 0) {
        highlighterItem currentItem = m_highlighters->itemAt(nIndex);
        currentItem.isActive = !currentItem.isActive;
        m_highlighters->setItem(nIndex, currentItem);
        emit highlightRulesChanged();
    }
}

void xHighlighter::setHighligherColor(const highlighterItem & item, const QColor & color) {
    int nIndex = m_highlighters->indexOf(item);
    if (nIndex >= 0) {
        highlighterItem currentItem = m_highlighters->itemAt(nIndex);
        if (currentItem.format.foreground().color() != color) {
            QBrush br = currentItem.format.foreground();
            br.setColor(color);
            currentItem.format.setForeground(br);
            m_highlighters->setItem(nIndex, currentItem);
            emit highlightRulesChanged();
        }
    }
}

void xHighlighter::removeAll() {
    if (m_highlighters->rowCount()) {
        m_highlighters->clear();
        emit highlightRulesChanged();
    }
}

void xHighlighter::appendHighlight(const QRegularExpression & match, const QTextCharFormat & fmt) {
    highlighterItem item;
    item.type   = RegExtHighlighter;
    item.format = fmt;
    item.regexp = match;
    
    if (std::find(m_highlighters->items().begin(), m_highlighters->items().end(), item) == m_highlighters->items().end()) {
        m_highlighters->appendItem(item);
        emit highlightRulesChanged();
    }
}

QVector<QTextLayout::FormatRange>   xHighlighter::highlight(const QString & str, const highlighterItem & item) const {
    QVector<QTextLayout::FormatRange>    result;

    if (!item.isActive)
       return result;

    int nStartIndex = 0;

    if (item.type == PositionHighlighter) {
        QTextLayout::FormatRange range;
        range.format = item.format;
        range.start = item.from;
        range.length = item.to - item.from;

        result << range;
    }
    else if (item.type == PatternHighlighter) {
        while ((nStartIndex = item.matcher.indexIn(str, nStartIndex)) != -1) {

            QTextLayout::FormatRange range;
            range.format = item.format;
            range.start = nStartIndex;
            range.length = item.matcher.pattern().length();

            result << range;

            nStartIndex += item.matcher.pattern().length();
        }
    }
    else if (item.type == RegExtHighlighter) {
        QRegularExpressionMatchIterator i = item.regexp.globalMatch(str);
        while (i.hasNext())
        {
            QRegularExpressionMatch match = i.next();

            QTextLayout::FormatRange range;
            range.format = item.format;
            range.start = match.capturedStart();
            range.length = match.capturedLength();

            result << range;
        }
    }

    return result;
}
