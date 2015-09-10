// Copyright & License details are available under JXCORE_LICENSE file

#include "Exception.h"

#define CAST_ERROR()                                 \
  is_exception_ = true;                              \
  JS::RootedValue _exc(err.GetContext(), err.GetRawValue()); \
  JS_SetPendingException(err.GetContext(), _exc);

namespace MozJS {
ThrowException::ThrowException(Exception::Error err) { CAST_ERROR() }

ThrowException::ThrowException(Exception::TypeError err) { CAST_ERROR() }

ThrowException::ThrowException(Exception::RangeError err) { CAST_ERROR() }

ThrowException::ThrowException(MozJS::Value err) { CAST_ERROR() }
}
