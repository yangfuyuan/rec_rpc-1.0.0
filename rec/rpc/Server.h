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

#ifndef _REC_RPC_SERVER_H_
#define _REC_RPC_SERVER_H_

#include "rec/rpc/defines.h"
#include "rec/rpc/common.h"

namespace rec
{
	namespace rpc
	{
		/*! \cond */
		namespace server
		{

            class Server;

		}

		/*! \endcond */

		/*!
		 *  \brief RPC function wrapper interface
		 *
		 *  The interface that RPC function wrappers must implement. It is recommended to use the preprocessor macros to do this.
		 *
		 *  \sa DECLARE_FUNCTION, BEGIN_FUNCTION_DEFINITION, END_FUNCTION_DEFINITION
		 */
		struct RPCFunctionBase
		{
			/*! This method creates an instance of the RPC function parameter type. */
			virtual serialization::SerializablePtr createParam() const = 0;

			/*! This method creates an instance of the RPC function return value type. */
			virtual serialization::SerializablePtr createResult() const = 0;

			/*!
			 *  This method is called by the server to invoke the function.
			 *
			 *  \param param RPC function parameters.
			 *  \param result RPC function return values.
			 *  \param address Network address of the calling client.
			 *  \param port TCP port of the calling client.
			 */
			virtual void invoke( const serialization::Serializable& param, serialization::Serializable& result, const QHostAddress& address, unsigned short port ) const = 0;
		};

		/*! \cond */
		namespace detail
		{
			template< typename Parent_t, typename Param_t, typename Result_t >
			struct RPCFunction : public rec::rpc::RPCFunctionBase
			{
				typedef void( Parent_t::*Invoke_f )( const Param_t&, Result_t&, const QHostAddress&, unsigned short );

				RPCFunction( Parent_t* parent, Invoke_f function ) : _parent( parent ), _function( function ) { }
				serialization::SerializablePtr createParam() const { return createSerializable< Param_t >(); }
				serialization::SerializablePtr createResult() const { return createSerializable< Result_t >(); }

				void invoke( const serialization::Serializable& param, serialization::Serializable& result, const QHostAddress& address, unsigned short port ) const
				{
					if ( typeid( param ) != typeid( Param_t ) || typeid( result ) != typeid( Result_t ) )
						throw Exception( rec::rpc::WrongDataFormat );
					( _parent->*_function )( static_cast< const Param_t& >( param ), static_cast< Result_t& >( result ), address, port );
				}

				Parent_t* _parent;
				Invoke_f _function;
			};
		}
		/*! \endcond */

		/*!
		 *  \brief RPC server base class
		 *
		 *  Base class to implement a RPC server. Derive from that class to implement your own server.
		 */
		class REC_RPC_EXPORT Server : public QObject
		{
			Q_OBJECT
		public:
			/*! \brief Default constructor */
			Server();
			/*! \brief Destructor */
			virtual ~Server();

			/*!
			 *  \brief Start server
			 *
			 *  After calling this method, the server will listen for connecting clients at the specified port.
			 *
			 *  \param port TCP port used by the server. Default is 9280.
			 */
			void listen( int port = rec::rpc::defaultPort );

			/*! \return true if the server is listening. */
			bool isListening() const;

			/*! \return TCP port the server is using. */
			unsigned short serverPort() const;

			/*!
			 *  \brief The server's greeting message.
			 *
			 *  When a client connects to the server, the server sends a "greeting" (which is just a short ASCII string) to the client. If you connect to the server via telnet, this is the first message you will see. Default is "REC RPC Server <Version>".
			 *
			 *  \return Greeting string.
			 *
			 *  \sa setGreeting()
			 */
			QString greeting() const;

			/*!
			 *  \brief Set a custom greeting message.
			 *
			 *  When a client connects to the server, the server sends a "greeting" (which is just a short ASCII string) to the client. If you connect to the server via telnet, this is the first message you will see. Default is "REC RPC Server <Version>".
			 *
			 *  \param greeting New custom greeting.
			 *
			 *  \sa greeting()
			 */
			void setGreeting( const QString& greeting );

		public Q_SLOTS:
			/*!
			 *  \brief Stop server
			 *
			 *  After calling this method, the server stops listening.
			 */
			void close();

		Q_SIGNALS:
			/*! This signal is emitted when the server has been started. */
			void listening();

			/*! This signal is emitted when the server has been stopped. */
			void closed();

			/*!
			 *  This signal is emitted when an error occurs on the server.
			 *
			 *  \param error Error code
			 *  \param errorString Human readable description of the error that occurred.
			 */
			void serverError( QAbstractSocket::SocketError error, const QString& errorString );

			/*!
			 *  This signal is emitted when an error occurs on a client.
			 *
			 *  \param error Error code
			 *  \param errorString Human readable description of the error that occurred.
			 */
			void clientError( QAbstractSocket::SocketError error, const QString& errorString );

			/*!
			 *  This signal is emitted when a client has established a connection to the server.
			 *
			 *  \param address The client's network address (is QHostAddress::Null if the client is connected via a local IPC channel).
			 *  \param port The client's port (is a reference number greater than 0 if the client is connected via a local IPC channel).
			 */
			void clientConnected( const QHostAddress& address, unsigned short port );

			/*!
			 *  This signal is emitted when a client has disconnected from the server.
			 *
			 *  \param address The client's network address (is QHostAddress::Null if the client is connected via a local IPC channel).
			 *  \param port The client's port (is a reference number greater than 0 if the client is connected via a local IPC channel).
			 */
			void clientDisconnected( const QHostAddress& address, unsigned short port );

			/*!
			 *  This signal is emitted when a client has registered a topic listener.
			 *
			 *  \param name Name of the topic.
			 *  \param address The client's network address (is QHostAddress::Null if the client is connected via a local IPC channel).
			 *  \param port The client's port (is a reference number greater than 0 if the client is connected via a local IPC channel).
			 */
			void registeredTopicListener( const QString& name, const QHostAddress& address, unsigned short port );

			/*!
			 *  This signal is emitted when a client has unregistered a topic listener.
			 *
			 *  \param name Name of the topic.
			 *  \param address The client's network address (is QHostAddress::Null if the client is connected via a local IPC channel).
			 *  \param port The client's port (is a reference number greater than 0 if the client is connected via a local IPC channel).
			 */
			void unregisteredTopicListener( const QString& name, const QHostAddress& address, unsigned short port );

		protected:
			/*!
			 *  \brief Register a RPC function.
			 *
			 *  This method is used to add a RPC function that can be invoked by a client.
			 *  It is recommended to use the preprocessor macro instead of calling the method directly.
			 *
			 *  \param name Name of the RPC function.
			 *  \param function RPC function wrapper. This must be an instance of a struct derived from RPCFunctionBase.
			 *
			 *  \throw rec::rpc::Exception Error codes: ImproperFunctionName.
			 *
			 *  \sa REGISTER_FUNCTION
			 */
			void registerFunction( const QString& name, RPCFunctionBase* function );

			/*!
			 *  \brief Unregister a RPC function.
			 *
			 *  This method is used to remove a RPC function.
			 *  It is recommended to use the preprocessor macro instead of calling the method directly.
			 *
			 *  \param name Name of the RPC function.
			 *
			 *  \sa UNREGISTER_FUNCTION
			 */
			void unregisterFunction( const QString& name );

			/*!
			 *  \brief Register a topic listener.
			 *
			 *  This method is used to add a topic listener that is invoked when the data of a topic change.
			 *  It is recommended to use the preprocessor macro instead of calling the method directly.
			 *
			 *  \param name Name of the topic.
			 *  \param listener Topic listener. This must be an instance of a struct derived from TopicListenerBase.
			 *
			 *  \throw rec::rpc::Exception Error codes: NoSuchTopic.
			 *
			 *  \sa REGISTER_TOPICLISTENER, REGISTER_TOPICINFOCHANGED
			 */
			void registerTopicListener( const QString& name, TopicListenerBase* listener );

			/*!
			 *  \brief Unregister a topic listener.
			 *
			 *  This method is used to remove a topic listener.
			 *  It is recommended to use the preprocessor macro instead of calling the method directly.
			 *
			 *  \param name Name of the topic.
			 *
			 *  \sa UNREGISTER_TOPICLISTENER, UNREGISTER_TOPICINFOCHANGED
			 */
			void unregisterTopicListener( const QString& name );

			/*!
			 *  \brief Add a topic.
			 *
			 *  This method is used to register a topic in the server. The server and the clients can publish data via the topic. The data is distributed via shared memory, local IPC or the TCP connection.
			 *  It is recommended to use the preprocessor macro instead of calling the method directly.
			 *
			 *  \param name Name of the topic.
			 *  \param sharedMemorySize Minimum size of the shared memory segment in bytes. If 0 (default), no shared memory will be used.
			 *
			 *  \throw rec::rpc::Exception Error codes: ImproperTopicName, TopicAlreadyExists.
			 *
			 *  \sa ADD_TOPIC
			 */
			void addTopic( const QString& name, int sharedMemorySize = 0 );

			/*!
			 *  \brief Publish new topic data.
			 *
			 *  Use this method to modify topic data and notify all clients that listen to that topic.
			 *  It is recommended to use the preprocessor macro instead of calling the method directly.
			 *
			 *  \param name Name of the topic.
			 *  \param data Pointer to the data to be serialized and published.
			 *
			 *  \throw rec::rpc::Exception Error codes: NoConnection, NoSuchTopic, AccessDenied.
			 *
			 *  \sa PREPARE_TOPIC, PUBLISH_TOPIC, PUBLISH_TOPIC_SIMPLE
			 */
			void publishTopic( const QString& name, serialization::SerializablePtrConst data );

		private:
            server::Server* _server;
		};
	}
}

/*!
 *  \brief Declare a RPC function in the server class definition.
 *
 *  Use this macro to declare a RPC function in the definition of your own server class derived from rec::rpc::Server.
 *  All code which is necessary to register and invoke the function is inserted automatically.
 *
 *  \param FUNCTIONNAME Name of the RPC function (without qoutes).
 */
#define DECLARE_FUNCTION( FUNCTIONNAME ) \
	private: \
		rec::rpc::RPCFunctionBase* create##FUNCTIONNAME##Wrapper(); \
		void FUNCTIONNAME( const FUNCTIONNAME##Param& param, FUNCTIONNAME##Result& result, const QHostAddress& address, unsigned short port );

/*!
 *  \brief Begin the implementation of a RPC function.
 *
 *  Place this macro above the implementation of a RPC function.
 *  The RPC function parameters are accessible via "param", the return values can be accessed via "result".
 *  The types of "param" and "result" are 'FUNCTIONNAME'Param and 'FUNCTIONNAME'Result. These type names must be defined and derived from Serializable.
 *  The IP address and the TCP port of the calling client are accessible via "address" (type QHostAddress) and "port" (type quint16).
 *
 *  \param CLASSNAME Name of your server class.
 *  \param FUNCTIONNAME Name of the RPC function (without qoutes).
 */
#define BEGIN_FUNCTION_DEFINITION( CLASSNAME, FUNCTIONNAME ) \
	rec::rpc::RPCFunctionBase* CLASSNAME::create##FUNCTIONNAME##Wrapper() \
	{ \
		return new rec::rpc::detail::RPCFunction< CLASSNAME, FUNCTIONNAME##Param, FUNCTIONNAME##Result >( this, &CLASSNAME::FUNCTIONNAME ); \
	} \
	void CLASSNAME::FUNCTIONNAME( const FUNCTIONNAME##Param& param, FUNCTIONNAME##Result& result, const QHostAddress& address, unsigned short port ) \
	{

/*!
 *  \brief End of a RPC function implementation.
 */
#define END_FUNCTION_DEFINITION }

/*!
 *  \brief Register a RPC function.
 *
 *  This macro is used to add a RPC function that can be invoked by a client.
 *  It creates the wrapper object and calls registerFunction automatically.
 *
 *  \param FUNCTIONNAME Name of the RPC function (without quotes).
 *
 *  \throw rec::rpc::Exception Error codes: ImproperFunctionName.
 *
 *  \sa rec::rpc::Server::registerFunction()
 */
#define REGISTER_FUNCTION( FUNCTIONNAME ) registerFunction( #FUNCTIONNAME, create##FUNCTIONNAME##Wrapper() );

/*!
 *  \brief Unregister a RPC function.
 *
 *  This macro is used to remove a RPC function.
 *
 *  \param FUNCTIONNAME Name of the RPC function (without quotes).
 *
 *  \sa rec::rpc::Server::unregisterFunction()
 */
#define UNREGISTER_FUNCTION( FUNCTIONNAME ) unregisterFunction( #FUNCTIONNAME );

/*!
 *  \brief Add a topic.
 *
 *  This macro is used to register a topic in the server. The server and the clients can publish data via the topic. The data is distributed via shared memory, local IPC or the TCP connection.
 *
 *  \param TOPICNAME Name of the topic.
 *  \param SHAREDMEMSIZE Minimum size of the shared memory segment in bytes. If 0 (default), no shared memory will be used.
 *
 *  \throw rec::rpc::Exception Error codes: ImproperTopicName, TopicAlreadyExists.
 *
 *  \sa rec::rpc::Server::addTopic()
 */
#define ADD_TOPIC( TOPICNAME, SHAREDMEMSIZE ) addTopic( #TOPICNAME, SHAREDMEMSIZE );

#endif //_REC_RPC_RPC_SERVER_H_
