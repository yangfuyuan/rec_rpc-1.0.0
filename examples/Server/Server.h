#ifndef _SERVER_H_
#define _SERVER_H_

#include "rec/rpc/Server.h"
#include "serialization/ImageMsg.h"

#include <QtCore>

class Server : public rec::rpc::Server
{
	Q_OBJECT
public:
	Server();

	void publishImage( const QImage& img );

public Q_SLOTS:
	void setTopicsEnabled( bool enabled );

Q_SIGNALS:
    void log( const QString& );
    //void imageReceived( const QImage& );
    void imageInfoChanged( const QSet< QPair< QHostAddress, quint16 > >& );

private:
    //DECLARE_TOPICLISTENER( image );
    DECLARE_TOPICINFOCHANGED( image );

};

#endif // _SERVER_H_
