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

#include "rec/rpc/Client.h"
#include "rec/rpc/client/Client.hpp"

#include <cassert>

using namespace rec::rpc;

Client::Client()
: _client( 0 )
{
    _client = new client::Client;

    //bool ok = true;
    connect( _client, SIGNAL( connected() ), SIGNAL( connected() ), Qt::QueuedConnection );
    connect( _client, SIGNAL( disconnected( rec::rpc::ErrorCode ) ), SIGNAL( disconnected( rec::rpc::ErrorCode ) ), Qt::QueuedConnection );
    connect( _client, SIGNAL( stateChanged( QAbstractSocket::SocketState ) ), SIGNAL( stateChanged( QAbstractSocket::SocketState ) ), Qt::QueuedConnection );
    connect( _client, SIGNAL( error( QAbstractSocket::SocketError, const QString& ) ), SIGNAL( error( QAbstractSocket::SocketError, const QString& ) ), Qt::QueuedConnection );
    //assert( ok );
}

Client::~Client()
{
	delete _client;
}

void Client::connectToServer( unsigned int msTimeout )
{
	_client->connectToServer( msTimeout );
}

void Client::setAutoReconnectEnabled( bool enable, unsigned int ms )
{
	_client->setAutoReconnectEnabled( enable, ms );
}

void Client::disconnectFromServer()
{
	_client->disconnectFromServer();
}

bool Client::isConnected() const
{
	return _client->isConnected();
}

QString Client::address() const
{
	return _client->address();
}

void Client::setAddress( const QString& address )
{
	_client->setAddress( address );
}

QString Client::expectedGreeting() const
{
	return _client->expectedGreeting();
}

void Client::setExpectedGreeting( const QString& greeting )
{
	_client->setExpectedGreeting( greeting );
}

unsigned int Client::msTimeout() const
{
	return _client->msTimeout();
}

void Client::setMsTimeout( unsigned int timeout )
{
	_client->setMsTimeout( timeout );
}

void Client::getServerVersion( int* major, int* minor, int* patch, int* date, QString* suffix )
{
	_client->getServerVersion( major, minor, patch, date, suffix );
}

void Client::invoke( const QString& name, serialization::SerializablePtrConst param, serialization::SerializablePtr result, bool blocking )
{
	_client->invoke( name, param, result, blocking );
}

void Client::publishTopic( const QString& name, serialization::SerializablePtrConst data )
{
	_client->publishTopic( name, data );
}

void Client::registerNotifier( const QString& name, NotifierBase* notifier )
{
	_client->registerNotifier( name, notifier );
}

void Client::unregisterNotifier( const QString& name )
{
	_client->unregisterNotifier( name );
}

void Client::registerTopicListener( const QString& name, TopicListenerBase* listener )
{
	_client->registerTopicListener( name, listener );
}

void Client::unregisterTopicListener( const QString& name )
{
	_client->unregisterTopicListener( name );
}
