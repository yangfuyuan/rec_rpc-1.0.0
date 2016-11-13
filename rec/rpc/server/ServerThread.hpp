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

#ifndef _REC_RPC_SERVER_SERVERTHREAD_H_
#define _REC_RPC_SERVER_SERVERTHREAD_H_

#include "rec/rpc/client/Socket.hpp"

#include <QtCore>
#include <QtNetwork>

namespace rec
{
	namespace rpc
	{
		namespace server
		{
			class ServerThread : public QThread
			{
				Q_OBJECT
			public:
				ServerThread( QObject* parent, QTcpSocket* tcpSocket );
				ServerThread( QObject* parent, QLocalSocket* localSocket );

				~ServerThread();

				void start();

				void sendGreeting( const QByteArray& greetingMessage );
				void sendConfiguration( const QByteArray& data );
				void sendRPCResponse( const QByteArray& data );
				void publishTopic( const QString& name, const QByteArray& data );

			Q_SIGNALS:
				void rpcRequestReceived( rec::rpc::server::ServerThread*, const QString&, quint32, const QByteArray&, const QHostAddress&, quint16 );
				void topicReceived( const QString&, const QHostAddress&, quint16, const QByteArray& );

				void error( QAbstractSocket::SocketError, const QString& );

				void connected( const QHostAddress&, quint16 );
				void disconnected( const QHostAddress&, quint16, const QStringList& );

				void channelRegistered( rec::rpc::server::ServerThread*, const QString&, const QHostAddress&, quint16 );
				void channelUnregistered( rec::rpc::server::ServerThread*, const QString&, const QHostAddress&, quint16 );

			private Q_SLOTS:
				void on_disconnected();
				void on_error( QAbstractSocket::SocketError );
				void on_rpcRequestReceived( const QString& name, quint32 seqNum, const QByteArray& serParam );
				void on_topicReceived( const QString& name, const QHostAddress&, quint16, const QByteArray& serData );
				void on_registerChannelReceived( const QString& );
				void on_unregisterChannelReceived( const QString& );

			private:
				void run();

				QTcpSocket* _tcpSocket;
				QLocalSocket* _localSocket;

				QMutex _socketMutex;
				rec::rpc::client::Socket* _socket;

				QSemaphore _startSemaphore;

				QHostAddress _peerAddress;
				quint16 _peerPort;

				QMutex _registeredChannelsMutex;
				QSet< QString > _registeredChannels;
			};
		}
	}
}

#endif //_REC_RPC_SERVER_SERVERTHREAD_H_
