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


#ifndef _xPlainTextViewer_h_
#define _xPlainTextViewer_h_ 1

#include <QAbstractScrollArea>
#include <QScrollBar>

#include "xdocument.h"
#include "xhighlighter.h"
#include "xscrollbar.h"
#include "xsearchwidget.h"

class QTextCodec;
class xScrollBar;

enum {
    bookmarksColumnLineNumber = 0,
    bookmarksColumnLineTimestamp = 1,
    bookmarksColumnLineTimestampFromPrevious = 2,
    bookmarksColumnLineTimestampFromStart    = 3,
};

struct documentBookmark {
    int         lineNumber = 0;
    QString     value;
    QString     name;
    QColor      color = Qt::red;
    bool        visible = true;

    bool    operator  ==(const documentBookmark & other) const {
        return (lineNumber == other.lineNumber);
    };

};


class xPlainTextViewer: public QAbstractScrollArea {
	Q_OBJECT
public:
	xPlainTextViewer(QWidget * pParent = nullptr);
	~xPlainTextViewer();

    void                setDocument(xDocument * pDocument);
    xDocument *         document() const;

    void                setHighlighter(xHighlighter * pHighlighter);
    xHighlighter *      highlighter() const;

    void                setWordWrap(bool bEnabled);
    bool                wordWrap() const;

    void                setShowLineNumbers(bool bEnabled);
    bool                showLineNumbers() const;

    void                setShowBookmarks(bool bEnabled);
    bool                showBookmarks() const;

    bool                hasSelection() const;
    void                resetSelection();
    QString             selection() const;
    quint64             selectionPositionStart() const;
    quint64             selectionPositionEnd() const;
    int                 selectionColumnStart() const;

    quint64             logicalSelectionLength() const;
    int                 logicalLinesInSelection() const;

    int                 logicalLineForPosition(const QPoint & pt, bool bExact = true) const;
    bool                selectLogicalLine(int line);
    bool                selectWord(const QPoint & pt);
        
    QString             hoveredLine() const;
    int                 hoveredLineNumber() const;

    QString             logicalLineText(int index) const;
        
    void                ensureLogicalLineVisible(int line);
    void                ensureBookmarkVisible(const documentBookmark & item);
    void                ensureSearchResultVisible(const searchResult & item);

    QTextCodec      *   textCodec() const;
    void                setTextCodec(QTextCodec * pCodec);

    bool                followTail() const;
    void                setFollowTail(bool f);

    bool                    hasBookmark(int nLine) const;
    bool                    toggleBookmark(int nLine, const QColor & color);
    int                     previousBookmark(int nLine = -1) const;
    int                     nextBookmark(int nLine = -1) const;
    QAbstractTableModel *   bookmarks() const { return m_bookmarkModel; };

    void                    setTimestampFormat(const QString & format, int start, int length);
    bool                    timestampDefined() const;
    QTime                   timestamp(int nBookmarkIndex) const;
    QString                 timestampFormat() const;
    int                     timestampPosition() const;
    int                     timestampLength() const;

    bool                    isSearchPanelShown() const;
    void                    showSearchPanel();
    void                    hideSearchPanel();
    xSearchWidget   *       searchPanel() const;

public slots:

    void    onLayoutChanged();
    void    onHighlightRulesChanged();
    void    onSearchChanged(const searchRequestItem & item);

    void    onAddFilter(const searchRequestItem & item);
    void    onFindAll(const searchRequestItem & item);
    
signals:

    void    showFindResults();
    void    showFilters();

protected:

    virtual void    paintEvent(QPaintEvent *event) override;
    virtual void    resizeEvent(QResizeEvent *event) override;
    virtual void    wheelEvent(QWheelEvent *e) override;
    virtual void    timerEvent(QTimerEvent *e) override;
    virtual void    keyPressEvent(QKeyEvent *e) override;

    virtual void mouseMoveEvent(QMouseEvent *event) override;
    virtual void mousePressEvent(QMouseEvent *event) override;
    virtual void mouseReleaseEvent(QMouseEvent *event) override;
    virtual void mouseDoubleClickEvent(QMouseEvent *event) override;


    void        invalidate();
    quint64     indexAtPoint(const QPoint & pt, int * column, bool * bFound) const;
    int         logicalLinesToFitFromBottom() const;
    void        setMaximumScrollBarValue();
    void        initModels();

protected:
    int             m_currentHoverLine    = -1;

    int             m_maximumFontSize     = 18;
    int             m_minimumFontSize     = 6;
    int             m_scrollTimer         = -1;
    int             m_scrollDelta         = 0;
    QPoint          m_lastPosition;
    
    quint64         m_selectionStart        = 0;
    quint64         m_selectionEnd          = 0;
    int             m_selectionColumnStart  = -1;

    bool            m_bInSelection          = false;
    bool            m_bMousePressed         = false;
    
    int             m_leftSpacing        = 2;
    int             m_bookmarkSpacing    = 20;
    int             m_lineNumbersSpacing = 5;
    int             m_textPanelSpacing   = 5;
    int             m_rightSpacing       = 5;

    int             m_topSpacing   = 2;
    int             m_lineSpacing  = 2;

    bool            m_wordWrapEnabled  = true;
    bool            m_bShowBookmarks   = true;
    bool            m_bShowLineNumbers = true;
    bool            m_bFollowTail      = false;

    xDocument        * m_document   = nullptr;
    xHighlighter     * m_highligher = nullptr;
    QTextCodec       * m_codec      = nullptr;
    
    QList< QPair< QSharedPointer<QTextLayout>, quint64> >          m_currentLayouts;
    xValueCollection<documentBookmark>   * m_bookmarkModel = nullptr;

    QString             m_timestampFormat;
    int                 m_timestampStart   = 0;
    int                 m_timestampLength  = 0;

    xSearchWidget    *  m_searchPanel = nullptr;
    highlighterItem     m_searchHighlighter;
};

#endif