/*
Copyright (c) 2011, REC Robotics Equipment Corporation GmbH, Planegg, Germany
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

- Redistributions of source code must retain the above copyright notice,
  this list of conditions and the following disclaimer.
- Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation and/or
  other materials provided with the distribution.
- Neither the name of the REC Robotics Equipment Corporation GmbH nor the names of
  its contributors may be used to endorse or promote products derived from this software
  without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS
OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "MainWindow.h"

#include <cassert>
#include <limits>

MainWindow::MainWindow()
: _address( new QLineEdit( "127.0.0.1" ) )
, _port( new QSpinBox )
, _connectPb( new QPushButton )
, _blocking( new QCheckBox( "blocking" ) )
, _enableTopics( new QCheckBox( "listen to topics" ) )
, _versionLabel( new QLabel )
, _imgView( new QLabel )
, _logView( new LogView )
{
    setWindowTitle( QString( "Client (RPC library %1)" ).arg( rec::rpc::getLibraryVersionString() ) );

	_port->setRange( 0, 0xFFFF );
	_port->setValue( rec::rpc::defaultPort );


	_logView->setAcceptRichText( false );
	_logView->setReadOnly( true );

	QVBoxLayout* layout = new QVBoxLayout;
	setLayout( layout );

	QHBoxLayout* connLayout = new QHBoxLayout;
	connLayout->addWidget( new QLabel( "Connect to:" ) );
	connLayout->addWidget( _address, 1 );
	connLayout->addWidget( _port );
	connLayout->addWidget( _connectPb );
	connLayout->addWidget( _blocking );
	connLayout->addWidget( _enableTopics );



	QGridLayout* bottomLayout = new QGridLayout;
	bottomLayout->addWidget( new QLabel( "Log:" ), 0, 0 );
	bottomLayout->addWidget( _logView, 1, 0 );

	layout->addLayout( connLayout );
	layout->addWidget( _versionLabel );
	layout->addWidget( _imgView );
	layout->addLayout( bottomLayout );


    connect( &_client, SIGNAL( connected() ),this,  SLOT( on_client_connected() ) );
    connect( &_client, SIGNAL( disconnected( rec::rpc::ErrorCode ) ),this,  SLOT( on_client_disconnected( rec::rpc::ErrorCode ) ) );
    connect( &_client, SIGNAL( log( const QString& ) ), _logView, SLOT( log( const QString& ) ) );
    connect( &_client, SIGNAL( imageReceived( const QImage& ) ), this, SLOT( on_client_imageReceived( const QImage& ) ) );
    connect( &_client, SIGNAL( imageInfoChanged( const QSet< QPair< QHostAddress, quint16 > >& ) ), this, SLOT( setInfo( const QSet< QPair< QHostAddress, quint16 > >& ) ) );

    connect( _connectPb, SIGNAL( clicked() ), this, SLOT( on_connectPb_clicked() ) );
    connect( _blocking, SIGNAL( toggled( bool ) ), &_client, SLOT( setBlocking( bool ) ) );
    connect( _enableTopics, SIGNAL( toggled( bool ) ), this, SLOT( on_enableTopics_toggled( bool ) ) );

	updateWidgets( false );
}

MainWindow::~MainWindow()
{
	_client.disconnectFromServer();
}

void MainWindow::updateWidgets( bool connected )
{
	if ( connected )
		_connectPb->setText( "Disconnect" );
	else
		_connectPb->setText( "Connect" );
	_address->setEnabled( !connected );
	_blocking->setEnabled( connected );
	_enableTopics->setEnabled( connected );
}

void MainWindow::on_connectPb_clicked()
{
	if ( _client.isConnected() )
		_client.disconnectFromServer();
	else
	{
		if ( _address->text().isEmpty() )
		{
			_logView->log( "Could not connect: address is empty." );
		}
		else
		{
			QString addr = _address->text();
			addr.append( ':' );
			addr.append( QString::number( _port->value() ) );
            _client.setAddress( addr.toLatin1().constData() );
			try
			{
				_client.connectToServer();
			}
			catch( const rec::rpc::Exception& e )
			{
				_logView->log( QString( "Could not connect: " ) + e.getMessage() );
			}
		}
	}
}

void MainWindow::on_enableTopics_toggled( bool enabled )
{
	if ( !enabled )
        _logView->clear();
	_client.setTopicsEnabled( enabled );
}

void MainWindow::on_client_connected()
{
	_logView->log( QString( "Connected to %1." ).arg( _client.address() ) );
	_client.setBlocking( _blocking->isChecked() );
	if ( _enableTopics->isChecked() )
		_client.setTopicsEnabled( true );
	updateWidgets( true );
	try
	{
		int major, minor, patch, build;
		_client.getServerVersion( &major, &minor, &patch, &build );
		_versionLabel->setText( QString( "Server version: %1.%2.%3 Build %4" ).arg( major ).arg( minor ).arg( patch ).arg( build ) );
	}
	catch( const rec::rpc::Exception& e )
	{
		_versionLabel->setText( QString( "Server version could not be retrieved. Reason: " ) + e.getMessage() );
	}
	catch( ... )
	{
		_versionLabel->setText( QString( "Server version could not be retrieved." ) );
	}
}

void MainWindow::on_client_disconnected( rec::rpc::ErrorCode errorCode )
{
	_versionLabel->clear();
    _logView->clear();
	if ( errorCode == rec::rpc::NoError )
	{
		_logView->log( "Disconnected regularly." );
	}
	else
	{
		_logView->log( QString( "Disconnected: " ) + rec::rpc::Exception::messageFromErrorCode( errorCode ) );
	}
	updateWidgets( false );
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



void MainWindow::on_client_imageReceived( const QImage& img )
{
	_imgView->setPixmap( QPixmap::fromImage( img ) );
	_imgView->adjustSize();
}

