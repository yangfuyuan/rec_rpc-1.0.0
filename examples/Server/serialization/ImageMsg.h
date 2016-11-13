#ifndef _IMAGEMSG_H_
#define _IMAGEMSG_H_

#include "rec/rpc/serialization/Complex.h"
#include "rec/rpc/serialization/Primitive.h"
#include "rec/rpc/serialization/Image.h"
#include "rec/rpc/Exception.h"

#include <QImage>

// Custom error codes.
enum ErrorCode
{
	DivisionByZero = rec::rpc::User,
};

DEFINE_PRIMITIVE_TOPICDATA( image, QImage );

#endif // _IMAGEMSG_H_
