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


#ifndef _xInfoPanel_h_
#define _xInfoPanel_h_ 1

#include <QWidget>
#include <QListWidget>

#include "xtreeview.h"
#include "xdocument.h"
#include "xhighlighter.h"
#include "xplaintextviewer.h"

class QTabWidget;
class QTreeView;

class 	xInfoPanel: public QWidget {
	Q_OBJECT
public:
	xInfoPanel(QWidget * pParent = nullptr);
	~xInfoPanel();

    QTreeView   *   bookmarks() const       { return m_bookmarks; };
    QTreeView   *   findResults() const     { return m_findResults; };
    QTreeView   *   filters() const         { return m_filters; };
    QTreeView   *   highlighters() const    { return m_highlighters; };

public slots:

    void    activateHighlights();
    void    activateBookmarks();
    void    activateFilters();
    void    activateFindResults();

    void    highlighterItemClicked(const QModelIndex & index);
    void    highlighterItemContextMenuRequested(const QPoint &pos);

    void    findResultItemClicked(const QModelIndex & index);
    void    findResultsContextMenuRequested(const QPoint &pos);

    void    bookmarksItemClicked(const QModelIndex & index);
    void    bookmarksItemContextMenuRequested(const QPoint &pos);
    
    void    filtersItemContextMenuRequested(const QPoint &pos);
    void    filtersItemClicked(const QModelIndex & index);

signals:

    void    toggleHighlighterVisibility(const highlighterItem & item);
    void    toggleFilterRuleVisibility(const filterRule & item);

    void    removeHighlighter(const highlighterItem & item);
    void    changeHighlighterColor(const highlighterItem & item, const QColor & color);
    void    removeAllHighlighters();

    void    createHightlightFromFilterRequest(const searchRequestItem & item);
    void    deleteFilterRequest(const filterRule & item);
    void    deleteAllFiltersRequest();

    void    ensureSearchResultVisible(const searchResult &);
    void    ensureBookmarkVisible(const documentBookmark &);

protected:

    QTabWidget       *   m_tabs         = nullptr;

    xTreeView       *   m_bookmarks     = nullptr;
    xTreeView       *   m_filters       = nullptr;
    xTreeView       *   m_findResults   = nullptr;
    xTreeView       *   m_highlighters  = nullptr;
};

#endif