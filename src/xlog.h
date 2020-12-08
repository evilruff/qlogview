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


#ifndef _xLog_h_
#define _xLog_h_    1

#include <QMutex>
#include <QLoggingCategory>
#include <QFileSystemWatcher>

class   xLogCategoryHandler {
	public:
		xLogCategoryHandler() {};
		virtual  ~xLogCategoryHandler() {};

		virtual  void		handleMessage(QtMsgType type, const QMessageLogContext &context, const QString &msg) = 0;
		virtual bool		dublicateInLog() {
			return true;
		}
};

class	xLog : public QObject {
    Q_OBJECT;
	public:

		enum {

			logFile			= 0x01,
			logStdError		= 0x02,
			logStdOut		= 0x04,
			logVSConsole	= 0x08,
			logSysLog		= 0x10
		} logDestination;

        enum {
            logFormatPlain  = 0x01,
            logFormatJson   = 0x02
        };

	
        static xLog * log(bool bAppend = true);

        static QTextStream &       stdOut();
        static void                writeInfoString(const QString & str);

		void				setPath(const QString & p);
		QString				path();
        

		bool				enabled();
		void				setEnabled(bool b);

		void				setFileNameMask(const QString & mask);
		QString				fileNameMask();
		QString				fileName();

		inline QMutex *     lock() { return m_pLock; }
		
		void				setMessagePattern(const QString & p);
		inline bool		    isAppend() { return m_bAppend; }
		inline int		    destinations() { return m_nDestinations; }
		inline void			setDestinations(int m) {
			m_nDestinations = m;
		}

		inline void			setFilterRules(const QString & f);
		void				addFilterRule(const QString & f);

		void				registerCustomHandler(const QString & category, xLogCategoryHandler *);
		void				unregisterCustomHandler(const QString & category);
		bool				processCustomHandlers(QtMsgType type, const QMessageLogContext &context, const QString &msg);

        int                 logFormat() const { return m_logFormat; }
        void                setLogFormat(int format) { m_logFormat = format; }

	protected:

		xLog( bool bAppend = true );
		~xLog();

		void		        init();
        bool                readRules();
        void		        adjustPath();
        
	
    public slots:

		void	             slotRulesChanged(const QString &);

	private:

		static   xLog *		m_pLog;
		QString				m_path;
		QString				m_fileName;
		QString				m_fileNameMask;
        QString             m_fileNameRules;

		QString				m_currentRules;

		bool				m_bEnabled;
        int                 m_logFormat;

		QMutex	*			m_pLock;		
		bool				m_bAppend;
		int					m_nDestinations;	
        QFileSystemWatcher  m_logFileWatcher;		

		QMap<QString, xLogCategoryHandler *>	m_customHandlers;
};

Q_DECLARE_LOGGING_CATEGORY(mainApp		        	/* "main.app"               */);
Q_DECLARE_LOGGING_CATEGORY(logicDocument			/* "logic.document"         */);
Q_DECLARE_LOGGING_CATEGORY(logicViewer   			/* "logic.viewer"           */);

#endif
