#ifndef _xMainWindow_h_
#define _xMainWindow_h_ 1

#include <QMainWindow>
#include <QActionGroup>
#include <QQueue>

#include "xplaintextviewer.h"
#include "xdocument.h"
#include "xhighlighter.h"

class QTabWidget;
class QToolBar;
class xInfoPanel;
class QProgressBar;

class xMainWindow : public QMainWindow {
    Q_OBJECT
public:

    xMainWindow(QWidget * pParent = nullptr);
    ~xMainWindow();

    xDocument *    load(const QString & fileName);

    virtual QMenu *     createPopupMenu() override {
        return nullptr;
    }

public slots:
    
    void    onDocumentReady();
    void    onOpenFile();
    void    onCloseFile();
    void    onCloseAllFiles();
    void    onExit();

    void    onViewerContextMenuRequested(const QPoint& pt);
    
    void    onToggledHighlighterVisibility(const highlighterItem & item);
    void    onToggledFilterRuleVisibility(const filterRule & rule);

    void    onRemoveHighlighter(const highlighterItem & item);
    
    void    onCreateHighlighterFromSearchRequest(const searchRequestItem & item);
    void    onDeleteFilterRequest(const filterRule & rule);
    void    onDeleteAllFiltersRequest();
    
    void    onRemoveAllHighlighters();
    void    onChangeHighlighterColor(const highlighterItem & item, const QColor & color);

    void    onCurrentDocumentChanged(int nIndex);

    void    showMessage(const QString & message, int timeout = 0);

    void    onTabCloseRequested(int index);

    void    ensureBookmarkVisible(const documentBookmark & item);
    void    ensureSearchResultVisible(const searchResult & item);
   
protected:

    void            saveSettings();
    void            restoreSettings();

    void            createMenu();
    QMenu *         createEncodingMenu();
    void            setCurrentDocumentEncoding(const QByteArray & codecName);

    void            copySelectionToClipboard();
    void            applyActiveFilters();

    xDocument *     appendDocument(const QString & fileName);
    
    xDocument *                             document(const QString & fileName) const;
    xPlainTextViewer*                       viewer(const QString & fileName) const;    
    QPair<xPlainTextViewer*, xDocument*>    documentAndViewer(const QString & fileName) const;

    xPlainTextViewer*                       currentViewer() const;
    xDocument*                              currentDocument() const;

    void                                    saveMarkup();
    void                                    loadMarkup(const QString & name = QString());


protected:
    const   int                 m_selectionWarningLimit = 10000000;
    const   int                 m_maxRecentSize         = 10;
        
    QTabWidget             *    m_tabDocuments       = nullptr;
    QToolBar               *    m_toolDocuments      = nullptr;

    xInfoPanel             *    m_infoPanel          = nullptr;

    QProgressBar           *    m_currentProgress    = nullptr;

    QActionGroup           *    m_viewerActions      = nullptr;    

    QMap<QByteArray, QAction*>  m_encodings;

    QQueue<QString>          m_recentFiles;
    QQueue<QString>          m_recentMarkups;

    QAction                * m_wordWrap           = nullptr;
    QAction                * m_searchPanel        = nullptr;
    QAction                * m_showLineNumbers    = nullptr;
    QAction                * m_showBookmarks      = nullptr;
    QAction                * m_followTail         = nullptr;
    QAction                * m_autoRefresh        = nullptr;
    QAction                * m_recentSeparator    = nullptr;
    QAction                * m_recentMarkupSeparator = nullptr;
    QAction                * m_copyAction         = nullptr;
};

#endif