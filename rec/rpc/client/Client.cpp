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

#include "rec/rpc/client/Client.hpp"
#include "rec/rpc/client/Socket.hpp"

#include "rec/rpc/messages/RPCRequest.h"
#include "rec/rpc/messages/Topic.h"
#include "rec/rpc/messages/RegisterChannel.h"
#include "rec/rpc/messages/UnregisterChannel.h"

#include "rec/rpc/serialization/LocalTopic.hpp"
#include "rec/rpc/serialization/VersionInfo.hpp"

#include "rec/rpc/Exception.h"

#include "rec/rpc/common_internal.hpp"

#include <QMetaType>
#include <cassert>

//Q_DECLARE_METATYPE( QAbstractSocket::SocketState )
//Q_DECLARE_METATYPE( QAbstractSocket::SocketError )
//Q_DECLARE_METATYPE( QHostAddress )

using namespace rec::rpc::client;

PendingRequests::PendingRequests( const QString& funcName_, quint32 seqNum_, bool blocking_, rec::rpc::serialization::SerializablePtr result_, unsigned int timeout )
: timer( this )
, funcName( funcName_ )
, seqNum( seqNum_ )
, blocking( blocking_ )
, errorCode( NoError )
, result( result_ )
{
	timer.setSingleShot( true );
	timer.setInterval( timeout );

    //bool ok = true;
     connect( &timer, SIGNAL( timeout() ), SLOT( on_timer_timeout() ) );
     connect( this, SIGNAL( startTimer() ), &timer, SLOT( start() ), Qt::QueuedConnection );
     connect( this, SIGNAL( stopTimer() ), &timer, SLOT( stop() ), Qt::QueuedConnection );
    //assert( ok );
}

void PendingRequests::on_timer_timeout()
{
	Q_EMIT timeout( this );
}

void PendingRequests::start()
{
	Q_EMIT startTimer();
}

void PendingRequests::stop()
{
	Q_EMIT stopTimer();
}

SequenceNumbers::SequenceNumbers()
: num( 0 )
{
}

SequenceNumbers::operator quint32()
{
	QMutexLocker lock( &mutex );
	return num++;
}

bool Client::_once = false;

Client::Client( QObject* parent )
: QThread( parent )
, _socket( NULL )
, _connectTimer( this )
, _timeout( 2000 )
, _lastError( NoError )
, _msAutoReconnect( 0 )
, _autoReconnectTimer( new QTimer( this ) )
{
	//if( NULL == QCoreApplication::instance() )
	//{
	//	static int argc = 1;
	//	static char ab[4] = { 'a', 0, 'b', 0 };
	//	static char* argv[2] = { ab, ab + 2 };
	//	new QCoreApplication( argc, argv );
	//}

	setObjectName( "Client" );

	if ( !_once )
	{
		_once = true;
        qRegisterMetaType< QHostAddress >("QHostAddress ");
        qRegisterMetaType< QAbstractSocket::SocketState >("QAbstractSocket::SocketState");
        qRegisterMetaType< QAbstractSocket::SocketError >("QAbstractSocket::SocketError");
//		qRegisterMetaType< QHostAddress >();
//		qRegisterMetaType< QAbstractSocket::SocketState >();
//		qRegisterMetaType< QAbstractSocket::SocketError >();
		qRegisterMetaType< rec::rpc::configuration::Configuration >();
		qRegisterMetaType< rec::rpc::ErrorCode >();
		qRegisterMetaType< rec::rpc::serialization::SerializablePtr >();
		qRegisterMetaType< rec::rpc::serialization::SerializablePtrConst >();
	}

    //bool ok = true;
     connect( &_connectTimer, SIGNAL( timeout() ), SLOT( on_connectTimer_timeout() ) );
     connect( this, SIGNAL( startConnectTimer( int ) ), &_connectTimer, SLOT( start( int ) ), Qt::QueuedConnection );
     connect( this, SIGNAL( stopConnectTimer() ), &_connectTimer, SLOT( stop() ), Qt::QueuedConnection );

     connect( this
		, SIGNAL( notify( rec::rpc::NotifierBase*, rec::rpc::serialization::SerializablePtrConst, rec::rpc::ErrorCode ) )
		, SLOT( on_notify( rec::rpc::NotifierBase*, rec::rpc::serialization::SerializablePtrConst, rec::rpc::ErrorCode ) )
		, Qt::QueuedConnection );

     connect( this
		, SIGNAL( listenTopic( rec::rpc::TopicListenerBase*, rec::rpc::serialization::SerializablePtrConst, const QHostAddress&, quint16, rec::rpc::ErrorCode ) )
		, SLOT( on_listenTopic( rec::rpc::TopicListenerBase*, rec::rpc::serialization::SerializablePtrConst, const QHostAddress&, quint16, rec::rpc::ErrorCode ) )
		, Qt::QueuedConnection );


     connect( _autoReconnectTimer, SIGNAL( timeout() ), SLOT( on_autoReconnectTimer_timeout() ) );

//	Q_ASSERT( ok );

	_autoReconnectTimer->setSingleShot( true );

	start();
	_startSemaphore.acquire();
}

Client::~Client()
{
	disconnectFromServer();
	exit();
	wait();
	qDeleteAll( _topicListeners );
	qDeleteAll( _notifiers );
	_topicListeners.clear();
	_notifiers.clear();
}

void Client::run()
{
	{
		QMutexLocker lk( &_socketMutex );
        _socket = new rec::rpc::client::Socket;
	}

    //bool ok = true;
    QObject::connect( _socket, SIGNAL( stateChanged( QAbstractSocket::SocketState ) ), SLOT( on_stateChanged( QAbstractSocket::SocketState ) ), Qt::DirectConnection );
    QObject::connect( _socket, SIGNAL( disconnected() ), SLOT( on_disconnected() ), Qt::DirectConnection );

     QObject::connect( _socket,
		SIGNAL( error( QAbstractSocket::SocketError ) ),
		SLOT( on_error( QAbstractSocket::SocketError ) ),
		Qt::DirectConnection );

     QObject::connect( _socket,
		SIGNAL( greetingReceived( const QString& ) ),
		SLOT( on_greetingReceived( const QString& ) ),
		Qt::DirectConnection );

     QObject::connect( _socket,
		SIGNAL( configurationReceived( const rec::rpc::configuration::Configuration& ) ),
		SLOT( on_configurationReceived( const rec::rpc::configuration::Configuration& ) ),
		Qt::DirectConnection );

     QObject::connect( _socket,
		SIGNAL( rpcResponseReceived( const QString&, quint32, quint16, const QByteArray& ) ),
		SLOT( on_rpcResponseReceived( const QString&, quint32, quint16, const QByteArray& ) ),
		Qt::DirectConnection );

     QObject::connect( _socket,
		SIGNAL( topicReceived( const QString&, const QHostAddress&, quint16, const QByteArray& ) ),
		SLOT( on_topicReceived( const QString&, const QHostAddress&, quint16, const QByteArray& ) ),
		Qt::DirectConnection );

    //assert( ok );

	_startSemaphore.release();

	exec();

	QMutexLocker lk( &_socketMutex );

	delete _socket;
	_socket = NULL;
}

void Client::customEvent( QEvent* e )
{
	switch( e->type() )
	{
	case AutoReconnectEvent:
		startAutoReconnect();
		break;
	}
}

void Client::connectToServer( unsigned int msTimeout )
{
	if( isConnected() )
	{
		return;
	}

	_autoReconnectTimer->stop();

	_connectTimer.setSingleShot( true );
	Q_EMIT startConnectTimer( msTimeout );
	qApp->postEvent( _socket, new ConnectToHostEvent( _address, _port ) );
}

void Client::disconnectFromServer()
{
	qApp->postEvent( _socket, new CloseEvent );
	{
		// Clean up pending requests.
		QMutexLocker l( &_pendingRequestsMutex );
        Q_FOREACH( PendingRequests* req, _pendingRequests )
		{
			req->mutex.lock();
			req->stop();
			req->mutex.unlock();
			req->deleteLater();
		}
		_pendingRequests.clear();
	}
}

void Client::setAutoReconnectEnabled( bool enable, unsigned int ms )
{
	if( enable )
	{
		_msAutoReconnect = ms;
	}
	else
	{
		_msAutoReconnect = -1;
	}
}

void Client::on_connectTimer_timeout()
{
	disconnectFromServer();
	Q_EMIT disconnected( rec::rpc::NoConnection );
	qApp->postEvent( this, new QEvent( (QEvent::Type)AutoReconnectEvent ) );
}

void Client::setAddress( const QString& address )
{
	QStringList l = address.split( ':' );

	_address = QHostAddress( l.first() );

	if( l.size() > 1 )
	{
		_port = l.at(1).toUInt();
	}
	else
	{
		_port = -1;
	}
}

QString Client::address() const
{
	if( _port > -1 )
	{
		return QString( "%1:%2" ).arg( _address.toString() ).arg( _port );
	}
	else
	{
		return _address.toString();
	}
}

QString Client::expectedGreeting() const
{
	return _expectedGreeting;
}

void Client::setExpectedGreeting( const QString& greeting )
{
	_expectedGreeting = greeting;
	if ( _expectedGreeting.length() > rec::rpc::Common::maxGreetingLength )
		_expectedGreeting.resize( rec::rpc::Common::maxGreetingLength );
}

bool Client::isConnected() const
{
	return ( QAbstractSocket::ConnectedState == _socket->state() );
}

void Client::on_stateChanged( QAbstractSocket::SocketState state )
{
	Q_EMIT stateChanged( state );
}

void Client::on_error( QAbstractSocket::SocketError err )
{
	Q_EMIT error( err, _socket->errorString() );
}

void Client::on_disconnected()
{
	_topicLocalMemories.clear();
	Q_EMIT disconnected( _lastError );
	_lastError = NoError;

	qApp->postEvent( this, new QEvent( (QEvent::Type)AutoReconnectEvent ) );
}

void Client::on_greetingReceived( const QString& greeting )
{
	if ( !_connectTimer.isActive() )
		return;

	if ( _expectedGreeting.isEmpty() || _expectedGreeting == greeting )
		return;

	Q_EMIT stopConnectTimer();
	_lastError = IncompatibleServer;
	disconnectFromServer();
}

void Client::on_configurationReceived( const rec::rpc::configuration::Configuration& cfg )
{
	if ( !_connectTimer.isActive() )
		return;

	_configuration = cfg;

	Q_EMIT stopConnectTimer();
	try
	{
		QMutexLocker lk( &_topicListenersMutex );
		for( QMap< QString, rec::rpc::TopicListenerBase* >::const_iterator iter = _topicListeners.constBegin();
			iter != _topicListeners.constEnd(); ++iter )
		{
			manageTopicListener( iter.key(), true );
		}
	}
	catch( const rec::rpc::Exception& e )
	{
		_lastError = e.errorCode();
		disconnectFromServer();
		return;
	}
	_lastError = NoError;
	Q_EMIT connected();
}

void Client::on_topicReceived( const QString& name, const QHostAddress& sourceAddress, quint16 sourcePort, const QByteArray& value )
{
	if( _configuration.setData( name, value, sourceAddress, sourcePort ) )
	{
		QString topicName = name;
		bool local = false;
		if ( name.endsWith( rec::rpc::Common::localSuffix ) )
		{
			topicName.chop( rec::rpc::Common::localSuffix.length() );
			local = true;
		}

		rec::rpc::TopicListenerBase* listener = 0;
		{
			QMutexLocker lk( &_topicListenersMutex );
			if ( _topicListeners.contains( topicName ) )
			{
				listener = _topicListeners.value( topicName );
				assert( listener );
				listener->active = true;
			}
		}

		if ( listener )
		{	// A listener for this topic exists. Deserialize data.
			serialization::SerializablePtr data = listener->createData();
			if ( data )
			{
				ErrorCode errorCode = NoError;
				if ( local )
				{	// topic data have been received via a local connection (shared memory).
					serialization::LocalTopic lData;
					{
						QDataStream s( value );
						if ( ( s >> lData ).status() != QDataStream::Ok )
							errorCode = WrongDataFormat;
					}
					if ( lData.segmentName.isEmpty() )
						errorCode = WrongDataFormat;

					// Now we know sequence number, the name of the shared memory segment and the size of the serialized data
					if ( errorCode == NoError )
					{
						SharedMemInfo* memInfo = &_topicLocalMemories[ topicName ];
						if ( !memInfo->mem || memInfo->mem->key() != lData.segmentName )
						{	// If not done yet, attach to the shared memory segment
							memInfo->mem = QSharedPointer< QSharedMemory >( new QSharedMemory( lData.segmentName ) );
							if ( !memInfo->mem->attach() )
							{
								memInfo = 0;
								_topicLocalMemories.remove( topicName );
								errorCode = WrongDataFormat;
							}
						}
						if ( errorCode == NoError )
						{	// access the segment and deserialize the topic data.
							memInfo->mem->lock();
							QByteArray memData = QByteArray::fromRawData( reinterpret_cast< const char* >( memInfo->mem->constData() ), lData.size );
							QDataStream s( memData );
							if ( ( s >> *data ).status() == QDataStream::Ok )
								memInfo->seqNum = lData.seqNum;
							else
								errorCode = WrongDataFormat;
							memInfo->mem->unlock();
						}
					}
				}
				else
				{	// topic data have been received via a remote connection.
					QDataStream s( value );
					s.setVersion( QDATASTREAM_VERSION );
					if ( ( s >> *data ).status() != QDataStream::Ok )
						errorCode = WrongDataFormat;
				}
				Q_EMIT listenTopic( listener, data, sourceAddress, sourcePort, errorCode );
			}
			else
			{
				// If necessary, delete the listener. This must be done here if it has not been invoked.
				if( listener->deleteLater )
					delete listener;
				else
					listener->active = false;
			}
		}
	}
}

void Client::registerNotifier( const QString& name, rec::rpc::NotifierBase* notifier )
{
	if ( _notifiers.contains( name ) )
	{
		delete _notifiers.value( name );
		_notifiers[ name ] = notifier;
	}
	else
	{
		_notifiers.insert( name, notifier );
	}
}

void Client::unregisterNotifier( const QString& name )
{
	delete _notifiers.value( name );
	_notifiers.remove( name );
}

void Client::registerTopicListener( const QString& name, rec::rpc::TopicListenerBase* listener )
{
	{
		QMutexLocker lk( &_topicListenersMutex );
		if ( _topicListeners.contains( name ) )
		{
			TopicListenerBase* oldListener = _topicListeners.value( name );
			// Special care has to be taken in case that the old listener is active.
			// If we delete it when it is active, the server will crash.
			// Instead, we set deleteLater true to delete is as soon as it has finished its work.
			if ( oldListener->active )
				oldListener->deleteLater = true;
			else
				delete oldListener;
			_topicListeners[ name ] = listener;
		}
		else
		{
			_topicListeners.insert( name, listener );
			if ( isConnected() )
				manageTopicListener( name, true );
		}
	}
	qApp->processEvents();
}

void Client::unregisterTopicListener( const QString& name )
{
	{
		QMutexLocker lk( &_topicListenersMutex );
		if ( _topicListeners.contains( name ) )
		{
			TopicListenerBase* listener = _topicListeners.value( name );
			// Special care has to be taken in case that the listener is active.
			// If we delete it when it is active, the server will crash.
			// Instead, we set deleteLater true to delete is as soon as it has finished its work.
			// But things like that can only happen if we unregister a topic and, afterwards, the
			// corresponding info channel at once.
			if ( listener->active )
				listener->deleteLater = true;
			else
				delete listener;
			_topicListeners.remove( name );
			if ( isConnected() )
				manageTopicListener( name, false );
		}
	}
	qApp->processEvents();
}

void Client::manageTopicListener( const QString& name, bool add )
{
	QString localName = rec::rpc::Common::getLocalName( name );
	if ( add )
	{
		if ( !_configuration.contains( name ) )
		{
			throw Exception( NoSuchTopic );
		}
		QByteArray message;
		if ( ( _socket->peerAddress() == QHostAddress::LocalHost || _socket->peerAddress() == QHostAddress::LocalHostIPv6 ) && _configuration.contains( localName ) )
        {	// If Client is running on the same host as the server, use a local connection (with shared memory).
			message = rec::rpc::messages::RegisterChannel::encode( localName );
		}
		else
		{
			message = rec::rpc::messages::RegisterChannel::encode( name );
		}
		qApp->postEvent( _socket, new RegisterChannelEvent( message ) );
	}
	else
	{
		QByteArray message;
		if ( _topicLocalMemories.contains( name ) )
		{
			_topicLocalMemories.remove( name );
			message = rec::rpc::messages::UnregisterChannel::encode( localName );
		}
		else
		{
			message = rec::rpc::messages::UnregisterChannel::encode( name );
		}
		qApp->postEvent( _socket, new UnregisterChannelEvent( message ) );
	}
}

void Client::invoke( const QString& name, rec::rpc::serialization::SerializablePtrConst param, rec::rpc::serialization::SerializablePtr result, bool blocking )
{
	if ( !param || !result )
		return;
	if ( isConnected() )
	{
		// serialize parameters and add a PendingRequest record for response and timeout handling.
		quint32 seqNum = _seqNum;
		QByteArray serParam;
		QDataStream s( &serParam, QIODevice::WriteOnly );
		s.setVersion( QDATASTREAM_VERSION );
		s << *param;
        PendingRequests* req = new PendingRequests( name, seqNum, blocking, result, _timeout );
        req->moveToThread( this ); // The timeout timer must run in the Client thread so let's make that sure.
		{
			QMutexLocker l( &_pendingRequestsMutex );
			_pendingRequests.insert( seqNum, req );
			// req will be removed from _pendingRequests when the response from the server arrives or the timeout is triggered.
		}

        connect( req, SIGNAL( timeout( PendingRequests* ) ), SLOT( on_pendingRequest_timeout( PendingRequests* ) ), Qt::DirectConnection );

		req->mutex.lock();
		req->start();
		{	// encode request message and send it.
			QByteArray messageData = rec::rpc::messages::RPCRequest::encode( name, seqNum, serParam );

			QMutexLocker lk( &_socketMutex );
			if( _socket )
			{
				qApp->postEvent( _socket, new SendRPCRequestEvent( messageData ) );
			}
		}
		if ( blocking )
		{	// Blocking call. Wait for response or timeout.
			ErrorCode errorCode = UnknownError;
			req->cond.wait( &req->mutex );

			// Response has been received or timeout error has occurred. Return values have already been deserialized.
			errorCode = req->errorCode;
			req->mutex.unlock();
			req->deleteLater();
			if ( errorCode != NoError )
			{
				throw Exception( errorCode );
			}
		}
		else
			req->mutex.unlock();
	}
	else
	{
		throw Exception( NoConnection );
	}
}

void Client::publishTopic( const QString& name, rec::rpc::serialization::SerializablePtrConst data )
{
	if ( !data )
		return;
	if ( isConnected() )
	{
		if ( _configuration.contains( name ) )
		{
			if ( _configuration.itemContainer().value( name ).serverOnly )
			{
				throw Exception( AccessDenied );
			}
			QByteArray serData;
			QDataStream s( &serData, QIODevice::WriteOnly );
			s.setVersion( QDATASTREAM_VERSION );
			s << *data;

			// First try to publish the topic data via shared memory.
			// If this doesn't work (because the topic doesn't support shared memory or
			// the server is running on another machine), use the network connection.
			if ( !publishTopicLocal( name, serData ) )
				publishTopicRemote( name, serData );
		}
		else
		{
			throw Exception( NoSuchTopic );
		}
	}
	else
	{
		throw Exception( NoConnection );
	}
}

bool Client::publishTopicLocal( const QString& name, const QByteArray& serData )
{
	if ( _topicLocalMemories.contains( name ) )
	{
		// If there is a shared memory segment for the topic and it is big enough, use it
		// to publish the new data.
		SharedMemInfo& memInfo = _topicLocalMemories[ name ];
		if ( memInfo.mem && memInfo.mem->size() >= serData.size() )
		{
			QByteArray serLocalData;
			{
				QDataStream s( &serLocalData, QIODevice::WriteOnly );
				s.setVersion( QDATASTREAM_VERSION );
				serialization::LocalTopic data( true, ++memInfo.seqNum, memInfo.mem->key(), serData.size() );
				s << data;
			}

			// access shared memory segment and copy the data into it.
			memInfo.mem->lock();
			memcpy( memInfo.mem->data(), serData.constData(), serData.size() );
			memInfo.mem->unlock();

			// send the notification (with sequence number, memory segment name and data size) to the server via network.
			publishTopicRemote( rec::rpc::Common::getLocalName( name ), serLocalData );
			return true;
		}
	}
	return false;
}

void Client::publishTopicRemote( const QString& name, const QByteArray& serData )
{	// send the data via network connection.
	QByteArray messageData = rec::rpc::messages::Topic::encode( name, QHostAddress::Null, 0, serData ); // the server will add address and port.

	QMutexLocker lk( &_socketMutex );
	if( _socket )
	{
		qApp->postEvent( _socket, new PublishTopicEvent( messageData ) );
	}
}

void Client::startAutoReconnect()
{
	if( _msAutoReconnect > 0 )
	{
		if( _msAutoReconnect < 100 )
		{
			_msAutoReconnect = 100;
		}
		_autoReconnectTimer->setInterval( _msAutoReconnect );
		_autoReconnectTimer->start();
	}
}

void Client::on_rpcResponseReceived( const QString& name, quint32 seqNum, quint16 errorCode, const QByteArray& serResult )
{
    PendingRequests* req = 0;
	{ // Get the registered pending request and remove it from the buffer
		QMutexLocker l( &_pendingRequestsMutex );
		if( !_pendingRequests.contains( seqNum ) )
			return;
		req = _pendingRequests.value( seqNum );
		_pendingRequests.remove( seqNum );
	}
	serialization::SerializablePtr result;
	{
		req->mutex.lock();
		req->stop(); // Stop the timeout timer
		QDataStream s( serResult ); // Deserialize the return values.
		s.setVersion( QDATASTREAM_VERSION );
		if ( ( s >> *req->result ).status() != QDataStream::Ok )
		{
			if ( errorCode == NoError )
				errorCode = WrongDataFormat;
		}
		if ( req->blocking )
		{	// Blocking call. Wake up the invoker thread.
			req->errorCode = static_cast< ErrorCode >( errorCode );
			req->cond.wakeAll();
			req->mutex.unlock();
			return;
		}
		result = req->result;
		req->mutex.unlock();
		req->deleteLater();
	}
	// Call was non-blocking --> call the notifier (if one exists)
	// and (important!) delete the return value (result).
	if ( _notifiers.contains( name ) )
	{
		rec::rpc::NotifierBase* notifier = _notifiers.value( name );
		assert( notifier );
		// Notifier is called in the main thread.
		Q_EMIT notify( notifier, result, static_cast< ErrorCode >( errorCode ) );
	}
}

void Client::on_pendingRequest_timeout( PendingRequests* req )
{	// timeout error. No response has been received within the given timeout interval.
	req->mutex.lock();
	{
		QMutexLocker l( &_pendingRequestsMutex );
		if( !_pendingRequests.contains( req->seqNum ) )
		{	// Kind if error: the request that triggered the timeout is no longer registered
			// --> ignore it, clean up (and if it is non-blocking delete the return value).
			req->mutex.unlock();
			req->deleteLater();
			return;
		}

		assert( req == _pendingRequests.value( req->seqNum ) ); // DEBUG
		_pendingRequests.remove( req->seqNum );
	}
	if ( req->blocking )
	{	// blocking call
		req->errorCode = ExecutionTimeout;
		req->cond.wakeAll();
		req->mutex.unlock();
		return; // req will be deleted in the invoke() method.
	}
	// Call was non-blocking --> call the notifier (if one exists)
	// and (important!) delete the return value (result).
	if ( _notifiers.contains( req->funcName ) )
	{
		rec::rpc::NotifierBase* notifier = _notifiers.value( req->funcName );
		assert( notifier );
		// Notifier is called in the main thread.
		Q_EMIT notify( notifier, req->result, ExecutionTimeout );
	}
	req->mutex.unlock();
	req->deleteLater();
}

void Client::on_notify( rec::rpc::NotifierBase* notifier, rec::rpc::serialization::SerializablePtrConst result, rec::rpc::ErrorCode errorCode )
{	// Asynchroneous notification in the main thread
	try
	{
		notifier->notify( *result, errorCode );
	}
	catch( ... )
	{
	}
}

void Client::on_listenTopic( rec::rpc::TopicListenerBase* listener, rec::rpc::serialization::SerializablePtrConst data, const QHostAddress& address, quint16 port, rec::rpc::ErrorCode errorCode )
{	// Asynchroneous topic listener invokation in the main thread
	try
	{
		listener->listen( *data, address, port, errorCode );
	}
	catch( ... )
	{
	}
	if ( listener->deleteLater )
		delete listener;
	else
		listener->active = false;
}

void Client::on_autoReconnectTimer_timeout()
{
	connectToServer( 1000 );
}

void Client::getServerVersion( int* major, int* minor, int* patch, int* date, QString* suffix )
{
	serialization::VersionInfoPtr v( new serialization::VersionInfo );
	invoke( rec::rpc::Common::versionRequest, serialization::SerializablePtr( new serialization::Serializable ), v, true );
	if ( major )
		*major = v->majorVer;
	if ( minor )
		*minor = v->minorVer;
	if ( patch )
		*patch = v->patchVer;
	if ( date )
		*date = v->buildVer;
	if ( suffix )
		*suffix = v->suffix;
}

