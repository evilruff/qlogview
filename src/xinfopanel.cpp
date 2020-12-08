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


#include <QVBoxLayout>
#include <QTreeView>
#include <QTabWidget>
#include <QMenu>
#include <QColorDialog>
#include <QHeaderView>

#include "xinfopanel.h"
#include "xplaintextviewer.h"
#include "xtreeview.h"


xInfoPanel::xInfoPanel(QWidget * pParent):
	QWidget(pParent) {
    QVBoxLayout * pLayout = new QVBoxLayout();
    pLayout->setMargin(0);
    pLayout->setSpacing(0);
    
    m_tabs = new QTabWidget();
    m_tabs->setTabPosition(QTabWidget::South);
        
    pLayout->addWidget(m_tabs);

    m_bookmarks = new xTreeView();

    QFont   placeholderFont = m_bookmarks->font();
    placeholderFont.setPointSize(12);

    m_bookmarks->setFrameShape(QFrame::NoFrame);
    m_bookmarks->setPlaceholderText(tr("No bookmarks defined"));
    m_bookmarks->setPlaceholderFont(placeholderFont);
    m_bookmarks->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_bookmarks, &QTreeView::clicked, this, &xInfoPanel::bookmarksItemClicked);
    connect(m_bookmarks, &QTreeView::customContextMenuRequested, this, &xInfoPanel::bookmarksItemContextMenuRequested);

    m_findResults = new xTreeView();
    m_findResults->setFrameShape(QFrame::NoFrame);
    m_findResults->setPlaceholderText(tr("No search results"));
    m_findResults->setPlaceholderFont(placeholderFont);
    m_findResults->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_findResults, &QTreeView::clicked, this, &xInfoPanel::findResultItemClicked);
    connect(m_findResults, &QTreeView::customContextMenuRequested, this, &xInfoPanel::findResultsContextMenuRequested);

    m_filters = new xTreeView();
    m_filters->setFrameShape(QFrame::NoFrame);
    m_filters->setContextMenuPolicy(Qt::CustomContextMenu);
    m_filters->setPlaceholderText(tr("No filters added"));
    m_filters->setPlaceholderFont(placeholderFont);
    connect(m_filters, &QTreeView::customContextMenuRequested, this, &xInfoPanel::filtersItemContextMenuRequested);
    connect(m_filters, &QTreeView::clicked, this, &xInfoPanel::filtersItemClicked);

    m_highlighters = new xTreeView();
    m_highlighters->setFrameShape(QFrame::NoFrame);
    m_highlighters->setContextMenuPolicy(Qt::CustomContextMenu);
    m_highlighters->setPlaceholderText(tr("No highlighters defined"));
    m_highlighters->setPlaceholderFont(placeholderFont);

    connect(m_highlighters, &QTreeView::clicked, this, &xInfoPanel::highlighterItemClicked);
    connect(m_highlighters, &QTreeView::customContextMenuRequested, this, &xInfoPanel::highlighterItemContextMenuRequested);

    m_tabs->addTab(m_bookmarks, tr("Bookmarks"));
    m_tabs->addTab(m_findResults, tr("Find results"));
    m_tabs->addTab(m_filters, tr("Filters"));
    m_tabs->addTab(m_highlighters, tr("Highlights"));

    setMinimumHeight(50);
    setLayout(pLayout);
}

xInfoPanel::~xInfoPanel() {
}

void    xInfoPanel::findResultItemClicked(const QModelIndex & index) {
    xValueCollection<searchResult>   * pResults = dynamic_cast<xValueCollection<searchResult>*>(m_findResults->model());
    if (pResults) {
        emit ensureSearchResultVisible(pResults->itemAt(index.row()));
    }
}

void    xInfoPanel::bookmarksItemClicked(const QModelIndex & index) {
    xValueCollection<documentBookmark>   * pResults = dynamic_cast<xValueCollection<documentBookmark>*>(m_bookmarks->model());
    if (pResults) {
        emit ensureBookmarkVisible(pResults->itemAt(index.row()));
    }
}

void    xInfoPanel::highlighterItemClicked(const QModelIndex & index) {
    switch (index.column()) {
        case highlighterViewColumnVisible: {
            xValueCollection<highlighterItem>   * pHighlighters = dynamic_cast<xValueCollection<highlighterItem>*>(m_highlighters->model());
            if (pHighlighters) {
                emit toggleHighlighterVisibility(pHighlighters->itemAt(index.row()));
            }
            
        }
        break; 
        case highlighterViewColumnColor: {
            xValueCollection<highlighterItem>   * pHighlighters = dynamic_cast<xValueCollection<highlighterItem>*>(m_highlighters->model());
            if (pHighlighters) {

                highlighterItem item = pHighlighters->itemAt(index.row());
                QColor  color = QColorDialog::getColor(item.format.foreground().color(), nullptr, "Choose highlighting color");
                if (color.isValid()) {
                    emit changeHighlighterColor(pHighlighters->itemAt(index.row()), color);
                }
            }

        }
        break;
    }
}

void    xInfoPanel::findResultsContextMenuRequested(const QPoint & pos) {
    QMenu contextMenu(this);
    QAction clearAction (tr("Clear"), &contextMenu);
    connect(&clearAction, &QAction::triggered, [this]() {
        xValueCollection<searchResult>   * pModel = dynamic_cast<xValueCollection<searchResult>*>(m_findResults->model());
        pModel->clear();
    });

    contextMenu.addAction(&clearAction);
    contextMenu.exec(m_findResults->mapToGlobal(pos));
}

void    xInfoPanel::bookmarksItemContextMenuRequested(const QPoint &pos) {
    if (m_bookmarks->currentIndex().isValid()) {
        xValueCollection<documentBookmark>   * pBookmarks = dynamic_cast<xValueCollection<documentBookmark>*>(m_bookmarks->model());
        documentBookmark item = pBookmarks->itemAt(m_bookmarks->currentIndex().row());

        QMenu contextMenu(this);
        QAction removeBookmark(tr("Delete"), &contextMenu);
        connect(&removeBookmark, &QAction::triggered, [this, item, pBookmarks]() {
            pBookmarks->removeItem(item);
        });
     
        contextMenu.addAction(&removeBookmark);
        contextMenu.exec(m_bookmarks->mapToGlobal(pos));
    }
}

void    xInfoPanel::highlighterItemContextMenuRequested(const QPoint &pos) {
    if (m_highlighters->currentIndex().isValid()) {

        xValueCollection<highlighterItem>   * pHighlighters = dynamic_cast<xValueCollection<highlighterItem>*>(m_highlighters->model());
        highlighterItem item = pHighlighters->itemAt(m_highlighters->currentIndex().row());


        QMenu contextMenu(this);
        QAction toggleHighlight( item.isActive ? tr("Disable") : tr("Enable"), &contextMenu);
        connect(&toggleHighlight, &QAction::triggered, [this, item]() {
            emit toggleHighlighterVisibility(item);
        });

        QAction changeColor(tr("Color..."), &contextMenu);
        connect(&changeColor, &QAction::triggered, [this, item]() {
            QColor  color = QColorDialog::getColor(item.format.foreground().color(), nullptr, "Choose highlighting color");
            if (color.isValid()) {
                emit changeHighlighterColor(item, color);
            }
        });

        QAction deleteHighlight(tr("Delete"), &contextMenu);
        connect(&deleteHighlight, &QAction::triggered, [this, item]() {
            emit removeHighlighter(item);
        });

        QAction deleteAllHighlight(tr("Delete All"), &contextMenu);
        connect(&deleteAllHighlight, &QAction::triggered, [this]() {
            emit removeAllHighlighters();
        });

        contextMenu.addAction(&toggleHighlight);
        contextMenu.addAction(&changeColor);

        contextMenu.addSeparator();
        contextMenu.addAction(&deleteHighlight);
        contextMenu.addAction(&deleteAllHighlight);
        contextMenu.exec(m_highlighters->mapToGlobal(pos));
    }
}

void    xInfoPanel::filtersItemContextMenuRequested(const QPoint &pos) {
    if (m_filters->currentIndex().isValid()) {
        QMenu contextMenu(this);
        xValueCollection<filterRule>   * pFilters = dynamic_cast<xValueCollection<filterRule>*>(m_filters->model());
        filterRule item = pFilters->itemAt(m_filters->currentIndex().row());

        QAction convertToHighlight(tr("Highlight"), &contextMenu);              
        connect(&convertToHighlight, &QAction::triggered, [this, item]() {
            emit createHightlightFromFilterRequest(item.filter);
        });
        contextMenu.addAction(&convertToHighlight);

        QAction deleteAction(tr("Delete"), &contextMenu);
        connect(&deleteAction, &QAction::triggered, [this, item]() {
            emit deleteFilterRequest(item);
        });
        contextMenu.addAction(&deleteAction);

        QAction deleteAllAction(tr("Delete All"), &contextMenu);
        connect(&deleteAllAction, &QAction::triggered, [this]() {
            emit deleteAllFiltersRequest();
        });
        contextMenu.addAction(&deleteAllAction);

        contextMenu.exec(m_filters->mapToGlobal(pos));
    }
}

void    xInfoPanel::activateHighlights() {
    m_tabs->setCurrentWidget(m_highlighters);
}

void    xInfoPanel::activateBookmarks() {
    m_tabs->setCurrentWidget(m_bookmarks);
}

void    xInfoPanel::activateFilters() {
    m_tabs->setCurrentWidget(m_filters);
}

void    xInfoPanel::activateFindResults() {
    m_tabs->setCurrentWidget(m_findResults);
}

void    xInfoPanel::filtersItemClicked(const QModelIndex & index) {
    xValueCollection<filterRule>   * pFilter = dynamic_cast<xValueCollection<filterRule>*>(m_filters->model());
    if (pFilter) {
        emit toggleFilterRuleVisibility(pFilter->itemAt(index.row()));
    }
}