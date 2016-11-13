#-------------------------------------------------
#
# Project created by QtCreator 2016-11-07T09:51:29
#
#-------------------------------------------------

QT       += core gui
QT       += network xml

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = Server
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    Server.cpp \
    LogView.cpp

HEADERS  += mainwindow.h \
    Server.h \
    LogView.h \
    serialization/ImageMsg.h

FORMS    +=

!win32{
INCLUDEPATH +=/usr/local/include
LIBS +=-L/usr/local/lib/ -lrec_rpc
}
