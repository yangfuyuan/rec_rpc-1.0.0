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

#include "rec/rpc/Exception.h"

using namespace rec::rpc;

QString Exception::messageFromErrorCode( ErrorCode code )
{
	switch( code )
	{
	case NoError:
		return tr( "No error." );
	case NoConnection:
		return tr( "No connection to the RPC server." );
	case NotAnRPCServer:
		return tr( "The specified server is not an RPC server." );
	case IncompatibleServer:
		return tr( "The server is not compatible." );
	case UnknownFunction:
		return tr( "The specified function is not supported by the remote server." );
	case ExecutionCancelled:
		return tr( "The execution of the specified function was cancelled by the remote server." );
	case ExecutionTimeout:
		return tr( "Time out error during execution." );
	case WrongDataFormat:
		return tr( "Wrong data format." );
	case NoSuchTopic:
		return tr( "No such topic." );
	case ImproperTopicName:
		return tr( "Improper topic name." );
	case TopicAlreadyExists:
		return tr( "A topic with this name already exists." );
	case AccessDenied:
		return tr( "Access denied." );
	case ImproperFunctionName:
		return tr( "Improper function name." );
	}
	return tr( "Unknown error." );
}

Exception::Exception( ErrorCode code )
: _code( code )
{
}

Exception::~Exception() throw()
{
}

ErrorCode Exception::errorCode() const
{
	return _code;
}

QString Exception::getMessage() const
{
	return messageFromErrorCode( _code );
}

const char* Exception::what() const throw()
{
    _msg = getMessage().toLatin1();
	return _msg.constData();
}
