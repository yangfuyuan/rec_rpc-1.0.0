#-------------------------------------------------
#
# Project created by QtCreator 2016-11-07T10:04:13
#
#-------------------------------------------------

QT       += core gui xml network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = Client
TEMPLATE = app


SOURCES += main.cpp\
    Client.cpp \
    MainWindow.cpp \
    LogView.cpp

HEADERS  += Client.h \
    MainWindow.h \
    LogView.h \
    serialization/ImageMsg.h

!win32{
INCLUDEPATH +=/usr/local/include
LIBS +=-L/usr/local/lib/ -lrec_rpc
}
