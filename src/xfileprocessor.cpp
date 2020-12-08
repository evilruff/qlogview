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

#include <QElapsedTimer>
#include <QTextCodec>
#include <QTimerEvent>

#include "xfileprocessor.h"
#include "xlog.h"

xFileProcessor::xFileProcessor():
    QObject() {
    m_shutdownFlag = 0;
    xFileProcessorThread * pThread = new xFileProcessorThread(this);
    pThread->setObjectName("xFileProcessorThread");
    connect(pThread, &QThread::finished, pThread, &QThread::deleteLater);
    moveToThread(pThread);
    qCDebug(logicDocument) << "xFileProcessor: created";
}

void    xFileProcessor::activate() {
    thread()->start();
}

xFileProcessor::~xFileProcessor() {
    qCDebug(logicDocument) << "xFileProcessor: destroying";
    shutdown();
    qCDebug(logicDocument) << "xFileProcessor: destroyed";
}

void    xFileProcessor::init() {
    qCDebug(logicDocument) << "xFileProcessor: init";
}

void    xFileProcessor::done() {
    qCDebug(logicDocument) << "xFileProcessor: done";
}

void    xFileProcessor::shutdown() {
    m_shutdownFlag = 1;
    qCDebug(logicDocument) << "xFileProcessor: finishing worker thread";
    thread()->quit();
    thread()->wait();
    qCDebug(logicDocument) << "xFileProcessor: worker thread finished";
    deleteLater();
}

xFileProcessorThread::xFileProcessorThread(xFileProcessor * pProcessor) :
    QThread() {
    qCDebug(logicDocument) << "xFileProcessorThread: created";
    m_pProcessor = pProcessor;
}

xFileProcessorThread::~xFileProcessorThread() {
    qCDebug(logicDocument) << "xFileProcessorThread: destroyed";
}

void xFileProcessorThread::run() {
    qCDebug(logicDocument) << "xFileProcessorThread: run() started ";
    if (m_pProcessor)
        m_pProcessor->init();

    exec();

    if (m_pProcessor) {
        m_pProcessor->done();       
    }
    qCDebug(logicDocument) << "xFileProcessorThread: run() finished";
}

void    xFileProcessor::searchData(QString fileName, QByteArray codecName, searchRequestItem request, quint64 /* fromPosition */, int maxOccurences, int notifyPerLines, int blockSize) {
    BusyFlag    busy(m_busyFlag);

    searchResults    currentPart;
    QElapsedTimer   et;
    et.start();
    int              nTotalFound = 0;

    setProgress(0);

    QTextCodec * pCodec = QTextCodec::codecForName(codecName);
    if (!pCodec) {
        pCodec = QTextCodec::codecForLocale();
    }
    
    processPerLine(fileName, pCodec, blockSize, [this, &currentPart, &request, notifyPerLines, maxOccurences, &nTotalFound](quint64 startPosition, int /* lineLength */, int lineNumber, const QString & content, bool bLastLine) {

        int nMatchLength = checkSearchItem(content, request);

        if (nMatchLength) {

            searchResult item;
            item.position             = startPosition;
            item.matchLength     = nMatchLength;
            item.lineNumber      = lineNumber;
            item.line            = content;

            currentPart << item;

            if ((!bLastLine &&currentPart.size() == notifyPerLines) || bLastLine) {
                emit searchResultsReady(currentPart, bLastLine);
                currentPart.clear();
            }

            nTotalFound++;

            if ((maxOccurences > 0) && (nTotalFound == maxOccurences)) {
                emit searchResultsReady(currentPart, true);
                currentPart.clear();
                return false;
            }
        }
        else {
            if (bLastLine) {
                emit searchResultsReady(currentPart, bLastLine);
                currentPart.clear();
            }
        }

        return true;
    }, true);

    setProgress(100);

    qCDebug(logicDocument) << "xFileProcessor: search in file " << fileName << " done in " << et.elapsed() << " ms";
}

void    xFileProcessor::createIndex(QString fileName, int notifyPerLines, int blockSize) {
    BusyFlag    busy(m_busyFlag);
 
    linesData    currentPart;

    setProgress(0);

    QElapsedTimer   et;
    et.start();

    processPerLine(fileName, nullptr, blockSize, [this, &currentPart, notifyPerLines](quint64 startPosition, int lineLength, int /*lineNumber*/, const QString & /* content */, bool bLastLine) {
        currentPart << lineData{ startPosition, lineLength };
        if ((!bLastLine &&currentPart.size() == notifyPerLines) || bLastLine) {
            emit indexDataReady(currentPart, bLastLine);
            currentPart.clear();
        }
        return true;
    }, false);

    qCDebug(logicDocument) << "xFileProcessor: rebuilding file " << fileName << " done in " << et.elapsed() << " ms";

    setProgress(100);
}

void    xFileProcessor::createFilter(QString fileName, QByteArray codecName, filterRules filter, int notifyPerLines, int blockSize) {
    BusyFlag    busy(m_busyFlag);

    setProgress(0);

    documentIndex    currentPart;
    QElapsedTimer   et;
    et.start();
    int              nFilterLines = 0;

    QTextCodec * pCodec = QTextCodec::codecForName(codecName);
    if (!pCodec) {
        pCodec = QTextCodec::codecForLocale();
    }


    processPerLine(fileName, pCodec, blockSize, [this, &currentPart, &filter, notifyPerLines, &nFilterLines](quint64 /*startPosition*/, int /*lineLength*/, int lineNumber, const QString & content, bool bLastLine) {
        if (checkFilters(content, filter)) {
            currentPart.forwardIndex[lineNumber]   = nFilterLines;
            currentPart.reverseIndex[nFilterLines] = lineNumber;
            nFilterLines++;

            if ((!bLastLine &&currentPart.forwardIndex.size() == notifyPerLines) || bLastLine) {
                emit filterDataReady(currentPart, bLastLine);
                currentPart = documentIndex();
            }
        } else{
            if (bLastLine) {
                emit filterDataReady(currentPart, bLastLine);
                currentPart = documentIndex();
            }
        }
        return true;
    }, true);

    setProgress(100);

    qCDebug(logicDocument) << "xFileProcessor: create filter in file " << fileName << " done in " << et.elapsed() << " ms";    
}

void xFileProcessor::doFileWatch() {
    linesData    currentPart;

    processPerLine(m_watchFileName, nullptr, m_watchBlockSize, [this, &currentPart](quint64 startPosition, int lineLength, int /*lineNumber*/, const QString & /* content */, bool bLastLine) {

        if ((lineLength > 0) && (m_watchLastKnownLine != lineData{startPosition, lineLength})) {
            currentPart << lineData{ startPosition, lineLength };
        }

        m_watchLastKnownLine = lineData{ startPosition, lineLength };

        if ((!bLastLine &&currentPart.size() == m_watchNotifyPerLine) || (bLastLine && currentPart.size())) {
            emit indexDataReady(currentPart, bLastLine);
            currentPart.clear();
        }
        return true;
    }, false, m_watchLastKnownLine.position, false);
}

xFileProcessor::operationResult    xFileProcessor::processPerLine(const QString & fileName, QTextCodec * pCodec, int blockSize, LineProcessFunction method, bool bTextRequired, quint64 startFromPosition, bool bProgress) {
    QFile f(fileName);
    if (!f.open(QIODevice::ReadOnly)) {
        return unableToOpenFile;
    }

    QByteArray          block;
    block.reserve(blockSize);
    quint64 nCurrentPosition  = startFromPosition;
    quint64 nCurrentLineStart = startFromPosition;
    quint64 blockStart        = startFromPosition;
    quint32 nBytesReaded = 0;
    int     nCurrentLineLength = 0;
    int     nCurrentLineNumber = 0;

    bool    bAtEnd = false;

    quint64 totalSize = f.size();
    f.seek(startFromPosition);

    QByteArray  lineTail;

    do {       
        nCurrentLineLength  = lineTail.size();

        blockStart          = f.pos();
        nBytesReaded        = f.read(block.data(), blockSize);
        bAtEnd              = f.atEnd();
        
        int currentProgress = 100.*(double)nCurrentPosition / (double)totalSize;

        if (bProgress)
            setProgress(currentProgress);
        
        for (quint32 i = 0; i < nBytesReaded; i++) {
            if (m_shutdownFlag)
                return systemInterrupted;

            if (m_interrupt) {
                m_interrupt = 0;
                return userInterrupted;
            }
        
            nCurrentLineLength++;
            if (*(block.constData() + i) == 0x0A) {
                QString text;

                if (bTextRequired) {
                    if (lineTail.size()) {
                        lineTail.append((const char*)block.constData(), i+1);
                        text = pCodec->toUnicode(lineTail, lineTail.size());
                        lineTail.clear();
                    }
                    else {
                        text = pCodec->toUnicode((block.constData() + nCurrentLineStart - blockStart), nCurrentLineLength);
                    }
                }

                if (!method(nCurrentLineStart, nCurrentLineLength, nCurrentLineNumber, text, (bAtEnd && (i == (nBytesReaded - 1))))) {
                    return requestCompleted;
                }

                nCurrentLineNumber++;
                nCurrentLineStart  = nCurrentPosition + 1;
                nCurrentLineLength = 0;
            }

            nCurrentPosition++;
        }

        if (!bAtEnd) {
            if (nCurrentLineLength) {
                lineTail = QByteArray((const char *)(block.constData() + nBytesReaded - nCurrentLineLength - 1), nCurrentLineLength);
            }
        }
        else {
            QString text;

            if (bTextRequired) {
                if (lineTail.size()) {
                    lineTail.append((block.constData() + nCurrentLineStart - blockStart), nCurrentLineLength);
                    text = pCodec->toUnicode(lineTail, lineTail.size());
                }
                else {
                    text = pCodec->toUnicode((block.constData() + nCurrentLineStart - blockStart), nCurrentLineLength);
                }
            }

            method(nCurrentLineStart, nCurrentLineLength, nCurrentLineNumber, text, true);
        }
    } while (!bAtEnd); 

    return requestCompleted;
}

void    xFileProcessor::enabledWatch(const QString & fileName, lineData lastKnownLine, int timeout) {
    if (m_watchEnabled) {
        disableWatch();
    }

    m_watchLastKnownLine = lastKnownLine;
    m_watchFileName     = fileName;
    m_watchTimer        = startTimer(timeout);
    m_watchEnabled      = 1;
}

void    xFileProcessor::disableWatch() {
    if (m_watchTimer > 0) {
        killTimer(m_watchTimer);
    }
    m_watchFileName = QString();
    m_watchLastKnownLine = { 0, 0 };
    m_watchEnabled = 0;
}

int xFileProcessor::isWatchEnabled() const {
    return m_watchEnabled;
}

int xFileProcessor::currentProgress() const {
    return m_currentProgress;
}

void    xFileProcessor::timerEvent(QTimerEvent * pEvent) {
    if (pEvent->timerId() == m_watchTimer) {
        doFileWatch();
    }
}

bool    xFileProcessor::checkFilters(const QString & text, const filterRules & filter) {
    
    bool bFound = true;

    for (const filterRule & rule : filter) {
        if (!rule.isActive)
            continue;

        bFound &= (checkSearchItem(text, rule.filter) > 0);

        if (!bFound)
            return false;            
    }

    return true;
}

int    xFileProcessor::checkSearchItem(const QString & text, const searchRequestItem & item) {
    int nStartIndex = 0;

    if (item.type == PatternSearch) {

        if (item.matcher.indexIn(text, nStartIndex) != -1)
            return item.matcher.pattern().size();
    }
    else if (item.type == RegExpSearch) {
        QRegularExpressionMatchIterator i = item.regexp.globalMatch(text);
        if (i.hasNext()) {
            return i.next().capturedLength();
        }
    }

    return 0;
}

void    xFileProcessor::interrupt() {
    m_interrupt = 1;
    while (isBusy());
    m_interrupt = 0;
}

void    xFileProcessor::setProgress(int value) {
    if (m_currentProgress == value)
        return;

    m_currentProgress = value;
    emit progressChanged(m_currentProgress);
}
