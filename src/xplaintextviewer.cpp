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


#include <QScrollBar>
#include <QPainter>
#include <QTextLayout>
#include <QApplication>
#include <QStyle>
#include <QIcon>
#include <QTextCodec>
#include <QTextBoundaryFinder>
#include <QTime>

#include "xplaintextviewer.h"
#include "xlog.h"
#include "xscrollbar.h"

//------------------------------------------------------------------------
xPlainTextViewer::xPlainTextViewer(QWidget * pParent):
    QAbstractScrollArea(pParent) {
    setVerticalScrollBar(new xScrollBar(this));
    verticalScrollBar()->setMinimum(0);
    verticalScrollBar()->setSingleStep(1);

    setMinimumHeight(300);
    setMinimumWidth(500);
    
    qCDebug(logicViewer) << "xPlainTextViewer: created";

    m_highligher = new xHighlighter(this);
    connect(m_highligher, &xHighlighter::highlightRulesChanged, [this]() {
        viewport()->update();
    });

    setMouseTracking(true);

    m_bookmarkModel = new xValueCollection<documentBookmark>(this);
    
    initModels();

    m_codec = QTextCodec::codecForLocale();

    m_searchPanel = new xSearchWidget(viewport());
    
    connect(m_searchPanel, &xSearchWidget::searchChanged, this, &xPlainTextViewer::onSearchChanged);
    connect(m_searchPanel, &xSearchWidget::addFilter, this, &xPlainTextViewer::onAddFilter);
    connect(m_searchPanel, &xSearchWidget::findAll, this, &xPlainTextViewer::onFindAll);

    m_searchHighlighter.isActive = false;
    QTextCharFormat fmt;
    fmt.setBackground(Qt::yellow);
    fmt.setForeground(Qt::black);
    m_searchHighlighter.format = fmt;

    hideSearchPanel();  
}

xPlainTextViewer::~xPlainTextViewer() {
    qCDebug(logicViewer) << "xPlainTextViewer: destroyed";
}
//------------------------------------------------------------------------
void       xPlainTextViewer::setDocument(xDocument * pDocument) {
    if (m_document == pDocument)
        return;

    xScrollBar * pScrollBar = qobject_cast<xScrollBar*>(verticalScrollBar());

    if (m_document) {
        disconnect(m_document, nullptr, this, nullptr);
        if (pScrollBar) {
            pScrollBar->setMarksModel(nullptr);
        }
    }

    m_document = pDocument;
   
    if (pScrollBar) {
        pScrollBar->setMarksModel(bookmarks());
    }

    connect(m_document, &xDocument::layoutChanged, this, &xPlainTextViewer::onLayoutChanged);

    invalidate();
}

xDocument * xPlainTextViewer::document() const {
    return m_document;
}

void    xPlainTextViewer::onLayoutChanged() {
    setUpdatesEnabled(false);
    setMaximumScrollBarValue();
    if (m_bFollowTail) {
        verticalScrollBar()->setValue(verticalScrollBar()->maximum());
    }
    
    m_bookmarkModel->layoutChanged();
    setUpdatesEnabled(true);
}

void        xPlainTextViewer::setMaximumScrollBarValue() {
    QScrollBar * pScrollBar = verticalScrollBar();
    int nLinesToFitFromBottom = logicalLinesToFitFromBottom();
    if (m_document) {
        int nMax = m_document->logicalLinesCount() - nLinesToFitFromBottom;

        if (nMax < 0)
            nMax = 0;

        pScrollBar->setMaximum(nMax);
    }
    else {
        pScrollBar->setMaximum(0);
    }
}

void       xPlainTextViewer::invalidate() {    
    setUpdatesEnabled(false);   
    setMaximumScrollBarValue();
    verticalScrollBar()->setValue(0);
    setUpdatesEnabled(true);
}

void          xPlainTextViewer::setHighlighter(xHighlighter * pHighlighter) {
    if (m_highligher) {
        disconnect(m_highligher, NULL, this, NULL);
    }

    m_highligher = pHighlighter;
    connect(m_highligher, &xHighlighter::highlightRulesChanged, this, &xPlainTextViewer::onHighlightRulesChanged);

    viewport()->update();
}

xHighlighter * xPlainTextViewer::highlighter() const {
    return m_highligher;
}

void xPlainTextViewer::setWordWrap(bool bEnabled) {
    if (m_wordWrapEnabled != bEnabled) {
        m_wordWrapEnabled = bEnabled;
        viewport()->update();
    }
}

bool xPlainTextViewer::wordWrap() const {
    return m_wordWrapEnabled;
}

void xPlainTextViewer::setShowLineNumbers(bool bEnabled) {
    if (m_bShowLineNumbers != bEnabled) {
        m_bShowLineNumbers = bEnabled;
        viewport()->update();
    }
}

bool xPlainTextViewer::showLineNumbers() const {
    return m_bShowLineNumbers;
}

void xPlainTextViewer::setShowBookmarks(bool bEnabled) {
    if (m_bShowBookmarks != bEnabled) {
        m_bShowBookmarks = bEnabled;
        viewport()->update();
    }
}

bool xPlainTextViewer::showBookmarks() const {
    return m_bShowBookmarks;
}

bool xPlainTextViewer::hasSelection() const
{
    return ((m_selectionStart > 0) && (m_selectionEnd - m_selectionStart) > 0);
}

quint64 xPlainTextViewer::logicalSelectionLength() const {
    if (!hasSelection())
        return 0;

    return selectionPositionEnd() - selectionPositionStart();
}

void    xPlainTextViewer::onHighlightRulesChanged() {
    viewport()->update();
}

quint64     xPlainTextViewer::indexAtPoint(const QPoint & pt, int * column, bool * bFound) const {
    if (bFound) {
        *bFound = false;
    }

    int leftSpacing = (m_bShowBookmarks ? m_bookmarkSpacing : 0) + (m_bShowLineNumbers ? m_lineNumbersSpacing : 0) + m_leftSpacing;

    for (const QPair<QSharedPointer<QTextLayout>, quint64> & layoutInfo : m_currentLayouts) {
        if (layoutInfo.first->boundingRect().contains(pt)) {
            for (int i = 0; i < layoutInfo.first->lineCount(); i++) {
                const QTextLine & line = layoutInfo.first->lineAt(i);
                if (line.rect().contains(pt)) {
                    if (bFound) {
                        *bFound = true;
                    }
                    if (column) {
                        *column = line.xToCursor(pt.x() - leftSpacing);
                    }
                    return layoutInfo.second + line.xToCursor(pt.x()  - leftSpacing);
                }
            }
        }
    }

    return 0;
}

void  xPlainTextViewer::mouseMoveEvent(QMouseEvent *event) {
    if (m_bMousePressed) {
        bool bFound = false;
        QPoint currentPosition = event->pos();

        if (currentPosition.y() < 0 && m_currentLayouts.size()) {
            currentPosition.setY(m_currentLayouts.first().first->boundingRect().top());               
        }

        if (currentPosition.y() > rect().bottom() && m_currentLayouts.size()) {
            currentPosition.setY(m_currentLayouts.last().first->boundingRect().bottom());
        }

        int nColumn = 0;

        quint64 charPosition = indexAtPoint(currentPosition, &nColumn, &bFound);
        if (bFound && charPosition != m_selectionEnd) {
            if (m_selectionEnd < m_selectionStart) {
                m_selectionColumnStart = nColumn;
            }
            m_selectionEnd       = charPosition;
            viewport()->update();
        }

        if (event->pos().y() > rect().bottom()) {
            if (m_lastPosition.y() > event->pos().y()) {
                if (m_scrollTimer > 0) {
                    qCDebug(logicViewer) << "xPlainTextViewer: stop scroll";
                    killTimer(m_scrollTimer);
                    m_scrollTimer = -1;
                    m_scrollDelta = 0;
                }
            }
            else {
                qCDebug(logicViewer) << "xPlainTextViewer: start scroll down";
                m_scrollTimer = startTimer(100);
                m_scrollDelta = (event->pos().y() - rect().bottom()) / 10;
                if (m_scrollDelta == 0) {
                    m_scrollDelta = 1;
                }
                m_lastPosition = event->pos();
            }           
        }
        else if (event->pos().y() < 0) {
            if (m_lastPosition.y() < event->pos().y()) {
                if (m_scrollTimer > 0) {
                    qCDebug(logicViewer) << "xPlainTextViewer: stop scroll";
                    killTimer(m_scrollTimer);
                    m_scrollTimer = -1;
                    m_scrollDelta = 0;
                }
            }
            else {
                qCDebug(logicViewer) << "xPlainTextViewer: start scroll up";
                m_scrollTimer = startTimer(100);
                m_scrollDelta = event->pos().y() / 10;
                if (m_scrollDelta == 0) {
                    m_scrollDelta = -1;
                }
                m_lastPosition = event->pos();
            }
        }
        else {
            if (m_scrollTimer > 0) {
                qCDebug(logicViewer) << "xPlainTextViewer: stop scroll";
                killTimer(m_scrollTimer);
                m_scrollTimer = -1;
                m_scrollDelta = 0;
            }
        }
    }
    else {
        int nCurrentHoveredLine = logicalLineForPosition(event->pos(), false);
        if (m_currentHoverLine != nCurrentHoveredLine) {
            m_currentHoverLine = nCurrentHoveredLine;
            qCDebug(logicViewer) << "xPlainTextViewer: hover line changed: " << m_currentHoverLine;
            viewport()->update();
        }
    }

    QAbstractScrollArea::mouseMoveEvent(event);    
} 

void  xPlainTextViewer::timerEvent(QTimerEvent *event) {
    if (event->timerId() == m_scrollTimer) {
        verticalScrollBar()->setValue(verticalScrollBar()->value() + m_scrollDelta);
        bool bFound = false;
        int  nColumn = 0;
        quint64 charPosition = indexAtPoint(m_lastPosition, &nColumn, &bFound);
        if (bFound && charPosition != m_selectionEnd) {
            qCDebug(logicViewer) << "xPlainTextViewer: extending selection to: " << charPosition;
            m_selectionEnd       = charPosition;
            viewport()->update();
        }
    }
}

void  xPlainTextViewer::mousePressEvent(QMouseEvent *event) {

    if (event->buttons() & Qt::LeftButton) {
        bool    bUpdateRequired = false;
        m_bMousePressed = true;

        if (m_bInSelection) {
            m_bInSelection = false;
            bUpdateRequired = true;
        }

        bool bFound = false;

        qCDebug(logicViewer) << "xPlainTextViewer: mouse press at " << event->pos();

        int nColumn = 0;
        quint64 charPosition = indexAtPoint(event->pos(), &nColumn, &bFound);

        qCDebug(logicViewer) << "xPlainTextViewer: char position: " << charPosition;

        if (bFound) {
            m_selectionStart = charPosition;
            m_selectionEnd = charPosition;

            m_selectionColumnStart = nColumn;

            m_bInSelection = true;
        }


        if (bUpdateRequired) {
            viewport()->update();
        }
    }

    QAbstractScrollArea::mousePressEvent(event);
}

void  xPlainTextViewer::mouseReleaseEvent(QMouseEvent *event) {
   
    if (m_scrollTimer > 0) {
       killTimer(m_scrollTimer);
       m_scrollTimer = -1;
       m_scrollDelta = 0;
    }

    m_bMousePressed = false;
    
    QAbstractScrollArea::mouseReleaseEvent(event);
}

bool xPlainTextViewer::selectWord(const QPoint & pt) {
    if (!document())
        return false;

    int nLine = logicalLineForPosition(pt);
    if (nLine == -1)
        return false;

    int  column;
    bool bFound = false;

    quint64     pos = indexAtPoint(pt, &column, &bFound);

    if (!bFound)
        return false;

    QString      text = logicalLineText(nLine);
    quint64      start = document()->logicalLineStart(nLine);
    pos -= start;
    
    int     nPreviousBoundry = 0;
    QTextBoundaryFinder boundryFinder(QTextBoundaryFinder::Word, text);    

    bFound = false;
    while (boundryFinder.toNextBoundary() > 0) {
        if (nPreviousBoundry <= pos && pos <= boundryFinder.position()) {
            bFound = true;
            break;
        }
        
        nPreviousBoundry = boundryFinder.position();        
    }
    
    if (bFound) {
        int nWordStart = nPreviousBoundry;
        int nWordEnd   = boundryFinder.position();

        m_selectionStart        = start + nWordStart;
        m_selectionEnd          = start + nWordEnd;
        m_selectionColumnStart  = column; 

        return true;
    }

    return false;
}

QString  xPlainTextViewer::logicalLineText(int index) const {
    return m_codec->toUnicode(document()->logicalLine(index));
}

quint64 xPlainTextViewer::selectionPositionEnd() const
{
    if (!hasSelection())
        return 0;

    return qMax(m_selectionStart, m_selectionEnd);
}

quint64 xPlainTextViewer::selectionPositionStart() const {
    if (!hasSelection())
        return 0;

    return qMin(m_selectionStart, m_selectionEnd);
}

void xPlainTextViewer::mouseDoubleClickEvent(QMouseEvent *event) {

    int            leftSpacing = (m_bShowBookmarks ? m_bookmarkSpacing : 0) + (m_bShowLineNumbers ? m_lineNumbersSpacing : 0) + m_leftSpacing;

    if (event->button() == Qt::LeftButton) {
        if (event->pos().x() < leftSpacing) {
            int nLine = logicalLineForPosition(event->pos());
            if (nLine != -1) {
                selectLogicalLine(nLine);
                viewport()->update();
            }
        }
        else {
            if (selectWord(event->pos())) {
                viewport()->update();
            }
        }
    }
}

bool xPlainTextViewer::selectLogicalLine(int line) {
    if (!document())
        return false;

    bool bOk = false;
    quint64 start = document()->logicalLineStart(line, &bOk);
    if (!bOk) return false;

    quint64 end = document()->logicalLineEnd(line);

    m_selectionStart = start;
    m_selectionEnd   = end;
    m_selectionColumnStart = 0;

    return true;
}

void                xPlainTextViewer::ensureLogicalLineVisible(int line) {
    if (line >= 0) {
        verticalScrollBar()->setValue(line);
    }
}

void    xPlainTextViewer::ensureSearchResultVisible(const searchResult & item) {
    ensureLogicalLineVisible(document()->sourceToLogicalLineNumber(item.lineNumber));
}

void    xPlainTextViewer::ensureBookmarkVisible(const documentBookmark & item) {
    ensureLogicalLineVisible(document()->sourceToLogicalLineNumber(item.lineNumber));
}

int         xPlainTextViewer::logicalLinesToFitFromBottom() const {
    
    int nCurrentLine = document()->logicalLinesCount() - 1;
    if (nCurrentLine < 0)
        return 0;

    int            leftSpacing = (m_bShowBookmarks ? m_bookmarkSpacing : 0) + (m_bShowLineNumbers ? m_lineNumbersSpacing : 0) + m_leftSpacing;
    int            targetWidth = width() - leftSpacing - m_rightSpacing - verticalScrollBar()->width();

    int            currentY = viewport()->height();

    do {
        QString text = logicalLineText(nCurrentLine);

        QTextLayout textLayout;
        textLayout.setText(text);
        textLayout.setFont(viewport()->font());

        QTextLine currentLine;

        textLayout.beginLayout();
       
        while ((currentLine = textLayout.createLine()).isValid()) {
            currentLine.setLineWidth(targetWidth);
            currentY -= m_lineSpacing;
            currentY -= currentLine.height();

            if (currentY < 0) {
                break;
            }

            if (!m_wordWrapEnabled) {
                break;
            }
        }

        textLayout.endLayout();
        nCurrentLine--;
    } while (currentY > 0 && nCurrentLine >= 0);

    int nLines = document()->logicalLinesCount() - 1 - nCurrentLine;

    if ((currentY < -m_lineSpacing) && (nLines > 0)) {
        nLines--;
    }

    return nLines;
}

void     xPlainTextViewer::keyPressEvent(QKeyEvent *e) {
    if (e->key() == Qt::Key_Escape) {
        hideSearchPanel();
    }

    QAbstractScrollArea::keyPressEvent(e);
}

void    xPlainTextViewer::paintEvent(QPaintEvent * /*event*/) {    
    if (!m_document) {
        return;
    }

    int currentScrollBarValue = verticalScrollBar()->value();
   
    QFontMetrics   metrics      = fontMetrics();
    int            textHeight   = metrics.height();
    int            windowHeight = height();
    int            linesEstimation = windowHeight / textHeight+1;
    int            digitWidth   = metrics.width("0");

    m_lineNumbersSpacing = digitWidth * 6;

    QVector<QByteArray>     lines = m_document->logicalLines(currentScrollBarValue, currentScrollBarValue + linesEstimation);
    
    int            leftSpacing = (m_bShowBookmarks ? m_bookmarkSpacing : 0) + (m_bShowLineNumbers ? m_lineNumbersSpacing : 0) + m_leftSpacing;
    int            currentY = m_topSpacing;
    int            targetWidth = width() - leftSpacing - m_rightSpacing - verticalScrollBar()->width();

    int            lineNumberX = m_bShowBookmarks ? (m_leftSpacing + m_bookmarkSpacing ) : (m_leftSpacing);
    
    QPainter painter(viewport());

    QTextOption textOptions;
    textOptions.setAlignment(Qt::AlignLeft);

    if (m_wordWrapEnabled) {
        textOptions.setWrapMode(QTextOption::WordWrap);
    }

    m_currentLayouts.clear();

    quint64 selectionStart = qMin(m_selectionStart, m_selectionEnd);
    quint64 selectionEnd   = qMax(m_selectionStart, m_selectionEnd);
    
    int nCurrentLineNumber = currentScrollBarValue;
    quint64 layoutPosition = m_document->logicalLinePosition(nCurrentLineNumber);

    for (const QByteArray & lineData : lines) {

        int sourceLineNumber = document()->logicalToSourceLineNumber(nCurrentLineNumber);
        if (sourceLineNumber == -1)
            continue;

        if (m_bShowLineNumbers) {                      
            QString lineNumberString = QString("%1").arg(sourceLineNumber+1);
            painter.setPen(Qt::darkCyan);
            painter.drawText(lineNumberX, currentY, m_lineNumbersSpacing, textHeight + metrics.lineSpacing(), Qt::AlignRight, lineNumberString);
        }

        QString text = m_codec->toUnicode(lineData);

        QVector<QTextLayout::FormatRange>  highlightRanges;
        if (m_highligher) {
            highlightRanges = m_highligher->highlight(text);
            if (m_searchHighlighter.isActive) {
                highlightRanges << m_highligher->highlight(text, m_searchHighlighter);
            }
        }

        if (m_bShowBookmarks) {
            if (hasBookmark(nCurrentLineNumber)) {
                QIcon markIcon = style()->standardIcon(QStyle::SP_ArrowRight);
                markIcon.paint(&painter, 0, currentY, m_bookmarkSpacing, textHeight, Qt::AlignRight | Qt::AlignVCenter);
            }
        }
        if (selectionStart <= (layoutPosition+text.length()) && ( selectionEnd >= layoutPosition) ) {

            int localSelectionStart = qMax(selectionStart, layoutPosition) - layoutPosition;
            int localSelectionEnd   = qMin(selectionEnd, layoutPosition + text.length()) - layoutPosition;

            QTextLayout::FormatRange    selectionRange;
            selectionRange.start  = localSelectionStart;
            selectionRange.length = localSelectionEnd - localSelectionStart;
            
            QTextCharFormat     textFormat;
            
            textFormat.setBackground(Qt::black);
            textFormat.setForeground(Qt::white);
            selectionRange.format = textFormat;

            highlightRanges << selectionRange;
        };

        QSharedPointer<QTextLayout> textLayout = QSharedPointer<QTextLayout>(new QTextLayout());
        textLayout->setTextOption(textOptions);
        textLayout->setCacheEnabled(true);
        textLayout->setText(text);
        textLayout->setFormats(highlightRanges);
        textLayout->setFont(viewport()->font());

        QTextLine currentLine;

        textLayout->beginLayout();
     
        while ((currentLine = textLayout->createLine()).isValid()) {
            currentLine.setLineWidth(targetWidth);
            currentLine.setPosition(QPointF(m_textPanelSpacing, currentY - m_topSpacing));
            currentY += m_lineSpacing;
            currentY += currentLine.height();
            if (currentY > height()) {                
                break;
            }

            if (!m_wordWrapEnabled) {
                break;
            }

           
        }

        textLayout->endLayout();
        painter.setPen(Qt::black);
        textLayout->draw(&painter, QPoint(leftSpacing, m_topSpacing));

        if (nCurrentLineNumber == m_currentHoverLine) {
            painter.setPen(Qt::lightGray);
            QRectF r = textLayout->boundingRect();            
            painter.drawRect(r.x() + leftSpacing, r.y()+m_leftSpacing/2, r.width()-2, r.height());
        }

        m_currentLayouts << QPair<QSharedPointer<QTextLayout>, quint64>(textLayout, layoutPosition);

        if (currentY > height()) {            
            break;
        }   

        nCurrentLineNumber++;
        layoutPosition = m_document->logicalLinePosition(nCurrentLineNumber);
    }    
}

void    xPlainTextViewer::resizeEvent(QResizeEvent *event) {
    setMaximumScrollBarValue();
    m_searchPanel->move(viewport()->width() - m_searchPanel->width(), 0);

    QAbstractScrollArea::resizeEvent(event);        
}

void xPlainTextViewer::wheelEvent(QWheelEvent *e)
{   
    if (e->modifiers() & Qt::CTRL) {
        if (e->delta() > 0) {
            QFont ft = viewport()->font();
            if (ft.pointSize() < m_maximumFontSize) {
                ft.setPointSize(ft.pointSize() + 1);
                viewport()->setFont(ft);
            }
        }
        else {
            QFont ft = viewport()->font();
            if (ft.pointSize() > m_minimumFontSize) {
                ft.setPointSize(ft.pointSize() - 1);
                viewport()->setFont(ft);
            }
        }

        qCDebug(logicViewer) << "xPlainTextViewer: adjusted font size to:" << viewport()->font().pointSize();

        m_searchPanel->adjustSize();
        m_searchPanel->move(viewport()->width() - m_searchPanel->width(), 0);

        return;
    }

    QAbstractScrollArea::wheelEvent(e);
}

int                 xPlainTextViewer::logicalLinesInSelection() const {
    if (!hasSelection())
        return 0;

    if (!document())
        return 0;

    return document()->logicalLinesBetweenPositions(m_selectionStart, m_selectionEnd);
}

QString             xPlainTextViewer::selection() const {
    if (!hasSelection())
        return 0;

    if (!document())
        return 0;

    QString     text = m_codec->toUnicode(m_document->text(selectionPositionStart(), selectionPositionEnd()));

    return text;
}
//------------------------------------------------------------------------
int                  xPlainTextViewer::selectionColumnStart() const {
    if (!hasSelection())
        return -1;

    return m_selectionColumnStart;
}

int                 xPlainTextViewer::logicalLineForPosition(const QPoint & pt, bool bExact) const {
    int nLine = 0;
    for (const QPair<QSharedPointer<QTextLayout>, quint64> & layoutInfo : m_currentLayouts) {
        if (layoutInfo.first->boundingRect().contains(pt)) {
            if (!bExact) {
                return verticalScrollBar()->value() + nLine;
            }

            for (int i = 0; i < layoutInfo.first->lineCount(); i++) {                
                const QTextLine & line = layoutInfo.first->lineAt(i);
                if (line.rect().contains(pt)) {                    
                    return verticalScrollBar()->value()+nLine;
                }                
            }
        }
        nLine++;
    }

    return -1;
}

int xPlainTextViewer::hoveredLineNumber() const {
    return m_currentHoverLine;
}

QString             xPlainTextViewer::hoveredLine() const {
    return   logicalLineText(m_currentHoverLine);
}

void xPlainTextViewer::setTextCodec(QTextCodec * pCodec)
{
    m_codec = pCodec;
    viewport()->update();
}

bool                xPlainTextViewer::followTail() const {
    return m_bFollowTail;
}

void                xPlainTextViewer::setFollowTail(bool f) {
    if (f == m_bFollowTail)
        return;

    m_bFollowTail = f;

    if (m_bFollowTail) {
        verticalScrollBar()->setValue(verticalScrollBar()->maximum());
        viewport()->update();
    }
}

QTextCodec * xPlainTextViewer::textCodec() const {
    return m_codec;
}

bool                xPlainTextViewer::hasBookmark(int nLine) const {
    nLine = document()->logicalToSourceLineNumber(nLine);
    QVector<documentBookmark>::const_iterator it = std::find_if(m_bookmarkModel->items().begin(), m_bookmarkModel->items().end(), [nLine](const documentBookmark & bookmark) {
        return bookmark.lineNumber == nLine;
    });

    return (it != m_bookmarkModel->items().end());
}

bool                xPlainTextViewer::toggleBookmark(int nLine, const QColor & color) {
    nLine = document()->logicalToSourceLineNumber(nLine);
    QVector<documentBookmark>::const_iterator it = std::find_if(m_bookmarkModel->items().begin(), m_bookmarkModel->items().end(), [nLine](const documentBookmark & bookmark) {
        return bookmark.lineNumber == nLine;
    });

    if (it == m_bookmarkModel->items().end()) {
        documentBookmark newItem;
        newItem.value = logicalLineText(nLine);
        newItem.lineNumber = nLine;
        newItem.color = color;

        it = std::find_if(m_bookmarkModel->items().begin(), m_bookmarkModel->items().end(), [nLine](const documentBookmark & bookmark) {
            return bookmark.lineNumber > nLine;
        });

        if (it != m_bookmarkModel->items().end()) {
            m_bookmarkModel->insertItem(newItem, std::distance(m_bookmarkModel->items().begin(), it));
        }
        else {
            m_bookmarkModel->appendItem(newItem);
        }
        return true;
    }

    m_bookmarkModel->removeItem(*it);
    return false;
}

int                  xPlainTextViewer::previousBookmark(int nLine) const {
    if (nLine == -1)
        nLine = verticalScrollBar()->value();

    nLine = document()->logicalToSourceLineNumber(nLine);   
    int nPreviousLine = -1;
    std::find_if(m_bookmarkModel->items().begin(), m_bookmarkModel->items().end(), [nLine, &nPreviousLine](const documentBookmark & item) { 
        if (item.lineNumber < nLine) {
            nPreviousLine = item.lineNumber;
        }

        return item.lineNumber >= nLine; 
    });

    if ((nPreviousLine == -1) && (m_bookmarkModel->items().size()))
        nPreviousLine = m_bookmarkModel->items().last().lineNumber;

    if (nPreviousLine != -1) {
        verticalScrollBar()->setValue(nPreviousLine);
    }

    return nPreviousLine;
}

int                  xPlainTextViewer::nextBookmark(int nLine) const {
    if (nLine == -1)
        nLine = verticalScrollBar()->value();

    nLine = document()->logicalToSourceLineNumber(nLine);
    int nNextLine = -1;

    QVector<documentBookmark>::const_iterator it = std::find_if(m_bookmarkModel->items().begin(), m_bookmarkModel->items().end(), [nLine](const documentBookmark & item) {
       return item.lineNumber > nLine;
    });
   
    if (it == m_bookmarkModel->items().end()) {
        if (m_bookmarkModel->items().size()) {
            nNextLine = m_bookmarkModel->items().first().lineNumber;
        }
    }
    else {
        nNextLine = (*it).lineNumber;
    }

    if (nNextLine != -1) {
        verticalScrollBar()->setValue(nNextLine);
    }

    
    return nNextLine;
}

void                 xPlainTextViewer::setTimestampFormat(const QString & format, int start, int length) {
    if (format.isEmpty() && !m_timestampFormat.isEmpty()) {
        highlighterItem     searchItem;
        searchItem.type = PositionHighlighter;
        searchItem.from = m_timestampStart;
        searchItem.to = m_timestampStart + m_timestampLength;

        QVector<highlighterItem>::const_iterator it = std::find_if(m_highligher->highlighters()->items().begin(), m_highligher->highlighters()->items().end(), [&searchItem](const highlighterItem & item) { return searchItem == item; });
        if (it != m_highligher->highlighters()->items().end()) {
            m_highligher->removeHighlighter(*it);
        }
    }

    m_timestampFormat = format;
    m_timestampStart  = start;
    m_timestampLength = length;
    m_bookmarkModel->layoutChanged();

    QTextCharFormat textFormat;
    textFormat.setFontWeight(QFont::Bold);
    textFormat.setForeground(Qt::blue);

    m_highligher->appendHighlight(start, start + length, textFormat);
}

QTime                   xPlainTextViewer::timestamp(int nBookmarkIndex) const {
    if (m_bookmarkModel->rowCount() <= nBookmarkIndex)
        return QTime();

    const documentBookmark & data = m_bookmarkModel->itemAt(nBookmarkIndex);

    if (!m_timestampFormat.isEmpty()) {
        QString timeString = data.value.mid(m_timestampStart, m_timestampLength);
        QTime timeValue = QTime::fromString(timeString, m_timestampFormat);
        return timeValue;
    }

    return QTime();
}

bool                   xPlainTextViewer::timestampDefined() const {
    return !m_timestampFormat.isEmpty();
}

QString                 xPlainTextViewer::timestampFormat() const {
    return m_timestampFormat;
}

int                     xPlainTextViewer::timestampPosition() const {
    return m_timestampStart;
}

int                     xPlainTextViewer::timestampLength() const {
    return m_timestampLength;
}

bool                    xPlainTextViewer::isSearchPanelShown() const {
    return m_searchPanel->isVisible();
}

void                    xPlainTextViewer::showSearchPanel() {
    m_searchPanel->show();
    m_searchPanel->move(viewport()->width() - m_searchPanel->width(), 0);
    m_searchPanel->setFocus();

    bool bOk = false;
    searchRequestItem   item = m_searchPanel->currentSearchItem(&bOk);
    if (bOk) {
        onSearchChanged(item);
    }
}

void                    xPlainTextViewer::hideSearchPanel() {
    m_searchPanel->hide();
    m_searchHighlighter.isActive = false;
    viewport()->update();
}

xSearchWidget   *       xPlainTextViewer::searchPanel() const {
    return m_searchPanel;
}

void    xPlainTextViewer::onSearchChanged(const searchRequestItem & item) {
    if (item.type == PatternSearch) {
        m_searchHighlighter.matcher     = item.matcher;
        m_searchHighlighter.type        = PatternHighlighter;
        m_searchHighlighter.isActive    = true;
    }
    else if (item.type == RegExpSearch) {
        qCDebug(logicViewer) << "xPlainTextViewer: searching for regular expression, pattern:" << item.regexp.pattern() << " length:" << item.regexp.pattern().length();
        m_searchHighlighter.regexp      = item.regexp;
        m_searchHighlighter.type        = RegExtHighlighter;
        m_searchHighlighter.isActive    = true;
    }

    viewport()->update();
}

void                xPlainTextViewer::resetSelection() {
    m_selectionStart = 0;
    m_selectionEnd   = 0;
    m_selectionColumnStart = -1;
    m_bInSelection = false;
    viewport()->update();
}

void    xPlainTextViewer::onAddFilter(const searchRequestItem & item) {
    m_document->appendFilterRule(item);
    emit showFilters();
}

void    xPlainTextViewer::onFindAll(const searchRequestItem & item) {
    m_document->search(item, m_codec->name(), false);
    emit showFindResults();
}


void    xPlainTextViewer::initModels() {
    m_bookmarkModel->setColumntCount(4);

    connect(m_bookmarkModel, &QAbstractItemModel::rowsInserted, [this]() {
        viewport()->update();
    });
    connect(m_bookmarkModel, &QAbstractItemModel::rowsRemoved, [this]() {
        viewport()->update();
    });
    connect(m_bookmarkModel, &QAbstractItemModel::dataChanged, [this]() {
        viewport()->update();
    });

    m_bookmarkModel->setDataCallback([this](int row, int column, int /*role*/) -> QVariant {

        const documentBookmark & bookmark = m_bookmarkModel->itemAt(row);
        switch (column) {
        case bookmarksColumnLineNumber:
            return bookmark.lineNumber + 1;
        case bookmarksColumnLineTimestamp: {
            QTime timeValue = timestamp(row);
            if (timeValue.isValid()) {
                return timeValue.toString("hh:mm:ss.zzz");
            }

            return tr("<not defined>");
        }
        case bookmarksColumnLineTimestampFromPrevious: {
            if (row == 0)
                return QVariant();

            QTime timeValue = timestamp(row);
            if (!timeValue.isValid())
                return QVariant();

            QTime startTime;
            for (int i = row - 1; i >= 0; i--) {
                startTime = timestamp(i);
                if (startTime.isValid())
                    break;
            }

            if (!startTime.isValid())
                return QVariant();

            int ms = startTime.msecsTo(timeValue);
            if (ms >= 0)
                return QChar(916) + tr(" %1 ms").arg(ms);

            return QVariant();
        }

        case bookmarksColumnLineTimestampFromStart: {
            if (row == 0)
                return QVariant();

            QTime timeValue = timestamp(row);
            if (!timeValue.isValid())
                return QVariant();

            QTime startTime;
            for (int i = 0; i < row; i++) {
                startTime = timestamp(i);
                if (startTime.isValid())
                    break;
            }

            if (!startTime.isValid())
                return QVariant();

            int ms = startTime.msecsTo(timeValue);
            if (ms >= 0)
                return tr("+%1 ms").arg(ms);
        }
        }
        return QVariant();
    }, Qt::DisplayRole);

    m_bookmarkModel->setDataCallback([this](int row, int column, int /*role*/) -> QVariant {

        const documentBookmark & bookmark = m_bookmarkModel->itemAt(row);
        switch (column) {
        case bookmarksColumnLineNumber:
            return bookmark.color;
        }
        return QVariant();

    }, Qt::UserRole + 1);

    m_bookmarkModel->setHeaderCallback([this](int section, Qt::Orientation orientation, int /*role = Qt::DisplayRole*/) -> QVariant {
        if (orientation != Qt::Horizontal)
            return QVariant();

        switch (section) {
        case bookmarksColumnLineNumber:
            return QString(tr("Line number"));
        case bookmarksColumnLineTimestamp:
            return QString(tr("Timestamp"));
        case bookmarksColumnLineTimestampFromStart:
            return QString(tr("From start"));
        case bookmarksColumnLineTimestampFromPrevious:
            return QString(tr("From previous"));

        }


        return QVariant();
    }, Qt::DisplayRole);

    m_bookmarkModel->setFlagsCallback([this](int row, int /*column*/, Qt::ItemFlags defaultFlags) -> Qt::ItemFlags {
        const documentBookmark & bookmark = m_bookmarkModel->itemAt(row);
        if (document()->logicalToSourceLineNumber(bookmark.lineNumber) == -1) {
            return (defaultFlags & ~(Qt::ItemIsEnabled));
        }

        return defaultFlags;
    });

}