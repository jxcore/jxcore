// Copyright & License details are available under JXCORE_LICENSE file
#include "../commons/common-posix.h"
#include "openssl/ssl.h"
#include "openssl/err.h"
#include "openssl/evp.h"
#include "openssl/pem.h"
#include "openssl/x509.h"
#include "openssl/x509v3.h"
#include "openssl/hmac.h"
#include "openssl/rand.h"
#include "openssl/pkcs12.h"

void callback(JXValue *results, int argc) {
  // do nothing
}


void sampleMethod(JXValue *params, int argc) {
  if (!JX_IsInt32(params+0)) {
    JX_SetError(params+argc, "Integer parameter is expected");
    return;
  }

  int param = JX_GetInt32(params+0);

  if (param != 256 && param != 512 && param !=1024) {
    JX_SetError(params+argc, "param has to be one of 256, 512, 1024");
    return;
  }

  RSA *rsaKeyPair = NULL;
  BIGNUM *e = NULL;

  rsaKeyPair = RSA_new();
  e = BN_new();

  BN_set_word(e, 65537);

  int ret = RSA_generate_key_ex(rsaKeyPair, param, e, NULL); //use 4096 for 4K

  JX_SetInt32(params+argc, ret);
}

void crashMe(JXValue *_, int argc) {
  assert(0 && "previous call to sampleMethod must be failed");
}

const char *contents =
    "var result = process.natives.sampleMethod(256);"
    "if (result != 1) process.natives.crashMe();";

int main(int argc, char **args) {
  JX_Initialize(args[0], callback);
  JX_InitializeNewEngine();

  JX_DefineMainFile(contents);
  JX_DefineExtension("crashMe", crashMe);
  JX_DefineExtension("sampleMethod", sampleMethod);
  JX_StartEngine();

  while (JX_LoopOnce() != 0) usleep(1);

  JX_StopEngine();
}
