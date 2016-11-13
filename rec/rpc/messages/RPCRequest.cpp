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

#include "rec/rpc/messages/RPCRequest.h"
#include "rec/rpc/messages/Message.h"

#include <QDataStream>

using namespace rec::rpc::messages;

// RPCRequest layout:
// 1 message id
// 4 message length without header
// QDataStream serialized data:
//   RPC function name
//   sequence number
//   RPC function parameters

QByteArray RPCRequest::encode( const QString& name, quint32 seqNum, const QByteArray& param )
{
	QByteArray data( headerSize, 0 );

	//Header (Teil 1)************************
	//message id
	data.data()[0] = rec::rpc::messages::RPCRequestId;

	//Nutzdaten
	{
		QDataStream s( &data, QIODevice::Append );
		s.setVersion( QDATASTREAM_VERSION );
		s << name;
		s << seqNum;
		s << param;
	}

	//Header (Teil 2)
	//Laenge berechnen
	quint32 dataSizeWithoutHeader = data.size() - headerSize;
	*reinterpret_cast< quint32* >( data.data() + 1 ) = dataSizeWithoutHeader;

	return data;
}

bool rec::rpc::messages::RPCRequest::decode( const QByteArray& data, QString* name, quint32* seqNum, QByteArray* param )
{
	//daten ohne header

	QDataStream s( data );
	s.setVersion( QDATASTREAM_VERSION );
	if ( ( s >> *name ).status() != QDataStream::Ok )
		return false;

	if ( ( s >> *seqNum ).status() != QDataStream::Ok )
		return false;

	if ( ( s >> *param ).status() != QDataStream::Ok )
		return false;

	if ( !s.atEnd() )
		return false;

	return true;
}
