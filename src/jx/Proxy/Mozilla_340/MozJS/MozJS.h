// Copyright & License details are available under JXCORE_LICENSE file

#ifndef SRC_JX_PROXY_MOZILLA_MOZJS_MOZJS_H_
#define SRC_JX_PROXY_MOZILLA_MOZJS_MOZJS_H_

#include "Isolate.h"
#include "MozValue.h"
#include "TryCatch.h"
#include "Exception.h"

namespace MozJS {

enum ExternalArrayType {
  kExternalByteArray = 1,
  kExternalUnsignedByteArray = 2,
  kExternalShortArray = 3,
  kExternalUnsignedShortArray = 4,
  kExternalIntArray = 5,
  kExternalUnsignedIntArray = 6,
  kExternalFloatArray = 7,
  kExternalDoubleArray = 8,
  kExternalPixelArray = 9
};

}  // namespace MozJS

#endif  // SRC_JX_PROXY_MOZILLA_MOZJS_MOZJS_H_
