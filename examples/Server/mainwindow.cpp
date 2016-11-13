#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QVBoxLayout>
#include <QHBoxLayout>

MainWindow::MainWindow() :
    _startStopPb( new QPushButton ),
    _port( new QSpinBox ),
    _imgPath( new QLineEdit ),
    _openImgPb( new QPushButton( "..." ) ),
    _logView( new LogView )
{


    _port->setRange( 0, 0xFFFF );
    _port->setValue( rec::rpc::defaultPort );


    _imgPath->setReadOnly( true );

    _logView->setAcceptRichText( false );
    _logView->setReadOnly( true );


    QVBoxLayout* layout = new QVBoxLayout;
    setLayout( layout );


    QHBoxLayout* mgmLayout = new QHBoxLayout;
    mgmLayout->addWidget( _startStopPb );
    mgmLayout->addWidget( new QLabel( "Port:" ) );
    mgmLayout->addWidget( _port );
    mgmLayout->addStretch();


    QHBoxLayout* imgLayout = new QHBoxLayout;
    imgLayout->addWidget( new QLabel( "Image topic:" ) );
    imgLayout->addWidget( _imgPath, 1 );
    imgLayout->addWidget( _openImgPb );


    QGridLayout* bottomLayout = new QGridLayout;
    bottomLayout->addWidget( new QLabel( "Log:" ), 0, 0 );
    bottomLayout->addWidget( _logView, 1, 0 );


    layout->addLayout( mgmLayout);
    layout->addLayout( imgLayout );
    layout->addLayout( bottomLayout );

    connect( &_server, SIGNAL( listening() ),this, SLOT( onserver_listening() ) );
    connect( &_server, SIGNAL( closed() ),this, SLOT( onserver_closed() ) );
    connect( &_server, SIGNAL( clientConnected( const QHostAddress&, unsigned short ) ), SLOT( onserver_clientConnected( const QHostAddress&, unsigned short ) ) );
    connect( &_server, SIGNAL( clientDisconnected( const QHostAddress&, unsigned short ) ), SLOT( onserver_clientDisconnected( const QHostAddress&, unsigned short ) ) );
    connect( &_server, SIGNAL( registeredTopicListener( const QString&, const QHostAddress&, unsigned short ) ), SLOT( on_server_registeredTopicListener( const QString&, const QHostAddress&, unsigned short ) ) );
    connect( &_server, SIGNAL( unregisteredTopicListener( const QString&, const QHostAddress&, unsigned short ) ), SLOT( on_server_unregisteredTopicListener( const QString&, const QHostAddress&, unsigned short ) ) );
    connect( &_server, SIGNAL( imageInfoChanged( const QSet< QPair< QHostAddress, quint16 > >& ) ), this, SLOT( setInfo( const QSet< QPair< QHostAddress, quint16 > >& ) ) );
    connect( &_server, SIGNAL( log( const QString& ) ), _logView, SLOT( log( const QString& ) ) );

    connect( _startStopPb, SIGNAL( clicked() ), SLOT( on_startStopPb_clicked() ) );
    connect( _openImgPb, SIGNAL( clicked() ), SLOT( on_openImgPb_clicked() ) );

    updateWidgets(false);

}

MainWindow::~MainWindow()
{
    _server.close();
}

void MainWindow::updateWidgets( bool listening )
{
    if ( listening )
        _startStopPb->setText( "Stop server" );
    else
        _startStopPb->setText( "Start server" );
}

void MainWindow::onserver_listening()
{
    _logView->log( QString( "Server is listening on port %1." ).arg( _server.serverPort() ) );
    updateWidgets( true );
}

void MainWindow::onserver_closed()
{	
    _logView->log( QString( "Server is closed." ) );
    updateWidgets( false);
}

void MainWindow::onserver_clientConnected( const QHostAddress& address, unsigned short port )
{
    if ( address.isNull() || address == QHostAddress::LocalHost || address == QHostAddress::LocalHostIPv6 ){
        _logView->log( QString( "Local client %1 connected." ).arg( port ) );
    }else{
        _logView->log( QString( "Remote client %1:%2 connected." ).arg( address.toString() ).arg( port ) );
    }
}

void MainWindow::onserver_clientDisconnected( const QHostAddress& address, unsigned short port )
{
    if ( address.isNull() || address == QHostAddress::LocalHost || address == QHostAddress::LocalHostIPv6 ){
        _logView->log( QString( "Local client %1 disconnected." ).arg( port ) );
    }else{
        _logView->log( QString( "Remote client %1:%2 disconnected." ).arg( address.toString() ).arg( port ) );
    }
}

void MainWindow::on_server_registeredTopicListener( const QString& name, const QHostAddress& address, unsigned short port )
{
    if ( address.isNull() || address == QHostAddress::LocalHost || address == QHostAddress::LocalHostIPv6 )
    {
        if ( port == 0 ){
            _logView->log( QString( "Server registered topic '%1'." ).arg( name ) );
        }else{
            _logView->log( QString( "Local client %1 registered topic '%2'." ).arg( port ).arg( name ) );
        }
    }
    else{
        _logView->log( QString( "Remote client %1:%2 registered topic '%3'." ).arg( address.toString() ).arg( port ).arg( name ) );
    }
}

void MainWindow::on_server_unregisteredTopicListener( const QString& name, const QHostAddress& address, unsigned short port )
{
    if ( address.isNull() || address == QHostAddress::LocalHost || address == QHostAddress::LocalHostIPv6 )
    {
        if ( port == 0 ){
            _logView->log( QString( "Server unregistered topic '%1'." ).arg( name ) );
        }else{
            _logView->log( QString( "Local client %1 unregistered topic '%2'." ).arg( port ).arg( name ) );
        }
    }
    else{
        _logView->log( QString( "Remote client %1:%2 unregistered topic '%3'." ).arg( address.toString() ).arg( port ).arg( name ) );
    }
}

void MainWindow::setInfo( const QSet< QPair< QHostAddress, quint16 > >& info )
{

    typedef QPair< QHostAddress, quint16 > ClientInfo;
    Q_FOREACH( const ClientInfo& i, info )
    {
        if ( i.first.isNull() || i.first == QHostAddress::LocalHost || i.first == QHostAddress::LocalHostIPv6 )
        {
            if ( i.second == 0 )
                _logView->log(QString("Server"));
            else
                _logView->log(QString( "Local %1" ).arg( i.second ) );
        }
        else
            _logView->log( QString( "Remote %1:%2" ).arg( i.first.toString() ).arg( i.second )) ;
    }
}

void MainWindow::on_startStopPb_clicked()
{
    if ( _server.isListening() )
        _server.close();
    else
        _server.listen( _port->value() );
}

void MainWindow::on_openImgPb_clicked()
{
    QString formatString = "Image (";
    Q_FOREACH( const QByteArray& format, QImageReader::supportedImageFormats() )
    {
        formatString.append( "*." );
        formatString.append( format );
        formatString.append( ' ' );
    }
    formatString[ formatString.length() - 1 ] = ')';
    QString filePath = QFileDialog::getOpenFileName( this, "Open image", QString::null, formatString );
    _imgPath->setText( filePath );
    if ( filePath.isEmpty() )
        return;
    QImage img( filePath );
    if ( !img.isNull() )
        _server.publishImage( img );
}
