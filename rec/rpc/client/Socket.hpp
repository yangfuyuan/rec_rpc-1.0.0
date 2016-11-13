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

#ifndef _REC_RPC_CLIENT_SOCKET_H_
#define _REC_RPC_CLIENT_SOCKET_H_

#include "rec/rpc/defines.h"
#include "rec/rpc/configuration/Configuration.hpp"

#include <QtCore>
#include <QtNetwork>

namespace rec
{
	namespace rpc
	{
		namespace client
		{
			class Socket : public QObject
			{
				Q_OBJECT
			public:
				enum EventType
				{
					SendGreetingEventId = QEvent::User,
					SendConfigurationEventId,
					SendRPCRequestEventId,
					SendRPCResponseEventId,
					PublishTopicEventId,
					ConnectToHostEventId,
					CloseEventId,
					RegisterChannelEventId,
					UnregisterChannelEventId,
				};

				Socket();

				QAbstractSocket::SocketState state() const;
				QString errorString() const;
				QHostAddress peerAddress() const;
				quint16 peerPort() const;

				void setTcpSocket( QTcpSocket* socket );
				void setLocalSocket( QLocalSocket* socket );

			Q_SIGNALS:
				void stateChanged( QAbstractSocket::SocketState );
				void disconnected();
				void error( QAbstractSocket::SocketError );
				void greetingReceived( const QString& );
				void configurationReceived( const rec::rpc::configuration::Configuration& config );
				void rpcRequestReceived( const QString&, quint32, const QByteArray& );
				void rpcResponseReceived( const QString&, quint32, quint16, const QByteArray& );
				void topicReceived( const QString&, const QHostAddress&, quint16, const QByteArray& );
				void registerChannelReceived( const QString& );
				void unregisterChannelReceived( const QString& );

			private Q_SLOTS:
				void on_readyRead();
				void on_localSocket_error( QLocalSocket::LocalSocketError err );
				void on_localSocket_stateChanged( QLocalSocket::LocalSocketState state );
				void on_tcpSocket_stateChanged( QAbstractSocket::SocketState );
				void on_tcpSocket_error( QAbstractSocket::SocketError );

			private:
				class Id
				{
				public:
					static quint16 getNext();

				private:
					static QMutex mutex;
					static quint16 id;
				};

				void customEvent( QEvent* e );

				void closeSocket();

				void readGreeting();
				void readHeader();
				void readMessage();

				enum ReceiveState { GreetingReceiveState, HeaderReceiveState, MessageDataReceiveState };
				ReceiveState _receiveState;

				int _bytesToRead;
				quint8 _currentMessageId;

				QTcpSocket* _tcpSocket;
				QLocalSocket* _localSocket;
				QIODevice* _currentSocket;

				quint16 _localSocketId;
			};

			class SendMessageEvent : public QEvent
			{
			public:
				SendMessageEvent( Socket::EventType type, const QByteArray& message_ )
					: QEvent( (QEvent::Type)type )
					, message( message_ )
				{
				}

				QByteArray message;
			};

			class SendGreetingEvent : public SendMessageEvent
			{
			public:
				SendGreetingEvent( const QByteArray& message_ )
					: SendMessageEvent( Socket::SendGreetingEventId, message_ )
				{
				}
			};

			class SendConfigurationEvent : public SendMessageEvent
			{
			public:
				SendConfigurationEvent( const QByteArray& message_ )
					: SendMessageEvent( Socket::SendConfigurationEventId, message_ )
				{
				}
			};

			class RegisterChannelEvent : public SendMessageEvent
			{
			public:
				RegisterChannelEvent( const QByteArray& message_ )
					: SendMessageEvent( Socket::RegisterChannelEventId, message_ )
				{
				}
			};

			class UnregisterChannelEvent : public SendMessageEvent
			{
			public:
				UnregisterChannelEvent( const QByteArray& message_ )
					: SendMessageEvent( Socket::UnregisterChannelEventId, message_ )
				{
				}
			};

			class SendRPCRequestEvent : public SendMessageEvent
			{
			public:
				SendRPCRequestEvent( const QByteArray& message_ )
					: SendMessageEvent( Socket::SendRPCRequestEventId, message_ )
				{
				}
			};

			class SendRPCResponseEvent : public SendMessageEvent
			{
			public:
				SendRPCResponseEvent( const QByteArray& message_ )
					: SendMessageEvent( Socket::SendRPCResponseEventId, message_ )
				{
				}
			};

			class PublishTopicEvent : public SendMessageEvent
			{
			public:
				PublishTopicEvent( const QByteArray& message_ )
					: SendMessageEvent( Socket::PublishTopicEventId, message_ )
				{
				}
			};

			class ConnectToHostEvent : public QEvent
			{
			public:
				ConnectToHostEvent( const QHostAddress& address_, int port_ )
					: QEvent( (QEvent::Type)Socket::ConnectToHostEventId )
					, address( address_ )
					, port( port_ )
				{
				}

				const QHostAddress address;
				int port;
			};

			class CloseEvent : public QEvent
			{
			public:
				CloseEvent()
					: QEvent( (QEvent::Type)Socket::CloseEventId )
				{
				}
			};
		}
	}
}

#endif //_REC_RPC_CLIENT_SOCKET_H_
