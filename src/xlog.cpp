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


#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QDateTime>
#include <QCoreApplication>

#include <QFileInfo>
#include <QDir>
#include <QMutexLocker>

#include <QJsonDocument>
#include <QJsonObject>
#include <QThread>

#include "xlog.h"
#include "xsysteminformation.h"

#ifdef Q_OS_WIN
	#include <windows.h>
#endif

#ifdef Q_OS_UNIX
	#include <syslog.h>
#endif

xLog * xLog::m_pLog = nullptr;
#ifdef Q_OS_WIN32
	static	wchar_t *  logFileName = nullptr;
#else
	static	char *  logFileName = nullptr;
#endif

Q_LOGGING_CATEGORY(mainApp, "main.app")
Q_LOGGING_CATEGORY(logicDocument,	    "logic.document")
Q_LOGGING_CATEGORY(logicViewer,  "logic.viewer" );

//-------------------------------------------------------------------------
void loggerHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg) {
	
	xLog * pLog = xLog::log();
	QMutexLocker loc(pLog->lock());

    if (pLog->processCustomHandlers(type, context, msg))
		return;


	//if (!pLog->enabled())
	//	return;

    QString rawMessage;

    if (pLog->logFormat() == xLog::logFormatJson) {
        QJsonDocument doc;
        QJsonObject   obj;
        obj["dt"]     = QTime::currentTime().toString("HH:mm:ss.z");
        obj["app"]    = QCoreApplication::applicationName();
        obj["pid"]    = QCoreApplication::applicationPid();
        obj["thread"] = QJsonValue::fromVariant((quint64)QThread::currentThreadId());

        QString levelString = "Error";

        switch (type) {
        case QtDebugMsg:
            levelString = "Debug";
            break;
        case QtInfoMsg:
            levelString = "Info";
            break;
        case QtWarningMsg:
            levelString = "Warning";
            break;
        case QtCriticalMsg:
            levelString = "Critical";
            break;
        case QtFatalMsg:
            levelString = "Fatal";
            break;
        }

        obj["level"]    = levelString;
        obj["msg"]      = msg;
        obj["context"]  = QString::fromLatin1(context.category);
        
        doc.setObject(obj);
        rawMessage = doc.toJson(QJsonDocument::Compact);
    } else {
        rawMessage = qFormatLogMessage(type, context, msg);
    }

#ifdef Q_OS_UNIX
	if (pLog->destinations() & xLog::logSysLog) {
        syslog(LOG_USER | LOG_INFO, (const char*)rawMessage.toLatin1().data());
	}
#endif

	if (pLog->destinations() & xLog::logFile && logFileName && pLog->enabled()) {
#ifdef Q_OS_WIN32
			FILE * pFile = _wfopen(logFileName, pLog->isAppend() ? L"a+" : L"w");
#else
        	FILE * pFile = fopen(logFileName, pLog->isAppend() ? "a+" : "w");
#endif
 		if (pFile) {
 			fprintf(pFile, "%s\n", rawMessage.toLatin1().data());
			fflush(pFile);
 			fclose(pFile);
 		}
	}

#ifdef Q_OS_WIN
	if (pLog->destinations() & xLog::logVSConsole) {
		OutputDebugStringW((const wchar_t*)rawMessage.utf16());
		OutputDebugStringA("\n");
	}
#endif

	if (pLog->destinations() & xLog::logStdOut) {
		fprintf(stdout, "%s\n", rawMessage.toLatin1().data());
        fflush(stdout);
	}

	if (pLog->destinations() & xLog::logStdError) {
		fprintf(stderr, "%s\n", rawMessage.toLatin1().data());
        fflush(stderr);
	}
}
//-----------------------------------------------------------
void		   xLog::setMessagePattern(const QString & p) {
	qSetMessagePattern(p);
}

//-------------------------------------------------------------------------
xLog * xLog::log(bool bAppend) {
    if (!m_pLog){
        m_pLog = new xLog(bAppend);
    }

	return m_pLog;
}
//-----------------------------------------------------------
void		xLog::setPath(const QString & p) {
	m_path = p;

    adjustPath();
}
//-----------------------------------------------------------
void		xLog::adjustPath() {
    #ifdef Q_OS_UNIX	
    if (logFileName) {
        free(logFileName);
        logFileName = nullptr;
    }
    #else 
    if (logFileName) {
        delete logFileName;
        logFileName = nullptr;
    }
    #endif

    if (m_path.isEmpty()) {
        logFileName = nullptr;
    }
    else {
        QFileInfo f;
        f.setFile(QDir(m_path), m_fileName);
        QString fullPath = f.absoluteFilePath();
        fullPath = QDir::toNativeSeparators(fullPath);

        logFileName = nullptr;

#ifdef Q_OS_WIN32
        logFileName = new wchar_t[fullPath.size() + 1]; //-V121
        fullPath.toWCharArray(logFileName);
        logFileName[fullPath.size()] = 0;
#else
        logFileName = (char*)malloc(fullPath.toUtf8().count() + 1);
        strcpy(logFileName, (char*)fullPath.toUtf8().data());
#endif
    }
}
//-----------------------------------------------------------
QString		xLog::path() {
	return m_path;
}
//-----------------------------------------------------------
xLog::xLog( bool bAppend ) {

    m_path      = xSystemInformation::appDataLocation();
	m_bEnabled	= false;
	m_pLock		= new QMutex(QMutex::Recursive);
	
	setFileNameMask("log-%D.%M.%Y %hh-%mm-%ss %PID.txt");	

	m_bAppend	  = bAppend;

    m_logFormat   = logFormatPlain;

#ifdef Q_OS_WIN
	m_nDestinations = logFile | logVSConsole;
#else
    #ifdef QT_DEBUG
        m_nDestinations = logFile | logStdOut;
    #else
		m_nDestinations = logFile | logStdOut;
    #endif
#endif

#ifdef Q_OS_UNIX
	openlog("utopia", 0, 0);
#endif

#ifdef QT_DEBUG
    m_fileNameRules = "debug.rules";
#else
    m_fileNameRules = QCoreApplication::applicationDirPath() + "/debug.rules";
#endif
    m_logFileWatcher.addPath(m_fileNameRules);
    connect(&m_logFileWatcher, &QFileSystemWatcher::fileChanged, this, &xLog::slotRulesChanged);

	init();
}
//-----------------------------------------------------------
xLog::~xLog() {

#ifdef Q_OS_UNIX
	closelog();
	free(logFileName);
	logFileName = nullptr;
#else 
	delete logFileName;
	logFileName = nullptr;
#endif
}
//-----------------------------------------------------------
void			xLog::addFilterRule(const QString & f) {
	setFilterRules(m_currentRules + "\n" + f);
}
//-----------------------------------------------------------
void			xLog::setFilterRules(const QString & f) {
	m_currentRules = f;
	QLoggingCategory::setFilterRules(f);
}
//-------------------------------------------------------------------------
void xLog::slotRulesChanged(const QString &file){
    Q_UNUSED(file);

    readRules();
}
//-------------------------------------------------------------------------
bool xLog::readRules(){

    QFile f(m_fileNameRules);
    if (f.open(QIODevice::ReadOnly)) {
        QByteArray rules = f.readAll();
        setFilterRules(rules);
        f.close();
        return true;
    }
    return false;
}

//-----------------------------------------------------------
void	xLog::init() {
	qInstallMessageHandler(loggerHandler);
	setMessagePattern("%{time hh:mm:ss.zzz } %{if-debug}D%{endif}%{if-warning}W%{endif}%{if-critical}C%{endif}%{if-fatal}F%{endif}  [%{threadid}]  %{category} %{message}");

    if (readRules()==false){

		QString rules = QStringLiteral("*=false\nconsole.*=true");
		setFilterRules(rules);
	}

}
//-----------------------------------------------------------
void		xLog::setFileNameMask(const QString & mask) {
	m_fileNameMask = mask;

	m_fileName = m_fileNameMask;

	QDateTime dt = QDateTime::currentDateTime();
	QString tempBuffer("%1");

	m_fileName = m_fileName.replace("%D", tempBuffer.arg((int)dt.date().day(), 2, 10, QChar('0'))); 
	m_fileName = m_fileName.replace("%M", tempBuffer.arg((int)dt.date().month(), 2, 10, QChar('0')));
	m_fileName = m_fileName.replace("%Y", tempBuffer.arg((int)dt.date().year(), 4, 10, QChar('0'))); 

	m_fileName = m_fileName.replace("%hh", tempBuffer.arg((int)dt.time().hour(), 2, 10, QChar('0'))); 
	m_fileName = m_fileName.replace("%mm", tempBuffer.arg((int)dt.time().minute(), 2, 10, QChar('0')));
	m_fileName = m_fileName.replace("%ss", tempBuffer.arg((int)dt.time().second(), 2, 10, QChar('0')));

	m_fileName = m_fileName.replace("%PID", tempBuffer.arg(QCoreApplication::applicationPid()));

    adjustPath();

}
//-----------------------------------------------------------
bool	xLog::enabled() {
	QMutexLocker	locker(lock());
	return m_bEnabled;
}
//-----------------------------------------------------------
void	xLog::setEnabled(bool b) {
	QMutexLocker	locker(lock());
	m_bEnabled = b;
}
//-----------------------------------------------------------
QString		xLog::fileNameMask() {
	return m_fileNameMask;
}
//-----------------------------------------------------------
QString		xLog::fileName() {
	return m_fileName;
}
//-----------------------------------------------------------
void	xLog::registerCustomHandler(const QString & category, xLogCategoryHandler * pHandler) {
	xLog * pLog = xLog::log();
	QMutexLocker loc(pLog->lock());

	m_customHandlers[category] = pHandler;
}
//-----------------------------------------------------------
void	xLog::unregisterCustomHandler(const QString & category) {
	xLog * pLog = xLog::log();
	QMutexLocker loc(pLog->lock());

	m_customHandlers.remove(category);
}
//-----------------------------------------------------------
bool	xLog::processCustomHandlers(QtMsgType type, const QMessageLogContext &context, const QString &msg) {
	if (!m_customHandlers.contains(context.category))
		return false;

	if (!m_customHandlers[context.category])
		return true;

	m_customHandlers[context.category]->handleMessage(type, context, msg);

	return !m_customHandlers[context.category]->dublicateInLog();
}
//-----------------------------------------------------------
void                xLog::writeInfoString(const QString & str) {
    stdOut() << str << endl;    
}
//-----------------------------------------------------------
QTextStream &       xLog::stdOut() {
    static QTextStream stdOutStream(stdout);
    return stdOutStream;
}
//-----------------------------------------------------------
