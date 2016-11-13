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

#include "rec/rpc/server/Impl.hpp"
#include "rec/rpc/server/ServerThread.hpp"
#include "rec/rpc/messages/Configurations.h"
#include "rec/rpc/messages/Topic.h"
#include "rec/rpc/common.h"
#include "rec/rpc/defines.h"

#include <cassert>

using namespace rec::rpc::server;

Impl::Impl( rec::rpc::configuration::Configuration* configuration, const QString& greeting )
: _configuration( configuration )
, _numClientsConnected( 0 )
, _tcpServer( new QTcpServer )
, _localServer( new QLocalServer )
{
	setObjectName( "rec::rpc::server::Impl" );

	//build configuration message the first time
	on_configuration_changed();

	setGreeting( greeting );

     connect( _configuration,
        SIGNAL( changed() ),
        SLOT( on_configuration_changed() ) );
     connect( _tcpServer,
        SIGNAL( newConnection() ),
        SLOT( on_tcpServer_newConnection() ),
        Qt::DirectConnection );
    connect( _localServer,
        SIGNAL( newConnection() ),
        SLOT( on_localServer_newConnection() ),
        Qt::DirectConnection );


    /*

    bool ok = true;
	ok &= connect( _configuration,
		SIGNAL( changed() ),
		SLOT( on_configuration_changed() ) );
	ok &= connect( _tcpServer,
		SIGNAL( newConnection() ),
		SLOT( on_tcpServer_newConnection() ),
		Qt::DirectConnection );
	ok &= connect( _localServer,
		SIGNAL( newConnection() ),
		SLOT( on_localServer_newConnection() ),
		Qt::DirectConnection );
    assert( ok );*/
}

Impl::~Impl()
{
	delete _tcpServer;
	delete _localServer;
}

QHostAddress Impl::serverAddress() const
{
	return _tcpServer->serverAddress();
}

quint16 Impl::serverPort() const
{
	return _tcpServer->serverPort();
}

bool Impl::isListening() const
{
	return _tcpServer->isListening() && _localServer->isListening();
}

void Impl::customEvent( QEvent* e )
{
	switch( e->type() )
	{
	case ListenEventId:
		{
			if( false == _tcpServer->isListening() && false == _localServer->isListening() )
			{
				ListenEvent* ev = static_cast< ListenEvent* >( e );
				if( -1 == ev->port )
				{
					ev->port = rec::rpc::defaultPort;
				}
				if( false == _tcpServer->listen( QHostAddress::Any, ev->port ) )
				{
					Q_EMIT serverError( _tcpServer->serverError(), _tcpServer->errorString() );
				}
				else
				{
					if( false == _localServer->listen( QString( "__REC__RPC__%1__" ).arg( ev->port ) ) )
					{
						_tcpServer->close();
						Q_EMIT serverError( _localServer->serverError(), _localServer->errorString() );
					}
					else
					{
						Q_EMIT listening();
					}
				}
			}
		}
		break;

	case DisconnectAllClientsId:
		disconnectAllClients();
		break;

	case CloseEventId:
		disconnectAllClients();
		_tcpServer->close();
		_localServer->close();
		Q_EMIT closed();
		break;

	default:
		break;
	}
}

void Impl::disconnectAllClients()
{
	QList<ServerThread*> servers = findChildren<ServerThread*>();

	Q_FOREACH( ServerThread* thread, servers )
	{
		thread->exit();
	}

	QStringList topics = _configuration->itemContainer().keys();
	Q_FOREACH( const QString& topic, topics )
	{
		_configuration->clearRegisteredClients( topic );
		if ( _configuration->isClientRegistered( topic, QHostAddress::Null, 0 ) )
		{
			_configuration->addRegisteredClient( topic, QHostAddress::Null, 0 );
		}
	}
}

int Impl::numClientsConnected() const
{
	return _numClientsConnected;
	//QList<ServerThread*> servers = findChildren<ServerThread*>();
	//return servers.size();
}

void Impl::on_tcpServer_newConnection()
{
	qDebug() << "new TCP connection";
	while( _tcpServer->hasPendingConnections() )
	{
		initThread( new ServerThread( this, _tcpServer->nextPendingConnection() ) );
	}
}

void Impl::on_localServer_newConnection()
{
	qDebug() << "new local connection";
	while( _localServer->hasPendingConnections() )
	{
		initThread( new ServerThread( this, _localServer->nextPendingConnection() ) );
	}
}

void Impl::initThread( ServerThread* thread )
{
    //bool ok = true;

     connect( thread,
		SIGNAL( finished() ),
		thread,
		SLOT( deleteLater() ) );

    connect( thread,
		SIGNAL( rpcRequestReceived( rec::rpc::server::ServerThread*, const QString&, quint32, const QByteArray&, const QHostAddress&, quint16 ) ),
		SLOT( on_rpcRequestReceived( rec::rpc::server::ServerThread*, const QString&, quint32, const QByteArray&, const QHostAddress&, quint16 ) ) );

     connect( thread,
		SIGNAL( topicReceived( const QString&, const QHostAddress&, quint16, const QByteArray& ) ),
		SLOT( on_topicReceived( const QString&, const QHostAddress&, quint16, const QByteArray& ) ) );

     connect( thread,
		SIGNAL( connected( const QHostAddress&, quint16 ) ),
		SLOT( on_connected( const QHostAddress&, quint16 ) ),
		Qt::QueuedConnection );

     connect( thread,
		SIGNAL( disconnected( const QHostAddress&, quint16, const QStringList& ) ),
		SLOT( on_disconnected( const QHostAddress&, quint16, const QStringList& ) ),
		Qt::QueuedConnection );

     connect( thread,
		SIGNAL( error( QAbstractSocket::SocketError, const QString& ) ),
		SLOT( on_client_error( QAbstractSocket::SocketError, const QString& ) ),
		Qt::DirectConnection );

     connect( thread,
		SIGNAL( channelRegistered( rec::rpc::server::ServerThread*, const QString&, const QHostAddress&, quint16 ) ),
		SLOT( on_channelRegistered( rec::rpc::server::ServerThread*, const QString&, const QHostAddress&, quint16 ) ),
		Qt::DirectConnection );

     connect( thread,
		SIGNAL( channelUnregistered( rec::rpc::server::ServerThread*, const QString&, const QHostAddress&, quint16 ) ),
		SLOT( on_channelUnregistered( rec::rpc::server::ServerThread*, const QString&, const QHostAddress&, quint16 ) ),
		Qt::DirectConnection );

    //assert( ok );

	thread->start();

	{
		QMutexLocker lk( &_greetingMessageMutex );
		thread->sendGreeting( _greetingMessage );
	}
	{
		QMutexLocker lk( &_configurationMessageMutex );
		thread->sendConfiguration( _configurationMessage );
	}

	//QMap< QString, rec::rpc::configuration::Item > container = _configuration->itemContainer();
	//QMap< QString, rec::rpc::configuration::Item >::const_iterator iter = container.constBegin();
	//while( container.constEnd() != iter )
	//{
	//	const rec::rpc::configuration::Item& cfgItem = iter.value();
	//	QByteArray messageData = rec::rpc::messages::Data::encode( cfgItem.name, cfgItem.type, cfgItem.data );

	//	thread->send( QString::null, messageData );

	//	++iter;
	//}
}

void Impl::on_configuration_changed()
{
	QMutexLocker lk( &_configurationMessageMutex );
    _configurationMessage = rec::rpc::messages::Configurations::encode( *_configuration );
	sendConfiguration( _configurationMessage );
}

void Impl::on_rpcRequestReceived( ServerThread* receiver, const QString& name, quint32 seqNum, const QByteArray& serParam, const QHostAddress& address, quint16 port )
{
	Q_EMIT rpcRequestReceived( name, seqNum, serParam, quintptr( receiver ), address, port );
}

void Impl::on_topicReceived( const QString& name, const QHostAddress& sourceAddress, quint16 sourcePort, const QByteArray& serData )
{
	QByteArray messageData;

	{
		rec::rpc::configuration::ConfigurationLocker lock( *_configuration );

		if ( false == _configuration->contains( name ) )
			return;

		if ( _configuration->isServerOnly( name ) )
			return;

		if ( false == _configuration->setData( name, serData, sourceAddress, sourcePort ) )
			return;

		messageData = rec::rpc::messages::Topic::encode( name, sourceAddress, sourcePort, serData );
	}

	publishTopic( name, messageData );

	Q_EMIT topicReceived( name, sourceAddress, sourcePort, serData );
}

void Impl::sendConfiguration( const QByteArray& data )
{
	QList<ServerThread*> servers = findChildren<ServerThread*>();
	
	Q_FOREACH( ServerThread* thread, servers )
	{
		thread->sendConfiguration( data );
	}
}

void Impl::sendRPCResponse( const QByteArray& data, rec::rpc::server::ServerThread* receiver )
{
#if 0
	if ( findChildren< rec::rpc::server::ServerThread* >().contains( receiver )
#endif
		receiver->sendRPCResponse( data );
}

void Impl::publishTopic( const QString& name, const QByteArray& data )
{
	QList<ServerThread*> servers = findChildren<ServerThread*>();
	
	Q_FOREACH( ServerThread* thread, servers )
	{
		thread->publishTopic( name, data );
	}
}

void Impl::on_connected( const QHostAddress& address, quint16 port )
{
	++_numClientsConnected;
	Q_EMIT clientConnected( address, port );
}

void Impl::on_disconnected( const QHostAddress& address, quint16 port, const QStringList& registeredChannels )
{
	if( _numClientsConnected > 0 )
	{
		--_numClientsConnected;
	}

	Q_EMIT clientDisconnected( address, port, registeredChannels );
}

void Impl::on_client_error( QAbstractSocket::SocketError err, const QString& errorString )
{
	Q_EMIT clientError( err, errorString );
}

void Impl::on_channelRegistered( rec::rpc::server::ServerThread* thread, const QString& name, const QHostAddress& address, quint16 port )
{
	Q_EMIT channelRegistered( name, address, port );

	if ( !_configuration->isInitialized( name ) )
		return;

	QByteArray messageData;
	{
		rec::rpc::configuration::ConfigurationLocker lock( *_configuration );

		if( false == _configuration->contains( name ) )
		{
			return;
		}

		QHostAddress sourceAddress;
		quint16 sourcePort;
		QByteArray data = _configuration->data( name, &sourceAddress, &sourcePort );

		messageData = rec::rpc::messages::Topic::encode( name, sourceAddress, sourcePort, data );
	}

	thread->publishTopic( name, messageData );
}

void Impl::on_channelUnregistered( rec::rpc::server::ServerThread* thread, const QString& name, const QHostAddress& address, quint16 port )
{
	Q_EMIT channelUnregistered( name, address, port );
}

void Impl::setGreeting( const QString& greeting )
{
	QMutexLocker lk( &_greetingMessageMutex );
	_greetingMessage = greeting.toLatin1();
	_greetingMessage.append( '\n' );
	_greetingMessage.append( '\r' );
}
