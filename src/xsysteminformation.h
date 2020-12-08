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


#ifndef _xSystemInformation_h_   
#define _xSystemInformation_h_   1

#include <QObject>
#include <QStringList>
#include <QStandardPaths>

class xSystemInformation: public QObject {
	Q_OBJECT

public:

	enum {
		PlatformAny = 0,
		Windows32 = 0x1000,
		Windows64,
		Linux32,
		Linux64,
		MacOSX,
		Android,
		IOS
	} Platform;

	Q_ENUMS(Platform);
		
	xSystemInformation(QObject * pObject);
	~xSystemInformation();

	Q_INVOKABLE static	QString					currentPlatformName();
	Q_INVOKABLE	static	int						currentPlatform();
	Q_INVOKABLE	static	QList<int>				availablePlatforms();
	Q_INVOKABLE	static	QString					platformName(int nIndex);

	Q_INVOKABLE	static QString					appDataLocation();
    Q_INVOKABLE	static QString                  appTempLocation();

	Q_INVOKABLE	static quint32					parentPID();

    Q_INVOKABLE	static int						getTotalMemorySize();
	Q_INVOKABLE	static int						getAvailableMemorySize();
	Q_INVOKABLE	static int						getUsedMemorySize();
	Q_INVOKABLE	static int						getHomeDriveAvailableSize();

	Q_INVOKABLE	static int						coresAvailable();
	Q_INVOKABLE	static int						memoryAvailable();

protected:

    static      QString                         findLocation(QStandardPaths::StandardLocation location);
};

#endif