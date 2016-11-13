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

#ifndef _REC_RPC_CLIENT_CLIENT_H_
#define _REC_RPC_CLIENT_CLIENT_H_

#include "rec/rpc/Client.h"
#include "rec/rpc/configuration/Configuration.hpp"

#include <QtCore>
#include <QtNetwork>


namespace rec
{
	namespace rpc
	{
		namespace client
		{
			class Socket;

            class PendingRequests : public QObject
			{
				Q_OBJECT
			public:
                PendingRequests( const QString& funcName, quint32 seqNum_, bool blocking_, serialization::SerializablePtr result_, unsigned int timeout );

				void start();
				void stop();

				QString funcName;
				quint32 seqNum;
				bool blocking;
				ErrorCode errorCode;
				serialization::SerializablePtr result;

				QMutex mutex;
				QWaitCondition cond;

			Q_SIGNALS:
                void timeout( PendingRequests* );
				void startTimer();
				void stopTimer();

			private Q_SLOTS:
				void on_timer_timeout();

			private:
				QTimer timer;
			};

            class SequenceNumbers
			{
			public:
                SequenceNumbers();

				operator quint32();

			private:
				QMutex mutex;
				quint32 num;
			};

            class Client : public QThread
			{
				Q_OBJECT
			public:
                Client( QObject* parent = NULL );

                ~Client();

				rec::rpc::configuration::Configuration* configuration() { return &_configuration; }

				void connectToServer( unsigned int msTimeout );
				void disconnectFromServer();

				void setAutoReconnectEnabled( bool enable, unsigned int ms );

				bool isConnected() const;

				QString address() const;
				void setAddress( const QString& address );

				QString expectedGreeting() const;
				void setExpectedGreeting( const QString& greeting );

				unsigned int msTimeout() const;
				void setMsTimeout( unsigned int timeout );

				void getServerVersion( int* major, int* minor, int* patch, int* date, QString* suffix );

				void registerNotifier( const QString& name, rec::rpc::NotifierBase* notifier );
				void unregisterNotifier( const QString& name );

				void registerTopicListener( const QString& name, rec::rpc::TopicListenerBase* listener );
				void unregisterTopicListener( const QString& name );

				void invoke( const QString& name, rec::rpc::serialization::SerializablePtrConst param, rec::rpc::serialization::SerializablePtr result, bool blocking );
				void publishTopic( const QString& name, rec::rpc::serialization::SerializablePtrConst data );

			Q_SIGNALS:
				void stateChanged( QAbstractSocket::SocketState );
				void error( QAbstractSocket::SocketError socketError, const QString& );
				void connected();
				void disconnected( rec::rpc::ErrorCode );
				void startConnectTimer( int );
				void stopConnectTimer();
				void notify( rec::rpc::NotifierBase*, rec::rpc::serialization::SerializablePtrConst, rec::rpc::ErrorCode );
				void listenTopic( rec::rpc::TopicListenerBase* listener, rec::rpc::serialization::SerializablePtrConst, const QHostAddress&, quint16, rec::rpc::ErrorCode );

			private Q_SLOTS:
				void on_stateChanged( QAbstractSocket::SocketState );
				void on_error( QAbstractSocket::SocketError );
				void on_greetingReceived( const QString& greeting );
				void on_configurationReceived( const rec::rpc::configuration::Configuration& );
				void on_disconnected();
				void on_topicReceived( const QString&, const QHostAddress& sourceAddress, quint16 sourcePort, const QByteArray& );
				void on_rpcResponseReceived( const QString& name, quint32 seqNum, quint16 errorCode, const QByteArray& serResult );
                void on_pendingRequest_timeout( PendingRequests* req );
				void on_connectTimer_timeout();
				void on_notify( rec::rpc::NotifierBase* notifier, rec::rpc::serialization::SerializablePtrConst result, rec::rpc::ErrorCode errorCode );
				void on_listenTopic( rec::rpc::TopicListenerBase* listener, rec::rpc::serialization::SerializablePtrConst data, const QHostAddress& address, quint16 port, rec::rpc::ErrorCode ErrorCode );
				void on_autoReconnectTimer_timeout();

			private:
				struct SharedMemInfo
				{
					SharedMemInfo() : seqNum( 0 ) { }
					unsigned int seqNum;
					QSharedPointer< QSharedMemory > mem;
				};

				enum { AutoReconnectEvent = QEvent::User };

				void run();

				void customEvent( QEvent* e );

				void manageTopicListener( const QString& name, bool add );

				bool publishTopicLocal( const QString& name, const QByteArray& serData );
				void publishTopicRemote( const QString& name, const QByteArray& serData );

				void startAutoReconnect();

				QString _expectedGreeting;

				rec::rpc::configuration::Configuration _configuration;

				QHostAddress _address;
				int _port;

				mutable QMutex _socketMutex;

				Socket* _socket;

				QSemaphore _startSemaphore;

				QMap< QString, rec::rpc::NotifierBase* > _notifiers;
				QMap< QString, rec::rpc::TopicListenerBase* > _topicListeners;
				QMutex _topicListenersMutex;

				QTimer _connectTimer;

				QMutex _pendingRequestsMutex;
                QMap< unsigned int, PendingRequests* > _pendingRequests;

                SequenceNumbers _seqNum;

				unsigned int _timeout;

				QMap< QString, SharedMemInfo > _topicLocalMemories;

				ErrorCode _lastError;

				unsigned int _msAutoReconnect;

				QTimer* _autoReconnectTimer;

				static bool _once;
			};

            inline unsigned int Client::msTimeout() const
			{
				return _timeout;
			}

            inline void Client::setMsTimeout( unsigned int timeout )
			{
				_timeout = timeout;
			}
		}
	}
}

#endif //_REC_RPC_CLIENT_CLIENT_H_
