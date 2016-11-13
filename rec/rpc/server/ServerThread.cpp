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

#include "rec/rpc/server/ServerThread.hpp"

#include "rec/rpc/rec_rpc_version.h"

#include <QtDebug>
#include <cassert>

using namespace rec::rpc::server;

ServerThread::ServerThread( QObject* parent, QTcpSocket* tcpSocket )
: QThread( parent )
, _tcpSocket( tcpSocket )
, _localSocket( 0 )
, _socket( NULL )
{
	_tcpSocket->setParent( 0 );
	_tcpSocket->moveToThread( this );
	setObjectName( "ServerThread" );
}

ServerThread::ServerThread( QObject* parent, QLocalSocket* localSocket )
: QThread( parent )
, _tcpSocket( 0 )
, _localSocket( localSocket )
, _socket( NULL )
{
	_localSocket->setParent( 0 );
	_localSocket->moveToThread( this );
	setObjectName( "ServerThread" );
}

ServerThread::~ServerThread()
{
	exit();
	wait();
	qDebug() << "ServerThread destroyed";
}

void ServerThread::start()
{
	QThread::start();
	_startSemaphore.acquire();
}

void ServerThread::sendGreeting( const QByteArray& greetingMessage )
{
	QMutexLocker lock( &_socketMutex );
	if ( _socket )
	{
		qApp->postEvent( _socket, new rec::rpc::client::SendGreetingEvent( greetingMessage ) );
	}
}

void ServerThread::sendConfiguration( const QByteArray& data )
{
	QMutexLocker lock( &_socketMutex );
	if( _socket )
	{
		qApp->postEvent( _socket, new rec::rpc::client::SendConfigurationEvent( data ) );
	}
}

void ServerThread::sendRPCResponse( const QByteArray& data )
{
	QMutexLocker lock( &_socketMutex );
	if( _socket )
	{
		qApp->postEvent( _socket, new rec::rpc::client::SendRPCResponseEvent( data ) );
	}
}

void ServerThread::publishTopic( const QString& name, const QByteArray& data )
{
	{
		QMutexLocker lk( &_registeredChannelsMutex );
		if( !_registeredChannels.contains( name ) )
		{
			return;
		}
	}

	QMutexLocker lock( &_socketMutex );
	if( _socket )
	{
		qApp->postEvent( _socket, new rec::rpc::client::PublishTopicEvent( data ) );
	}
}

void ServerThread::run()
{
	{
		QMutexLocker lock( &_socketMutex );
		_socket = new rec::rpc::client::Socket;
	}

//	bool ok = true;

     QObject::connect( _socket, SIGNAL( disconnected() ), SLOT( on_disconnected() ), Qt::DirectConnection );
     QObject::connect( _socket, SIGNAL( error( QAbstractSocket::SocketError ) ), SLOT( on_error( QAbstractSocket::SocketError ) ), Qt::DirectConnection );
	
     QObject::connect( _socket,
		SIGNAL( rpcRequestReceived( const QString&, quint32, const QByteArray& ) ),
		SLOT( on_rpcRequestReceived( const QString&, quint32, const QByteArray& ) ),
		Qt::DirectConnection );

    QObject::connect( _socket,
		SIGNAL( topicReceived( const QString&, const QHostAddress&, quint16, const QByteArray& ) ),
		SLOT( on_topicReceived( const QString&, const QHostAddress&, quint16, const QByteArray& ) ),
		Qt::DirectConnection );

     QObject::connect( _socket, SIGNAL( registerChannelReceived( const QString& ) ), SLOT( on_registerChannelReceived( const QString& ) ), Qt::DirectConnection );
     QObject::connect( _socket, SIGNAL( unregisterChannelReceived( const QString& ) ), SLOT( on_unregisterChannelReceived( const QString& ) ), Qt::DirectConnection );

    //assert( ok );

	_startSemaphore.release();

	if ( _localSocket )
		_socket->setLocalSocket( _localSocket );
	else if ( _tcpSocket )
		_socket->setTcpSocket( _tcpSocket );
	else
	{
		Q_EMIT error( QAbstractSocket::UnknownSocketError, tr( "Error setting socket" ) );
		return;
	}

	_peerAddress = _socket->peerAddress();
	_peerPort = _socket->peerPort();
	Q_EMIT connected( _peerAddress, _peerPort );

	exec();

	{
		QMutexLocker lock( &_socketMutex );
		delete _socket;
		_socket = NULL;
	}
}

void ServerThread::on_disconnected()
{
	Q_EMIT disconnected( _peerAddress, _peerPort, _registeredChannels.toList() );
	exit();
}

void ServerThread::on_error( QAbstractSocket::SocketError err )
{
	Q_EMIT error( err, _socket->errorString() );
	exit();
}

void ServerThread::on_rpcRequestReceived( const QString& name, quint32 seqNum, const QByteArray& serParam )
{
	Q_EMIT rpcRequestReceived( this, name, seqNum, serParam, _socket->peerAddress(), _socket->peerPort() );
}

void ServerThread::on_topicReceived( const QString& name, const QHostAddress&, quint16, const QByteArray& serData )
{
	Q_EMIT topicReceived( name, _socket->peerAddress(), _socket->peerPort(), serData );
}

void ServerThread::on_registerChannelReceived( const QString& name )
{
	bool newRegistration = true;
	{
		QMutexLocker lk( &_registeredChannelsMutex );
		if( _registeredChannels.contains( name ) )
		{
			newRegistration = false;
		}
		else
		{
			_registeredChannels.insert( name );
		}
	}

	if( newRegistration )
	{
		Q_EMIT channelRegistered( this, name, _socket->peerAddress(), _socket->peerPort() );
	}
}

void ServerThread::on_unregisterChannelReceived( const QString& name )
{
	{
		QMutexLocker lk( &_registeredChannelsMutex );
		_registeredChannels.remove( name );
	}
	Q_EMIT channelUnregistered( this, name, _socket->peerAddress(), _socket->peerPort() );
}
