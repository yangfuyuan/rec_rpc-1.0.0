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

#include "rec/rpc/serialization/String.h"

using namespace rec::rpc::serialization;

String::String()
{
}

String::String( const QString& s )
: Primitive< QString >( s )
{
}

String::String( const std::string& s )
: Primitive< QString >( QString::fromStdString( s ) )
{
}

String::String( const std::wstring& s )
#if !defined WIN32 || ( QT_VERSION_MINOR >= 7 && QT_VERSION_PATCH >= 2 )
: Primitive< QString >( QString::fromStdWString( s ) )
#else
: Primitive< QString >( QString::fromUtf16( reinterpret_cast< const ushort* >( s.c_str() ) ) )
#endif
{
}

String::String( const char* s )
    : Primitive< QString >( QString::fromLatin1(s) )
{
}

String::String( const wchar_t* s )
#ifdef WIN32
: Primitive< QString >( QString::fromUtf16( reinterpret_cast< const ushort* >( s ) ) )
#else
: Primitive< QString >( QString::fromUcs4( reinterpret_cast< const uint* >( s ) ) )
#endif
{
}

std::string String::toStdString() const
{
	return ref().toStdString();
}

void String::setStdString( const std::string& s )
{
	ref() = QString::fromStdString( s );
}

std::wstring String::toStdWString() const
{
#if !defined WIN32 || ( QT_VERSION_MINOR >= 7 && QT_VERSION_PATCH >= 3 )
	return ref().toStdWString();
#else
	return std::wstring( reinterpret_cast< const wchar_t* >( ref().utf16() ), ref().size() );
#endif
}

void String::setStdWString( const std::wstring& s )
{
#if !defined WIN32 || ( QT_VERSION_MINOR >= 7 && QT_VERSION_PATCH >= 2 )
	ref() = QString::fromStdWString( s );
#else
	ref() = QString::fromUtf16( reinterpret_cast< const ushort* >( s.c_str() ) );
#endif
}

void String::setCString( const char* s )
{
    //ref() = QString::fromAscii( s );
    ref() = QString::fromLatin1(s);
}

void String::setCWString( const wchar_t* s )
{
#ifdef WIN32
	ref() = QString::fromUtf16( reinterpret_cast< const ushort* >( s ) );
#else
	ref() = QString::fromUcs4( reinterpret_cast< const uint* >( s ) );
#endif
}
