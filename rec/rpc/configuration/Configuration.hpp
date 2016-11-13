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

#ifndef _REC_RPC_CONFIGURATION_CONFIGURATION_H_
#define _REC_RPC_CONFIGURATION_CONFIGURATION_H_

#include "rec/rpc/defines.h"
#include "Item.hpp"

#include <QObject>
#include <QByteArray>
#include <QHostAddress>
#include <QMap>
#include <QDomDocument>
#include <QDomElement>
#include <QMutex>

namespace rec
{
	namespace rpc
	{
		namespace configuration
		{
			class ConfigurationLocker;

			class REC_RPC_EXPORT Configuration : public QObject
			{
				Q_OBJECT
				friend class ConfigurationLocker;
			public:
				Configuration( QObject* parent = NULL );

				Configuration( const Configuration& other );

				Configuration& operator=( const Configuration& other );

				bool isEmpty() const { return _itemFromName.isEmpty(); }

				bool contains( const QString& name ) const { return _itemFromName.contains( name ); }

				QMap< QString, Item > itemContainer() const { return _itemFromName; }

				bool isInitialized( const QString& name ) const;

				QByteArray data( const QString& name, QHostAddress* publisherAddress, quint16* publisherPort ) const;
				bool setData( const QString& name, const QByteArray& data, const QHostAddress& publisherAddress, quint16 publisherPort );

				bool isServerOnly( const QString& name ) const;

				ClientContainer registeredClients( const QString& name ) const;
				bool isClientRegistered( const QString& name, const QHostAddress& address, quint16 port ) const;
				ClientContainer addRegisteredClient( const QString& name, const QHostAddress& address, quint16 port );
				ClientContainer removeRegisteredClient( const QString& name, const QHostAddress& address, quint16 port );
				void clearRegisteredClients( const QString& name );

				/**
				@return Returns false if the name is already in use of if name or type are empty strings.
				*/
				bool addItem( const QString& name, bool serverOnly = false );

				bool removeItem( const QString& name );

				bool renameItem( const QString& oldName, const QString& newName );

				QByteArray save() const;

				bool load( const QByteArray& data );

			Q_SIGNALS:
				void changed();

			private:
				bool addItem_i( const QString& name, bool serverOnly );
				
				QMap< QString, Item > _itemFromName;
				mutable QMutex _mutex;
			};

			class ConfigurationLocker
			{
			public:
				ConfigurationLocker( Configuration& configuration_ )
					: configuration( configuration_ )
				{
					configuration._mutex.lock();
				}

				~ConfigurationLocker()
				{
					configuration._mutex.unlock();
				}

				Configuration& configuration;
			};
		}
	}
}

#include <QMetaType>
Q_DECLARE_METATYPE( rec::rpc::configuration::Configuration )

#endif //_REC_RPC_CONFIGURATION_CONFIGURATION_H_
