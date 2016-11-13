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

#include "rec/rpc/server/Server.hpp"
#include "rec/rpc/server/Impl.hpp"
#include "rec/rpc/server/ServerThread.hpp"

#include "rec/rpc/messages/Configurations.h"
#include "rec/rpc/messages/RPCResponse.h"
#include "rec/rpc/messages/Topic.h"

#include "rec/rpc/serialization/Primitive.h"
#include "rec/rpc/serialization/LocalTopic.hpp"
#include "rec/rpc/serialization/VersionInfo.hpp"

#include "rec/rpc/Exception.h"

#include "rec/rpc/common_internal.hpp"

#include "rec/rpc/rec_rpc_version.h"

#include <QMetaType>
#include <cassert>

//Q_DECLARE_METATYPE( QAbstractSocket::SocketState )
//Q_DECLARE_METATYPE( QAbstractSocket::SocketError )
//Q_DECLARE_METATYPE( QHostAddress )

using namespace rec::rpc::server;

bool Server::_once = false;

const QString Server::_infoSuffix( "__info" );

inline QString Server::getInfoName( const QString& name )
{
	return name + _infoSuffix;
}

inline QString Server::getMemName( const QString& name, unsigned int suffix ) const
{
	return QString( "REC_RPC_%1_%2_%3" ).arg( _impl->serverPort() ).arg( name ).arg( suffix );
}

Server::Server( QObject* parent )
: QThread( parent )
, _run( true )
, _impl( NULL )
, _greeting( QString( "REC RPC Server %1" ).arg( VersionString ) )
{
	//if( NULL == QCoreApplication::instance() )
	//{
	//	static int argc = 1;
	//	static char ab[4] = { 'a', 0, 'b', 0 };
	//	static char* argv[2] = { ab, ab + 2 };
	//	new QCoreApplication( argc, argv );
	//}

	setObjectName( "rec::rpc::server::Server" );

	if ( !_once )
	{
		_once = true;
//		qRegisterMetaType< QAbstractSocket::SocketState >();
//		qRegisterMetaType< QAbstractSocket::SocketError >();

        qRegisterMetaType< QAbstractSocket::SocketState >("QAbstractSocket::SocketState");
        qRegisterMetaType< QAbstractSocket::SocketError >("QAbstractSocket::SocketError");

		qRegisterMetaType< rec::rpc::configuration::Configuration >();
		qRegisterMetaType< quintptr >( "quintptr" );
//		qRegisterMetaType< QHostAddress >();
        qRegisterMetaType< QHostAddress >("QHostAddress ");
		qRegisterMetaType< rec::rpc::ErrorCode >();
		qRegisterMetaType< rec::rpc::serialization::SerializablePtr >();
		qRegisterMetaType< rec::rpc::serialization::SerializablePtrConst >();
	}

    //bool ok = true;
     connect( this
		, SIGNAL( invoke( rec::rpc::RPCFunctionBase*, rec::rpc::serialization::SerializablePtrConst, rec::rpc::serialization::SerializablePtr, const QHostAddress&, quint16, rec::rpc::ErrorCode* ) )
		, SLOT( on_invoke( rec::rpc::RPCFunctionBase*, rec::rpc::serialization::SerializablePtrConst, rec::rpc::serialization::SerializablePtr, const QHostAddress&, quint16, rec::rpc::ErrorCode* ) )
		, Qt::BlockingQueuedConnection );

    connect( this
		, SIGNAL( listenTopic( rec::rpc::TopicListenerBase*, rec::rpc::serialization::SerializablePtrConst, const QHostAddress&, quint16, rec::rpc::ErrorCode ) )
		, SLOT( on_listenTopic( rec::rpc::TopicListenerBase*, rec::rpc::serialization::SerializablePtrConst, const QHostAddress&, quint16, rec::rpc::ErrorCode ) )
		, Qt::QueuedConnection );
    //assert( ok );

	start();
	_startSemaphore.acquire();
}

Server::~Server()
{
	_run = false;
	exit();
	wait();
	qDeleteAll( _rpcFunctions );
	qDeleteAll( _topicListeners );
	_rpcFunctions.clear();
	_topicListeners.clear();
	_topicLocalMemories.clear();
}

bool Server::listen( int port, bool blocking )
{
	QMutexLocker lk( &_listenMutex );
	if( NULL == _impl )
	{
		return false;
	}

	qApp->postEvent( _impl, new ListenEvent( port ) );

	if( blocking )
	{
		_listenCondition.wait( &_listenMutex );
		return _impl->isListening();
	}
	else
	{
		return true;
	}
}

bool Server::isListening() const
{
	QMutexLocker lk( &_listenMutex );

	if( NULL == _impl )
	{
		return false;
	}

	return _impl->isListening();
}

quint16 Server::serverPort() const
{
	QMutexLocker lk( &_listenMutex );

	if( NULL == _impl )
	{
		return 0;
	}

	return _impl->serverPort();
}

void Server::close()
{
	// Remove shared memory segments to save memory when the server is not running.
	for( QMap< QString, SharedMemInfo >::iterator iter = _topicLocalMemories.begin();
		iter != _topicLocalMemories.end(); ++iter )
	{
		iter->mem = QSharedPointer< QSharedMemory >();
	}

	QMutexLocker lk( &_listenMutex );

	if( NULL == _impl )
	{
		return;
	}

	qApp->postEvent( _impl, new CloseEvent );
	_listenCondition.wait( &_listenMutex );
}

void Server::disconnectAllClients()
{
	QMutexLocker lk( &_listenMutex );

	if( NULL == _impl )
	{
		return;
	}

	qApp->postEvent( _impl, new DisconnectAllClients );
}

void Server::run()
{
	setupServer();
	_startSemaphore.release();

	while( _run )
	{
		exec();

		Q_EMIT closed();

		setupServer();

		_listenCondition.wakeAll();
	}
}

void Server::setupServer()
{
	QMutexLocker lk( &_listenMutex );

	delete _impl;
	_impl = NULL;

	if( false == _run )
	{
		return;
	}

	_impl = new rec::rpc::server::Impl( &_configuration, _greeting );

    //bool ok = true;

     connect( _impl,
		SIGNAL( clientConnected( const QHostAddress&, quint16 ) ),
		SLOT( on_clientConnected( const QHostAddress&, quint16 ) ),
		Qt::DirectConnection );

     connect( _impl,
		SIGNAL( clientDisconnected( const QHostAddress&, quint16, const QStringList& ) ),
		SLOT( on_clientDisconnected( const QHostAddress&, quint16, const QStringList& ) ),
		Qt::DirectConnection );

     connect( _impl,
		SIGNAL( serverError( QAbstractSocket::SocketError, const QString& ) ),
		SLOT( on_serverError( QAbstractSocket::SocketError, const QString& ) ),
		Qt::DirectConnection );

     connect( _impl,
		SIGNAL( clientError( QAbstractSocket::SocketError, const QString& ) ),
		SLOT( on_clientError( QAbstractSocket::SocketError, const QString& ) ),
		Qt::DirectConnection );

     connect( _impl,
		SIGNAL( listening() ),
		SLOT( on_listening() ),
		Qt::DirectConnection );

     connect( _impl,
		SIGNAL( closed() ),
		SLOT( on_closed() ),
		Qt::DirectConnection );

     connect( _impl,
		SIGNAL( rpcRequestReceived( const QString&, quint32, const QByteArray&, quintptr, const QHostAddress&, quint16 ) ),
		SLOT( on_rpcRequestReceived( const QString&, quint32, const QByteArray&, quintptr, const QHostAddress&, quint16 ) ),
		Qt::DirectConnection );

     connect( _impl,
		SIGNAL( topicReceived( const QString&, const QHostAddress&, quint16, const QByteArray& ) ),
		SLOT( on_topicReceived( const QString&, const QHostAddress&, quint16, const QByteArray& ) ),
		Qt::DirectConnection );

     connect( _impl,
		SIGNAL( channelRegistered( const QString&, const QHostAddress&, quint16 ) ),
		SLOT( on_channelRegistered( const QString&, const QHostAddress&, quint16 ) ),
		Qt::DirectConnection );

    connect( _impl,
		SIGNAL( channelUnregistered( const QString&, const QHostAddress&, quint16 ) ),
		SLOT( on_channelUnregistered( const QString&, const QHostAddress&, quint16 ) ),
		Qt::DirectConnection );

    //assert( ok );
}

void Server::publishTopic( const QString& name, rec::rpc::serialization::SerializablePtrConst data )
{
	if ( !isListening() )
		return;
	{
		rec::rpc::configuration::ConfigurationLocker lk( _configuration );
		if ( !_configuration.contains( name ) )
		{
			throw Exception( NoSuchTopic );
		}
	}

	// Serialize the data
	QByteArray serData;
	QDataStream s( &serData, QIODevice::WriteOnly );
	s.setVersion( QDATASTREAM_VERSION );
	s << *data;

	// Publish the topic data to local clients via shared memory.
	publishTopicLocal( name, QHostAddress::Null, 0, serData );
	// Publish the topic data to remote clients via network.
	publishTopicRemote( name, QHostAddress::Null, 0, serData );

	// Call local topic listener (if present)
	rec::rpc::TopicListenerBase* listener = 0;
	QMutexLocker lk( &_topicListenersMutex );
	if ( _topicListeners.contains( name ) )
	{
		listener = _topicListeners.value( name );
		assert( listener );
		listener->active = true;
	}
	if ( listener )
	{
		if ( data )
		{
			Q_EMIT listenTopic( listener, data, QHostAddress::Null, 0, NoError );
		}
		else
		{
			if ( listener->deleteLater )
				delete listener;
			else
				listener->active = false;
		}
	}
}

void Server::publishTopicRemote( const QString& name, const QHostAddress& sourceAddress, quint16 sourcePort, const QByteArray& serData )
{
	if ( !isListening() )
		return;
	{
		rec::rpc::configuration::ConfigurationLocker lk( _configuration );
		_configuration.setData( name, serData, sourceAddress, sourcePort );
	}

	{
		// Encode message and send it to the clients.
		QByteArray messageData = rec::rpc::messages::Topic::encode( name, sourceAddress, sourcePort, serData );
		QMutexLocker lk( &_listenMutex );
		_impl->publishTopic( name, messageData );
	}
}

void Server::publishTopicLocal( const QString& name, const QHostAddress& sourceAddress, quint16 sourcePort, const QByteArray& serData )
{
	if ( !isListening() )
		return;
	if( _topicLocalMemories.contains( name ) )
	{
		SharedMemInfo& memInfo = _topicLocalMemories[ name ];
		if ( !memInfo.mem || memInfo.mem->size() < serData.size() )
		{
			// If there is no memory segment or the existing one is too small, create a new one which is big enough.
			// Size is always a multiple of 4096 bytes.
			int newSize = serData.size();
			{
				int r = newSize % 4096;
				if ( r != 0 )
				{
					newSize += 4096 - r;
				}
			}
			assert( newSize >= serData.size() );
			QSharedPointer< QSharedMemory > mem( new QSharedMemory( getMemName( name, memInfo.keySuffix ) ) );
			if ( mem->create( newSize ) )
			{
				memInfo.mem = mem;
			}
			++memInfo.keySuffix;
		}
		if ( memInfo.mem && memInfo.mem->size() >= serData.size() )
		{
			// Create a message with all necessary info about the shared memory segment to notify
			// local clients that data has changed.
			QByteArray serLocalData;
			{
				QDataStream s( &serLocalData, QIODevice::WriteOnly );
				s.setVersion( QDATASTREAM_VERSION );
				serialization::LocalTopic localData( true, ++memInfo.seqNum, memInfo.mem->key(), serData.size() );
				s << localData;
			}

			// Copy data to the shared memory
			memInfo.mem->lock();
			memcpy( memInfo.mem->data(), serData.constData(), serData.size() );
			memInfo.mem->unlock();

			// Send LocalTopic message.
			publishTopicRemote( rec::rpc::Common::getLocalName( name ), sourceAddress, sourcePort, serLocalData );
		}
	}
}

int Server::numClientsConnected() const
{
	if( NULL == _impl )
	{
		return 0;
	}

	return _impl->numClientsConnected();
}

void Server::on_clientConnected( const QHostAddress& address, quint16 port )
{
	Q_EMIT clientConnected( address, port );
}

void Server::on_clientDisconnected( const QHostAddress& address, quint16 port, const QStringList& registeredChannels )
{
	Q_FOREACH( const QString& ch, registeredChannels )
	{
		QString topicName = ch;
		if( topicName.endsWith( rec::rpc::Common::localSuffix ) )
			topicName.chop( rec::rpc::Common::localSuffix.length() );
		if ( !ch.endsWith( _infoSuffix ) && _configuration.contains( ch ) && _configuration.contains( topicName ) )
		{	// Update the list of connected clients.
			configuration::ClientContainer clients = _configuration.removeRegisteredClient( topicName, address, port );
			rec::rpc::serialization::PrimitivePtr< configuration::ClientContainer >::Type serClients(
				new rec::rpc::serialization::Primitive< configuration::ClientContainer >( clients ) );
			publishTopic( getInfoName( topicName ), serClients );
		}
	}
	Q_EMIT clientDisconnected( address, port );
}

void Server::on_channelRegistered( const QString& name, const QHostAddress& address, quint16 port )
{
	QString topicName = name;
	if( topicName.endsWith( rec::rpc::Common::localSuffix ) )
		topicName.chop( rec::rpc::Common::localSuffix.length() );
	if ( !name.endsWith( _infoSuffix ) && _configuration.contains( name ) && _configuration.contains( topicName ) )
	{	// Update the list of connected clients.
		configuration::ClientContainer clients = _configuration.addRegisteredClient( topicName, address, port );
		rec::rpc::serialization::PrimitivePtr< configuration::ClientContainer >::Type serClients(
			new rec::rpc::serialization::Primitive< configuration::ClientContainer >( clients ) );
		publishTopic( getInfoName( topicName ), serClients );
	}
	Q_EMIT registeredTopicListener( name, address, port );
}

void Server::on_channelUnregistered( const QString& name, const QHostAddress& address, quint16 port )
{
	QString topicName = name;
	if( topicName.endsWith( rec::rpc::Common::localSuffix ) )
		topicName.chop( rec::rpc::Common::localSuffix.length() );
	if ( !name.endsWith( _infoSuffix ) && _configuration.contains( name ) && _configuration.contains( topicName ) )
	{	// Update the list of connected clients.
		configuration::ClientContainer clients = _configuration.removeRegisteredClient( topicName, address, port );
		rec::rpc::serialization::PrimitivePtr< configuration::ClientContainer >::Type serClients(
			new rec::rpc::serialization::Primitive< configuration::ClientContainer >( clients ) );
		publishTopic( getInfoName( topicName ), serClients );
	}
	Q_EMIT unregisteredTopicListener( name, address, port );
}

void Server::on_serverError( QAbstractSocket::SocketError err, const QString& errorString )
{
	Q_EMIT serverError( err, errorString );
	quit();
}

void Server::on_clientError( QAbstractSocket::SocketError err, const QString& errorString )
{
	Q_EMIT clientError( err, errorString );
}

void Server::on_listening()
{
	{
		QMutexLocker lk( &_listenMutex );
		_listenCondition.wakeAll();
	}

	// Create a shared memory segment with a unique name and initialize the <name>__local data for each topic that supports shared memory.
	for( QMap< QString, SharedMemInfo >::iterator iter = _topicLocalMemories.begin();
		iter != _topicLocalMemories.end(); ++iter )
	{
		iter->mem = QSharedPointer< QSharedMemory >( new QSharedMemory( getMemName( iter.key(), iter->keySuffix ) ) );
		++iter->keySuffix;
		iter->mem->create( iter->initialSize );

		QSharedPointer< rec::rpc::serialization::LocalTopic > localTopic(
			new rec::rpc::serialization::LocalTopic( false, 0xFFFFFFFF, iter->mem->key(), 0 ) ); // uninitialized
		publishTopic( rec::rpc::Common::getLocalName( iter.key() ), localTopic );
	}

	Q_EMIT listening();
}

void Server::on_closed()
{
	quit();
}

void Server::registerTopicListener( const QString& name, rec::rpc::TopicListenerBase* listener )
{
	if( _configuration.contains( name ) )
	{
		// Register a local topic listener (yes, the server can also handle topics :-))
		if ( !name.endsWith( _infoSuffix ) )
		{
			configuration::ClientContainer clients = _configuration.addRegisteredClient( name, QHostAddress::Null, 0 );
			rec::rpc::serialization::PrimitivePtr< configuration::ClientContainer >::Type serClients(
				new rec::rpc::serialization::Primitive< configuration::ClientContainer >( clients ) );
			publishTopic( getInfoName( name ), serClients );
		}
		{
			QMutexLocker lk( &_topicListenersMutex );
			if ( _topicListeners.contains( name ) )
			{
				TopicListenerBase* oldListener = _topicListeners.value( name );
				_topicListeners.remove( name );
				// Special care has to be taken in case that the old listener is active.
				// If we delete it when it is active, the server will crash.
				// Instead, we set deleteLater true to delete is as soon as it has finished its work.
				if ( oldListener->active )
					oldListener->deleteLater = true;
				else
					delete oldListener;
			}
			_topicListeners.insert( name, listener );
		}
		if ( _configuration.isInitialized( name ) && !name.endsWith( _infoSuffix ) )
		{	// If topic data is value, invoke the local topic listener
			// (clients' topic listeners will also be invoked)
			QHostAddress address;
			quint16 port;
			QByteArray topicData = _configuration.data( name, &address, &port );
			on_topicReceived( name, address, port, topicData );
		}
		Q_EMIT registeredTopicListener( name, QHostAddress::Null, 0 );
		qApp->processEvents();
	}
}

void Server::unregisterTopicListener( const QString& name )
{
	if( _configuration.contains( name ) )
	{
		if ( !name.endsWith( _infoSuffix ) && _configuration.contains( name ) )
		{
			configuration::ClientContainer clients = _configuration.removeRegisteredClient( name, QHostAddress::Null, 0 );
			rec::rpc::serialization::PrimitivePtr< configuration::ClientContainer >::Type serClients(
				new rec::rpc::serialization::Primitive< configuration::ClientContainer >( clients ) );
			publishTopic( getInfoName( name ), serClients );
		}
		{
			QMutexLocker lk( &_topicListenersMutex );
			TopicListenerBase* listener = _topicListeners.value( name );
			_topicListeners.remove( name );
			// Special care has to be taken in case that the listener is active.
			// If we delete it when it is active, the server will crash.
			// Instead, we set deleteLater true to delete is as soon as it has finished its work.
			// But things like that can only happen if we unregister a topic and, afterwards, the
			// corresponding info channel at once.
			if ( listener->active )
				listener->deleteLater = true;
			else
				delete listener;
		}
		Q_EMIT unregisteredTopicListener( name, QHostAddress::Null, 0 );
		qApp->processEvents();
	}
}

void Server::registerFunction( const QString& name, rec::rpc::RPCFunctionBase* function )
{
	if( name.contains( "__" ) )
		throw Exception( ImproperFunctionName );
	if ( _rpcFunctions.contains( name ) )
	{
		delete _rpcFunctions.value( name );
		_rpcFunctions[ name ] = function;
	}
	else
	{
		_rpcFunctions.insert( name, function );
	}
}

void Server::unregisterFunction( const QString& name )
{
	delete _rpcFunctions.value( name );
	_rpcFunctions.remove( name );
}

void Server::on_rpcRequestReceived( const QString& name, quint32 seqNum, const QByteArray& serParam, quintptr receiver, const QHostAddress& address, quint16 port )
{
	ErrorCode errorCode = NoError;
	QByteArray serResult;
	if ( name == rec::rpc::Common::versionRequest )
	{
		QDataStream os( &serResult, QIODevice::WriteOnly );
		os.setVersion( QDATASTREAM_VERSION );
		serialization::VersionInfo v( MajorVer, MinorVer, PatchVer, BuildVer, Suffix );
		os << v;
	}
	else
	{
		if ( _rpcFunctions.contains( name ) )
		{
			// Get RPC function wrapper object
			rec::rpc::RPCFunctionBase* func = _rpcFunctions.value( name );
			assert( func );
			serialization::SerializablePtr param = func->createParam();
			serialization::SerializablePtr result = func->createResult();
			assert( param && result );

			// Deserialize parameters
			QDataStream is( serParam );
			is.setVersion( QDATASTREAM_VERSION );
			if ( ( is >> *param ).status() == QDataStream::Ok )
			{
				// invoke function. It will be executed in the main thread.
				// This thread will sleep until the function execution has finished.
				Q_EMIT invoke( func, param, result, address, port, &errorCode );
				QDataStream os( &serResult, QIODevice::WriteOnly );
				os.setVersion( QDATASTREAM_VERSION );
				os << *result;
			}
			else
			{
				errorCode = WrongDataFormat;
			}
		}
		else
		{
			errorCode = UnknownFunction;
		}
	}

	// Now encode the response message (return values) and send it to the client.
	QByteArray messageData = rec::rpc::messages::RPCResponse::encode( name, seqNum, errorCode, serResult );

	if( _impl )
	{
		QMutexLocker lk( &_listenMutex );
		_impl->sendRPCResponse( messageData, reinterpret_cast< rec::rpc::server::ServerThread* >( receiver ) );
	}
}

void Server::on_topicReceived( const QString& name, const QHostAddress& sourceAddress, quint16 sourcePort, const QByteArray& serData )
{
	ErrorCode errorCode = NoError;
	QByteArray mySerData;

	QString topicName = name;
	bool local = false;
	if ( name.endsWith( rec::rpc::Common::localSuffix ) )
	{	// A local client has published the data. Let's read from shared memory.
		topicName.chop( rec::rpc::Common::localSuffix.length() );
		local = true;
		serialization::LocalTopic lData;
		{
			QDataStream s( serData );
			s.setVersion( QDATASTREAM_VERSION );
			if ( ( s >> lData ).status() != QDataStream::Ok )
				errorCode = WrongDataFormat;
		}
		if ( lData.segmentName.isEmpty() )
			errorCode = WrongDataFormat;

		// Now we know sequence number, the name of the shared memory segment and the size of the serialized data
		if ( errorCode == NoError )
		{
			if ( _topicLocalMemories.contains( topicName ) )
			{
				SharedMemInfo& memInfo = _topicLocalMemories[ topicName ];
				if ( !memInfo.mem  )
					errorCode = WrongDataFormat;
				if ( errorCode == NoError )
				{
					// Now access the shared memory segment and copy the serialized data.
					memInfo.mem->lock();
					mySerData = QByteArray( reinterpret_cast< const char* >( memInfo.mem->constData() ), lData.size );
					memInfo.mem->unlock();
					memInfo.seqNum = lData.seqNum;
				}
			}
			else
				errorCode = UnknownError;
		}
		if ( errorCode == NoError ) // publish the new data also to the remote clients
			publishTopicRemote( topicName, sourceAddress, sourcePort, mySerData );
	}
	else
	{	// A remote client has published the data.
		mySerData = serData;
		// publish the new data also to the local clients
		publishTopicLocal( topicName, sourceAddress, sourcePort, mySerData );
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
	{	// A local topic listener has been registered. Deserialize the data and invoke the listener.
		rec::rpc::serialization::SerializablePtr data = listener->createData();

		if ( data )
		{
			QDataStream s( mySerData );
			s.setVersion( QDATASTREAM_VERSION );
			if ( errorCode == NoError )
			{
				if ( ( s >> *data ).status() != QDataStream::Ok )
					errorCode = WrongDataFormat;
			}
			Q_EMIT listenTopic( listener, data, sourceAddress, sourcePort, errorCode );
		}
		else
		{
			// If necessary, delete the listener.
			if ( listener->deleteLater )
				delete listener;
			else
				listener->active = false;
		}
	}
}

void Server::on_invoke( rec::rpc::RPCFunctionBase* func, rec::rpc::serialization::SerializablePtrConst param, rec::rpc::serialization::SerializablePtr result, const QHostAddress& address, quint16 port, rec::rpc::ErrorCode* errorCode )
{	// Asynchroneous RPC function invokation in the main thread
	try
	{
		func->invoke( *param, *result, address, port );
	}
	catch( const rec::rpc::Exception& e )
	{
		*errorCode = e.errorCode();
	}
	catch( ... )
	{
		*errorCode = rec::rpc::ExecutionCancelled;
	}
}

void Server::on_listenTopic( rec::rpc::TopicListenerBase* listener, rec::rpc::serialization::SerializablePtrConst data, const QHostAddress& address, quint16 port, rec::rpc::ErrorCode errorCode )
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

void Server::addTopic( const QString& name, int sharedMemorySize )
{
	if ( name.isEmpty() || name.contains( "__" ) )
		throw Exception( ImproperTopicName );
	if ( _configuration.contains( name ) )
		throw Exception( TopicAlreadyExists );

	// Add two channels: one called <name>, the other one called <name>__info which stores a list of clients that registered the topic.
	QString infoName = getInfoName( name );
	_configuration.addItem( name );
	_configuration.addItem( infoName, true );

	// Initialize the data of channel <name>__info with an empty list.
	rec::rpc::serialization::PrimitivePtr< rec::rpc::configuration::ClientContainer >::Type registeredClients(
		new rec::rpc::serialization::Primitive< rec::rpc::configuration::ClientContainer >( _configuration.registeredClients( name ) ) );
	publishTopic( infoName, registeredClients );

	if ( sharedMemorySize > 0 )
	{	// For local clients, create a shared memory segment for data exchange (size is always a multiple of 4096 bytes).
		SharedMemInfo& memInfo = _topicLocalMemories[ name ];
		memInfo.initialSize = sharedMemorySize;
		{
			int r = memInfo.initialSize % 4096;
			if ( r != 0 )
			{
				memInfo.initialSize += 4096 - r;
			}
		}
		assert( memInfo.initialSize >= sharedMemorySize );

		// Add a channel called <name>__local which is used to notify the clients that topic data have changed.
		QString localName = rec::rpc::Common::getLocalName( name );
		_configuration.addItem( localName );

		if ( isListening() )
		{	// If the server is listening, create a shared memory segment with a unique name and initialize the <name>__local data.
			// If the server is not listening, this step will be performed later.
			memInfo.mem = QSharedPointer< QSharedMemory >( new QSharedMemory( getMemName( name, memInfo.keySuffix ) ) );
			++memInfo.keySuffix;
			memInfo.mem->create( memInfo.initialSize );

			QSharedPointer< rec::rpc::serialization::LocalTopic > localTopic(
				new rec::rpc::serialization::LocalTopic( false, 0xFFFFFFFF, memInfo.mem->key(), 0 ) ); // uninitialized
			publishTopic( localName, localTopic );
		}
	}
}

QString Server::greeting() const
{
	return _greeting;
}

void Server::setGreeting( const QString& greeting )
{
	if ( greeting.trimmed().isEmpty() )
		return;
	_greeting = greeting;
	if ( _greeting.length() > rec::rpc::Common::maxGreetingLength )
		_greeting.resize( rec::rpc::Common::maxGreetingLength );
	if ( _impl )
		_impl->setGreeting( _greeting );
}
