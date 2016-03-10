// Copyright & License details are available under JXCORE_LICENSE file

#ifndef SRC_WRAPPERS_NODE_CRYPTO_H_
#define SRC_WRAPPERS_NODE_CRYPTO_H_

#include "node.h"

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <openssl/hmac.h>
#include <openssl/rand.h>
#include <openssl/pkcs12.h>

#ifdef OPENSSL_NPN_NEGOTIATED
#include "node_buffer.h"
#endif

#include "jx/extend.h"

#define EVP_F_EVP_DECRYPTFINAL 101

namespace node {

extern bool SSL2_ENABLE;
extern bool SSL3_ENABLE;

namespace crypto {

static X509_STORE* root_cert_store;

// Forward declaration
class Connection;

class SecureContext : ObjectWrap {
 public:
  SSL_CTX* ctx_;
  // TODO(?) ca_store_ should probably be removed, it's not used anywhere.
  X509_STORE* ca_store_;

 protected:
  static const int kMaxSessionSize = 10 * 1024;

  static DEFINE_JS_METHOD(New);
  static DEFINE_JS_METHOD(Init);
  static DEFINE_JS_METHOD(SetKey);
  static DEFINE_JS_METHOD(SetCert);
  static DEFINE_JS_METHOD(AddCACert);
  static DEFINE_JS_METHOD(AddCRL);
  static DEFINE_JS_METHOD(AddRootCerts);
  static DEFINE_JS_METHOD(SetCiphers);
  static DEFINE_JS_METHOD(SetOptions);
  static DEFINE_JS_METHOD(SetSessionIdContext);
  static DEFINE_JS_METHOD(Close);
  static DEFINE_JS_METHOD(LoadPKCS12);
  static DEFINE_JS_METHOD(SetPskHint);
  static DEFINE_JS_METHOD(SetPskServerCallback);


  static SSL_SESSION* GetSessionCallback(SSL* s, unsigned char* key, int len,
                                         int* copy);
  static int NewSessionCallback(SSL* s, SSL_SESSION* sess);

  SecureContext() : ObjectWrap() {
    ctx_ = NULL;
    ca_store_ = NULL;
  }

  void FreeCTXMem() {
    if (ctx_) {
      if (ctx_->cert_store == root_cert_store) {
        // SSL_CTX_free() will attempt to free the cert_store as well.
        // Since we want our root_cert_store to stay around forever
        // we just clear the field. Hopefully OpenSSL will not modify this
        // struct in future versions.
        ctx_->cert_store = NULL;
      }
      SSL_CTX_free(ctx_);
      ctx_ = NULL;
      ca_store_ = NULL;
    } else {
      assert(ca_store_ == NULL);
    }
  }

  ~SecureContext() { FreeCTXMem(); }

 private:
  JS_PERSISTENT_FUNCTION(psk_server_cb_);

  INIT_NAMED_CLASS_MEMBERS(SecureContext, SecureContext) {
    NODE_SET_PROTOTYPE_METHOD(constructor, "init", SecureContext::Init);
    NODE_SET_PROTOTYPE_METHOD(constructor, "setKey", SecureContext::SetKey);
    NODE_SET_PROTOTYPE_METHOD(constructor, "setCert", SecureContext::SetCert);
    NODE_SET_PROTOTYPE_METHOD(constructor, "addCACert",
                              SecureContext::AddCACert);
    NODE_SET_PROTOTYPE_METHOD(constructor, "addCRL", SecureContext::AddCRL);
    NODE_SET_PROTOTYPE_METHOD(constructor, "addRootCerts",
                              SecureContext::AddRootCerts);
    NODE_SET_PROTOTYPE_METHOD(constructor, "setCiphers",
                              SecureContext::SetCiphers);
    NODE_SET_PROTOTYPE_METHOD(constructor, "setOptions",
                              SecureContext::SetOptions);
    NODE_SET_PROTOTYPE_METHOD(constructor, "setSessionIdContext",
                              SecureContext::SetSessionIdContext);
    NODE_SET_PROTOTYPE_METHOD(constructor, "close", SecureContext::Close);
    NODE_SET_PROTOTYPE_METHOD(constructor, "loadPKCS12",
                              SecureContext::LoadPKCS12);

    JS_NEW_PERSISTENT_FUNCTION_TEMPLATE(com->secure_context_constructor,
                                        constructor);

    NODE_SET_PROTOTYPE_METHOD(constructor, "setPskHint", SecureContext::SetPskHint);
    NODE_SET_PROTOTYPE_METHOD(constructor, "setPskServerCallback", SecureContext::SetPskServerCallback);

  }
  END_INIT_NAMED_MEMBERS(SecureContext)


    static unsigned int PskServerCallback_(SSL *ssl, const char *identity,
      unsigned char *psk, unsigned int max_psk_len);

};

class ClientHelloParser {
 public:
  enum FrameType {
    kChangeCipherSpec = 20,
    kAlert = 21,
    kHandshake = 22,
    kApplicationData = 23,
    kOther = 255
  };

  enum HandshakeType { kClientHello = 1 };

  enum ParseState { kWaiting, kTLSHeader, kSSLHeader, kPaused, kEnded };

  explicit ClientHelloParser(Connection* c)
      : conn_(c), state_(kWaiting), offset_(0), body_offset_(0) {
    data_ = new uint8_t[kBufferSize];
    if (!data_) {
      error_console("out of memory at node_crypto.h (ClientHelloParser)\n");
      abort();
    }
  }

  ~ClientHelloParser() {
    if (data_) {
      delete[] data_;
      data_ = NULL;
    }
  }

  size_t Write(const uint8_t* data, size_t len);
  void Finish();

  inline bool ended() { return state_ == kEnded; }

 private:
  static const int kBufferSize = 18432;
  Connection* conn_;
  ParseState state_;
  size_t frame_len_;

  uint8_t* data_;
  size_t offset_;
  size_t body_offset_;
};

class Connection : ObjectWrap {
 public:
#ifdef OPENSSL_NPN_NEGOTIATED
  JS_PERSISTENT_OBJECT npnProtos_;
  JS_PERSISTENT_VALUE selectedNPNProto_;
#endif

#ifdef SSL_CTRL_SET_TLSEXT_SERVERNAME_CB
  JS_PERSISTENT_OBJECT sniObject_;
  JS_PERSISTENT_VALUE sniContext_;
  JS_PERSISTENT_STRING servername_;
#endif

 protected:
  static DEFINE_JS_METHOD(New);
  static DEFINE_JS_METHOD(EncIn);
  static DEFINE_JS_METHOD(ClearOut);
  static DEFINE_JS_METHOD(ClearPending);
  static DEFINE_JS_METHOD(EncPending);
  static DEFINE_JS_METHOD(EncOut);
  static DEFINE_JS_METHOD(ClearIn);
  static DEFINE_JS_METHOD(GetPeerCertificate);
  static DEFINE_JS_METHOD(GetSession);
  static DEFINE_JS_METHOD(SetSession);
  static DEFINE_JS_METHOD(LoadSession);
  static DEFINE_JS_METHOD(IsSessionReused);
  static DEFINE_JS_METHOD(IsInitFinished);
  static DEFINE_JS_METHOD(VerifyError);
  static DEFINE_JS_METHOD(GetCurrentCipher);
  static DEFINE_JS_METHOD(Shutdown);
  static DEFINE_JS_METHOD(Start);
  static DEFINE_JS_METHOD(Close);

  static void InitNPN(SecureContext* sc, bool is_server);

#ifdef OPENSSL_NPN_NEGOTIATED
  // NPN
  static DEFINE_JS_METHOD(GetNegotiatedProto);
  static DEFINE_JS_METHOD(SetNPNProtocols);
  static int AdvertiseNextProtoCallback_(SSL* s, const unsigned char** data,
                                         unsigned int* len, void* arg);
  static int SelectNextProtoCallback_(SSL* s, unsigned char** out,
                                      unsigned char* outlen,
                                      const unsigned char* in,
                                      unsigned int inlen, void* arg);
#endif

#ifdef SSL_CTRL_SET_TLSEXT_SERVERNAME_CB
  // SNI
  static DEFINE_JS_METHOD(GetServername);
  static DEFINE_JS_METHOD(SetSNICallback);
  static int SelectSNIContextCallback_(SSL* s, int* ad, void* arg);
#endif

  static DEFINE_JS_METHOD(SetPskClientCallback);
  static unsigned int PskClientCallback_(SSL *ssl, const char *hint, char *identity, unsigned int max_identity_len, unsigned char *psk, unsigned int max_psk_len);
  JS_PERSISTENT_FUNCTION(psk_client_cb_);

  int HandleBIOError(BIO* bio, const char* func, int rv);

  enum ZeroStatus { kZeroIsNotAnError, kZeroIsAnError };

  enum SyscallStatus { kIgnoreSyscall, kSyscallError };

  int HandleSSLError(const char* func, int rv, ZeroStatus zs, SyscallStatus ss);

  void ClearError();
  void SetShutdownFlags();

  static Connection* Unwrap(void* holder) {
    Connection* ss = static_cast<Connection*>(holder);
    ss->ClearError();
    return ss;
  }

  Connection() : ObjectWrap(), hello_parser_(this) {
    bio_read_ = bio_write_ = NULL;
    ssl_ = NULL;
    next_sess_ = NULL;
  }

  ~Connection() {
    if (ssl_ != NULL) {
      SSL_free(ssl_);
      ssl_ = NULL;
    }

    if (next_sess_ != NULL) {
      SSL_SESSION_free(next_sess_);
      next_sess_ = NULL;
    }

#ifdef OPENSSL_NPN_NEGOTIATED
    if (!npnProtos_.IsEmpty()) JS_CLEAR_PERSISTENT(npnProtos_);
    if (!selectedNPNProto_.IsEmpty()) JS_CLEAR_PERSISTENT(selectedNPNProto_);
#endif

#ifdef SSL_CTRL_SET_TLSEXT_SERVERNAME_CB
    if (!sniObject_.IsEmpty()) JS_CLEAR_PERSISTENT(sniObject_);
    if (!sniContext_.IsEmpty()) JS_CLEAR_PERSISTENT(sniContext_);
    if (!servername_.IsEmpty()) JS_CLEAR_PERSISTENT(servername_);
#endif
  }

 private:
  static void SSLInfoCallback(const SSL* ssl, int where, int ret);

  BIO* bio_read_;
  BIO* bio_write_;
  SSL* ssl_;

  ClientHelloParser hello_parser_;

  bool is_server_; /* coverity[member_decl] */
  SSL_SESSION* next_sess_;


  friend class ClientHelloParser;
  friend class SecureContext;

  INIT_NAMED_CLASS_MEMBERS(Connection, Connection) {
    NODE_SET_PROTOTYPE_METHOD(constructor, "encIn", Connection::EncIn);
    NODE_SET_PROTOTYPE_METHOD(constructor, "clearOut", Connection::ClearOut);
    NODE_SET_PROTOTYPE_METHOD(constructor, "clearIn", Connection::ClearIn);
    NODE_SET_PROTOTYPE_METHOD(constructor, "encOut", Connection::EncOut);
    NODE_SET_PROTOTYPE_METHOD(constructor, "clearPending",
                              Connection::ClearPending);
    NODE_SET_PROTOTYPE_METHOD(constructor, "encPending",
                              Connection::EncPending);
    NODE_SET_PROTOTYPE_METHOD(constructor, "getPeerCertificate",
                              Connection::GetPeerCertificate);
    NODE_SET_PROTOTYPE_METHOD(constructor, "getSession",
                              Connection::GetSession);
    NODE_SET_PROTOTYPE_METHOD(constructor, "setSession",
                              Connection::SetSession);
    NODE_SET_PROTOTYPE_METHOD(constructor, "loadSession",
                              Connection::LoadSession);
    NODE_SET_PROTOTYPE_METHOD(constructor, "isSessionReused",
                              Connection::IsSessionReused);
    NODE_SET_PROTOTYPE_METHOD(constructor, "isInitFinished",
                              Connection::IsInitFinished);
    NODE_SET_PROTOTYPE_METHOD(constructor, "verifyError",
                              Connection::VerifyError);
    NODE_SET_PROTOTYPE_METHOD(constructor, "getCurrentCipher",
                              Connection::GetCurrentCipher);
    NODE_SET_PROTOTYPE_METHOD(constructor, "start", Connection::Start);
    NODE_SET_PROTOTYPE_METHOD(constructor, "shutdown", Connection::Shutdown);
    NODE_SET_PROTOTYPE_METHOD(constructor, "close", Connection::Close);

#ifdef OPENSSL_NPN_NEGOTIATED
    NODE_SET_PROTOTYPE_METHOD(constructor, "getNegotiatedProtocol",
                              Connection::GetNegotiatedProto);
    NODE_SET_PROTOTYPE_METHOD(constructor, "setNPNProtocols",
                              Connection::SetNPNProtocols);
#endif

#ifdef SSL_CTRL_SET_TLSEXT_SERVERNAME_CB
    NODE_SET_PROTOTYPE_METHOD(constructor, "getServername",
                              Connection::GetServername);
    NODE_SET_PROTOTYPE_METHOD(constructor, "setSNICallback",
                              Connection::SetSNICallback);
#endif

    NODE_SET_PROTOTYPE_METHOD(constructor, "setPskClientCallback", Connection::SetPskClientCallback);
  }
  END_INIT_NAMED_MEMBERS(Connection)
};

bool EntropySource(unsigned char* buffer, size_t length);
DECLARE_CLASS_INITIALIZER(InitCrypto);

}  // namespace crypto
}  // namespace node

#endif  // SRC_WRAPPERS_NODE_CRYPTO_H_
