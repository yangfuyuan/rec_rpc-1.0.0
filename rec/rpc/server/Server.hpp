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

#ifndef _REC_RPC_SERVER_SERVER_H_
#define _REC_RPC_SERVER_SERVER_H_

#include "rec/rpc/Server.h"
#include "rec/rpc/configuration/Configuration.hpp"

#include <QtCore>
#include <QtNetwork>

namespace rec
{
	namespace rpc
	{
		namespace server
		{
			class Impl;

            class Server : public QThread
			{
				Q_OBJECT
			public:
                Server( QObject* parent = NULL );
				
                ~Server();

				bool listen( int port = -1, bool blocking = false );

				bool isListening() const;

				quint16 serverPort() const;

				QString greeting() const;
				void setGreeting( const QString& greeting );

				void close();

				void registerFunction( const QString& name, rec::rpc::RPCFunctionBase* function );
				void unregisterFunction( const QString& name );

				void registerTopicListener( const QString& name, rec::rpc::TopicListenerBase* listener );
				void unregisterTopicListener( const QString& name );

				void addTopic( const QString& name, int sharedMemorySize );

				void publishTopic( const QString& name, rec::rpc::serialization::SerializablePtrConst data );

				int numClientsConnected() const;

				void disconnectAllClients();

				rec::rpc::configuration::Configuration* configuration() { return &_configuration; }

			Q_SIGNALS:
				void clientConnected( const QHostAddress&, unsigned short );
				void clientDisconnected( const QHostAddress&, unsigned short );
				void serverError( QAbstractSocket::SocketError error, const QString& errorString );
				void clientError( QAbstractSocket::SocketError error, const QString& errorString );
				void registeredTopicListener( const QString&, const QHostAddress&, unsigned short );
				void unregisteredTopicListener( const QString&, const QHostAddress&, unsigned short );
				void closed();
				void listening();
				void invoke( rec::rpc::RPCFunctionBase*, rec::rpc::serialization::SerializablePtrConst, rec::rpc::serialization::SerializablePtr, const QHostAddress&, quint16, rec::rpc::ErrorCode* );
				void listenTopic( rec::rpc::TopicListenerBase* listener, rec::rpc::serialization::SerializablePtrConst, const QHostAddress&, quint16, rec::rpc::ErrorCode );

			private Q_SLOTS:
				void on_clientConnected( const QHostAddress& address, quint16 port );
				void on_clientDisconnected( const QHostAddress& address, quint16 port, const QStringList& );
				void on_channelRegistered( const QString& name, const QHostAddress& address, quint16 port );
				void on_channelUnregistered( const QString& name, const QHostAddress& address, quint16 port );
				void on_serverError( QAbstractSocket::SocketError error, const QString& errorString );
				void on_clientError( QAbstractSocket::SocketError error, const QString& errorString );
				void on_listening();
				void on_closed();

				void on_rpcRequestReceived( const QString& name, quint32 seqNum, const QByteArray& serParam, quintptr receiver, const QHostAddress& address, quint16 port );
				void on_topicReceived( const QString& name, const QHostAddress& sourceAddress, quint16 sourcePort, const QByteArray& serData );

				void on_invoke( rec::rpc::RPCFunctionBase* func, rec::rpc::serialization::SerializablePtrConst param, rec::rpc::serialization::SerializablePtr result, const QHostAddress& address, quint16 port, rec::rpc::ErrorCode* errorCode );
				void on_listenTopic( rec::rpc::TopicListenerBase* listener, rec::rpc::serialization::SerializablePtrConst data, const QHostAddress& address, quint16 port, rec::rpc::ErrorCode ErrorCode );

			private:
				struct SharedMemInfo
				{
					SharedMemInfo() : seqNum( 0 ), keySuffix( 0 ), initialSize( 0 ) { }
					unsigned int seqNum;
					unsigned int keySuffix;
					int initialSize;
					QSharedPointer< QSharedMemory > mem;
				};

				static const QString _infoSuffix;

				static QString getInfoName( const QString& name );
				QString getMemName( const QString& name, unsigned int suffix ) const;

				void run();
				void setupServer();

				void publishTopicLocal( const QString& name, const QHostAddress& sourceAddress, quint16 sourcePort, const QByteArray& serData );
				void publishTopicRemote( const QString& name, const QHostAddress& sourceAddress, quint16 sourcePort, const QByteArray& serData );

				bool _run;

				Impl* _impl;

				QSemaphore _startSemaphore;

				mutable QMutex _listenMutex;
				QWaitCondition _listenCondition;

				int _serverPort;

				rec::rpc::configuration::Configuration _configuration;

				QMap< QString, rec::rpc::RPCFunctionBase* > _rpcFunctions;
				QMap< QString, rec::rpc::TopicListenerBase* > _topicListeners;
				QMutex _topicListenersMutex;

				QMap< QString, SharedMemInfo > _topicLocalMemories;

				QString _greeting;

				static bool _once;
			};
		}
	}
}

#endif //_REC_RPC_SERVER_SERVER_H_
