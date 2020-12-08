#ifndef _xApplication_h_
#define _xApplication_h_

#include <QApplication>

#include "xmainwindow.h"


class xApplication: public QApplication {
	Q_OBJECT
public:
	xApplication(int & argc, char ** argv);
	~xApplication();

protected:

    xMainWindow         *   m_mainWindow;
};

#endif