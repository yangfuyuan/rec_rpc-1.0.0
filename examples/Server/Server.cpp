#include "Server.h"

#include "rec/rpc/Exception.h"

Server::Server()
{
	// Add the topic "image" with initially 4kB shared memory.
	ADD_TOPIC( image, 0x100000 );

    setGreeting( "REC RPC server" );

    setTopicsEnabled( true );
}

void Server::publishImage( const QImage& img )
{	// Publish the given QImage in the topic "image".
    try
    {
        PUBLISH_TOPIC_SIMPLE( image, img );
    }
    catch( rec::rpc::Exception& e )
    {
            Q_EMIT log( e.what() );
    }

}

void Server::setTopicsEnabled( bool enabled )
{	// Register or unregister topic listener and topic info listener for topic "image".
	if ( enabled )
	{
        REGISTER_TOPICINFOCHANGED( image );
        //REGISTER_TOPICLISTENER( image );
	}
	else
	{
        UNREGISTER_TOPICINFOCHANGED( image );
        //UNREGISTER_TOPICLISTENER( image );
	}
}


/*
BEGIN_TOPICLISTENER_DEFINITION( Server, image )
	// Topic listener definition for "image". Just notify the GUI that there is a new image.
    Q_EMIT log( QString( "Image received from %1:%2" ).arg( address.toString() ).arg( port ) );
	Q_EMIT imageReceived( data );
END_TOPICLISTENER_DEFINITION
*/


BEGIN_TOPICINFOCHANGED_DEFINITION( Server, image )
    // Topic listener definition for "image". Just notify the GUI that the list of registered clients has changed.
    Q_EMIT imageInfoChanged( info );
END_TOPICINFOCHANGED_DEFINITION
