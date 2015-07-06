// Copyright & License details are available under JXCORE_LICENSE file

#ifndef SRC_WRAPPERS_CRYPTO_EXTENSION_WRAP_H_
#define SRC_WRAPPERS_CRYPTO_EXTENSION_WRAP_H_

#include "jx/Proxy/JSEngine.h"
#include "jx/commons.h"

namespace node {

class CryptoExtensionWrap {
 private:
  static DEFINE_JS_METHOD(CreateBundle);
  static DEFINE_JS_METHOD(ExtractPublicKey);
  static DEFINE_JS_METHOD(GenerateHKDF);

 public:
  INIT_CLASS_MEMBERS() {
    SET_CLASS_METHOD("createBundle", CreateBundle, 0);
    SET_CLASS_METHOD("extractPublicKey", ExtractPublicKey, 0);
    SET_CLASS_METHOD("generateHKDF", GenerateHKDF, 0);
  }
  END_INIT_MEMBERS
};
}  // namespace node

#endif  // SRC_WRAPPERS_CRYPTO_EXTENSION_WRAP_H_

