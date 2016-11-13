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

#include "rec/rpc/client/Socket.hpp"
#include "rec/rpc/messages/Message.h"
#include "rec/rpc/messages/Configurations.h"
#include "rec/rpc/messages/RPCRequest.h"
#include "rec/rpc/messages/RPCResponse.h"
#include "rec/rpc/messages/Topic.h"
#include "rec/rpc/messages/RegisterChannel.h"
#include "rec/rpc/messages/UnregisterChannel.h"
#include "rec/rpc/common.h"
#include "rec/rpc/common_internal.hpp"

#include <cassert>

using namespace rec::rpc::client;

quint16 Socket::Id::id = 0;
QMutex Socket::Id::mutex;

quint16 Socket::Id::getNext()
{
	QMutexLocker lk( &mutex );
	if ( ++id == 0 )
		++id;
	return id;
}

Socket::Socket()
: _tcpSocket( 0 )
, _localSocket( 0 )
, _currentSocket( 0 )
, _receiveState( HeaderReceiveState )
, _bytesToRead( messages::headerSize )
, _localSocketId( 0 )
{
}

QAbstractSocket::SocketState Socket::state() const
{
	if( _tcpSocket )
		return _tcpSocket->state();
	if( _localSocket )
		return (QAbstractSocket::SocketState)_localSocket->state();
	return QAbstractSocket::UnconnectedState;
}

QString Socket::errorString() const
{
	if( _tcpSocket )
		return _tcpSocket->errorString();
	if( _localSocket )
		return _localSocket->errorString();
	return QString::null;
}

QHostAddress Socket::peerAddress() const
{
	if ( _tcpSocket )
		return _tcpSocket->peerAddress();
	return QHostAddress::Null;
}

quint16 Socket::peerPort() const
{
	if ( _tcpSocket )
		return _tcpSocket->peerPort();
	return _localSocketId;
}

void Socket::setTcpSocket( QTcpSocket* socket )
{
	assert( !_tcpSocket && !_localSocket );
	_tcpSocket = socket;
	_tcpSocket->setParent( this );
	_currentSocket = _tcpSocket;

    //bool ok = true;
     connect( _tcpSocket, SIGNAL( readyRead() ), SLOT( on_readyRead() ) );
     connect( _tcpSocket,
		SIGNAL( stateChanged( QAbstractSocket::SocketState ) ),
		SLOT( on_tcpSocket_stateChanged( QAbstractSocket::SocketState ) ),
		Qt::DirectConnection );
     connect( _tcpSocket,
		SIGNAL( error( QAbstractSocket::SocketError ) ),
		SLOT( on_tcpSocket_error( QAbstractSocket::SocketError ) ),
		Qt::DirectConnection );
     connect( _tcpSocket, SIGNAL( disconnected() ), SIGNAL( disconnected() ), Qt::DirectConnection );
    //assert( ok );
}

void Socket::setLocalSocket( QLocalSocket* socket )
{
	assert( !_tcpSocket && !_localSocket );
	_localSocket = socket;
	_localSocket->setParent( this );
	_currentSocket = _localSocket;

	_localSocketId = Id::getNext();

    //bool ok = true;
     connect( _localSocket, SIGNAL( readyRead() ), SLOT( on_readyRead() ) );
     connect( _localSocket,
		SIGNAL( stateChanged( QLocalSocket::LocalSocketState ) ),
		SLOT( on_localSocket_stateChanged( QLocalSocket::LocalSocketState ) ),
		Qt::DirectConnection );
     connect( _localSocket,
		SIGNAL( error( QLocalSocket::LocalSocketError ) ),
		SLOT( on_localSocket_error( QLocalSocket::LocalSocketError ) ),
		Qt::DirectConnection );
     connect( _localSocket, SIGNAL( disconnected() ), SIGNAL( disconnected() ), Qt::DirectConnection );
    //assert( ok );
}

void Socket::customEvent( QEvent* e )
{
	//qDebug() << "Socket::customEvent " << e->type();

	switch( e->type() )
	{
	case SendGreetingEventId:
	case SendConfigurationEventId:
	case SendRPCRequestEventId:
	case SendRPCResponseEventId:
	case PublishTopicEventId:
	case RegisterChannelEventId:
	case UnregisterChannelEventId:
		{
			if ( _tcpSocket )
				_tcpSocket->write( static_cast< SendMessageEvent* >( e )->message );
			if ( _localSocket )
				_localSocket->write( static_cast< SendMessageEvent* >( e )->message );
		}
		break;

	case ConnectToHostEventId:
		{
			if( !_currentSocket )
			{
				ConnectToHostEvent* ev = static_cast< ConnectToHostEvent* >( e );
				if( -1 == ev->port )
				{
					ev->port = rec::rpc::defaultPort;
				}
				_receiveState = GreetingReceiveState;
				assert( !_tcpSocket && !_localSocket );
				bool connectTcp = true;
				if ( ev->address == QHostAddress::LocalHost || ev->address == QHostAddress::LocalHostIPv6 )
				{
					setLocalSocket( new QLocalSocket( this ) );
					_localSocket->connectToServer( QString( "__REC__RPC__%1__" ).arg( ev->port ) );
					if( NULL != _localSocket )
					{
						if ( _localSocket->waitForConnected() )
						{
							connectTcp = false;
						}
						else
						{
							_localSocket->close();
							delete _localSocket;
							_localSocket = 0;
						}
					}
				}
				if( connectTcp )
				{
					setTcpSocket( new QTcpSocket( this ) );
					_tcpSocket->connectToHost( ev->address, ev->port );
				}
			}
		}
		break;

	case CloseEventId:
		closeSocket();
		break;

	default:
		break;
	}
}

void Socket::closeSocket()
{
	if ( _tcpSocket )
	{
		_tcpSocket->close();
		_tcpSocket->deleteLater();
		_tcpSocket = 0;
	}
	if ( _localSocket )
	{
		_localSocket->close();
		_localSocket->deleteLater();
		_localSocket = 0;
	}
	_currentSocket = 0;
}

void Socket::on_readyRead()
{
	assert( _currentSocket );

	switch( _receiveState )
	{
	case GreetingReceiveState:
		if ( _currentSocket->bytesAvailable() == 0 )
		{
			return;
		}
		else
		{
			readGreeting();
		}

	case HeaderReceiveState:
		if( _currentSocket->bytesAvailable() < _bytesToRead )
		{
			return;
		}
		else
		{
			readHeader();
		}
		break;

	case MessageDataReceiveState:
		if( _currentSocket->bytesAvailable() < _bytesToRead )
		{
			return;
		}
		else
		{
			readMessage();
		}
		break;

	default:
		break;
	}
}

void Socket::readGreeting()
{
	QByteArray greetingMessage;
	char byte;
	quint64 len;
	quint64 bytesRead = 0;
	while( bytesRead < rec::rpc::Common::maxGreetingLength + 2 )
	{
		len = _currentSocket->read( &byte, 1 );
		bytesRead += len;
		if ( 0 == len || byte == '\n' )
		{
			bytesRead += _currentSocket->read( &byte, 1 ); // read \r (last byte)
			break;
		}
		greetingMessage.append( byte );
	}
	_receiveState = HeaderReceiveState;
	Q_EMIT greetingReceived( QString::fromLatin1( greetingMessage.constData(), greetingMessage.size() ) );
}

void Socket::readHeader()
{
	QByteArray headerData = _currentSocket->read( _bytesToRead );

	_currentMessageId = headerData.at( 0 );

	_bytesToRead = *( reinterpret_cast<const quint32*>( headerData.data() + 1 ) );

	if( _currentSocket->bytesAvailable() < _bytesToRead )
	{
		_receiveState = MessageDataReceiveState;
	}
	else
	{
		readMessage();
	}
}

void Socket::readMessage()
{
	QByteArray messageData = _currentSocket->read( _bytesToRead );

	switch( _currentMessageId )
	{
	case rec::rpc::messages::ConfigurationId:
		{
			//qDebug() << "Client configuration received";

			rec::rpc::configuration::Configuration cfg;
            if( false == rec::rpc::messages::Configurations::decode( messageData, &cfg ) )
			{
				qDebug() << "Client error decoding configuration";
			}
			else
			{
				Q_EMIT configurationReceived( cfg );
			}
		}
		break;

	case rec::rpc::messages::RPCRequestId:
		{
			QString name;
			quint32 seqNum;
			QByteArray param;

			if ( rec::rpc::messages::RPCRequest::decode( messageData, &name, &seqNum, &param ) )
			{
				Q_EMIT rpcRequestReceived( name, seqNum, param );
			}
		}
		break;

	case rec::rpc::messages::RPCResponseId:
		{
			QString name;
			quint32 seqNum;
			quint16 errorCode;
			QByteArray param;

			if ( rec::rpc::messages::RPCResponse::decode( messageData, &name, &seqNum, &errorCode, &param ) )
			{
				Q_EMIT rpcResponseReceived( name, seqNum, errorCode, param );
			}
		}
		break;

	case rec::rpc::messages::TopicId:
		{
			QString name;
			QHostAddress sourceAddress;
			quint16 sourcePort;
			QByteArray data;

			if ( rec::rpc::messages::Topic::decode( messageData, &name, &sourceAddress, &sourcePort, &data ) )
			{
				Q_EMIT topicReceived( name, sourceAddress, sourcePort, data );
			}
		}
		break;

	case rec::rpc::messages::RegisterChannelId:
		{
			QString name = rec::rpc::messages::RegisterChannel::decode( messageData );
			//qDebug() << "Register channel " << name << " received";
			Q_EMIT registerChannelReceived( name );
		}
		break;

	case rec::rpc::messages::UnregisterChannelId:
		{
			QString name = rec::rpc::messages::UnregisterChannel::decode( messageData );
			//qDebug() << "Unregister channel " << name << " received";
			Q_EMIT unregisterChannelReceived( name );
		}
		break;

	default:
		//qDebug() << "Unknown message id " << _currentMessageId;
		break;
	}

	_bytesToRead = rec::rpc::messages::headerSize;
	_receiveState = HeaderReceiveState;

	if( _currentSocket->bytesAvailable() >= _bytesToRead )
	{
		readHeader();
	}
}

void Socket::on_localSocket_error( QLocalSocket::LocalSocketError err )
{
	Q_EMIT error( (QAbstractSocket::SocketError)err );
}

void Socket::on_localSocket_stateChanged( QLocalSocket::LocalSocketState state )
{
	if( QLocalSocket::UnconnectedState == state )
	{
		closeSocket();
	}

	Q_EMIT stateChanged( (QAbstractSocket::SocketState)state );
}

void Socket::on_tcpSocket_error( QAbstractSocket::SocketError err )
{
	Q_EMIT error( err );
}

void Socket::on_tcpSocket_stateChanged( QAbstractSocket::SocketState state )
{
	if( QAbstractSocket::UnconnectedState == state )
	{
		closeSocket();
	}

	Q_EMIT stateChanged( state );
}

