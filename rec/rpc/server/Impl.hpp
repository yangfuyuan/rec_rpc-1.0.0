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

#ifndef _REC_RPC_SERVER_IMPL_H_
#define _REC_RPC_SERVER_IMPL_H_

#include "rec/rpc/defines.h"
#include "rec/rpc/configuration/Configuration.hpp"

#include <QtCore>
#include <QtNetwork>

namespace rec
{
	namespace rpc
	{
		namespace server
		{
			class ServerThread;

			class Impl : public QObject
			{
				Q_OBJECT
			public:
				enum {
					ListenEventId = QEvent::User,
					CloseEventId,
					DisconnectAllClientsId,
					SendDataEventId
				};

				Impl( rec::rpc::configuration::Configuration* configuration, const QString& greeting );
				
				~Impl();

				int numClientsConnected() const;

				void sendRPCResponse( const QByteArray& data, ServerThread* receiver );
				void publishTopic( const QString& name, const QByteArray& data );

				void setGreeting( const QString& greeting );

				QHostAddress serverAddress() const;
				quint16 serverPort() const;
				bool isListening() const;

			Q_SIGNALS:
				void clientConnected( const QHostAddress&, quint16 );
				void clientDisconnected( const QHostAddress&, quint16, const QStringList& );
				void serverError( QAbstractSocket::SocketError error, const QString& errorString );
				void clientError( QAbstractSocket::SocketError error, const QString& errorString );
				void rpcRequestReceived( const QString&, quint32, const QByteArray&, quintptr, const QHostAddress&, quint16 );
				void topicReceived( const QString&, const QHostAddress&, quint16, const QByteArray& );
				void channelRegistered( const QString&, const QHostAddress&, quint16 );
				void channelUnregistered( const QString&, const QHostAddress&, quint16 );
				void listening();
				void closed();

			private Q_SLOTS:
				void on_rpcRequestReceived( rec::rpc::server::ServerThread* receiver, const QString& name, quint32 seqNum, const QByteArray& serParam, const QHostAddress& address, quint16 port );
				void on_topicReceived( const QString& name, const QHostAddress& sourceAddress, quint16 sourcePort, const QByteArray& serData );
				void on_configuration_changed();
				void on_connected( const QHostAddress&, quint16 );
				void on_disconnected( const QHostAddress&, quint16, const QStringList& );
				void on_client_error( QAbstractSocket::SocketError error, const QString& errorString );
				void on_channelRegistered( rec::rpc::server::ServerThread* thread, const QString& name, const QHostAddress&, quint16 );
				void on_channelUnregistered( rec::rpc::server::ServerThread* thread, const QString& name, const QHostAddress&, quint16 );
				void on_tcpServer_newConnection();
				void on_localServer_newConnection();

			private:
				void customEvent( QEvent* e );

				void disconnectAllClients();

				void sendConfiguration( const QByteArray& data );

				void initThread( ServerThread* thread );

				QTcpServer* _tcpServer;
				QLocalServer* _localServer;

				rec::rpc::configuration::Configuration* _configuration;

				QMutex _configurationMessageMutex;
				QByteArray _configurationMessage;

				QMutex _greetingMessageMutex;
				QByteArray _greetingMessage;

				int _numClientsConnected;
			};

			class ListenEvent : public QEvent
			{
			public:
				ListenEvent( int port_ )
					: QEvent( (QEvent::Type)Impl::ListenEventId )
					, port( port_ )
				{
				}

				int port;
			};

			class CloseEvent : public QEvent
			{
			public:
				CloseEvent()
					: QEvent( (QEvent::Type)Impl::CloseEventId )
				{
				}
			};

			class DisconnectAllClients : public QEvent
			{
			public:
				DisconnectAllClients()
					: QEvent( (QEvent::Type)Impl::DisconnectAllClientsId )
				{
				}
			};
		}
	}
}

#endif //_REC_RPC_SERVER_SERVER_H_
