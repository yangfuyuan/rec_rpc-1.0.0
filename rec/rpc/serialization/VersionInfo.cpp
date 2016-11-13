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

#include "rec/rpc/serialization/VersionInfo.hpp"

using namespace rec::rpc::serialization;

VersionInfo::VersionInfo( int majorVer_, int minorVer_, int patchVer_, int buildVer_, const QString& suffix_ )
: majorVer( majorVer_ )
, minorVer( minorVer_ )
, patchVer( patchVer_ )
, buildVer( buildVer_ )
, suffix( suffix_ )
{
}

VersionInfo::VersionInfo()
: majorVer( 0 )
, minorVer( 0 )
, patchVer( 0 )
, buildVer( 0 )
{
}

void VersionInfo::serialize( QDataStream& s ) const
{
	s << majorVer;
	s << minorVer;
	s << patchVer;
	s << buildVer;
	s << suffix;
}

void VersionInfo::deserialize( QDataStream& s )
{
	if ( ( s >> majorVer ).status() != QDataStream::Ok )
		return;
	if ( ( s >> minorVer ).status() != QDataStream::Ok )
		return;
	if ( ( s >> patchVer ).status() != QDataStream::Ok )
		return;
	if ( ( s >> buildVer ).status() != QDataStream::Ok )
		return;
	if ( ( s >> suffix ).status() != QDataStream::Ok )
		return;
}
