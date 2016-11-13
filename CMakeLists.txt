#Copyright (c) 2011, REC Robotics Equipment Corporation GmbH, Planegg, Germany
#All rights reserved.

#Redistribution and use in source and binary forms, with or without modification,
#are permitted provided that the following conditions are met:

#- Redistributions of source code must retain the above copyright notice,
#  this list of conditions and the following disclaimer.
#- Redistributions in binary form must reproduce the above copyright notice,
#  this list of conditions and the following disclaimer in the documentation and/or
#  other materials provided with the distribution.
#- Neither the name of the REC Robotics Equipment Corporation GmbH nor the names of
#  its contributors may be used to endorse or promote products derived from this software
#  without specific prior written permission.

#THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS
#OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
#AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
#CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
#DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
#DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
#IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
#OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

cmake_minimum_required( VERSION 2.6 )

PROJECT( REC_RPC )
#set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_compile_options(-std=c++11)

find_package( Qt5 COMPONENTS Core Xml Network Gui Sql REQUIRED )
#include(${QT_USE_FILE})
include_directories(${Qt5Core_INCLUDE_DIRS} ${Qt5Xml_INCLUDE_DIRS} ${Qt5Network_INCLUDE_DIRS} ${Qt5Gui_INCLUDE_DIRS} ${Qt5Sql_INCLUDE_DIRS})
ADD_DEFINITIONS( -DQT_NO_KEYWORDS )

INCLUDE_DIRECTORIES(
	${CMAKE_CURRENT_SOURCE_DIR}
)

# Modified copy from FindQt4.cmake since the QT4_WRAP_CPP macro does not
# generate unique moc names for files with identical names residing in
# different subdirectories
MACRO (MY_WRAP_CPP outfiles prefix)
	# get include dirs
	IF ( COMMAND QT5_GET_MOC_INC_DIRS )
		QT5_GET_MOC_INC_DIRS(moc_includes)
	ELSE ( COMMAND QT5_GET_MOC_INC_DIRS )
		IF ( COMMAND QT5_GET_MOC_FLAGS )
			QT5_GET_MOC_FLAGS(moc_includes)
		ELSE ( COMMAND QT5_GET_MOC_FLAGS )
			message("Your CMake version is incompatible!")
		ENDIF ( COMMAND QT5_GET_MOC_FLAGS )
	ENDIF ( COMMAND QT5_GET_MOC_INC_DIRS )


	FOREACH (it ${ARGN})
		GET_FILENAME_COMPONENT(it ${it} ABSOLUTE)
		GET_FILENAME_COMPONENT(outfile ${it} NAME_WE)

		SET(outfile ${CMAKE_CURRENT_BINARY_DIR}/moc_${prefix}_${outfile}.cxx)
		IF( NOT WIN32 )
			SET( ENV{LD_LIBRARY_PATH} ${QT_LIBRARY_DIR} )
		ENDIF( NOT WIN32 )
		ADD_CUSTOM_COMMAND(OUTPUT ${outfile}
			COMMAND ${QT_MOC_EXECUTABLE}
			ARGS -nw ${moc_includes} -o ${outfile} ${it}
			DEPENDS ${it}
		)
		SET(${outfiles} ${${outfiles}} ${outfile})
	ENDFOREACH(it)
ENDMACRO (MY_WRAP_CPP)

#Main source file
FILE( GLOB HDRS rec/rpc/*.h )
FILE( GLOB HDRS_PRIVATE rec/rpc/*.hpp )
FILE( GLOB SRCS rec/rpc/*.cpp )

MY_WRAP_CPP(
	MOC_SRCS
	"rec_rpc"
	${HDRS}
	${HDRS_PRIVATE}
)

#client source files
FILE( GLOB client_HDRS rec/rpc/client/*.h )
FILE( GLOB client_HDRS_PRIVATE rec/rpc/client/*.hpp )
FILE( GLOB client_SRCS rec/rpc/client/*.cpp )

MY_WRAP_CPP(
	client_MOC_SRCS
	"rec_rpc_client"
	${client_HDRS}
	${client_HDRS_PRIVATE}
)

#configuration source files
FILE( GLOB configuration_HDRS rec/rpc/configuration/*.h )
FILE( GLOB configuration_HDRS_PRIVATE rec/rpc/configuration/*.hpp )
FILE( GLOB configuration_SRCS rec/rpc/configuration/*.cpp )

MY_WRAP_CPP(
	configuration_MOC_SRCS
	"rec_rpc_configuration"
	${configuration_HDRS}
	${configuration_HDRS_PRIVATE}
)

#messages source files
FILE( GLOB messages_HDRS rec/rpc/messages/*.h )
FILE( GLOB messages_HDRS_PRIVATE rec/rpc/messages/*.hpp )
FILE( GLOB messages_SRCS rec/rpc/messages/*.cpp )

MY_WRAP_CPP(
	messages_MOC_SRCS
	"rec_rpc_messages"
	${messages_HDRS}
	${messages_HDRS_PRIVATE}
)

#serialization source files
FILE( GLOB serialization_HDRS rec/rpc/serialization/*.h )
FILE( GLOB serialization_HDRS_PRIVATE rec/rpc/serialization/*.hpp )
FILE( GLOB serialization_SRCS rec/rpc/serialization/*.cpp )

MY_WRAP_CPP(
	serialization_MOC_SRCS
	"rec_rpc_serialization"
	${serialization_HDRS}
	${serialization_HDRS_PRIVATE}
)

#server source files
FILE( GLOB server_HDRS rec/rpc/server/*.h )
FILE( GLOB server_HDRS_PRIVATE rec/rpc/server/*.hpp )
FILE( GLOB server_SRCS rec/rpc/server/*.cpp )

MY_WRAP_CPP(
	server_MOC_SRCS
	"rec_rpc_server"
	${server_HDRS}
	${server_HDRS_PRIVATE}
)

ADD_LIBRARY(
	rec_rpc
#	SHARED
        STATIC

	${SRCS}
	${HDRS}
	${HDRS_PRIVATE}
	${MOC_SRCS}
	
	${client_SRCS}
	${client_HDRS}
	${client_HDRS_PRIVATE}
	${client_MOC_SRCS}
	
	${configuration_SRCS}
	${configuration_HDRS}
	${configuration_HDRS_PRIVATE}
	${configuration_MOC_SRCS}
	
	${messages_SRCS}
	${messages_HDRS}
	${messages_HDRS_PRIVATE}
	${messages_MOC_SRCS}
	
	${serialization_SRCS}
	${serialization_HDRS}
	${serialization_HDRS_PRIVATE}
	${serialization_MOC_SRCS}
	
	${server_SRCS}
	${server_HDRS}
	${server_HDRS_PRIVATE}
	${server_MOC_SRCS}
)

TARGET_LINK_LIBRARIES(
	rec_rpc
        ${QT_LIBRARIES} 
#	${QT_QTCORE_LIBRARY}
#	${QT_QTNETWORK_LIBRARY}
#	${QT_QTXML_LIBRARY}
)

qt5_use_modules(rec_rpc Core Xml Network Gui Sql)

SET( INSTALL_ROOT ${CMAKE_CURRENT_BINARY_DIR}/install )

IF( WIN32 )
	SET(
		CMAKE_INSTALL_PREFIX
		${INSTALL_ROOT}
	)
ELSE( WIN32 )
	SET(
		CMAKE_INSTALL_PREFIX
		/usr/local
	)
ENDIF( WIN32 )

INSTALL(
	TARGETS rec_rpc
	ARCHIVE DESTINATION lib
	LIBRARY DESTINATION lib
	RUNTIME DESTINATION bin
)

INSTALL(
	FILES
	${HDRS}
	DESTINATION include/rec/rpc
)

INSTALL(
	FILES
	${client_HDRS}
	DESTINATION include/rec/rpc/client
)

INSTALL(
	FILES
	${configuration_HDRS}
	DESTINATION include/rec/rpc/configuration
)

INSTALL(
	FILES
	${messages_HDRS}
	DESTINATION include/rec/rpc/messages
)

INSTALL(
	FILES
	${serialization_HDRS}
	DESTINATION include/rec/rpc/serialization
)

INSTALL(
	FILES
	${server_HDRS}
	DESTINATION include/rec/rpc/server
)
