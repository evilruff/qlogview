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


#ifndef _xFileProcessor_h_
#define _xFileProcessor_h_  1

#include <QObject>
#include <QThread>

#include "xdocument.h"

class xFileProcessorThread;

class   BusyFlag {
public:
    BusyFlag(QAtomicInt & flag):
        m_flag(flag) {
        m_flag    = 1;
    }

    ~BusyFlag() {
        m_flag = 0;
    }
protected:

    QAtomicInt &    m_flag;
};

typedef std::function<bool(quint64 startPosition, int lineLength, int lineNumber, const QString & content, bool bLastLine)> LineProcessFunction;

class	xFileProcessor: public QObject {
	Q_OBJECT
public:

    enum operationResult {
        requestCompleted   = 0,
        unableToOpenFile   = 1,
        userInterrupted    = 2,
        systemInterrupted  = 3
    };

	xFileProcessor();
    ~xFileProcessor();

    void    activate();   
    void    shutdown();
    bool    isBusy() const {
        return m_busyFlag;
    }
    void    interrupt();

    Q_INVOKABLE void    createIndex(QString fileName, int notifyPerLines, int blockSize);
    Q_INVOKABLE void    searchData(QString fileName, QByteArray codecName, searchRequestItem request, quint64 fromPosition, int maxOccurencies, int notifyPerLines, int blockSize);
    Q_INVOKABLE void    createFilter(QString fileName, QByteArray codecName, filterRules filter, int notifyPerLines, int blockSize);

    Q_INVOKABLE void    enabledWatch(const QString & fileName, lineData    lastKnownLine, int timeout = 1000);
    Q_INVOKABLE void    disableWatch();

    int isWatchEnabled() const;
    int currentProgress() const;
    
signals:

    void    indexDataReady(linesData indexData, bool bCompleted);
    void    filterDataReady(documentIndex indexData, bool bCompleted);
    void    searchResultsReady(searchResults indexData, bool bCompleted);

    void    progressChanged(int);
        
protected:

    void    setProgress(int value);

    operationResult    processPerLine(const QString & fileName, QTextCodec * pCodec, int blockSize, LineProcessFunction method, bool bTextRequired = true, quint64 startFromPosition = 0, bool bProgress = true);

    bool    checkFilters(const QString & text, const filterRules & filter);
    int     checkSearchItem(const QString & text, const searchRequestItem & item);

    void doFileWatch();
    
    void    init();
    void    done();

    virtual void    timerEvent(QTimerEvent * pEvent) override;

protected:

    const           int         m_watchBlockSize     = 1000000;
    const           int         m_watchNotifyPerLine = 1000;

    QAtomicInt                  m_currentProgress   = 0;
    QAtomicInt                  m_busyFlag          = 0;
    QAtomicInt                  m_shutdownFlag      = 0; 
    QAtomicInt                  m_interrupt         = 0;
    QAtomicInt                  m_watchEnabled      = 0;

    int                         m_watchTimer         = -1;
    lineData                    m_watchLastKnownLine = { 0, 0 };
    QString                     m_watchFileName;

    friend class                xFileProcessorThread;    
};

class xFileProcessorThread : public QThread {
        Q_OBJECT

    public:
        xFileProcessorThread(xFileProcessor * pProcessor);
        ~xFileProcessorThread();

    private:

        virtual void run() override;

    protected:

        xFileProcessor  *   m_pProcessor = nullptr;

};

#endif