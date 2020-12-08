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


#include "xsysteminformation.h"

#include <QStandardPaths>
#include <QDir>
#include <QStorageInfo>
#include <QUrl>
#include <QCoreApplication>
#include <QThread>

#ifdef Q_OS_WIN
 #include <windows.h>
 #include <qt_windows.h>
#include <tlhelp32.h>
#include <iphlpapi.h>
#include <psapi.h>
#endif

#ifdef Q_OS_UNIX
 #include <unistd.h>
#endif

#ifdef Q_OS_LINUX
 #include <sys/sysinfo.h>
#endif

#ifdef Q_OS_OSX
#include <sys/types.h>
#include <sys/param.h>
#include <sys/sysctl.h>
#endif


xSystemInformation::xSystemInformation(QObject * pObject):
	QObject(pObject) {
}

xSystemInformation::~xSystemInformation() {
}

int						xSystemInformation::coresAvailable() {
	return QThread::idealThreadCount();
}

int						xSystemInformation::memoryAvailable() {
	return getTotalMemorySize()*1024;
}

int				xSystemInformation::getTotalMemorySize() {
#ifdef Q_OS_WIN
	MEMORYSTATUSEX memory_status;
	ZeroMemory(&memory_status, sizeof(MEMORYSTATUSEX));
	memory_status.dwLength = sizeof(MEMORYSTATUSEX);
	if (GlobalMemoryStatusEx(&memory_status)) {
		return memory_status.ullTotalPhys/(1024 * 1024);
	}
	
	return 0;
#elif defined Q_OS_LINUX

	struct sysinfo		memInfo;
	sysinfo(&memInfo);
	return memInfo.totalram / (1024 * 1024);
#elif defined Q_OS_OSX

    int mib [] = { CTL_HW, HW_MEMSIZE };
    int64_t value = 0;
    size_t length = sizeof(value);

    if(-1 == sysctl(mib, 2, &value, &length, nullptr, 0))
        return 0L;

    return value / (1024 * 1024);

#endif

	return 0;
}

int					xSystemInformation::getAvailableMemorySize() {
#ifdef Q_OS_WIN
	MEMORYSTATUSEX memory_status;
	ZeroMemory(&memory_status, sizeof(MEMORYSTATUSEX));
	memory_status.dwLength = sizeof(MEMORYSTATUSEX);
	if (GlobalMemoryStatusEx(&memory_status)) {
		return memory_status.ullAvailPhys / (1024 * 1024);
	}

	return 0;
#elif defined Q_OS_LINUX

	struct sysinfo		memInfo;
	sysinfo(&memInfo);
	return memInfo.freeram / (1024 * 1024);

#else
	return 0;
#endif
}
//---------------------------------------------------------------------------
int					xSystemInformation::getUsedMemorySize() {
#ifdef Q_OS_WIN
	PROCESS_MEMORY_COUNTERS memcount;
	if (!GetProcessMemoryInfo(GetCurrentProcess(), &memcount, sizeof(memcount))) return 0;
	return memcount.WorkingSetSize / (1024 * 1024);
#elif defined Q_OS_LINUX
	long rss = 0L;
	FILE* fp = nullptr;
	if ((fp = fopen("/proc/self/statm", "r")) == nullptr)
		return (size_t)0L;      /* Can't open? */
	if (fscanf(fp, "%*s%ld", &rss) != 1)
	{
		fclose(fp);
		return (size_t)0L;      /* Can't read? */
	}
	fclose(fp);
	return (size_t)rss * (size_t)sysconf(_SC_PAGESIZE)/(1024*1024);
#else
	return 0;
#endif
}

int					xSystemInformation::getHomeDriveAvailableSize() {
	QString homePath = QDir::homePath();
	QStorageInfo storage(homePath);
	return storage.bytesAvailable() / (1024 * 1024);
}

quint32			xSystemInformation::parentPID() {
#ifdef Q_OS_WIN
	HANDLE h = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	PROCESSENTRY32 pe = { 0 };
	pe.dwSize = sizeof(PROCESSENTRY32);
	int	pid = GetCurrentProcessId();

	if (Process32First(h, &pe)) {
		do {
			if (pe.th32ProcessID == pid) {
				return pe.th32ParentProcessID;
			}
		} while (Process32Next(h, &pe));
	}
	CloseHandle(h);
	return 0;
#else
	return getppid();
#endif
}

QString			xSystemInformation::currentPlatformName() {

	switch (currentPlatform()) {
		case Windows32:
			return "MS Windows 32";
		case Windows64:
			return "MS Windows 64";
		case Linux32:
			return "Linux Generic 32";
		case Linux64:
			return "Linux Generic 64";
		case Android:
			return "Android Generic";
		case IOS:
			return "Apple iOS";
		case MacOSX:
			return "Apple Mac OSX";
	}

	return QString("Unknown");
}

int				xSystemInformation::currentPlatform() {
#ifdef Q_OS_WIN32
	#ifndef Q_OS_WIN64
		return Windows32;
	#endif
#endif

#ifdef Q_OS_WIN64
	return Windows64;
#endif

#ifdef Q_OS_LINUX
	#ifdef Q_PROCESSOR_X86_32 
		return Linux32;
	#endif
	#ifdef Q_PROCESSOR_X86_32
		return Linux64;
	#endif
#endif

#ifdef Q_OS_OSX
	return MacOSX;
#endif

#ifdef Q_OS_ANDROID
	return Android;
#endif

#ifdef Q_OS_IOS	
	return IOS;
#endif	
	return PlatformAny;
}

QList<int>		xSystemInformation::availablePlatforms() {
	QList<int>		platformIds;

	platformIds << Windows32;
	platformIds << Windows64;
	platformIds << Linux32;
	platformIds << Linux64;
	platformIds << MacOSX;
	platformIds << IOS;
	platformIds << Android;

	return platformIds;
}

QString			xSystemInformation::platformName(int index) {
	
	switch (index) {
		case Windows32:
			return "MS Windows x32";
		case Windows64:
			return "MS Windows x64";
		case Linux32:
			return "Linux x32 Generic";
		case Linux64:
			return "Linux x64 Generic";
		case MacOSX:
			return "Apple MacOSX";
		case IOS:
			return "Apple iOS";
		case Android:
			return "Android Generic";
	}
	return QString();
}

Q_INVOKABLE	QString xSystemInformation::appDataLocation() {	
    return findLocation(QStandardPaths::AppDataLocation);
}

Q_INVOKABLE	QString xSystemInformation::appTempLocation(){
    return findLocation(QStandardPaths::TempLocation);
}

QString    xSystemInformation::findLocation(QStandardPaths::StandardLocation location) {
    QStringList    paths = QStandardPaths::standardLocations(location);

    for (const QString & path : paths) {
        QDir  d(path);
        if (d.exists() || d.mkpath(path))
            return path;
    }
    return QString();
}
