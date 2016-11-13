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

#include <QtCore>
#include <QDebug>

#include "serialization/ImageMsg.h"
#include "rec/rpc/Client.h"

class Client : public rec::rpc::Client
{
	Q_OBJECT
public:
	Client();
	~Client();

	bool blocking() const;


    //void publishImage( const QImage& img );

public Q_SLOTS:	
	void setTopicsEnabled( bool enabled );
	void setBlocking( bool blocking );

Q_SIGNALS:
    void log( const QString& );
	void imageReceived( const QImage& );
    void imageInfoChanged( const QSet< QPair< QHostAddress, quint16 > >& );

private:
	bool _blocking;


	// Declare the topic listener and the topic info listener for the topic "image".
	// Serialization data types are defined in Serialization.h and Serialization.cpp.
	// Data type for info listeners is always QSet< QPair< QHostAddress, quint16 > >.
	DECLARE_TOPICLISTENER( image );
    DECLARE_TOPICINFOCHANGED( image );
};
