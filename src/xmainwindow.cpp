#include <QWidget>
#include <QDir>
#include <QMenuBar>
#include <QGuiApplication>
#include <QFileDialog>
#include <QTabBar>
#include <QToolBar>
#include <QDockWidget>
#include <QTreeView>
#include <QMessageBox>
#include <QRandomGenerator>
#include <QFontDialog>
#include <QProgressBar>
#include <QStatusBar>
#include <QSettings>
#include <QTextCodec>
#include <QClipboard>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDesktopWidget>
#include <QApplication>

#include "xmainwindow.h"
#include "xlog.h"
#include "xinfopanel.h"
#include "xtimestamppanel.h"

//-------------------------------------------------
xMainWindow::xMainWindow(QWidget * pParent) :
    QMainWindow(pParent) {
    qCDebug(mainApp) << "xMainWindow: created";

    m_tabDocuments = new QTabWidget(this);
    connect(m_tabDocuments, &QTabWidget::currentChanged, this, &xMainWindow::onCurrentDocumentChanged);
    connect(m_tabDocuments, &QTabWidget::tabCloseRequested, this, &xMainWindow::onTabCloseRequested);

    setCentralWidget(m_tabDocuments);

    createMenu();

    m_infoPanel = new xInfoPanel();

    connect(m_infoPanel, &xInfoPanel::toggleHighlighterVisibility, this, &xMainWindow::onToggledHighlighterVisibility);
    connect(m_infoPanel, &xInfoPanel::toggleFilterRuleVisibility, this, &xMainWindow::onToggledFilterRuleVisibility);
    connect(m_infoPanel, &xInfoPanel::removeHighlighter, this, &xMainWindow::onRemoveHighlighter);
    connect(m_infoPanel, &xInfoPanel::createHightlightFromFilterRequest, this, &xMainWindow::onCreateHighlighterFromSearchRequest);
    connect(m_infoPanel, &xInfoPanel::removeAllHighlighters, this, &xMainWindow::onRemoveAllHighlighters);
    connect(m_infoPanel, &xInfoPanel::changeHighlighterColor, this, &xMainWindow::onChangeHighlighterColor);
    connect(m_infoPanel, &xInfoPanel::ensureBookmarkVisible, this, &xMainWindow::ensureBookmarkVisible);
    connect(m_infoPanel, &xInfoPanel::ensureSearchResultVisible, this, &xMainWindow::ensureSearchResultVisible);
    connect(m_infoPanel, &xInfoPanel::deleteFilterRequest, this, &xMainWindow::onDeleteFilterRequest);
    connect(m_infoPanel, &xInfoPanel::deleteAllFiltersRequest, this, &xMainWindow::onDeleteAllFiltersRequest);



    void    deleteFilterRequest(const filterRule & item);
    void    deleteAllFiltersRequest(const filterRule & item);


    QDockWidget * pInfoPanelDock = new QDockWidget();

    pInfoPanelDock->setWidget(m_infoPanel);
    pInfoPanelDock->setAllowedAreas(Qt::BottomDockWidgetArea | Qt::TopDockWidgetArea);
    pInfoPanelDock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);

    addDockWidget(Qt::BottomDockWidgetArea, pInfoPanelDock);

    QStatusBar * pStatusBar = statusBar();
    m_currentProgress = new QProgressBar();
    m_currentProgress->setMinimum(0);
    m_currentProgress->setMaximum(100);
    m_currentProgress->setFixedWidth(150);
    m_currentProgress->setAlignment(Qt::AlignCenter);
    pStatusBar->addPermanentWidget(m_currentProgress);

    resize(QDesktopWidget().availableGeometry(this).size() * 0.6);

    onCurrentDocumentChanged(-1);

    restoreSettings();

    showMessage(tr("Ready"));

    setWindowTitle(tr("QLogView"));
}
//-------------------------------------------------
xMainWindow::~xMainWindow() {
    qCDebug(mainApp) << "xMainWindow: destroyed";
}
//-------------------------------------------------
xDocument *    xMainWindow::load(const QString & fileName) {
    return appendDocument(fileName);
}
//-------------------------------------------------
void    xMainWindow::onCurrentDocumentChanged(int /*nIndex*/) {
    xPlainTextViewer * pViewer = currentViewer();

    showMessage(QString());

    disconnect(m_currentProgress);

    if (!pViewer) {
        m_viewerActions->setDisabled(true);
        m_infoPanel->bookmarks()->setModel(nullptr);
        m_infoPanel->findResults()->setModel(nullptr);
        m_infoPanel->filters()->setModel(nullptr);
        m_infoPanel->highlighters()->setModel(nullptr);

        m_currentProgress->setValue(0);
        m_currentProgress->setVisible(false);

        m_showLineNumbers->setChecked(false);
        m_wordWrap->setChecked(false);
        m_showBookmarks->setChecked(false);

        m_followTail->setChecked(false);
        m_autoRefresh->setChecked(false);
        return;
    }
    
    m_infoPanel->bookmarks()->setModel(pViewer->bookmarks());
    m_infoPanel->findResults()->setModel(pViewer->document()->findResults());
    m_infoPanel->filters()->setModel(pViewer->document()->filters());
    m_infoPanel->highlighters()->setModel(pViewer->highlighter()->highlighters());

    m_currentProgress->setValue(pViewer->document()->currentOperationProgress());
    m_currentProgress->setVisible((pViewer->document()->currentOperationProgress()) > 0 && (pViewer->document()->currentOperationProgress() < 100));

    connect(pViewer->document(), &xDocument::progressChanged, [this](int value) {
        m_currentProgress->setValue(value);
        m_currentProgress->setVisible((value > 0) && (value < 100));        
    });

    m_showLineNumbers->setChecked(pViewer->showLineNumbers());
    m_wordWrap->setChecked(pViewer->wordWrap());
    m_showBookmarks->setChecked(pViewer->showBookmarks());
    m_viewerActions->setDisabled(false);

    m_followTail->setChecked(pViewer->followTail());
    m_autoRefresh->setChecked(pViewer->document()->autoRefresh());

}
//-------------------------------------------------
xDocument *    xMainWindow::appendDocument(const QString & fileName) {
    QDir d;
    
    QString absolutePath = d.absoluteFilePath(fileName);
    if (!d.exists(absolutePath)) {
        qCDebug(mainApp) << "xMainWindow: open error, file " << fileName << " does not exists";
        return nullptr;
    }

    xDocument * pDocument = document(fileName);
    if (pDocument) {
        qCDebug(mainApp) << "xMainWindow: file " << fileName << " already opened";
        return pDocument;
    }

    if (!m_recentFiles.contains(fileName)) {
        m_recentFiles.enqueue(fileName);
        if (m_recentFiles.size() == m_maxRecentSize) {
            m_recentFiles.dequeue();
        }
    }

    xPlainTextViewer * pViewer = new xPlainTextViewer();
    
    pViewer->setFrameShape(QFrame::NoFrame);
    pViewer->setFont(QFont("Courier New", 8));
    pViewer->setContextMenuPolicy(Qt::CustomContextMenu);
    pViewer->searchPanel()->setFont(font());
   
    connect(pViewer, &QWidget::customContextMenuRequested, this, &xMainWindow::onViewerContextMenuRequested);
    connect(pViewer, &xPlainTextViewer::showFindResults, [this]() {
        m_infoPanel->activateFindResults();
    });

    connect(pViewer, &xPlainTextViewer::showFilters, [this]() {
        m_infoPanel->activateFilters();
    });

    xDocument * pNewDocument = new xDocument(pViewer); 

    connect(pNewDocument, &xDocument::message, this, &xMainWindow::showMessage);
    
    pNewDocument->setFilePath(absolutePath);
    pViewer->setDocument(pNewDocument);

    int nIndex = m_tabDocuments->addTab(pViewer, fileName);
    m_tabDocuments->setTabText(nIndex, pNewDocument->fileName());
    m_tabDocuments->setTabsClosable(true);
    m_tabDocuments->setMovable(true);

    m_tabDocuments->setCurrentIndex(nIndex);

    pNewDocument->invalidate();
    
    qCDebug(mainApp) << "xMainWindow: file " << fileName << " registered";

    return pNewDocument;
}
//-------------------------------------------------
void    xMainWindow::onDocumentReady() {
    xDocument * pDoc = qobject_cast<xDocument*>(sender());
    if (pDoc) {
        xPlainTextViewer*   pViewer = viewer(pDoc->filePath());
        m_tabDocuments->setCornerWidget(pViewer);
    }    
}
//-------------------------------------------------
void            xMainWindow::createMenu() {

    m_viewerActions = new QActionGroup(this);
    m_viewerActions->setExclusive(false);

    /*------------------------------------------------------------------------*/
    QMenu * pMenu = menuBar()->addMenu(tr("&File"));
    
    QAction * pAction = new QAction(tr("Open"), this);
    pAction->setShortcut(QKeySequence::Open);
    pAction->setStatusTip(tr("Open..."));
    connect(pAction, &QAction::triggered, this, &xMainWindow::onOpenFile);
    pMenu->addAction(pAction);

    pAction = new QAction(tr("Close"), this);
    pAction->setShortcut(QKeySequence::Close);
    pAction->setStatusTip(tr("Close file"));
    connect(pAction, &QAction::triggered, this, &xMainWindow::onCloseFile);
    pMenu->addAction(pAction);

    pAction = new QAction(tr("Close All"), this);
    pAction->setStatusTip(tr("Close all"));
    connect(pAction, &QAction::triggered, this, &xMainWindow::onCloseAllFiles);
    pMenu->addAction(pAction);

    pMenu->addSeparator();

    QMenu * pRecentMenu = new QMenu(this);
    pRecentMenu->setTitle("Recent files");
    m_recentSeparator = pRecentMenu->addSeparator();

    pAction = new QAction(tr("Clear recent"), pRecentMenu);
    connect(pAction, &QAction::triggered, [this]() {
        m_recentFiles.clear();
    });
    pRecentMenu->addAction(pAction);

    pRecentMenu->connect(pRecentMenu, &QMenu::aboutToShow, [pRecentMenu, this]() {
        QList<QAction*> actions = pRecentMenu->findChildren<QAction*>();
        for (QAction * action : actions) {
            if (action->property("recentFile").toBool())
                delete action;
        }

        QAction * insertBefore = m_recentSeparator;

        for (const QString & fileName : m_recentFiles) {
            QAction * pAction = new QAction(fileName, pRecentMenu);
            pAction->setProperty("recentFile", true);
            connect(pAction, &QAction::triggered, [this, insertBefore, fileName]() {
                load(fileName);
            });
            pRecentMenu->insertAction(insertBefore, pAction);
            insertBefore = pAction;
        }
    });

    pMenu->addMenu(pRecentMenu);
        
    pMenu->addSeparator();
    
    pAction = new QAction(tr("Open markup..."), this);
    connect(pAction, &QAction::triggered, [this]() { 
        loadMarkup();
    });
    pMenu->addAction(pAction);

    pAction = new QAction(tr("Save markup..."), this);
    connect(pAction, &QAction::triggered, [this]() {
        saveMarkup();
    });
    pMenu->addAction(pAction);

    pRecentMenu = new QMenu(this);
    pRecentMenu->setTitle("Recent markups");
    m_recentMarkupSeparator = pRecentMenu->addSeparator();

    pAction = new QAction(tr("Clear recent"), pRecentMenu);
    connect(pAction, &QAction::triggered, [this]() {
        m_recentMarkups.clear();
    });
    pRecentMenu->addAction(pAction);

    pRecentMenu->connect(pRecentMenu, &QMenu::aboutToShow, [pRecentMenu, this]() {
        QList<QAction*> actions = pRecentMenu->findChildren<QAction*>();
        for (QAction * action : actions) {
            if (action->property("recentFile").toBool())
                delete action;
        }

        QAction * insertBefore = m_recentMarkupSeparator;

        for (const QString & fileName : m_recentMarkups) {
            QAction * pAction = new QAction(fileName, pRecentMenu);
            pAction->setProperty("recentFile", true);
            connect(pAction, &QAction::triggered, [this, insertBefore, fileName]() {
                loadMarkup(fileName);
            });
            pRecentMenu->insertAction(insertBefore, pAction);
            insertBefore = pAction;
        }
    });

    pMenu->addMenu(pRecentMenu);

    pMenu->addSeparator();

    pAction = new QAction(tr("E&xit"), this);
    pAction->setShortcut(QKeySequence::Quit);
    pAction->setStatusTip(tr("Quit"));
    connect(pAction, &QAction::triggered, this, &xMainWindow::onExit);
    pMenu->addAction(pAction);

   

    /*------------------------------------------------------------------------*/

    pMenu = menuBar()->addMenu(tr("&View"));

    m_wordWrap = new QAction(tr("Word wrap"), this);
    m_wordWrap->setCheckable(true);
    m_wordWrap->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_W));
    m_wordWrap->setStatusTip(tr("Word wrap text"));
    connect(m_wordWrap, &QAction::triggered, [this]() {
        currentViewer()->setWordWrap(!currentViewer()->wordWrap());
        m_wordWrap->setChecked(currentViewer()->wordWrap());
    });       
    m_viewerActions->addAction(m_wordWrap);
    pMenu->addAction(m_wordWrap);
    

    m_showLineNumbers = new QAction(tr("Line numbers"), this);
    m_showLineNumbers->setCheckable(true);
    m_showLineNumbers->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_L));
    m_showLineNumbers->setStatusTip(tr("Show Line Numbers"));
    connect(m_showLineNumbers, &QAction::triggered, [this]() {
        currentViewer()->setShowLineNumbers(!currentViewer()->showLineNumbers());
        m_showLineNumbers->setChecked(currentViewer()->showLineNumbers());
    });
    m_viewerActions->addAction(m_showLineNumbers);
    pMenu->addAction(m_showLineNumbers);

    m_showBookmarks = new QAction(tr("Bookmarks"), this);
    m_showBookmarks->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_B));
    m_showBookmarks->setCheckable(true);
    m_showBookmarks->setStatusTip(tr("Show bookmark panel"));
    connect(m_showBookmarks, &QAction::triggered, [this]() {
        currentViewer()->setShowBookmarks(!currentViewer()->showBookmarks());
        m_showBookmarks->setChecked(currentViewer()->showBookmarks());
    });
    pMenu->addAction(m_showBookmarks);
    m_viewerActions->addAction(m_showBookmarks);
    
    pAction = new QAction(tr("Select font..."), this);
    pAction->setStatusTip(tr("Select font"));
    connect(pAction, &QAction::triggered, [this, pAction]() {
       bool bOk = false;
       QFont newFont = QFontDialog::getFont(&bOk, currentViewer()->viewport()->font(), this, "Select font");
       if (bOk) {
           currentViewer()->viewport()->setFont(newFont);
       }
    });
    m_viewerActions->addAction(pAction);
    pMenu->addAction(pAction);
    pMenu->addMenu(createEncodingMenu());
    pMenu->addSeparator();
    
    m_autoRefresh = new QAction(tr("Auto refresh"), this);
    m_autoRefresh->setCheckable(true);
    m_autoRefresh->setStatusTip(tr("Auto refresh active document"));
    connect(m_autoRefresh, &QAction::triggered, [this]() {
        bool bRefresh = currentViewer()->document()->autoRefresh();

        currentViewer()->document()->setAutoRefresh(!bRefresh);
        m_autoRefresh->setChecked(!bRefresh);

        if (bRefresh) {
            currentViewer()->setFollowTail(false);
            m_followTail->setChecked(false);
        }
    });
    pMenu->addAction(m_autoRefresh);
    m_viewerActions->addAction(m_autoRefresh);

    m_followTail = new QAction(tr("Follow tail"), this);
    m_followTail->setCheckable(true);
    m_followTail->setStatusTip(tr("Follow tail on active document"));
    connect(m_followTail, &QAction::triggered, [this]() {
        currentViewer()->setFollowTail(!currentViewer()->followTail());
        m_followTail->setChecked(currentViewer()->followTail());

        if (currentViewer()->followTail()) {
            currentViewer()->document()->setAutoRefresh(true);
            m_autoRefresh->setChecked(true);
        }
    });
    pMenu->addAction(m_followTail);
    m_viewerActions->addAction(m_followTail);

    /*------------------------------------------------------------------------*/

    pMenu = menuBar()->addMenu(tr("&Edit"));
    m_copyAction = new QAction(tr("C&opy"), this);
    m_copyAction->setShortcut(QKeySequence::Copy);
    m_copyAction->setStatusTip(tr("Copy selected text"));

    connect(m_copyAction, &QAction::triggered, [this]() {
        copySelectionToClipboard();
    });

    m_viewerActions->addAction(m_copyAction);
    pMenu->addAction(m_copyAction);

    connect(pMenu, &QMenu::aboutToShow, [this]() {
        xPlainTextViewer * pViewer = currentViewer();
        m_copyAction->setEnabled(pViewer && pViewer->hasSelection());
    });
    /*------------------------------------------------------------------------*/

    pMenu = menuBar()->addMenu(tr("&Search"));
    m_searchPanel = new QAction(tr("S&earch"), this);
    m_searchPanel->setShortcut(Qt::CTRL + Qt::Key_F);
    m_searchPanel->setCheckable(true);
    m_searchPanel->setStatusTip(tr("Show search panel"));

    connect(m_searchPanel, &QAction::triggered, [this]() {
        xPlainTextViewer * pViewer = currentViewer();
        if (pViewer) {
            if (pViewer->isSearchPanelShown())
                pViewer->hideSearchPanel();
            else
                pViewer->showSearchPanel();
        }
    });

    m_viewerActions->addAction(m_searchPanel);
    pMenu->addAction(m_searchPanel);

    connect(pMenu, &QMenu::aboutToShow, [this]() {
        xPlainTextViewer * pViewer = currentViewer();
        m_searchPanel->setChecked(pViewer && pViewer->isSearchPanelShown());
    });

    /*------------------------------------------------------------------------*/

    pMenu = menuBar()->addMenu(tr("&Bookmarks"));

    pAction = new QAction(tr("Next bookmark"), this);
    pAction->setShortcut(QKeySequence(Qt::Key_F2));
    pAction->setStatusTip(tr("Jump to next book mark"));
    connect(pAction, &QAction::triggered, [this]() {
        currentViewer()->nextBookmark();
    });
    pMenu->addAction(pAction);
    m_viewerActions->addAction(pAction);

    pAction = new QAction(tr("Previous bookmark"), this);
    pAction->setShortcut(QKeySequence(Qt::SHIFT + Qt::Key_F2));
    pAction->setStatusTip(tr("Jump to previous bookmark"));
    connect(pAction, &QAction::triggered, [this]() {
        currentViewer()->previousBookmark();
    });
    pMenu->addAction(pAction);
    m_viewerActions->addAction(pAction);

    /*------------------------------------------------------------------------*/
    pMenu = menuBar()->addMenu(tr("&About"));
    
    pAction = new QAction(tr("Help"), this);
    pAction->setDisabled(true);
    pAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_F1));
    pAction->setStatusTip(tr("Show help"));
    connect(pAction, &QAction::triggered, [this]() {
    });
    pMenu->addAction(pAction);

    pAction = new QAction(tr("About"), this);
    pAction->setStatusTip(tr("Show information about QLogViewer"));
    connect(pAction, &QAction::triggered, [this]() {
        QMessageBox::about(this, "About QLogView", tr(
            "<b>QLogView</b></br>"
            "<p>Open source log file viewer</p>"
            "<p>Copyright (C) 2020 Yuri Alexandrov</p>"
            "<p>Released under the GNU General Public License version 3</p>"
            "<p>evilruff@gmail.com<p>"
        ));
    });
    pMenu->addAction(pAction);

    pMenu->addSeparator();

    pAction = new QAction(tr("About Qt"), this);
    pAction->setStatusTip(tr("Show information about Qt"));
    connect(pAction, &QAction::triggered, [this]() {
        QApplication::aboutQt();
    });
    pMenu->addAction(pAction);



    pAction = new QAction(tr("Toggle bookmark"), this);
    pAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_F2));

    connect(pAction, &QAction::triggered, [this]() {
        xPlainTextViewer * pViewer = currentViewer();
        if (pViewer) {

            int nCurrentHover = pViewer->hoveredLineNumber();
            if (nCurrentHover >= 0) {
                pViewer->toggleBookmark(nCurrentHover, Qt::red);
                pViewer->viewport()->update();
            }
        }
    });
    m_viewerActions->addAction(pAction);
    addAction(pAction);

}
//-------------------------------------------------
xDocument *  xMainWindow::document(const QString & fileName) const {
    return documentAndViewer(fileName).second;
}
//-------------------------------------------------
void  xMainWindow::copySelectionToClipboard() {
    if (currentViewer()->logicalSelectionLength() > m_selectionWarningLimit) {
        if (QMessageBox::question(this,
            tr("Copy large text warning"),
            tr("Are you sure you want to copy so large selection into clipboard?"), QMessageBox::Yes, QMessageBox::Cancel) != QMessageBox::Yes) {
            return;
        }
    }

    QClipboard * pClipboard = QGuiApplication::clipboard();
    pClipboard->setText(currentViewer()->selection());
}
//-------------------------------------------------
xPlainTextViewer*                       xMainWindow::viewer(const QString & fileName) const {
    return documentAndViewer(fileName).first;
}
//-------------------------------------------------
QPair<xPlainTextViewer*, xDocument *>   xMainWindow::documentAndViewer(const QString & fileName) const {
    for (int i = 0; i < m_tabDocuments->count(); i++) {
        xPlainTextViewer * pViewer = qobject_cast<xPlainTextViewer*>(m_tabDocuments->widget(i));
        if (pViewer && pViewer->document() && pViewer->document()->filePath() == fileName)
            return QPair<xPlainTextViewer*, xDocument *>(pViewer, pViewer->document());
    }

    return QPair<xPlainTextViewer*, xDocument *>(nullptr,nullptr);
}
//-------------------------------------------------
xPlainTextViewer*                       xMainWindow::currentViewer() const {
    return qobject_cast<xPlainTextViewer*>(m_tabDocuments->currentWidget());
}
//-------------------------------------------------
xDocument*                          xMainWindow::currentDocument() const {
    xPlainTextViewer * pViewer = currentViewer();
    return pViewer ? pViewer->document() : nullptr;
}
void            xMainWindow::onOpenFile() {
    QString fileName = QFileDialog::getOpenFileName(this,
        tr("Open Log File"), "", tr("Log Files (*.txt *.log);;All Files (*.*)"));

   
    if (!fileName.isEmpty()) {
        load(fileName);
    }
}
//-------------------------------------------------
void            xMainWindow::onCloseFile() {
    QWidget * pWidget = m_tabDocuments->currentWidget();
    if (pWidget) {
        pWidget->deleteLater();
    }
}
//-------------------------------------------------
void            xMainWindow::onCloseAllFiles() {
    while (m_tabDocuments->count()) {
        m_tabDocuments->removeTab(0);
    }
}
//-------------------------------------------------
void            xMainWindow::onExit() {
    saveSettings();
    QCoreApplication::quit();
}
//-------------------------------------------------
QMenu *         xMainWindow::createEncodingMenu() {
    QActionGroup * encodingGroup = new QActionGroup(this);

    QList<QByteArray>       availableCodes = QTextCodec::availableCodecs();

    if (!availableCodes.size())
        return nullptr;

    std::sort(availableCodes.begin(), availableCodes.end(), [](const QByteArray & item1, const QByteArray & item2) { return QString::fromLatin1(item1).toUpper() < QString::fromLatin1(item2).toUpper(); });
    QMenu * pMenu = new QMenu;
    pMenu->setTitle(tr("Encodings"));

    for (const QByteArray & name : availableCodes) {
        QAction * codecAction = new QAction(QString::fromLatin1(name.constData()), pMenu);
        codecAction->setCheckable(true);
        pMenu->addAction(codecAction);
        encodingGroup->addAction(codecAction);
        
        m_encodings[name] = codecAction;

        connect(pMenu, &QMenu::aboutToShow, [this]() {

            xPlainTextViewer * pViewer = currentViewer();
            if (pViewer) {
                for (QMap<QByteArray, QAction*>::key_value_iterator it = m_encodings.keyValueBegin(); it != m_encodings.keyValueEnd(); it++) {
                    (*it).second->setDisabled(false);
                }

                m_encodings[pViewer->textCodec()->name()]->setChecked(true);
            }
            else {
                for (QMap<QByteArray, QAction*>::key_value_iterator it = m_encodings.keyValueBegin(); it != m_encodings.keyValueEnd(); it++) {
                    (*it).second->setDisabled(true);
                }
            }
        });

        connect(codecAction, &QAction::triggered, [this, name]() {
            setCurrentDocumentEncoding(name);
        });
    }

    return pMenu;
}
//-------------------------------------------------
void            xMainWindow::setCurrentDocumentEncoding(const QByteArray & codecName) {
    xPlainTextViewer * pViewer = currentViewer();
    if (!pViewer)
        return;

    QTextCodec * pCodec = QTextCodec::codecForName(codecName);
    if (!pCodec)
        return;

    pViewer->setTextCodec(pCodec);
}
//-------------------------------------------------
void    xMainWindow::onViewerContextMenuRequested(const QPoint& pt) {  

    xPlainTextViewer * pViewer = qobject_cast<xPlainTextViewer*>(sender());
    if (!pViewer)
        return;

    QMenu contextMenu(tr("Viewer"));

    QAction createPatternMatch(tr("Highlight selected pattern"), &contextMenu);
    createPatternMatch.setDisabled(!pViewer->hasSelection());
    connect(&createPatternMatch, &QAction::triggered, [pViewer,this]() {
        if (pViewer->logicalLinesInSelection() > 1) {
            QMessageBox::warning(this,
                tr("Unable to create pattern match"),
                tr("Can't create pattern match with selection contains more then one line"));
        }
        else {
            QTextCharFormat     textFormat;
            textFormat.setFontWeight(QFont::Bold);
            textFormat.setForeground(QColor::fromRgb(QRandomGenerator::global()->generate()));
            pViewer->highlighter()->appendHighlight(QStringMatcher(pViewer->selection()), textFormat);
            m_infoPanel->activateHighlights();
        }
    });
    contextMenu.addAction(&createPatternMatch);
    createPatternMatch.setDisabled(!pViewer->hasSelection());

    QAction createPositionMatch(tr("Highlight selected range"), &contextMenu);
    createPositionMatch.setDisabled(!pViewer->hasSelection());
    connect(&createPositionMatch, &QAction::triggered, [pViewer, this]() {
        if (pViewer->logicalLinesInSelection() > 1) {
            QMessageBox::warning(this,
                tr("Unable to create position match"),
                tr("Can't create position match with selection contains more then one line"));
        }
        else {
            QTextCharFormat     textFormat;
            textFormat.setFontWeight(QFont::Bold);
            textFormat.setForeground(QColor::fromRgb(QRandomGenerator::global()->generate()));
            pViewer->highlighter()->appendHighlight(pViewer->selectionColumnStart(), pViewer->selectionColumnStart() + pViewer->logicalSelectionLength(), textFormat);
            m_infoPanel->activateHighlights();
        }
    });

    contextMenu.addAction(&createPositionMatch);

    QAction searchGlobal(tr("Search selected..."), &contextMenu);
    searchGlobal.setDisabled(!pViewer->hasSelection());
    connect(&searchGlobal, &QAction::triggered, [pViewer, this]() {
        if (pViewer->logicalLinesInSelection() > 1) {
            QMessageBox::warning(this,
                tr("Unable to search"), 
                tr("Can't search with selection contains more then one line"));
        }
        else {
            pViewer->showSearchPanel();
            pViewer->searchPanel()->setSearchText(pViewer->selection());    
            pViewer->resetSelection();
        }
    });

    contextMenu.addAction(&searchGlobal);
    

    QAction toggleBookmarkMatch(tr("Toggle bookmark"), &contextMenu);
    toggleBookmarkMatch.setShortcut(QKeySequence(Qt::CTRL + Qt::Key_F2));
    toggleBookmarkMatch.setDisabled(pViewer->hoveredLineNumber() == -1);

    connect(&toggleBookmarkMatch, &QAction::triggered, [pt, pViewer, this]() {       
            int nLine = pViewer->logicalLineForPosition(pt);

            if (nLine >= 0) {
                if (pViewer->toggleBookmark(nLine, Qt::red)) {
                    m_infoPanel->activateBookmarks();
                }
                pViewer->viewport()->update();
            }        
    });

    contextMenu.addAction(&toggleBookmarkMatch);
    
    contextMenu.addSeparator();

    QAction copyAction(tr("C&opy"), &contextMenu);
    copyAction.setShortcut(QKeySequence::Copy);
    copyAction.setStatusTip(tr("Copy selected text"));
    copyAction.setDisabled(!currentViewer()->hasSelection());

    connect(&copyAction, &QAction::triggered, [this]() {
        copySelectionToClipboard();
    });

    contextMenu.addAction(&copyAction);

    contextMenu.addSeparator();

    QAction defineTimestamp(tr("Define timestamp..."), &contextMenu);
    defineTimestamp.setDisabled(!currentViewer()->hasSelection());

    connect(&defineTimestamp, &QAction::triggered, [pt, pViewer, this]() {
        xTimeStampPanel panel(this);
        QString sampleText = pViewer->selection();
        if (!sampleText.isEmpty()) {
            panel.setSampleText(pViewer->selection());
        }
        else if (pViewer->hoveredLineNumber() != -1) {
            panel.setSampleText(pViewer->hoveredLine());
        }

        if (panel.exec() == QDialog::Accepted) {
            if (panel.isValidFormat()) {
                pViewer->setTimestampFormat(panel.format(), pViewer->selectionColumnStart(), panel.detectedSourceDataLength());
                m_infoPanel->activateHighlights();
            }
        }
    });

    contextMenu.addAction(&defineTimestamp);

    QAction resetTimestamp(tr("Reset timestamp"), &contextMenu);
    resetTimestamp.setDisabled(!currentViewer()->timestampDefined());

    connect(&resetTimestamp, &QAction::triggered, [pt, pViewer, this]() {
        pViewer->setTimestampFormat(QString(), 0, 0);        
    });

    contextMenu.addAction(&resetTimestamp);


    
    contextMenu.exec(pViewer->mapToGlobal(pt));
}
//-------------------------------------------------
void    xMainWindow::onToggledFilterRuleVisibility(const filterRule & rule) {
    xPlainTextViewer * pViewer = currentViewer();
    if (pViewer) {
        pViewer->document()->toggleFilterRuleVisibility(rule);
        applyActiveFilters();
    }
}
//-------------------------------------------------
void    xMainWindow::onToggledHighlighterVisibility(const highlighterItem & item) {
    xPlainTextViewer * pViewer = currentViewer();
    if (pViewer) {
        pViewer->highlighter()->toggleHighligherVisibility(item);
    }
}
//-------------------------------------------------
void    xMainWindow::onRemoveHighlighter(const highlighterItem & item) {
    xPlainTextViewer * pViewer = currentViewer();
    if (pViewer) {
        pViewer->highlighter()->removeHighlighter(item);
    }
}
//-------------------------------------------------
void    xMainWindow::onCreateHighlighterFromSearchRequest(const searchRequestItem & item) {
    xPlainTextViewer * pViewer = currentViewer();

    if (pViewer) {
        QTextCharFormat     textFormat;
        textFormat.setFontWeight(QFont::Bold);
        textFormat.setForeground(QColor::fromRgb(QRandomGenerator::global()->generate()));
        if (item.type == PatternSearch) {
            pViewer->highlighter()->appendHighlight(item.matcher, textFormat);
        } else if (item.type == RegExpSearch) {
            pViewer->highlighter()->appendHighlight(item.regexp, textFormat);
        }
        m_infoPanel->activateHighlights();
    }    
}
//-------------------------------------------------
void     xMainWindow::onRemoveAllHighlighters() {  
    xPlainTextViewer * pViewer = currentViewer();
    if (pViewer) {
        pViewer->highlighter()->removeAll();
    }
}
//-------------------------------------------------
void     xMainWindow::onChangeHighlighterColor(const highlighterItem & item, const QColor & color) {
    xPlainTextViewer * pViewer = currentViewer();
    if (pViewer) {
        pViewer->highlighter()->setHighligherColor(item, color);
    }
}
//-------------------------------------------------
void    xMainWindow::showMessage(const QString & message, int timeout) {
    statusBar()->showMessage(message, timeout);
}
//-------------------------------------------------
void            xMainWindow::saveSettings() {
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "LogViewer", "LogViewer");
    settings.setValue("main/geometry", saveGeometry());
    settings.setValue("main/windowState", saveState());
    
    settings.setValue("main/recent files", QVariant::fromValue<QStringList>(m_recentFiles.toVector().toList()));
    settings.setValue("main/recent markups", QVariant::fromValue<QStringList>(m_recentMarkups.toVector().toList()));

}
//-------------------------------------------------
void            xMainWindow::restoreSettings() {
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "LogViewer", "LogViewer");

    restoreGeometry(settings.value("main/geometry").toByteArray());
    restoreState(settings.value("main/windowState").toByteArray());

    QStringList recentFiles = settings.value("main/recent files").toStringList();
    for (const QString & fileName : recentFiles) {
        m_recentFiles << fileName;
    }

    recentFiles = settings.value("main/recent markups").toStringList();
    for (const QString & fileName : recentFiles) {
        m_recentMarkups << fileName;
    }
}

void    xMainWindow::onTabCloseRequested(int index) {
    QWidget * pWidget = m_tabDocuments->widget(index);
    if (pWidget) {
        pWidget->deleteLater();
    }
}

void   xMainWindow::ensureSearchResultVisible(const searchResult & item) {
    xPlainTextViewer * pViewer = currentViewer();
    if (pViewer) {
        pViewer->ensureSearchResultVisible(item);
    }
}

void   xMainWindow::ensureBookmarkVisible(const documentBookmark & item) {
    xPlainTextViewer * pViewer = currentViewer();
    if (pViewer) {
        pViewer->ensureBookmarkVisible(item);
    }
}

void xMainWindow::saveMarkup() {
    xPlainTextViewer * pViewer = currentViewer();
    if (!pViewer)
        return;

    QString fileName = QFileDialog::getSaveFileName(this,
        tr("Save Markup File"), "", tr("Markup Files (*.markup);;All Files (*.*)"));

    if (fileName.isEmpty())
        return;

    if (!m_recentMarkups.contains(fileName)) {
        m_recentMarkups.enqueue(fileName);
        if (m_recentMarkups.size() == m_maxRecentSize) {
            m_recentMarkups.dequeue();
        }
    }

    QJsonDocument   d;
    QJsonObject     object;
   
    if (pViewer->timestampDefined()) {
        QJsonObject     timeStampObject;

        timeStampObject["format"] = pViewer->timestampFormat();
        timeStampObject["position"] = pViewer->timestampPosition();
        timeStampObject["length"] = pViewer->timestampLength();

        object["timestamp"] = timeStampObject;
    }

    QJsonArray  highlighters;

    for (const highlighterItem & item : pViewer->highlighter()->highlighters()->items()) {
        
        QJsonObject object;
        if (item.format.fontWeight() == QFont::Bold) {
            object["bold"] = true;
        }

        object["color"] = QString::number(item.format.foreground().color().rgb(), 16);

        object["type"] = item.type;

        if (item.type == PositionHighlighter) {
            object["from"] = item.from;
            object["to"] = item.to;
        }
        else {
            object["text"] = item.pattern();
        }

        highlighters << object;
    }

    object["highlighters"] = highlighters;

    d.setObject(object);
    QByteArray  content = d.toJson();

    QFile   f(fileName);
    if (f.open(QIODevice::WriteOnly)) {
        f.write(content);
    }

    f.close();
}

void xMainWindow::loadMarkup(const QString & name) {

    xPlainTextViewer * pViewer = currentViewer();
    if (!pViewer)
        return;

    QString fileName = name.isEmpty() ? (QFileDialog::getOpenFileName(this,
        tr("Open Markup File"), "", tr("Markup Files (*.markup);;All Files (*.*)"))) : name;

    if (fileName.isEmpty())
        return;

    if (!m_recentMarkups.contains(fileName)) {
        m_recentMarkups.enqueue(fileName);
        if (m_recentMarkups.size() == m_maxRecentSize) {
            m_recentMarkups.dequeue();
        }
    }

    QFile f(fileName);
    if (!f.open(QIODevice::ReadOnly))
        return;

    QByteArray  data = f.readAll();

    f.close();

    QJsonDocument   d = QJsonDocument::fromJson(data);
    
    QString     timestampFormat = (d.object()["timestamp"]).toObject()["format"].toString();
    int         startPosition  = (d.object()["timestamp"]).toObject()["position"].toInt();
    int         length         = (d.object()["timestamp"]).toObject()["length"].toInt();

    pViewer->highlighter()->removeAll();
    pViewer->setTimestampFormat(timestampFormat, startPosition, length);
    
    for (const QJsonValue & val : (d.object()["highlighters"].toArray())) {
        QJsonObject object = val.toObject();
        highlighterItem item;
        
        item.type = (HighlighterType)object["type"].toInt();
        
        if (object["bold"].toBool()) {
            item.format.setFontWeight(QFont::Bold);
        }
        
        item.format.setForeground(QColor::fromRgb(object["color"].toString().toInt(nullptr, 16)));

        if (item.type == PositionHighlighter) {
            item.from   = object["from"].toInt();
            item.to     = object["to"].toInt();
        }

        if (item.type == PatternHighlighter) {
            item.matcher = QStringMatcher(object["text"].toString());
        }

        if (item.type == RegExpSearch) {
            item.regexp = QRegularExpression(object["text"].toString());
        }
              
        item.isActive = true;
        pViewer->highlighter()->appendHighlight(item);
    }

}

void    xMainWindow::onDeleteFilterRequest(const filterRule & rule) {
    xPlainTextViewer * pViewer = currentViewer();
    if (!pViewer)
        return;

    xValueCollection<filterRule>   * pFilters = dynamic_cast<xValueCollection<filterRule>*>(pViewer->document()->filters());
    pFilters->removeItem(rule);
    
    applyActiveFilters();
}

void    xMainWindow::onDeleteAllFiltersRequest() {
    xPlainTextViewer * pViewer = currentViewer();
    if (!pViewer)
        return;

    xValueCollection<filterRule>   * pFilters = dynamic_cast<xValueCollection<filterRule>*>(pViewer->document()->filters());
    pFilters->clear();
    pViewer->document()->setFilterRulesEnabled(false);
}

void    xMainWindow::applyActiveFilters() {
    xPlainTextViewer * pViewer = currentViewer();
    if (!pViewer)
        return;

    xValueCollection<filterRule>   * pFilters = dynamic_cast<xValueCollection<filterRule>*>(pViewer->document()->filters());

    bool bHasActiveFilters = false;
    for (const filterRule & item : pFilters->items()) {
        bHasActiveFilters |= item.isActive;
    }
    if (bHasActiveFilters) {
        pViewer->document()->filter(pFilters->items(), pViewer->textCodec()->name());
    }
    else {
        pViewer->document()->setFilterRulesEnabled(false);
        showMessage(tr("Filters disabled"), 5000);
    }
}
