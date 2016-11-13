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

#include "rec/rpc/configuration/Configuration.hpp"

using namespace rec::rpc::configuration;

Configuration::Configuration( QObject* parent )
: QObject( parent )
, _mutex( QMutex::Recursive )
{
}

Configuration::Configuration( const Configuration& other )
{
	QMutexLocker lk( &other._mutex );
	_itemFromName = other._itemFromName;
}

Configuration& Configuration::operator=( const Configuration& other )
{
	{
		QMutexLocker lk( &_mutex );
		QMutexLocker lk2( &other._mutex );
		_itemFromName = other._itemFromName;
	}

	Q_EMIT changed();
	return *this;
}

bool Configuration::isInitialized( const QString& name ) const
{
	QMutexLocker lk( &_mutex );

	if( false == _itemFromName.contains( name ) )
	{
		return false;
	}

	return _itemFromName[ name ].isInitialized();
}

QByteArray Configuration::data( const QString& name, QHostAddress* publisherAddress, quint16* publisherPort ) const
{
	QMutexLocker lk( &_mutex );

	if( false == _itemFromName.contains( name ) )
	{
		*publisherAddress = QHostAddress::Null;
		*publisherPort = 0;
		return QByteArray();
	}

	return _itemFromName[ name ].data( publisherAddress, publisherPort );
}

bool Configuration::isServerOnly( const QString& name ) const
{
	QMutexLocker lk( &_mutex );

	return _itemFromName[ name ].serverOnly;
}

bool Configuration::setData( const QString& name, const QByteArray& data, const QHostAddress& publisherAddress, quint16 publisherPort )
{
	QMutexLocker lk( &_mutex );

	if( false == _itemFromName.contains( name ) )
	{
		return false;
	}

	_itemFromName[ name ].setData( data, publisherAddress, publisherPort );

	return true;
}

ClientContainer Configuration::registeredClients( const QString& name ) const
{
	QMutexLocker lk( &_mutex );

	if ( false == _itemFromName.contains( name ) )
		return ClientContainer();

	return _itemFromName[ name ].registeredClients;
}

bool Configuration::isClientRegistered( const QString& name, const QHostAddress& address, quint16 port ) const
{
	QMutexLocker lk( &_mutex );

	if ( false == _itemFromName.contains( name ) )
		return false;

	return _itemFromName[ name ].registeredClients.contains( qMakePair( address, port ) );
}

ClientContainer Configuration::addRegisteredClient( const QString& name, const QHostAddress& address, quint16 port )
{
	QMutexLocker lk( &_mutex );

	if ( false == _itemFromName.contains( name ) )
		return ClientContainer();

	ClientContainer& clients = _itemFromName[ name ].registeredClients;
	clients.insert( qMakePair( address, port ) );
	return clients;
}

ClientContainer Configuration::removeRegisteredClient( const QString& name, const QHostAddress& address, quint16 port )
{
	QMutexLocker lk( &_mutex );

	if ( false == _itemFromName.contains( name ) )
		return ClientContainer();

	ClientContainer& clients = _itemFromName[ name ].registeredClients;
	clients.remove( qMakePair( address, port ) );
	return clients;
}

void Configuration::clearRegisteredClients( const QString& name )
{
	QMutexLocker lk( &_mutex );

	if ( false == _itemFromName.contains( name ) )
		return;

	_itemFromName[ name ].registeredClients.clear();
}

bool Configuration::addItem( const QString& name, bool serverOnly )
{
	bool ret;

	{
		QMutexLocker lk( &_mutex );
		ret = addItem_i( name, serverOnly );
	}

	if( ret )
	{
		Q_EMIT changed();
		return true;
	}

	return false;
}

bool Configuration::addItem_i( const QString& name, bool serverOnly )
{
	if( name.isEmpty() )
	{
		return false;
	}

	if( _itemFromName.contains( name ) )
	{
		return false;
	}

	_itemFromName[ name ] = Item( name, serverOnly );

	return true;
}

bool Configuration::removeItem( const QString& name )
{
	{
		QMutexLocker lk( &_mutex );

		if( false == _itemFromName.contains( name ) )
		{
			return false;
		}

		_itemFromName.take( name );
	}

	Q_EMIT changed();

	return true;
}

bool Configuration::renameItem( const QString& oldName, const QString& newName )
{
	{
		QMutexLocker lk( &_mutex );

		if( oldName == newName )
		{
			return true;
		}

		if( false == _itemFromName.contains( oldName ) )
		{
			return false;
		}

		if( _itemFromName.contains( newName ) )
		{
			return false;
		}

		Item item = _itemFromName.take( oldName );
		item.name = newName;
		_itemFromName[ newName ] = item;
	}

	Q_EMIT changed();

	return true;
}

QByteArray Configuration::save() const
{
	QMutexLocker lk( &_mutex );

	QDomDocument doc;
	QDomElement root = doc.createElement( "configuration" );
	doc.appendChild( root );

	QMap< QString, Item >::const_iterator iter = _itemFromName.constBegin();

	while( _itemFromName.constEnd() != iter )
	{
		const Item& item = iter.value();

		QDomElement e = doc.createElement( "item" );
		e.setAttribute( "name", item.name );

		if ( item.serverOnly )
			e.setAttribute( "serveronly", "true" );

		root.appendChild( e );

		++iter;
	}

	return doc.toByteArray();
}

bool Configuration::load( const QByteArray& data )
{
	{
		QMutexLocker lk( &_mutex );

		_itemFromName.clear();

		QDomDocument doc;

		if( false == doc.setContent( data ) )
		{
			return false;
		}

		QDomElement configuration = doc.documentElement();
		if( "configuration" != configuration.tagName() )
		{
			return false;
		}

		for (int i=0; i<configuration.childNodes().size(); i++)
		{
			QDomElement e = configuration.childNodes().at(i).toElement();

			if( e.isNull() )
			{
				continue;
			}

			QString name;
			bool serverOnly = false;
			if( e.hasAttribute( "name" ) )
			{
				name = e.attribute( "name" );
			}
			else
			{
				name = e.tagName();
			}

			if ( e.hasAttribute( "serveronly" ) )
			{
				if ( e.attribute( "serveronly", "false" ).trimmed().toLower() == "true" )
					serverOnly = true;
			}

			if( false == addItem_i( name, serverOnly ) )
			{
				return false;
			}
		}
	}

	Q_EMIT changed();

	return true;
}

