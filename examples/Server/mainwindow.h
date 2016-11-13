#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtGui>
#include <QImageReader>
#include <QBitArray>
#include <QImage>
#include <QSpinBox>
#include <QLabel>
#include <QLineEdit>
#include <QFileDialog>
#include <QHostAddress>
#include "Server.h"
#include "LogView.h"


class MainWindow : public QWidget
{
    Q_OBJECT

public:
    explicit MainWindow();
    ~MainWindow();


private Q_SLOTS:
    void onserver_listening();
    void onserver_closed();
    void onserver_clientConnected( const QHostAddress& address, unsigned short port );
    void onserver_clientDisconnected( const QHostAddress& address, unsigned short port );
    void on_server_registeredTopicListener( const QString& name, const QHostAddress& address, unsigned short port );
    void on_server_unregisteredTopicListener( const QString& name, const QHostAddress& address, unsigned short port );
    void setInfo( const QSet< QPair< QHostAddress, quint16 > >& info );

    void on_openImgPb_clicked();
    void on_startStopPb_clicked();


private:

    void updateWidgets( bool listening );

    Server _server;

    LogView* _logView;
    QPushButton *_startStopPb;
    QSpinBox *_port;
    QLineEdit *_imgPath;
    QPushButton *_openImgPb;
};

#endif // MAINWINDOW_H
