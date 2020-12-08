TEMPLATE = app
TARGET 	 = qlogview

QT 	+= network concurrent core widgets

DEFINES += QT_FATAL_WARNINGS

INCLUDEPATH += ./src 

SOURCES += ./src/main.cpp \
    ./src/xapplication.cpp \
	./src/xsysteminformation.cpp \
	./src/xlog.cpp \
	./src/xplaintextviewer.cpp \
	./src/xmainwindow.cpp \
	./src/xdocument.cpp \
	./src/xhighlighter.cpp \
	./src/xscrollbar.cpp \
	./src/xvaluelistmodel.cpp \
	./src/xinfopanel.cpp \
	./src/xfileprocessor.cpp \
	./src/xtreeview.cpp \
	./src/xtableview.cpp \
	./src/xtimestamppanel.cpp \
	./src/xsearchwidget.cpp

HEADERS += \
    ./src/xapplication.h \
	./src/xsysteminformation.h \
	./src/xlog.h \
	./src/xplaintextviewer.h \
	./src/xmainwindow.h \
	./src/xdocument.h \
	./src/xhighlighter.h \
	./src/xscrollbar.h \
	./src/xvaluelistmodel.h \
	./src/xinfopanel.h \
	./src/xfileprocessor.h \
	./src/xtreeview.h \
	./src/xtableview.h \
	./src/xtimestamppanel.h \
	./src/xsearchwidget.h