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

#include "rec/rpc/Server.h"
#include "rec/rpc/server/Server.hpp"

#include <cassert>

using namespace rec::rpc;

Server::Server()
: _server( 0 )
{
    _server = new server::Server;

    //bool ok = true;

     connect( _server, SIGNAL( listening() ), SIGNAL( listening() ), Qt::QueuedConnection );
     connect( _server, SIGNAL( closed() ), SIGNAL( closed() ), Qt::QueuedConnection );
     connect( _server
		, SIGNAL( clientConnected( const QHostAddress&, unsigned short ) )
		, SIGNAL( clientConnected( const QHostAddress&, unsigned short ) )
		, Qt::QueuedConnection );
     connect( _server
		, SIGNAL( clientDisconnected( const QHostAddress&, unsigned short ) )
		, SIGNAL( clientDisconnected( const QHostAddress&, unsigned short ) )
		, Qt::QueuedConnection );
     connect( _server
		, SIGNAL( serverError( QAbstractSocket::SocketError, const QString& ) )
		, SIGNAL( serverError( QAbstractSocket::SocketError, const QString& ) )
		, Qt::QueuedConnection );
     connect( _server
		, SIGNAL( clientError( QAbstractSocket::SocketError, const QString& ) )
		, SIGNAL( clientError( QAbstractSocket::SocketError, const QString& ) )
		, Qt::QueuedConnection );
     connect( _server
		, SIGNAL( registeredTopicListener( const QString&, const QHostAddress&, unsigned short ) )
		, SIGNAL( registeredTopicListener( const QString&, const QHostAddress&, unsigned short ) )
		, Qt::QueuedConnection );
     connect( _server
		, SIGNAL( unregisteredTopicListener( const QString&, const QHostAddress&, unsigned short ) )
		, SIGNAL( unregisteredTopicListener( const QString&, const QHostAddress&, unsigned short ) )
		, Qt::QueuedConnection );

     connect( qApp, SIGNAL( aboutToQuit() ), SLOT( close() ) );

    //assert( ok );
}

Server::~Server()
{
	delete _server;
}

void Server::listen( int port )
{
	_server->listen( port );
}

void Server::close()
{
	_server->close();
}

bool Server::isListening() const
{
	return _server->isListening();
}

unsigned short Server::serverPort() const
{
	return _server->serverPort();
}

QString Server::greeting() const
{
	return _server->greeting();
}

void Server::setGreeting( const QString& greeting )
{
	_server->setGreeting( greeting );
}

void Server::publishTopic( const QString& name, serialization::SerializablePtrConst data )
{
	_server->publishTopic( name, data );
}

void Server::addTopic( const QString& name, int sharedMemorySize )
{
	_server->addTopic( name, sharedMemorySize );
}

void Server::registerTopicListener( const QString& name, TopicListenerBase* listener )
{
	_server->registerTopicListener( name, listener );
}

void Server::unregisterTopicListener( const QString& name )
{
	_server->unregisterTopicListener( name );
}

void Server::registerFunction( const QString& name, RPCFunctionBase* function )
{
	_server->registerFunction( name, function );
}

void Server::unregisterFunction( const QString& name )
{
	_server->unregisterFunction( name );
}
