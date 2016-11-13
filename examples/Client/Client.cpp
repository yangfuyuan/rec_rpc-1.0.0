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

#include "Client.h"

Client::Client()
: _blocking( false )
{


    setExpectedGreeting( "REC RPC server" );
}

Client::~Client()
{
}

void Client::setTopicsEnabled( bool enabled )
{	// Register or unregister topic listener and topic info listener for topic "image".
	if ( enabled )
	{
		try
		{
			REGISTER_TOPICLISTENER( image );
            REGISTER_TOPICINFOCHANGED( image );
		}
		catch( const rec::rpc::Exception& e )
		{
            Q_EMIT log( QString( "Failed to register topic listeners: " ) + e.getMessage() );
		}
	}
	else
	{
		try
		{
			UNREGISTER_TOPICLISTENER( image );
            UNREGISTER_TOPICINFOCHANGED( image );
		}
		catch( const rec::rpc::Exception& e )
		{
            Q_EMIT log( QString( "Failed to unregister topic listeners: " ) + e.getMessage() );
		}
	}
}

bool Client::blocking() const
{
	return _blocking;
}

void Client::setBlocking( bool blocking )
{
	_blocking = blocking;
}




BEGIN_TOPICLISTENER_DEFINITION( Client, image )
	// Topic listener definition for "image". Just notify the GUI that there is a new image.
    Q_EMIT log( QString( "Image received from %1:%2" ).arg( address.toString() ).arg( port ) );
	Q_EMIT imageReceived( data.ref() );
END_TOPICLISTENER_DEFINITION

BEGIN_TOPICINFOCHANGED_DEFINITION( Client, image )
    // Topic listener definition for "image". Just notify the GUI that the list of registered clients has changed.
    Q_EMIT imageInfoChanged( info );
END_TOPICINFOCHANGED_DEFINITION
