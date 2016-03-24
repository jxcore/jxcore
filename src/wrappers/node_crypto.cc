// Copyright & License details are available under JXCORE_LICENSE file

#include "node_crypto.h"
#include "node_crypto_groups.h"
#include "node.h"
#include "node_buffer.h"
#include "string_bytes.h"
#include <string.h>
#ifdef _MSC_VER
#define strcasecmp _stricmp
#endif

#include <stdlib.h>
#include <errno.h>

#if OPENSSL_VERSION_NUMBER >= 0x10000000L
#define OPENSSL_CONST const
#else
#define OPENSSL_CONST
#endif

#define ASSERT_IS_STRING_OR_BUFFER(val)                         \
  if (!Buffer::jxHasInstance(val, com) && !JS_IS_STRING(val)) { \
    THROW_TYPE_EXCEPTION("Not a string or buffer");             \
  }

#define ASSERT_IS_BUFFER(val)             \
  if (!Buffer::jxHasInstance(val, com)) { \
    THROW_TYPE_EXCEPTION("Not a buffer"); \
  }

static const char PUBLIC_KEY_PFX[] = "-----BEGIN PUBLIC KEY-----";
static const int PUBLIC_KEY_PFX_LEN = sizeof(PUBLIC_KEY_PFX) - 1;
static const char PUBRSA_KEY_PFX[] = "-----BEGIN RSA PUBLIC KEY-----";
static const int PUBRSA_KEY_PFX_LEN = sizeof(PUBRSA_KEY_PFX) - 1;
static const char CERTIFICATE_PFX[] = "-----BEGIN CERTIFICATE-----";
static const int CERTIFICATE_PFX_LEN = sizeof(CERTIFICATE_PFX) - 1;
static const int X509_NAME_FLAGS = ASN1_STRFLGS_ESC_CTRL |
                                   ASN1_STRFLGS_ESC_MSB |
                                   XN_FLAG_SEP_MULTILINE | XN_FLAG_FN_SN;

namespace node {

const char* root_certs[] = {
#include "node_root_certs.h"  // NOLINT(build/include_order)
    NULL};

bool SSL2_ENABLE = false;
bool SSL3_ENABLE = false;

namespace crypto {

// Forcibly clear OpenSSL's error stack on return. This stops stale errors
// from popping up later in the lifecycle of crypto operations where they
// would cause spurious failures. It's a rather blunt method, though.
// ERR_clear_error() isn't necessarily cheap either.
struct ClearErrorOnReturn {
  ~ClearErrorOnReturn() { ERR_clear_error(); }
};

static uv_rwlock_t* locks;

static void crypto_threadid_cb(CRYPTO_THREADID* tid) {
  CRYPTO_THREADID_set_numeric(tid, uv_thread_self());
}

static void crypto_lock_init(void) {
  int i, n;

  n = CRYPTO_num_locks();
  locks = new uv_rwlock_t[n];

  for (i = 0; i < n; i++)
    if (uv_rwlock_init(locks + i)) {
      error_console("rwlock initialization is failed. (node_crypto.cc)\n");
      abort();
    }
}

static void crypto_lock_cb(int mode, int n, const char* file, int line) {
  assert((mode & CRYPTO_LOCK) || (mode & CRYPTO_UNLOCK));
  assert((mode & CRYPTO_READ) || (mode & CRYPTO_WRITE));

  if (mode & CRYPTO_LOCK) {
    if (mode & CRYPTO_READ)
      uv_rwlock_rdlock(locks + n);
    else
      uv_rwlock_wrlock(locks + n);
  } else {
    if (mode & CRYPTO_READ)
      uv_rwlock_rdunlock(locks + n);
    else
      uv_rwlock_wrunlock(locks + n);
  }
}

static int CryptoPemCallback(char *buf, int size, int rwflag, void *u) {
  if (u) {
    size_t buflen = static_cast<size_t>(size);
    size_t len = strlen(static_cast<const char*>(u));
    len = len > buflen ? buflen : len;
    memcpy(buf, u, len);
    return len;
  }

  return 0;
}

#define ThrowCryptoErrorHelper(err, is_type_error)   \
  do {                                               \
    char errmsg[128];                                \
    ERR_error_string_n(err, errmsg, sizeof(errmsg)); \
    if (is_type_error)                               \
      THROW_TYPE_EXCEPTION(errmsg);                  \
    else                                             \
      THROW_EXCEPTION(errmsg);                       \
  } while (0)

#define ThrowCryptoError(err) ThrowCryptoErrorHelper(err, false)

#define ThrowCryptoTypeError(err) ThrowCryptoErrorHelper(err, true)

// Ensure that OpenSSL has enough entropy (at least 256 bits) for its PRNG.
// The entropy pool starts out empty and needs to fill up before the PRNG
// can be used securely.  Once the pool is filled, it never dries up again;
// its contents is stirred and reused when necessary.
//
// OpenSSL normally fills the pool automatically but not when someone starts
// generating random numbers before the pool is full: in that case OpenSSL
// keeps lowering the entropy estimate to thwart attackers trying to guess
// the initial state of the PRNG.
//
// When that happens, we will have to wait until enough entropy is available.
// That should normally never take longer than a few milliseconds.
//
// OpenSSL draws from /dev/random and /dev/urandom.  While /dev/random may
// block pending "true" randomness, /dev/urandom is a CSPRNG that doesn't
// block under normal circumstances.
//
// The only time when /dev/urandom may conceivably block is right after boot,
// when the whole system is still low on entropy.  That's not something we can
// do anything about.
inline void CheckEntropy() {
  for (;;) {
    int status = RAND_status();
    assert(status >= 0);  // Cannot fail.
    if (status != 0) break;
    if (RAND_poll() == 0)  // Give up, RAND_poll() not supported.
      break;
  }
}

bool EntropySource(unsigned char* buffer, size_t length) {
  // Ensure that OpenSSL's PRNG is properly seeded.
  CheckEntropy();
  // RAND_bytes() can return 0 to indicate that the entropy data is not truly
  // random. That's okay, it's still better than V8's stock source of entropy,
  // which is /dev/urandom on UNIX platforms and the current time on Windows.
  return RAND_bytes(buffer, length) != -1;
}

JS_METHOD(SecureContext, New) {
  SecureContext* p = new SecureContext();
  JS_CLASS_NEW_INSTANCE(obj, SecureContext);
  p->Wrap(obj);
  RETURN_POINTER(obj);
}
JS_METHOD_END

JS_METHOD(SecureContext, Init) {
  SecureContext* sc = ObjectWrap::Unwrap<SecureContext>(args.Holder());

  OPENSSL_CONST SSL_METHOD* method = SSLv23_method();

  if (args.Length() == 1 && args.IsString(0)) {
    jxcore::JXString sslmethod;
    args.GetString(0, &sslmethod);

    if (strcmp(*sslmethod, "SSLv2_method") == 0) {
#ifndef OPENSSL_NO_SSL2
      method = SSLv2_method();
#else
      THROW_EXCEPTION("SSLv2 methods disabled");
#endif
    } else if (strcmp(*sslmethod, "SSLv2_server_method") == 0) {
#ifndef OPENSSL_NO_SSL2
      method = SSLv2_server_method();
#else
      THROW_EXCEPTION("SSLv2 methods disabled");
#endif
    } else if (strcmp(*sslmethod, "SSLv2_client_method") == 0) {
#ifndef OPENSSL_NO_SSL2
      method = SSLv2_client_method();
#else
      THROW_EXCEPTION("SSLv2 methods disabled");
#endif
    } else if (strcmp(*sslmethod, "SSLv3_method") == 0) {
#ifndef OPENSSL_NO_SSL3
      method = SSLv3_method();
#else
      THROW_EXCEPTION("SSLv3 methods are disabled");
#endif
    } else if (strcmp(*sslmethod, "SSLv3_server_method") == 0) {
#ifndef OPENSSL_NO_SSL3
      method = SSLv3_server_method();
#else
      THROW_EXCEPTION("SSLv3 methods are disabled");
#endif
    } else if (strcmp(*sslmethod, "SSLv3_client_method") == 0) {
#ifndef OPENSSL_NO_SSL3
      method = SSLv3_client_method();
#else
      THROW_EXCEPTION("SSLv3 methods are disabled");
#endif
    } else if (strcmp(*sslmethod, "SSLv23_method") == 0) {
      method = SSLv23_method();
    } else if (strcmp(*sslmethod, "SSLv23_server_method") == 0) {
      method = SSLv23_server_method();
    } else if (strcmp(*sslmethod, "SSLv23_client_method") == 0) {
      method = SSLv23_client_method();
    } else if (strcmp(*sslmethod, "TLSv1_method") == 0) {
      method = TLSv1_method();
    } else if (strcmp(*sslmethod, "TLSv1_server_method") == 0) {
      method = TLSv1_server_method();
    } else if (strcmp(*sslmethod, "TLSv1_client_method") == 0) {
      method = TLSv1_client_method();
    } else {
      THROW_EXCEPTION("Unknown method");
    }
  }

  sc->ctx_ = SSL_CTX_new(method);

  // SSL session cache configuration
  SSL_CTX_set_session_cache_mode(sc->ctx_, SSL_SESS_CACHE_SERVER |
                                               SSL_SESS_CACHE_NO_INTERNAL |
                                               SSL_SESS_CACHE_NO_AUTO_CLEAR);
  SSL_CTX_sess_set_get_cb(sc->ctx_, GetSessionCallback);
  SSL_CTX_sess_set_new_cb(sc->ctx_, NewSessionCallback);

  sc->ca_store_ = NULL;

  // Establish a back link from the SSL_CTX struct to the SecureContext object
  // using OpenSSL's application-specific data storage. Used by PSK support.
  SSL_CTX_set_app_data(sc->ctx_, sc);

  RETURN_TRUE();
}
JS_METHOD_END

SSL_SESSION* SecureContext::GetSessionCallback(SSL* s, unsigned char* key,
                                               int len, int* copy) {
  JS_ENTER_SCOPE();

  Connection* p = static_cast<Connection*>(SSL_get_app_data(s));

  *copy = 0;
  SSL_SESSION* sess = p->next_sess_;
  p->next_sess_ = NULL;

  return sess;
}

void SessionDataFree(char* data, void* hint) { delete[] data; }

int SecureContext::NewSessionCallback(SSL* s, SSL_SESSION* sess) {
  JS_ENTER_SCOPE_COM();

  Connection* p = static_cast<Connection*>(SSL_get_app_data(s));

  // Check if session is small enough to be stored
  int size = i2d_SSL_SESSION(sess, NULL);
  if (size > kMaxSessionSize) return 0;

  // Serialize session
  char* serialized = new char[size];
  unsigned char* pserialized = reinterpret_cast<unsigned char*>(serialized);
  memset(serialized, 0, size);
  i2d_SSL_SESSION(sess, &pserialized);

  JS_HANDLE_VALUE argv[2] = {
      JS_OBJECT_FROM_PERSISTENT(
          Buffer::New(reinterpret_cast<char*>(sess->session_id),
                      sess->session_id_length, com)->handle_),
      JS_OBJECT_FROM_PERSISTENT(
          Buffer::New(serialized, size, SessionDataFree, NULL, com)->handle_)};

  MakeCallback(com, JS_OBJECT_FROM_PERSISTENT(p->handle_),
               JS_PREDEFINED_STRING(onnewsession), ARRAY_SIZE(argv), argv);

  return 0;
}

// Takes a string or buffer and loads it into a BIO.
// Caller responsible for BIO_free-ing the returned object.
static BIO* LoadBIO(JS_HANDLE_VALUE v) {
  BIO* bio = BIO_new(BIO_s_mem());
  if (!bio) return NULL;

  JS_ENTER_SCOPE_COM();

  int r = -1;

  if (JS_IS_STRING(v)) {
    jxcore::JXString s(v);
    r = BIO_write(bio, *s, s.length());
  } else if (Buffer::jxHasInstance(v, com)) {
    char* buffer_data = BUFFER__DATA(v);
    size_t buffer_length = BUFFER__LENGTH(v);
    r = BIO_write(bio, buffer_data, buffer_length);
  }

  if (r <= 0) {
    BIO_free(bio);
    return NULL;
  }

  return bio;
}

// Takes a string or buffer and loads it into an X509
// Caller responsible for X509_free-ing the returned object.
static X509* LoadX509(JS_HANDLE_VALUE v) {
  JS_ENTER_SCOPE();  // necessary?

  BIO* bio = LoadBIO(v);
  if (!bio) return NULL;

  X509* x509 = PEM_read_bio_X509(bio, NULL, NULL, NULL);
  if (!x509) {
    BIO_free(bio);
    return NULL;
  }

  BIO_free(bio);
  return x509;
}

JS_METHOD(SecureContext, SetKey) {
  SecureContext* sc = ObjectWrap::Unwrap<SecureContext>(args.Holder());

  unsigned int len = args.Length();
  if (len != 1 && len != 2) {
    THROW_TYPE_EXCEPTION("Bad parameter");
  }
  if (len == 2 && !args.IsString(1)) {
    THROW_TYPE_EXCEPTION("Bad parameter");
  }

  BIO* bio = LoadBIO(GET_ARG(0));
  if (!bio) RETURN_FALSE();

  jxcore::JXString passphrase;
  args.GetString(1, &passphrase);

  EVP_PKEY* key =
      PEM_read_bio_PrivateKey(bio, NULL, NULL, len == 1 ? NULL : *passphrase);

  if (!key) {
    BIO_free(bio);
    unsigned long err = ERR_get_error();
    if (!err) {
      THROW_EXCEPTION("PEM_read_bio_PrivateKey");
    }
    ThrowCryptoError(err);
  }

  SSL_CTX_use_PrivateKey(sc->ctx_, key);
  EVP_PKEY_free(key);
  BIO_free(bio);

  RETURN_TRUE();
}
JS_METHOD_END

// Read a file that contains our certificate in "PEM" format,
// possibly followed by a sequence of CA certificates that should be
// sent to the peer in the Certificate message.
//
// Taken from OpenSSL - editted for style.
int SSL_CTX_use_certificate_chain(SSL_CTX* ctx, BIO* in) {
  int ret = 0;
  X509* x = NULL;

  x = PEM_read_bio_X509_AUX(in, NULL, NULL, NULL);

  if (x == NULL) {
    SSLerr(SSL_F_SSL_CTX_USE_CERTIFICATE_CHAIN_FILE, ERR_R_PEM_LIB);
    goto end;
  }

  ret = SSL_CTX_use_certificate(ctx, x);

  if (ERR_peek_error() != 0) {
    // Key/certificate mismatch doesn't imply ret==0 ...
    ret = 0;
  }

  if (ret) {
    // If we could set up our certificate, now proceed to
    // the CA certificates.
    X509* ca;
    int r;
    unsigned long err;

    if (ctx->extra_certs != NULL) {
      sk_X509_pop_free(ctx->extra_certs, X509_free);
      ctx->extra_certs = NULL;
    }

    while ((ca = PEM_read_bio_X509(in, NULL, NULL, NULL))) {
      r = SSL_CTX_add_extra_chain_cert(ctx, ca);

      if (!r) {
        X509_free(ca);
        ret = 0;
        goto end;
      }
      // Note that we must not free r if it was successfully
      // added to the chain (while we must free the main
      // certificate, since its reference count is increased
      // by SSL_CTX_use_certificate).
    }

    // When the while loop ends, it's usually just EOF.
    err = ERR_peek_last_error();
    if (ERR_GET_LIB(err) == ERR_LIB_PEM &&
        ERR_GET_REASON(err) == PEM_R_NO_START_LINE) {
      ERR_clear_error();
    } else {
      // some real error
      ret = 0;
    }
  }

end:
  if (x != NULL) X509_free(x);
  return ret;
}

JS_METHOD(SecureContext, SetCert) {
  SecureContext* sc = ObjectWrap::Unwrap<SecureContext>(args.Holder());

  if (args.Length() != 1) {
    THROW_TYPE_EXCEPTION("Bad parameter");
  }

  BIO* bio = LoadBIO(GET_ARG(0));
  if (!bio) RETURN_FALSE();

  int rv = SSL_CTX_use_certificate_chain(sc->ctx_, bio);

  BIO_free(bio);

  if (!rv) {
    unsigned long err = ERR_get_error();
    if (!err) {
      THROW_EXCEPTION("SSL_CTX_use_certificate_chain");
    }
    ThrowCryptoError(err);
  }

  RETURN_TRUE();
}
JS_METHOD_END

JS_METHOD(SecureContext, AddCACert) {
  bool newCAStore = false;

  SecureContext* sc = ObjectWrap::Unwrap<SecureContext>(args.Holder());

  if (args.Length() != 1) {
    THROW_TYPE_EXCEPTION("Bad parameter");
  }

  if (!sc->ca_store_) {
    sc->ca_store_ = X509_STORE_new();
    newCAStore = true;
  }

  X509* x509 = LoadX509(GET_ARG(0));
  if (!x509) RETURN_FALSE();

  X509_STORE_add_cert(sc->ca_store_, x509);
  SSL_CTX_add_client_CA(sc->ctx_, x509);

  X509_free(x509);

  if (newCAStore) {
    SSL_CTX_set_cert_store(sc->ctx_, sc->ca_store_);
  }

  RETURN_TRUE();
}
JS_METHOD_END

JS_METHOD(SecureContext, AddCRL) {
  SecureContext* sc = ObjectWrap::Unwrap<SecureContext>(args.Holder());

  if (args.Length() != 1) {
    THROW_TYPE_EXCEPTION("Bad parameter");
  }

  BIO* bio = LoadBIO(GET_ARG(0));
  if (!bio) RETURN_FALSE();

  X509_CRL* x509 = PEM_read_bio_X509_CRL(bio, NULL, NULL, NULL);

  if (x509 == NULL) {
    BIO_free(bio);
    RETURN_FALSE();
  }

  X509_STORE_add_crl(sc->ca_store_, x509);

  X509_STORE_set_flags(sc->ca_store_,
                       X509_V_FLAG_CRL_CHECK | X509_V_FLAG_CRL_CHECK_ALL);

  BIO_free(bio);
  X509_CRL_free(x509);

  RETURN_TRUE();
}
JS_METHOD_END

JS_METHOD(SecureContext, AddRootCerts) {
  SecureContext* sc = ObjectWrap::Unwrap<SecureContext>(args.Holder());

  assert(sc->ca_store_ == NULL);

  if (!root_cert_store) {
    root_cert_store = X509_STORE_new();

    for (int i = 0; root_certs[i]; i++) {
      BIO* bp = BIO_new(BIO_s_mem());

      if (!BIO_write(bp, root_certs[i], strlen(root_certs[i]))) {
        BIO_free(bp);
        RETURN_FALSE();
      }

      X509* x509 = PEM_read_bio_X509(bp, NULL, NULL, NULL);

      if (x509 == NULL) {
        BIO_free(bp);
        RETURN_FALSE();
      }

      X509_STORE_add_cert(root_cert_store, x509);

      BIO_free(bp);
      X509_free(x509);
    }
  }

  sc->ca_store_ = root_cert_store;
  SSL_CTX_set_cert_store(sc->ctx_, sc->ca_store_);

  RETURN_TRUE();
}
JS_METHOD_END

JS_METHOD(SecureContext, SetCiphers) {
  SecureContext* sc = ObjectWrap::Unwrap<SecureContext>(args.Holder());

  if (args.Length() != 1 || !args.IsString(0)) {
    THROW_TYPE_EXCEPTION("Bad parameter");
  }

  jxcore::JXString ciphers;
  args.GetString(0, &ciphers);
  SSL_CTX_set_cipher_list(sc->ctx_, *ciphers);

  RETURN_TRUE();
}
JS_METHOD_END

JS_METHOD(SecureContext, SetOptions) {
  SecureContext* sc = ObjectWrap::Unwrap<SecureContext>(args.Holder());

  if (args.Length() != 1 && !args.IsUnsigned(0)) {
    THROW_TYPE_EXCEPTION("Bad parameter");
  }

  SSL_CTX_set_options(sc->ctx_, args.GetInteger(0));

  RETURN_TRUE();
}
JS_METHOD_END

JS_METHOD(SecureContext, SetSessionIdContext) {
  SecureContext* sc = ObjectWrap::Unwrap<SecureContext>(args.Holder());

  if (args.Length() != 1 || !args.IsString(0)) {
    THROW_TYPE_EXCEPTION("Bad parameter");
  }

  jxcore::JXString sessionIdContext;
  args.GetString(0, &sessionIdContext);

  const unsigned char* sid_ctx = (const unsigned char*)*sessionIdContext;
  unsigned int sid_ctx_len = sessionIdContext.length();

  int r = SSL_CTX_set_session_id_context(sc->ctx_, sid_ctx, sid_ctx_len);
  if (r != 1) {
    std::string message;
    BIO* bio;
    BUF_MEM* mem;
    if ((bio = BIO_new(BIO_s_mem()))) {
      ERR_print_errors(bio);
      BIO_get_mem_ptr(bio, &mem);
      message = std::string(mem->data, mem->length);
      BIO_free(bio);
    } else {
      message = "SSL_CTX_set_session_id_context error";
    }
    THROW_TYPE_EXCEPTION(message.c_str());
  }

  RETURN_TRUE();
}
JS_METHOD_END

JS_METHOD(SecureContext, Close) {
  SecureContext* sc = ObjectWrap::Unwrap<SecureContext>(args.Holder());
  sc->FreeCTXMem();
  RETURN_FALSE();
}
JS_METHOD_END

// Takes .pfx or .p12 and password in string or buffer format
JS_METHOD(SecureContext, LoadPKCS12) {
  BIO* in = NULL;
  PKCS12* p12 = NULL;
  EVP_PKEY* pkey = NULL;
  X509* cert = NULL;
  STACK_OF(X509)* extraCerts = NULL;
  char* pass = NULL;
  bool ret = false;

  SecureContext* sc = ObjectWrap::Unwrap<SecureContext>(args.Holder());

  if (args.Length() < 1) {
    THROW_TYPE_EXCEPTION("Bad parameter");
  }

  in = LoadBIO(GET_ARG(0));
  if (in == NULL) {
    THROW_EXCEPTION("Unable to load BIO");
  }

  if (args.Length() >= 2) {
    ASSERT_IS_BUFFER(GET_ARG(1));

    int passlen = BUFFER__LENGTH(GET_ARG(1));
    if (passlen < 0) {
      BIO_free(in);
      THROW_TYPE_EXCEPTION("Bad password");
    }
    pass = new char[passlen + 1];
    int pass_written = DecodeWrite(pass, passlen, GET_ARG(1), BINARY);

    assert(pass_written == passlen);
    pass[passlen] = '\0';
  }

  if (d2i_PKCS12_bio(in, &p12) &&
      PKCS12_parse(p12, pass, &pkey, &cert, &extraCerts) &&
      SSL_CTX_use_certificate(sc->ctx_, cert) &&
      SSL_CTX_use_PrivateKey(sc->ctx_, pkey)) {
    // set extra certs
    while (X509* x509 = sk_X509_pop(extraCerts)) {
      if (!sc->ca_store_) {
        sc->ca_store_ = X509_STORE_new();
        SSL_CTX_set_cert_store(sc->ctx_, sc->ca_store_);
      }

      X509_STORE_add_cert(sc->ca_store_, x509);
      SSL_CTX_add_client_CA(sc->ctx_, x509);
      X509_free(x509);
    }

    EVP_PKEY_free(pkey);
    X509_free(cert);
    sk_X509_free(extraCerts);

    ret = true;
  }

  PKCS12_free(p12);
  BIO_free(in);
  delete[] pass;

  if (!ret) {
    unsigned long err = ERR_get_error();
    const char* str = ERR_reason_error_string(err);
    THROW_EXCEPTION(str);
  }

  RETURN_TRUE();
}
JS_METHOD_END

JS_METHOD(SecureContext, SetPskHint) {
  SecureContext* sc = ObjectWrap::Unwrap<SecureContext>(args.Holder());

  if (args.Length() != 1 || !args.IsString(0)) {
    THROW_TYPE_EXCEPTION("Bad parameter");
  }

  jxcore::JXString str1(args.GetAsString(0));

  if (SSL_CTX_use_psk_identity_hint(sc->ctx_, *str1))
    RETURN_TRUE();
  else
    RETURN_FALSE();

}
JS_METHOD_END


JS_METHOD(SecureContext, SetPskServerCallback) {

  JS_LOCAL_VALUE _item = GET_ARG(0);
  if (args.Length() < 1 || !JS_IS_FUNCTION(_item)) {
    THROW_TYPE_EXCEPTION("Must give a Function as first argument");
  }

  SecureContext* sc = ObjectWrap::Unwrap<SecureContext>(args.Holder());

  JS_LOCAL_FUNCTION fnc = TO_LOCAL_FUNCTION(args.GetAsFunction(0));
  JS_NEW_PERSISTENT_FUNCTION(sc->psk_server_cb_, fnc);
  
  SSL_CTX_set_psk_server_callback(sc->ctx_, SecureContext::PskServerCallback_);

  RETURN_TRUE();
}
JS_METHOD_END

unsigned int SecureContext::PskServerCallback_(SSL *ssl, const char *identity, unsigned char *psk, unsigned int max_psk_len) {
  node::commons* com = node::commons::getInstance();
  JS_DEFINE_STATE_MARKER(com);

  // translate back from the connection to the context
  SSL_CTX *ctx = SSL_get_SSL_CTX(ssl);

  // then from the SSL_CTX to the SecureContext
  SecureContext *sc = static_cast<SecureContext*>(SSL_CTX_get_app_data(ctx));

  JS_LOCAL_VALUE(argv[1]);

  argv[0] = STD_TO_STRING(identity);

  JS_LOCAL_FUNCTION jlf = JS_TYPE_TO_LOCAL_FUNCTION(sc->psk_server_cb_);
  
  JS_LOCAL_VALUE(result) = jlf->Call(JS_GET_GLOBAL(), 1, argv);

  //// The result is expected to be a buffer containing the key. If this
  //// isn't the case then return 0, indicating that the identity isn't found.
  if (Buffer::HasInstance(result)) {
    // write the key into the buffer provided
    JS_LOCAL_OBJECT(keyBuffer) = JS_VALUE_TO_OBJECT(result);
    int len = Buffer::Length(keyBuffer);
    if (len <= max_psk_len) {
      memcpy(psk, Buffer::Data(keyBuffer), len);
      return len;
    }
 }
  return 0;
}


size_t ClientHelloParser::Write(const uint8_t* data, size_t len) {
  JS_ENTER_SCOPE_COM();
  JS_DEFINE_STATE_MARKER(com);

  // Just accumulate data, everything will be pushed to BIO later
  if (state_ == kPaused) return 0;

  // Copy incoming data to the internal buffer
  // (which has a size of the biggest possible TLS frame)
  size_t available = kBufferSize - offset_;
  size_t copied = len < available ? len : available;
  memcpy(data_ + offset_, data, copied);
  offset_ += copied;

  // Vars for parsing hello
  bool is_clienthello = false;
  uint8_t session_size = -1;
  uint8_t* session_id = NULL;
  JS_LOCAL_OBJECT hello;
  JS_HANDLE_VALUE argv[1];

  switch (state_) {
    case kWaiting: {
      // >= 5 bytes for header parsing
      if (offset_ < 5) break;

      if (data_[0] == kChangeCipherSpec || data_[0] == kAlert ||
          data_[0] == kHandshake || data_[0] == kApplicationData) {
        frame_len_ = (data_[3] << 8) + data_[4];
        state_ = kTLSHeader;
        body_offset_ = 5;
      } else {
        frame_len_ = (data_[0] << 8) + data_[1];
        state_ = kSSLHeader;
        if (*data_ & 0x40) {
          // header with padding
          body_offset_ = 3;
        } else {
          // without padding
          body_offset_ = 2;
        }
      }

      // Sanity check (too big frame, or too small)
      if (frame_len_ >= kBufferSize) {
        // Let OpenSSL handle it
        Finish();
        return copied;
      }
    }
    case kTLSHeader:
    case kSSLHeader: {
      // >= 5 + frame size bytes for frame parsing
      if (offset_ < body_offset_ + frame_len_) break;

      // Skip unsupported frames and gather some data from frame

      if (data_[body_offset_] == kClientHello) {
        is_clienthello = true;
        uint8_t* body;
        size_t session_offset;

        if (state_ == kTLSHeader) {
          // Skip frame header, hello header, protocol version and random data
          session_offset = body_offset_ + 4 + 2 + 32;

          if (session_offset + 1 < offset_) {
            body = data_ + session_offset;
            session_size = *body;
            session_id = body + 1;
          }
        } else if (state_ == kSSLHeader) {
          // Skip header, version
          session_offset = body_offset_ + 3;

          if (session_offset + 4 < offset_) {
            body = data_ + session_offset;

            int ciphers_size = (body[0] << 8) + body[1];

            if (body + 4 + ciphers_size < data_ + offset_) {
              session_size = (body[2] << 8) + body[3];
              session_id = body + 4 + ciphers_size;
            }
          }
        } else {
          // Whoa? How did we get here?
          abort();
        }

        // Check if we overflowed (do not reply with any private data)
        if (session_id == NULL || session_size > 32 ||
            session_id + session_size > data_ + offset_) {
          Finish();
          return copied;
        }

        // TODO(?) Parse other things?
      }

      // Not client hello - let OpenSSL handle it
      if (!is_clienthello) {
        Finish();
        return copied;
      }

      state_ = kPaused;
      hello = JS_NEW_EMPTY_OBJECT();
      JS_LOCAL_OBJECT objl = JS_OBJECT_FROM_PERSISTENT(
          Buffer::New(reinterpret_cast<char*>(session_id), session_size, com)
              ->handle_);
      JS_NAME_SET(hello, JS_PREDEFINED_STRING(sessionId), objl);

      argv[0] = hello;
      JS_LOCAL_OBJECT objk = JS_OBJECT_FROM_PERSISTENT(conn_->handle_);
      MakeCallback(com, objk, JS_PREDEFINED_STRING(onclienthello), 1, argv);

    } break;

    default:
      break;
  }

  return copied;
}

void ClientHelloParser::Finish() {
  assert(state_ != kEnded);
  state_ = kEnded;

  // Write all accumulated data
  int r = BIO_write(conn_->bio_read_, reinterpret_cast<char*>(data_), offset_);
  conn_->HandleBIOError(conn_->bio_read_, "BIO_write", r);
  conn_->SetShutdownFlags();

  delete[] data_;
  data_ = NULL;
}

#ifdef SSL_PRINT_DEBUG
#define DEBUG_PRINT(...) fprintf(stderr, __VA_ARGS__)
#else
#define DEBUG_PRINT(...)
#endif

int Connection::HandleBIOError(BIO* bio, const char* func, int rv) {
  if (rv >= 0) return rv;

  int retry = BIO_should_retry(bio);
  (void)retry;  // unused if !defined(SSL_PRINT_DEBUG)

  if (BIO_should_write(bio)) {
    DEBUG_PRINT("[%p] BIO: %s want write. should retry %d\n", ssl_, func,
                retry);
    return 0;

  } else if (BIO_should_read(bio)) {
    DEBUG_PRINT("[%p] BIO: %s want read. should retry %d\n", ssl_, func, retry);
    return 0;

  } else {
    JS_ENTER_SCOPE_COM();
    JS_DEFINE_STATE_MARKER(com);
    static char ssl_error_buf[512];
    ERR_error_string_n(rv, ssl_error_buf, sizeof(ssl_error_buf));

    JS_LOCAL_VALUE e = JS_GET_ERROR_VALUE(
        ENGINE_NS::Exception::Error(STD_TO_STRING(ssl_error_buf)));

    JS_LOCAL_OBJECT objl = JS_OBJECT_FROM_PERSISTENT(handle_);
    JS_NAME_SET(objl, JS_STRING_ID("error"), e);

    DEBUG_PRINT("[%p] BIO: %s failed: (%d) %s\n", ssl_, func, rv,
                ssl_error_buf);

    return rv;
  }

  return 0;
}

int Connection::HandleSSLError(const char* func, int rv, ZeroStatus zs,
                               SyscallStatus ss) {
  ClearErrorOnReturn clear_error_on_return;
  (void)&clear_error_on_return;  // Silence unused variable warning.

  if (rv > 0) return rv;
  if ((rv == 0) && (zs == kZeroIsNotAnError)) return rv;

  JS_ENTER_SCOPE_COM();
  JS_DEFINE_STATE_MARKER(com);

  int err = SSL_get_error(ssl_, rv);

  if (err == SSL_ERROR_NONE) {
    return 0;

  } else if (err == SSL_ERROR_WANT_WRITE) {
    DEBUG_PRINT("[%p] SSL: %s want write\n", ssl_, func);
    return 0;

  } else if (err == SSL_ERROR_WANT_READ) {
    DEBUG_PRINT("[%p] SSL: %s want read\n", ssl_, func);
    return 0;
  }

  JS_LOCAL_OBJECT objl = JS_OBJECT_FROM_PERSISTENT(handle_);
  if (err == SSL_ERROR_ZERO_RETURN) {
    JS_NAME_SET(objl, JS_STRING_ID("error"),
                JS_GET_ERROR_VALUE(
                    ENGINE_NS::Exception::Error(STD_TO_STRING("ZERO_RETURN"))));
    return rv;

  } else if ((err == SSL_ERROR_SYSCALL) && (ss == kIgnoreSyscall)) {
    return 0;

  } else {
    BUF_MEM* mem;
    BIO* bio;

    assert(err == SSL_ERROR_SSL || err == SSL_ERROR_SYSCALL);

    // XXX We need to drain the error queue for this thread or else OpenSSL
    // has the possibility of blocking connections? This problem is not well
    // understood. And we should be somehow propagating these errors up
    // into JavaScript. There is no test which demonstrates this problem.
    // https://github.com/joyent/node/issues/1719
    if ((bio = BIO_new(BIO_s_mem()))) {
      ERR_print_errors(bio);
      BIO_get_mem_ptr(bio, &mem);
      JS_LOCAL_VALUE e = JS_GET_ERROR_VALUE(ENGINE_NS::Exception::Error(
          STD_TO_STRING_WITH_LENGTH(mem->data, mem->length)));
      JS_NAME_SET(objl, JS_STRING_ID("error"), e);
      BIO_free(bio);
    }

    return rv;
  }

  return 0;
}

void Connection::ClearError() {
#if !defined(NDEBUG) && defined(JS_ENGINE_V8)
  JS_ENTER_SCOPE();

  // We should clear the error in JS-land
  assert(BOOLEAN_TO_STD(JS_GET_NAME(handle_, JS_STRING_ID("error"))) == false);
#endif  // NDEBUG
}

void Connection::SetShutdownFlags() {
  JS_ENTER_SCOPE_COM();
  JS_DEFINE_STATE_MARKER(com);

  int flags = SSL_get_shutdown(ssl_);
  JS_LOCAL_OBJECT objl = JS_OBJECT_FROM_PERSISTENT(handle_);

  if (flags & SSL_SENT_SHUTDOWN) {
    JS_NAME_SET(objl, JS_STRING_ID("sentShutdown"), STD_TO_BOOLEAN(true));
  }

  if (flags & SSL_RECEIVED_SHUTDOWN) {
    JS_NAME_SET(objl, JS_STRING_ID("receivedShutdown"), STD_TO_BOOLEAN(true));
  }
}

static int VerifyCallback(int preverify_ok, X509_STORE_CTX* ctx) {
  // Quoting SSL_set_verify(3ssl):
  //
  //   The VerifyCallback function is used to control the behaviour when
  //   the SSL_VERIFY_PEER flag is set. It must be supplied by the
  //   application and receives two arguments: preverify_ok indicates,
  //   whether the verification of the certificate in question was passed
  //   (preverify_ok=1) or not (preverify_ok=0). x509_ctx is a pointer to
  //   the complete context used for the certificate chain verification.
  //
  //   The certificate chain is checked starting with the deepest nesting
  //   level (the root CA certificate) and worked upward to the peer's
  //   certificate.  At each level signatures and issuer attributes are
  //   checked.  Whenever a verification error is found, the error number is
  //   stored in x509_ctx and VerifyCallback is called with preverify_ok=0.
  //   By applying X509_CTX_store_* functions VerifyCallback can locate the
  //   certificate in question and perform additional steps (see EXAMPLES).
  //   If no error is found for a certificate, VerifyCallback is called
  //   with preverify_ok=1 before advancing to the next level.
  //
  //   The return value of VerifyCallback controls the strategy of the
  //   further verification process. If VerifyCallback returns 0, the
  //   verification process is immediately stopped with "verification
  //   failed" state. If SSL_VERIFY_PEER is set, a verification failure
  //   alert is sent to the peer and the TLS/SSL handshake is terminated. If
  //   VerifyCallback returns 1, the verification process is continued. If
  //   VerifyCallback always returns 1, the TLS/SSL handshake will not be
  //   terminated with respect to verification failures and the connection
  //   will be established. The calling process can however retrieve the
  //   error code of the last verification error using
  //   SSL_get_verify_result(3) or by maintaining its own error storage
  //   managed by VerifyCallback.
  //
  //   If no VerifyCallback is specified, the default callback will be
  //   used.  Its return value is identical to preverify_ok, so that any
  //   verification failure will lead to a termination of the TLS/SSL
  //   handshake with an alert message, if SSL_VERIFY_PEER is set.
  //
  // Since we cannot perform I/O quickly enough in this callback, we ignore
  // all preverify_ok errors and let the handshake continue. It is
  // imparative that the user use Connection::VerifyError after the
  // 'secure' callback has been made.
  return 1;
}

#ifdef OPENSSL_NPN_NEGOTIATED

int Connection::AdvertiseNextProtoCallback_(SSL* s, const unsigned char** data,
                                            unsigned int* len, void* arg) {
  Connection* p = static_cast<Connection*>(SSL_get_app_data(s));

  if (JS_IS_EMPTY((p->npnProtos_))) {
    // No initialization - no NPN protocols
    *data = reinterpret_cast<const unsigned char*>("");
    *len = 0;
  } else {
    JS_LOCAL_OBJECT objl = JS_OBJECT_FROM_PERSISTENT(p->npnProtos_);
    *data = reinterpret_cast<const unsigned char*>(BUFFER__DATA(objl));
    *len = BUFFER__LENGTH(objl);
  }

  return SSL_TLSEXT_ERR_OK;
}

int Connection::SelectNextProtoCallback_(SSL* s, unsigned char** out,
                                         unsigned char* outlen,
                                         const unsigned char* in,
                                         unsigned int inlen, void* arg) {
  Connection* p = static_cast<Connection*> SSL_get_app_data(s);
  JS_ENTER_SCOPE_COM();
  JS_DEFINE_STATE_MARKER(com);

  // Release old protocol handler if present
  if (!JS_IS_EMPTY((p->selectedNPNProto_))) {
    JS_CLEAR_PERSISTENT(p->selectedNPNProto_);
  }

  if (JS_IS_EMPTY((p->npnProtos_))) {
    // We should at least select one protocol
    // If server is using NPN
    *out = reinterpret_cast<unsigned char*>(const_cast<char*>("http/1.1"));
    *outlen = 8;

    // set status unsupported
    JS_NEW_PERSISTENT_VALUE(p->selectedNPNProto_, STD_TO_BOOLEAN(false));

    return SSL_TLSEXT_ERR_OK;
  }

  JS_LOCAL_OBJECT objl = JS_OBJECT_FROM_PERSISTENT(p->npnProtos_);
  const unsigned char* npnProtos =
      reinterpret_cast<const unsigned char*>(BUFFER__DATA(objl));

  int status = SSL_select_next_proto(out, outlen, in, inlen, npnProtos,
                                     BUFFER__LENGTH(objl));

  switch (status) {
    case OPENSSL_NPN_UNSUPPORTED:
      JS_NEW_PERSISTENT_VALUE(p->selectedNPNProto_, JS_NULL());
      break;
    case OPENSSL_NPN_NEGOTIATED:
      JS_NEW_PERSISTENT_VALUE(
          p->selectedNPNProto_,
          STD_TO_STRING_WITH_LENGTH(reinterpret_cast<const char*>(*out),
                                    *outlen));
      break;
    case OPENSSL_NPN_NO_OVERLAP:
      JS_NEW_PERSISTENT_VALUE(p->selectedNPNProto_, STD_TO_BOOLEAN(false));
      break;
    default:
      break;
  }

  return SSL_TLSEXT_ERR_OK;
}
#endif

#ifdef SSL_CTRL_SET_TLSEXT_SERVERNAME_CB
int Connection::SelectSNIContextCallback_(SSL* s, int* ad, void* arg) {
  JS_ENTER_SCOPE_COM();
  JS_DEFINE_STATE_MARKER(com);

  Connection* p = static_cast<Connection*> SSL_get_app_data(s);

  const char* servername = SSL_get_servername(s, TLSEXT_NAMETYPE_host_name);

  if (servername) {
    if (!JS_IS_EMPTY((p->servername_))) {
      JS_CLEAR_PERSISTENT(p->servername_);
    }

    JS_NEW_PERSISTENT_STRING(p->servername_, STD_TO_STRING(servername));

    // Call the SNI callback and use its return value as context
    if (!JS_IS_EMPTY((p->sniObject_))) {
      if (!JS_IS_EMPTY((p->sniContext_))) {
        JS_CLEAR_PERSISTENT(p->sniContext_);
      }

      // Get callback init args
      JS_LOCAL_VALUE argv[1] = {JS_TYPE_TO_LOCAL_STRING(p->servername_)};

      JS_LOCAL_OBJECT objsn = JS_OBJECT_FROM_PERSISTENT(p->sniObject_);
      // Call it
      JS_LOCAL_VALUE ret = JS_TYPE_TO_LOCAL_VALUE(MakeCallback(
          com, objsn, JS_PREDEFINED_STRING(onselect), ARRAY_SIZE(argv), argv));

      JS_LOCAL_FUNCTION_TEMPLATE objscc =
          JS_TYPE_TO_LOCAL_FUNCTION_TEMPLATE(com->secure_context_constructor);
      // If ret is SecureContext
      if (JS_HAS_INSTANCE(objscc, ret)) {
        JS_NEW_PERSISTENT_VALUE(p->sniContext_, ret);
        SecureContext* sc =
            ObjectWrap::Unwrap<SecureContext>(JS_CAST_OBJECT(ret));
        p->InitNPN(sc, true);
        SSL_set_SSL_CTX(s, sc->ctx_);
      } else {
        return SSL_TLSEXT_ERR_NOACK;
      }
    }
  }

  return SSL_TLSEXT_ERR_OK;
}
#endif

JS_METHOD(Connection, New) {
  Connection* p = new Connection();
  JS_CLASS_NEW_INSTANCE(obj, Connection);

  p->Wrap(obj);

  if (args.Length() < 1 || !args.IsObject(0)) {
    THROW_EXCEPTION("First argument must be a crypto module Credentials");
  }

  SecureContext* sc =
      ObjectWrap::Unwrap<SecureContext>(JS_VALUE_TO_OBJECT(GET_ARG(0)));

  bool is_server = args.GetBoolean(1);

  p->ssl_ = SSL_new(sc->ctx_);
  p->bio_read_ = BIO_new(BIO_s_mem());
  p->bio_write_ = BIO_new(BIO_s_mem());

  SSL_set_app_data(p->ssl_, p);

  if (is_server) SSL_set_info_callback(p->ssl_, SSLInfoCallback);

  p->InitNPN(sc, is_server);

#ifdef SSL_CTRL_SET_TLSEXT_SERVERNAME_CB
  if (is_server) {
    SSL_CTX_set_tlsext_servername_callback(sc->ctx_, SelectSNIContextCallback_);
  } else {
    jxcore::JXString servername;
    args.GetString(2, &servername);
    SSL_set_tlsext_host_name(p->ssl_, *servername);
  }
#endif

  SSL_set_bio(p->ssl_, p->bio_read_, p->bio_write_);

#ifdef SSL_MODE_RELEASE_BUFFERS
  long mode = SSL_get_mode(p->ssl_);
  SSL_set_mode(p->ssl_, mode | SSL_MODE_RELEASE_BUFFERS);
#endif

  int verify_mode;
  if (is_server) {
    bool request_cert = args.GetBoolean(2);
    if (!request_cert) {
      // Note reject_unauthorized ignored.
      verify_mode = SSL_VERIFY_NONE;
    } else {
      bool reject_unauthorized = args.GetBoolean(3);
      verify_mode = SSL_VERIFY_PEER;
      if (reject_unauthorized) verify_mode |= SSL_VERIFY_FAIL_IF_NO_PEER_CERT;
    }
  } else {
    // Note request_cert and reject_unauthorized are ignored for clients.
    verify_mode = SSL_VERIFY_NONE;
  }

  // Always allow a connection. We'll reject in javascript.
  SSL_set_verify(p->ssl_, verify_mode, VerifyCallback);

  if ((p->is_server_ = is_server)) {
    SSL_set_accept_state(p->ssl_);
  } else {
    SSL_set_connect_state(p->ssl_);
  }

  RETURN_POINTER(obj);
}
JS_METHOD_END

void Connection::SSLInfoCallback(const SSL* ssl_, int where, int ret) {
  // Be compatible with older versions of OpenSSL. SSL_get_app_data() wants
  // a non-const SSL* in OpenSSL <= 0.9.7e.
  SSL* ssl = const_cast<SSL*>(ssl_);
  if (where & SSL_CB_HANDSHAKE_START) {
    JS_ENTER_SCOPE_COM();
    Connection* c = static_cast<Connection*>(SSL_get_app_data(ssl));

    JS_LOCAL_OBJECT objl = JS_OBJECT_FROM_PERSISTENT(c->handle_);
    MakeCallback(com, objl, JS_PREDEFINED_STRING(onhandshakedone), 0, NULL);
  }

  if (where & SSL_CB_HANDSHAKE_DONE) {
    JS_ENTER_SCOPE_COM();
    Connection* c = static_cast<Connection*>(SSL_get_app_data(ssl));
    JS_LOCAL_OBJECT objl = JS_OBJECT_FROM_PERSISTENT(c->handle_);

    MakeCallback(com, objl, JS_PREDEFINED_STRING(onhandshakedone), 0, NULL);
  }
}

JS_METHOD(Connection, EncIn) {
  Connection* ss = Connection::Unwrap(args.GetHolder());

  if (args.Length() < 3) {
    THROW_TYPE_EXCEPTION("Takes 3 parameters");
  }

  JS_LOCAL_VALUE _item = GET_ARG(0);
  if (!Buffer::jxHasInstance(_item, com)) {
    THROW_TYPE_EXCEPTION("Second argument should be a buffer");
  }

  char* buffer_data = BUFFER__DATA(_item);
  size_t buffer_length = BUFFER__LENGTH(_item);

  size_t off = args.GetInt32(1);
  size_t len = args.GetInt32(2);
  if (!Buffer::IsWithinBounds(off, len, buffer_length)) {
    THROW_TYPE_EXCEPTION("off + len > buffer.length");
  }

  int bytes_written;
  char* data = buffer_data + off;

  if (ss->is_server_ && !ss->hello_parser_.ended()) {
    bytes_written =
        ss->hello_parser_.Write(reinterpret_cast<uint8_t*>(data), len);
  } else {
    bytes_written = BIO_write(ss->bio_read_, data, len);
    ss->HandleBIOError(ss->bio_read_, "BIO_write", bytes_written);
    ss->SetShutdownFlags();
  }

  RETURN_PARAM(STD_TO_INTEGER(bytes_written));
}
JS_METHOD_END

JS_METHOD(Connection, ClearOut) {
  Connection* ss = Connection::Unwrap(args.GetHolder());

  if (args.Length() < 3) {
    THROW_TYPE_EXCEPTION("Takes 3 parameters");
  }

  JS_LOCAL_VALUE _item = GET_ARG(0);
  if (!Buffer::jxHasInstance(_item, com)) {
    THROW_TYPE_EXCEPTION("Second argument should be a buffer");
  }

  char* buffer_data = BUFFER__DATA(_item);
  size_t buffer_length = BUFFER__LENGTH(_item);

  size_t off = args.GetInt32(1);
  size_t len = args.GetInt32(2);
  if (!Buffer::IsWithinBounds(off, len, buffer_length)) {
    THROW_TYPE_EXCEPTION("off + len > buffer.length");
  }

  if (!SSL_is_init_finished(ss->ssl_)) {
    int rv;

    if (ss->is_server_) {
      rv = SSL_accept(ss->ssl_);
      ss->HandleSSLError("SSL_accept:ClearOut", rv, kZeroIsAnError,
                         kSyscallError);
    } else {
      rv = SSL_connect(ss->ssl_);
      ss->HandleSSLError("SSL_connect:ClearOut", rv, kZeroIsAnError,
                         kSyscallError);
    }

    if (rv < 0) RETURN_PARAM(STD_TO_INTEGER(rv));
  }

  int bytes_read = SSL_read(ss->ssl_, buffer_data + off, len);
  ss->HandleSSLError("SSL_read:ClearOut", bytes_read, kZeroIsNotAnError,
                     kSyscallError);
  ss->SetShutdownFlags();

  RETURN_PARAM(STD_TO_INTEGER(bytes_read));
}
JS_METHOD_END

JS_METHOD(Connection, ClearPending) {
  Connection* ss = Connection::Unwrap(args.GetHolder());

  int bytes_pending = BIO_pending(ss->bio_read_);

  RETURN_PARAM(STD_TO_INTEGER(bytes_pending));
}
JS_METHOD_END

JS_METHOD(Connection, EncPending) {
  Connection* ss = Connection::Unwrap(args.GetHolder());

  int bytes_pending = BIO_pending(ss->bio_write_);

  RETURN_PARAM(STD_TO_INTEGER(bytes_pending));
}
JS_METHOD_END

JS_METHOD(Connection, EncOut) {
  Connection* ss = Connection::Unwrap(args.GetHolder());

  if (args.Length() < 3) {
    THROW_TYPE_EXCEPTION("Takes 3 parameters");
  }

  JS_LOCAL_VALUE _item = GET_ARG(0);

  if (!Buffer::jxHasInstance(_item, com)) {
    THROW_TYPE_EXCEPTION("Second argument should be a buffer");
  }

  char* buffer_data = BUFFER__DATA(_item);
  size_t buffer_length = BUFFER__LENGTH(_item);

  size_t off = args.GetInt32(1);
  size_t len = args.GetInt32(2);
  if (!Buffer::IsWithinBounds(off, len, buffer_length)) {
    THROW_TYPE_EXCEPTION("off + len > buffer.length");
  }

  int bytes_read = BIO_read(ss->bio_write_, buffer_data + off, len);

  ss->HandleBIOError(ss->bio_write_, "BIO_read:EncOut", bytes_read);
  ss->SetShutdownFlags();

  RETURN_PARAM(STD_TO_INTEGER(bytes_read));
}
JS_METHOD_END

JS_METHOD(Connection, ClearIn) {
  Connection* ss = Connection::Unwrap(args.GetHolder());

  if (args.Length() < 3) {
    THROW_TYPE_EXCEPTION("Takes 3 parameters");
  }

  JS_LOCAL_VALUE _item = GET_ARG(0);

  if (!Buffer::jxHasInstance(_item, com)) {
    THROW_TYPE_EXCEPTION("Second argument should be a buffer");
  }

  char* buffer_data = BUFFER__DATA(_item);
  size_t buffer_length = BUFFER__LENGTH(_item);

  size_t off = args.GetInt32(1);
  size_t len = args.GetInt32(2);
  if (!Buffer::IsWithinBounds(off, len, buffer_length)) {
    THROW_TYPE_EXCEPTION("off + len > buffer.length");
  }

  if (!SSL_is_init_finished(ss->ssl_)) {
    int rv;
    if (ss->is_server_) {
      rv = SSL_accept(ss->ssl_);
      ss->HandleSSLError("SSL_accept:ClearIn", rv, kZeroIsAnError,
                         kSyscallError);
    } else {
      rv = SSL_connect(ss->ssl_);
      ss->HandleSSLError("SSL_connect:ClearIn", rv, kZeroIsAnError,
                         kSyscallError);
    }

    if (rv < 0) RETURN_PARAM(STD_TO_INTEGER(rv));
  }

  int bytes_written = SSL_write(ss->ssl_, buffer_data + off, len);

  ss->HandleSSLError("SSL_write:ClearIn", bytes_written,
                     len == 0 ? kZeroIsNotAnError : kZeroIsAnError,
                     kSyscallError);
  ss->SetShutdownFlags();

  RETURN_PARAM(STD_TO_INTEGER(bytes_written));
}
JS_METHOD_END

JS_METHOD(Connection, GetPeerCertificate) {
  Connection* ss = Connection::Unwrap(args.GetHolder());

  if (ss->ssl_ == NULL) RETURN();
  JS_LOCAL_OBJECT info = JS_NEW_EMPTY_OBJECT();
  X509* peer_cert = SSL_get_peer_certificate(ss->ssl_);
  if (peer_cert != NULL) {
    BIO* bio = BIO_new(BIO_s_mem());
    BUF_MEM* mem;
    if (X509_NAME_print_ex(bio, X509_get_subject_name(peer_cert), 0,
                           X509_NAME_FLAGS) > 0) {
      BIO_get_mem_ptr(bio, &mem);
      JS_NAME_SET(info, JS_STRING_ID("subject"),
                  STD_TO_STRING_WITH_LENGTH(mem->data, mem->length));
    }
    (void)BIO_reset(bio);

    if (X509_NAME_print_ex(bio, X509_get_issuer_name(peer_cert), 0,
                           X509_NAME_FLAGS) > 0) {
      BIO_get_mem_ptr(bio, &mem);
      JS_NAME_SET(info, JS_STRING_ID("issuer"),
                  STD_TO_STRING_WITH_LENGTH(mem->data, mem->length));
    }
    (void)BIO_reset(bio);

    int index = X509_get_ext_by_NID(peer_cert, NID_subject_alt_name, -1);
    if (index >= 0) {
      X509_EXTENSION* ext;
      int rv;

      ext = X509_get_ext(peer_cert, index);
      assert(ext != NULL);

      rv = X509V3_EXT_print(bio, ext, 0, 0);
      assert(rv == 1);

      BIO_get_mem_ptr(bio, &mem);
      JS_NAME_SET(info, JS_STRING_ID("subjectaltname"),
                  STD_TO_STRING_WITH_LENGTH(mem->data, mem->length));

      (void)BIO_reset(bio);
    }

    EVP_PKEY* pkey = NULL;
    RSA* rsa = NULL;
    if (NULL != (pkey = X509_get_pubkey(peer_cert)) &&
        NULL != (rsa = EVP_PKEY_get1_RSA(pkey))) {
      BN_print(bio, rsa->n);
      BIO_get_mem_ptr(bio, &mem);
      JS_NAME_SET(info, JS_STRING_ID("modulus"),
                  STD_TO_STRING_WITH_LENGTH(mem->data, mem->length));
      (void)BIO_reset(bio);

      BN_print(bio, rsa->e);
      BIO_get_mem_ptr(bio, &mem);
      JS_NAME_SET(info, JS_STRING_ID("exponent"),
                  STD_TO_STRING_WITH_LENGTH(mem->data, mem->length));
      (void)BIO_reset(bio);
    }

    if (pkey != NULL) {
      EVP_PKEY_free(pkey);
      pkey = NULL;
    }
    if (rsa != NULL) {
      RSA_free(rsa);
      rsa = NULL;
    }

    ASN1_TIME_print(bio, X509_get_notBefore(peer_cert));
    BIO_get_mem_ptr(bio, &mem);
    JS_NAME_SET(info, JS_STRING_ID("valid_from"),
                STD_TO_STRING_WITH_LENGTH(mem->data, mem->length));
    (void)BIO_reset(bio);

    ASN1_TIME_print(bio, X509_get_notAfter(peer_cert));
    BIO_get_mem_ptr(bio, &mem);
    JS_NAME_SET(info, JS_STRING_ID("valid_to"),
                STD_TO_STRING_WITH_LENGTH(mem->data, mem->length));
    BIO_free(bio);

    unsigned int md_size, i;
    unsigned char md[EVP_MAX_MD_SIZE];
    if (X509_digest(peer_cert, EVP_sha1(), md, &md_size)) {
      const char hex[] = "0123456789ABCDEF";
      char fingerprint[EVP_MAX_MD_SIZE * 3];

      for (i = 0; i < md_size; i++) {
        fingerprint[3 * i] = hex[(md[i] & 0xf0) >> 4];
        fingerprint[(3 * i) + 1] = hex[(md[i] & 0x0f)];
        fingerprint[(3 * i) + 2] = ':';
      }

      if (md_size > 0) {
        fingerprint[(3 * (md_size - 1)) + 2] = '\0';
      } else {
        fingerprint[0] = '\0';
      }

      JS_NAME_SET(info, JS_STRING_ID("fingerprint"),
                  STD_TO_STRING(fingerprint));
    }

    STACK_OF(ASN1_OBJECT)* eku = (STACK_OF(ASN1_OBJECT)*)X509_get_ext_d2i(
        peer_cert, NID_ext_key_usage, NULL, NULL);
    if (eku != NULL) {
      JS_LOCAL_ARRAY ext_key_usage = JS_NEW_ARRAY();
      char buf[256];

      for (int i = 0; i < sk_ASN1_OBJECT_num(eku); i++) {
        memset(buf, 0, sizeof(buf));
        OBJ_obj2txt(buf, sizeof(buf) - 1, sk_ASN1_OBJECT_value(eku, i), 1);
        JS_INDEX_SET(ext_key_usage, i, STD_TO_STRING(buf));
      }

      sk_ASN1_OBJECT_pop_free(eku, ASN1_OBJECT_free);
      JS_NAME_SET(info, JS_STRING_ID("ext_key_usage"), ext_key_usage);
    }

    X509_free(peer_cert);
  }
  RETURN_POINTER(info);
}
JS_METHOD_END

JS_METHOD(Connection, GetSession) {
  Connection* ss = Connection::Unwrap(args.GetHolder());

  if (ss->ssl_ == NULL) RETURN();

  SSL_SESSION* sess = SSL_get_session(ss->ssl_);
  if (!sess) RETURN();

  int slen = i2d_SSL_SESSION(sess, NULL);
  assert(slen > 0);

  if (slen > 0) {
    unsigned char* sbuf = new unsigned char[slen];
    unsigned char* p = sbuf;
    i2d_SSL_SESSION(sess, &p);
    JS_LOCAL_VALUE s = Encode(sbuf, slen, BINARY);
    delete[] sbuf;
    RETURN_POINTER(s);
  }
}
JS_METHOD_END

JS_METHOD(Connection, SetSession) {
  Connection* ss = Connection::Unwrap(args.GetHolder());

  if (args.Length() < 1 ||
      (!args.IsString(0) && !Buffer::jxHasInstance(GET_ARG(0), com))) {
    THROW_TYPE_EXCEPTION("Bad argument");
  }

  JS_LOCAL_VALUE _item = GET_ARG(0);
  ASSERT_IS_BUFFER(_item);
  ssize_t slen = BUFFER__LENGTH(_item);

  if (slen < 0) {
    THROW_TYPE_EXCEPTION("Bad argument");
  }

  char* sbuf = new char[slen];

  ssize_t wlen = DecodeWrite(sbuf, slen, _item, BINARY);
  assert(wlen == slen);

  const unsigned char* p = reinterpret_cast<const unsigned char*>(sbuf);
  SSL_SESSION* sess = d2i_SSL_SESSION(NULL, &p, wlen);

  delete[] sbuf;

  if (!sess) RETURN();

  int r = SSL_set_session(ss->ssl_, sess);
  SSL_SESSION_free(sess);

  if (!r) {
    THROW_EXCEPTION("SSL_set_session error");
  }

  RETURN_TRUE();
}
JS_METHOD_END

JS_METHOD(Connection, LoadSession) {
  Connection* ss = Connection::Unwrap(args.GetHolder());

  JS_LOCAL_VALUE _item = GET_ARG(0);
  if (args.Length() >= 1 && Buffer::jxHasInstance(_item, com)) {
    ssize_t slen = BUFFER__LENGTH(_item);
    char* sbuf = BUFFER__DATA(_item);

    const unsigned char* p = reinterpret_cast<unsigned char*>(sbuf);
    SSL_SESSION* sess = d2i_SSL_SESSION(NULL, &p, slen);

    // Setup next session and move hello to the BIO buffer
    if (ss->next_sess_ != NULL) {
      SSL_SESSION_free(ss->next_sess_);
    }
    ss->next_sess_ = sess;
  }

  ss->hello_parser_.Finish();

  RETURN_TRUE();
}
JS_METHOD_END

JS_METHOD(Connection, IsSessionReused) {
  Connection* ss = Connection::Unwrap(args.GetHolder());

  if (ss->ssl_ == NULL || SSL_session_reused(ss->ssl_) == false) {
    RETURN_FALSE();
  }

  RETURN_TRUE();
}
JS_METHOD_END

JS_METHOD(Connection, Start) {
  Connection* ss = Connection::Unwrap(args.GetHolder());

  if (!SSL_is_init_finished(ss->ssl_)) {
    int rv;
    if (ss->is_server_) {
      rv = SSL_accept(ss->ssl_);
      ss->HandleSSLError("SSL_accept:Start", rv, kZeroIsAnError, kSyscallError);
    } else {
      rv = SSL_connect(ss->ssl_);
      ss->HandleSSLError("SSL_connect:Start", rv, kZeroIsAnError,
                         kSyscallError);
    }

    RETURN_PARAM(STD_TO_INTEGER(rv));
  }

  RETURN_PARAM(STD_TO_INTEGER(0));
}
JS_METHOD_END

JS_METHOD(Connection, Shutdown) {
  Connection* ss = Connection::Unwrap(args.GetHolder());

  if (ss->ssl_ == NULL) RETURN_FALSE();
  int rv = SSL_shutdown(ss->ssl_);
  ss->HandleSSLError("SSL_shutdown", rv, kZeroIsNotAnError, kIgnoreSyscall);
  ss->SetShutdownFlags();

  RETURN_PARAM(STD_TO_INTEGER(rv));
}
JS_METHOD_END

JS_METHOD(Connection, IsInitFinished) {
  Connection* ss = Connection::Unwrap(args.GetHolder());

  const char* pskId = SSL_get_psk_identity(ss->ssl_);

  if (pskId) {
    JS_HANDLE_OBJECT sshandle_ = JS_TYPE_TO_LOCAL_OBJECT(ss->handle_);
    JS_NAME_SET(sshandle_, JS_STRING_ID("pskIdentity"), STD_TO_STRING(pskId));
  }

  if (ss->ssl_ == NULL || SSL_is_init_finished(ss->ssl_) == false) {
    RETURN_FALSE();
  }
  RETURN_TRUE();
}
JS_METHOD_END

JS_METHOD(Connection, VerifyError) {
  Connection* ss = Connection::Unwrap(args.GetHolder());

  if (ss->ssl_ == NULL) RETURN_PARAM(JS_NULL());

  // XXX Do this check in JS land?
  X509* peer_cert = SSL_get_peer_certificate(ss->ssl_);
  if (peer_cert == NULL) {
    // We requested a certificate and they did not send us one.
    // Definitely an error.
    // XXX is this the right error message?
    THROW_EXCEPTION("UNABLE_TO_GET_ISSUER_CERT");
  }
  X509_free(peer_cert);

  long x509_verify_error = SSL_get_verify_result(ss->ssl_);

  JS_LOCAL_STRING s;

  switch (x509_verify_error) {
    case X509_V_OK: {
      RETURN_PARAM(JS_NULL());
    } break;

    case X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT:
      s = STD_TO_STRING("UNABLE_TO_GET_ISSUER_CERT");
      break;

    case X509_V_ERR_UNABLE_TO_GET_CRL:
      s = STD_TO_STRING("UNABLE_TO_GET_CRL");
      break;

    case X509_V_ERR_UNABLE_TO_DECRYPT_CERT_SIGNATURE:
      s = STD_TO_STRING("UNABLE_TO_DECRYPT_CERT_SIGNATURE");
      break;

    case X509_V_ERR_UNABLE_TO_DECRYPT_CRL_SIGNATURE:
      s = STD_TO_STRING("UNABLE_TO_DECRYPT_CRL_SIGNATURE");
      break;

    case X509_V_ERR_UNABLE_TO_DECODE_ISSUER_PUBLIC_KEY:
      s = STD_TO_STRING("UNABLE_TO_DECODE_ISSUER_PUBLIC_KEY");
      break;

    case X509_V_ERR_CERT_SIGNATURE_FAILURE:
      s = STD_TO_STRING("CERT_SIGNATURE_FAILURE");
      break;

    case X509_V_ERR_CRL_SIGNATURE_FAILURE:
      s = STD_TO_STRING("CRL_SIGNATURE_FAILURE");
      break;

    case X509_V_ERR_CERT_NOT_YET_VALID:
      s = STD_TO_STRING("CERT_NOT_YET_VALID");
      break;

    case X509_V_ERR_CERT_HAS_EXPIRED:
      s = STD_TO_STRING("CERT_HAS_EXPIRED");
      break;

    case X509_V_ERR_CRL_NOT_YET_VALID:
      s = STD_TO_STRING("CRL_NOT_YET_VALID");
      break;

    case X509_V_ERR_CRL_HAS_EXPIRED:
      s = STD_TO_STRING("CRL_HAS_EXPIRED");
      break;

    case X509_V_ERR_ERROR_IN_CERT_NOT_BEFORE_FIELD:
      s = STD_TO_STRING("ERROR_IN_CERT_NOT_BEFORE_FIELD");
      break;

    case X509_V_ERR_ERROR_IN_CERT_NOT_AFTER_FIELD:
      s = STD_TO_STRING("ERROR_IN_CERT_NOT_AFTER_FIELD");
      break;

    case X509_V_ERR_ERROR_IN_CRL_LAST_UPDATE_FIELD:
      s = STD_TO_STRING("ERROR_IN_CRL_LAST_UPDATE_FIELD");
      break;

    case X509_V_ERR_ERROR_IN_CRL_NEXT_UPDATE_FIELD:
      s = STD_TO_STRING("ERROR_IN_CRL_NEXT_UPDATE_FIELD");
      break;

    case X509_V_ERR_OUT_OF_MEM:
      s = STD_TO_STRING("OUT_OF_MEM");
      break;

    case X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT:
      s = STD_TO_STRING("DEPTH_ZERO_SELF_SIGNED_CERT");
      break;

    case X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN:
      s = STD_TO_STRING("SELF_SIGNED_CERT_IN_CHAIN");
      break;

    case X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY:
      s = STD_TO_STRING("UNABLE_TO_GET_ISSUER_CERT_LOCALLY");
      break;

    case X509_V_ERR_UNABLE_TO_VERIFY_LEAF_SIGNATURE:
      s = STD_TO_STRING("UNABLE_TO_VERIFY_LEAF_SIGNATURE");
      break;

    case X509_V_ERR_CERT_CHAIN_TOO_LONG:
      s = STD_TO_STRING("CERT_CHAIN_TOO_LONG");
      break;

    case X509_V_ERR_CERT_REVOKED:
      s = STD_TO_STRING("CERT_REVOKED");
      break;

    case X509_V_ERR_INVALID_CA:
      s = STD_TO_STRING("INVALID_CA");
      break;

    case X509_V_ERR_PATH_LENGTH_EXCEEDED:
      s = STD_TO_STRING("PATH_LENGTH_EXCEEDED");
      break;

    case X509_V_ERR_INVALID_PURPOSE:
      s = STD_TO_STRING("INVALID_PURPOSE");
      break;

    case X509_V_ERR_CERT_UNTRUSTED:
      s = STD_TO_STRING("CERT_UNTRUSTED");
      break;

    case X509_V_ERR_CERT_REJECTED:
      s = STD_TO_STRING("CERT_REJECTED");
      break;

    default:
      s = STD_TO_STRING(X509_verify_cert_error_string(x509_verify_error));
      break;
  }

  JS_LOCAL_VALUE ret_val = JS_GET_ERROR_VALUE(ENGINE_NS::Exception::Error(s));
  // It is important here not to send Exception::Error
  // since it throws the exception on MozJS land.
  RETURN_PARAM(ret_val);
}
JS_METHOD_END

JS_METHOD(Connection, GetCurrentCipher) {
  Connection* ss = Connection::Unwrap(args.GetHolder());

  OPENSSL_CONST SSL_CIPHER* c;

  if (ss->ssl_ == NULL) RETURN();
  c = SSL_get_current_cipher(ss->ssl_);
  if (c == NULL) RETURN();
  JS_LOCAL_OBJECT info = JS_NEW_EMPTY_OBJECT();
  const char* cipher_name = SSL_CIPHER_get_name(c);
  JS_NAME_SET(info, JS_STRING_ID("name"), STD_TO_STRING(cipher_name));
  const char* cipher_version = SSL_CIPHER_get_version(c);
  JS_NAME_SET(info, JS_STRING_ID("version"), STD_TO_STRING(cipher_version));
  RETURN_POINTER(info);
}
JS_METHOD_END

JS_METHOD(Connection, Close) {
  Connection* ss = Connection::Unwrap(args.GetHolder());

  if (ss->ssl_ != NULL) {
    SSL_free(ss->ssl_);
    ss->ssl_ = NULL;
  }
  RETURN_TRUE();
}
JS_METHOD_END

void Connection::InitNPN(SecureContext* sc, bool is_server) {
#ifdef OPENSSL_NPN_NEGOTIATED
  if (is_server) {
    // Server should advertise NPN protocols
    SSL_CTX_set_next_protos_advertised_cb(sc->ctx_, AdvertiseNextProtoCallback_,
                                          NULL);
  } else {
    // Client should select protocol from advertised
    // If server supports NPN
    SSL_CTX_set_next_proto_select_cb(sc->ctx_, SelectNextProtoCallback_, NULL);
  }
#endif
}

#ifdef OPENSSL_NPN_NEGOTIATED
JS_METHOD(Connection, GetNegotiatedProto) {
  Connection* ss = Connection::Unwrap(args.GetHolder());

  if (ss->is_server_) {
    const unsigned char* npn_proto;
    unsigned int npn_proto_len;

    SSL_get0_next_proto_negotiated(ss->ssl_, &npn_proto, &npn_proto_len);

    if (!npn_proto) {
      RETURN_FALSE();
    }

    RETURN_PARAM(STD_TO_STRING_WITH_LENGTH(
        reinterpret_cast<const char*>(npn_proto), npn_proto_len));
  } else {
    RETURN_PARAM(JS_TYPE_TO_LOCAL_VALUE(ss->selectedNPNProto_));
  }
}
JS_METHOD_END

JS_METHOD(Connection, SetNPNProtocols) {
  Connection* ss = Connection::Unwrap(args.GetHolder());

  JS_LOCAL_VALUE _item = GET_ARG(0);
  if (args.Length() < 1 || !Buffer::jxHasInstance(_item, com)) {
    THROW_TYPE_EXCEPTION("Must give a Buffer as first argument");
  }

  // Release old handle
  JS_CLEAR_PERSISTENT(ss->npnProtos_);
  JS_NEW_PERSISTENT_OBJECT(ss->npnProtos_, JS_VALUE_TO_OBJECT(_item));

  RETURN_TRUE();
}
JS_METHOD_END
#endif

#ifdef SSL_CTRL_SET_TLSEXT_SERVERNAME_CB
JS_METHOD(Connection, GetServername) {
  Connection* ss = Connection::Unwrap(args.GetHolder());

  if (ss->is_server_ && !JS_IS_EMPTY((ss->servername_))) {
    RETURN_PARAM(JS_TYPE_TO_LOCAL_STRING(ss->servername_));
  } else {
    RETURN_FALSE();
  }
}
JS_METHOD_END

JS_METHOD(Connection, SetSNICallback) {
  Connection* ss = Connection::Unwrap(args.GetHolder());

  JS_LOCAL_VALUE _item = GET_ARG(0);
  if (args.Length() < 1 || !JS_IS_FUNCTION(_item)) {
    THROW_TYPE_EXCEPTION("Must give a Function as first argument");
  }

  // Release old handle
  JS_CLEAR_PERSISTENT(ss->sniObject_);
  JS_NEW_PERSISTENT_OBJECT(ss->sniObject_, JS_NEW_EMPTY_OBJECT());
  JS_NAME_SET(JS_OBJECT_FROM_PERSISTENT(ss->sniObject_), JS_STRING_ID("onselect"), _item);

  RETURN_TRUE();
}
JS_METHOD_END
#endif


JS_METHOD(Connection, SetPskClientCallback) {
  Connection* ss = Connection::Unwrap(args.GetHolder());

  JS_LOCAL_VALUE _item = GET_ARG(0);
  if (args.Length() < 1 || !JS_IS_FUNCTION(_item)) {
    THROW_TYPE_EXCEPTION("Must give a Function as first argument");
  }

  JS_NEW_PERSISTENT_FUNCTION(ss->psk_client_cb_, JS_TYPE_AS_FUNCTION(_item));

  SSL_set_psk_client_callback(ss->ssl_, Connection::PskClientCallback_);

  RETURN_TRUE();
}
JS_METHOD_END


unsigned int node::crypto::
  Connection::PskClientCallback_( SSL * ssl, 
                                  const char * hint, 
                                  char * identity, 
                                  unsigned int max_identity_len, 
                                  unsigned char * psk, 
                                  unsigned int max_psk_len)
{

  node::commons* com = node::commons::getInstance();
  JS_DEFINE_STATE_MARKER(com);

  Connection *ss = static_cast<Connection*>(SSL_get_app_data(ssl));

  JS_LOCAL_VALUE(argv[1]);

  if (hint) {
    argv[0] = STD_TO_STRING(hint);
  }
  else {
    argv[0] = JS_UNDEFINED();
  }
  
  JS_LOCAL_FUNCTION jlf = JS_TYPE_TO_LOCAL_FUNCTION(ss->psk_client_cb_);
  if (JS_IS_NULL(jlf)) {
    return 0;
  }

  // call the JS callback. It's wrapped in a TryCatch because otherwise if it throws
  // we wind up with a segfault. Eating the error isn't great; it would be better to
  // put the Connection into an error state and hopefully get the message back to the
  // application code somehow.
  JS_TRY_CATCH(try_catch);
  JS_LOCAL_VALUE(result) = jlf->Call(JS_GET_GLOBAL(), 1, argv);

  if (try_catch.HasCaught()) {
    if (try_catch.CanContinue()) node::ReportException(try_catch, true);
    return 0;
  }

  // The result is expected to be an object with "identity" string and "key"
  // buffer values. If this isn't the case then return 0, indicating that the
  // identity isn't found.
  if (result->IsObject()) {
    node::commons* com = node::commons::getInstance();
    JS_DEFINE_STATE_MARKER(com);

    JS_LOCAL_OBJECT(idAndKey) = JS_VALUE_TO_OBJECT(result);

    JS_LOCAL_VALUE(id) = idAndKey->Get(STD_TO_STRING("identity"));
    JS_LOCAL_VALUE(key) = idAndKey->Get(STD_TO_STRING("key"));

    if (JS_IS_STRING(id) && Buffer::HasInstance(key)) {
      // write the chosen client identity string into the buffer provided
      ssize_t hlen = StringBytes::JXSize(id, ASCII, false);
      StringBytes::JXWrite(identity, hlen, id, ASCII, false);

      // write the id's binary key into the buffer provided
      JS_LOCAL_OBJECT(keyBuffer) = JS_VALUE_TO_OBJECT(key);
      int len = Buffer::Length(keyBuffer);
      if (len <= max_psk_len) {
        memcpy(psk, Buffer::Data(keyBuffer), len);
        return len;
      }
    }
  }

  return 0;
}


class Cipher : public ObjectWrap {
 public:
  INIT_NAMED_CLASS_MEMBERS(Cipher, Cipher)

  NODE_SET_PROTOTYPE_METHOD(constructor, "init", CipherInit);
  NODE_SET_PROTOTYPE_METHOD(constructor, "initiv", CipherInitIv);
  NODE_SET_PROTOTYPE_METHOD(constructor, "update", CipherUpdate);
  NODE_SET_PROTOTYPE_METHOD(constructor, "setAutoPadding", SetAutoPadding);
  NODE_SET_PROTOTYPE_METHOD(constructor, "final", CipherFinal);

  END_INIT_NAMED_MEMBERS(Cipher)

  bool CipherInit(char* cipherType, char* key_buf, int key_buf_len) {
    cipher = EVP_get_cipherbyname(cipherType);
    if (!cipher) {
      fprintf(stderr, "node-crypto : Unknown cipher %s\n", cipherType);
      return false;
    }

    unsigned char key[EVP_MAX_KEY_LENGTH], iv[EVP_MAX_IV_LENGTH];
    int key_len =
        EVP_BytesToKey(cipher, EVP_md5(), NULL, (unsigned char*)key_buf,
                       key_buf_len, 1, key, iv);

    EVP_CIPHER_CTX_init(&ctx);
    EVP_CipherInit_ex(&ctx, cipher, NULL, NULL, NULL, true);
    if (!EVP_CIPHER_CTX_set_key_length(&ctx, key_len)) {
      fprintf(stderr, "node-crypto : Invalid key length %d\n", key_len);
      EVP_CIPHER_CTX_cleanup(&ctx);
      return false;
    }
    EVP_CipherInit_ex(&ctx, NULL, NULL, (unsigned char*)key, (unsigned char*)iv,
                      true);
    initialised_ = true;
    return true;
  }

  bool CipherInitIv(char* cipherType, char* key, int key_len, char* iv,
                    int iv_len) {
    cipher = EVP_get_cipherbyname(cipherType);
    if (!cipher) {
      fprintf(stderr, "node-crypto : Unknown cipher %s\n", cipherType);
      return false;
    }
    /* OpenSSL versions up to 0.9.8l failed to return the correct
       iv_length (0) for ECB ciphers */
    if (EVP_CIPHER_iv_length(cipher) != iv_len &&
        !(EVP_CIPHER_mode(cipher) == EVP_CIPH_ECB_MODE && iv_len == 0)) {
      fprintf(stderr, "node-crypto : Invalid IV length %d\n", iv_len);
      return false;
    }
    EVP_CIPHER_CTX_init(&ctx);
    EVP_CipherInit_ex(&ctx, cipher, NULL, NULL, NULL, true);
    if (!EVP_CIPHER_CTX_set_key_length(&ctx, key_len)) {
      fprintf(stderr, "node-crypto : Invalid key length %d\n", key_len);
      EVP_CIPHER_CTX_cleanup(&ctx);
      return false;
    }
    EVP_CipherInit_ex(&ctx, NULL, NULL, (unsigned char*)key, (unsigned char*)iv,
                      true);
    initialised_ = true;
    return true;
  }

  int CipherUpdate(char* data, int len, unsigned char** out, int* out_len) {
    if (!initialised_) return 0;
    *out_len = len + EVP_CIPHER_CTX_block_size(&ctx);
    *out = new unsigned char[*out_len];
    return EVP_CipherUpdate(&ctx, *out, out_len, (unsigned char*)data, len);
  }

  int SetAutoPadding(bool auto_padding) {
    if (!initialised_) return 0;
    return EVP_CIPHER_CTX_set_padding(&ctx, auto_padding ? 1 : 0);
  }

  int CipherFinal(unsigned char** out, int* out_len) {
    if (!initialised_) return 0;
    *out = new unsigned char[EVP_CIPHER_CTX_block_size(&ctx)];
    int r = EVP_CipherFinal_ex(&ctx, *out, out_len);
    EVP_CIPHER_CTX_cleanup(&ctx);
    initialised_ = false;
    return r;
  }

 protected:
  static JS_LOCAL_METHOD(New) {
    JS_CLASS_NEW_INSTANCE(obj, Cipher);
    Cipher* cipher = new Cipher();
    cipher->Wrap(obj);
    RETURN_POINTER(obj);
  }
  JS_METHOD_END

  static JS_LOCAL_METHOD(CipherInit) {
    Cipher* cipher = ObjectWrap::Unwrap<Cipher>(args.This());

    if (args.Length() <= 1 || !args.IsString(0) ||
        !(args.IsString(1) || Buffer::jxHasInstance(GET_ARG(1), com))) {
      THROW_TYPE_EXCEPTION("Must give cipher-type, key");
    }

    JS_LOCAL_VALUE _item = GET_ARG(1);

    ASSERT_IS_BUFFER(_item);
    ssize_t key_buf_len = BUFFER__LENGTH(_item);

    if (key_buf_len < 0) {
      THROW_TYPE_EXCEPTION(
          "Bad argument - buffer length of second parameter < 0");
    }

    char* key_buf = new char[key_buf_len];
    ssize_t key_written = DecodeWrite(key_buf, key_buf_len, _item, BINARY);
    assert(key_written == key_buf_len);

    jxcore::JXString cipherType;
    args.GetString(0, &cipherType);

    bool r = cipher->CipherInit(*cipherType, key_buf, key_buf_len);

    delete[] key_buf;

    if (!r) ThrowCryptoError(ERR_get_error());

    RETURN_PARAM(args.This());
  }
  JS_METHOD_END

  static JS_LOCAL_METHOD(CipherInitIv) {
    Cipher* cipher = ObjectWrap::Unwrap<Cipher>(args.This());

    if (args.Length() <= 2 || !args.IsString(0) ||
        !(args.IsString(1) || Buffer::jxHasInstance(GET_ARG(1), com)) ||
        !(args.IsString(2) || Buffer::jxHasInstance(GET_ARG(2), com))) {
      THROW_TYPE_EXCEPTION("Must give cipher-type, key, and iv as argument");
    }

    JS_LOCAL_VALUE _item1 = GET_ARG(1), _item2 = GET_ARG(2);

    ASSERT_IS_BUFFER(_item1);
    ssize_t key_len = BUFFER__LENGTH(_item1);

    if (key_len < 0) {
      THROW_TYPE_EXCEPTION("Bad argument");
    }

    ASSERT_IS_BUFFER(_item2);
    ssize_t iv_len = BUFFER__LENGTH(_item2);

    if (iv_len < 0) {
      THROW_TYPE_EXCEPTION("Bad argument");
    }

    char* key_buf = new char[key_len];
    ssize_t key_written = DecodeWrite(key_buf, key_len, _item1, BINARY);
    assert(key_written == key_len);

    char* iv_buf = new char[iv_len];
    ssize_t iv_written = DecodeWrite(iv_buf, iv_len, _item2, BINARY);
    assert(iv_written == iv_len);

    jxcore::JXString cipherType;
    args.GetString(0, &cipherType);

    bool r =
        cipher->CipherInitIv(*cipherType, key_buf, key_len, iv_buf, iv_len);

    delete[] key_buf;
    delete[] iv_buf;

    if (!r) ThrowCryptoError(ERR_get_error());

    RETURN_PARAM(args.This());
  }
  JS_METHOD_END

  static JS_LOCAL_METHOD(CipherUpdate) {
    Cipher* cipher = ObjectWrap::Unwrap<Cipher>(args.This());

    if (args.Length() < 1) {
      THROW_EXCEPTION("expects a parameter 'string' or 'buffer'");
    }

    JS_LOCAL_VALUE _item1 = GET_ARG(0);
    ASSERT_IS_STRING_OR_BUFFER(_item1);

    // Only copy the data if we have to, because it's a string
    unsigned char* out = 0;
    int out_len = 0, r;
    if (args.IsString(0)) {
      JS_HANDLE_STRING string = args.GetAsString(0);

      enum encoding encoding;
      if (!args.IsString(1)) {
        encoding = BINARY;
      } else {
        jxcore::JXString str;
        args.GetString(1, &str);
        encoding = ParseEncoding(*str, str.length(), BINARY);
      }

      if (!StringBytes::IsValidString(string, encoding))
        THROW_TYPE_EXCEPTION("Bad input string");

      size_t buflen = StringBytes::JXStorageSize(string, encoding, false);
      char* buf = new char[buflen];
      size_t written =
          StringBytes::JXWrite(buf, buflen, string, encoding, false);
      r = cipher->CipherUpdate(buf, written, &out, &out_len);
      delete[] buf;
    } else {
      char* buf = BUFFER__DATA(_item1);
      size_t buflen = BUFFER__LENGTH(_item1);
      r = cipher->CipherUpdate(buf, buflen, &out, &out_len);
    }

    if (r == 0) {
      delete[] out;
      ThrowCryptoTypeError(ERR_get_error());
    }

    JS_LOCAL_VALUE outString;
    outString = Encode(out, out_len, BUFFER);

    delete[] out;

    RETURN_PARAM(outString);
  }
  JS_METHOD_END

  static JS_LOCAL_METHOD(SetAutoPadding) {
    Cipher* cipher = ObjectWrap::Unwrap<Cipher>(args.This());

    cipher->SetAutoPadding(args.Length() < 1 || args.GetBoolean(0));
  }
  JS_METHOD_END

  static JS_LOCAL_METHOD(CipherFinal) {
    Cipher* cipher = ObjectWrap::Unwrap<Cipher>(args.This());

    unsigned char* out_value = NULL;
    int out_len = -1;
    JS_LOCAL_VALUE outString;

    int r = cipher->CipherFinal(&out_value, &out_len);

    if (out_len <= 0 || r == 0) {
      delete[] out_value;
      out_value = NULL;
      if (r == 0) ThrowCryptoTypeError(ERR_get_error());
    }

    outString = Encode(out_value, out_len, BUFFER);

    delete[] out_value;
    RETURN_PARAM(outString);
  }
  JS_METHOD_END

  Cipher() : ObjectWrap() { initialised_ = false; }

  ~Cipher() {
    if (initialised_) {
      EVP_CIPHER_CTX_cleanup(&ctx);
    }
  }

 private:
  EVP_CIPHER_CTX ctx;       /* coverity[member_decl] */
  const EVP_CIPHER* cipher; /* coverity[member_decl] */
  bool initialised_;
};

class Decipher : public ObjectWrap {
 public:
  INIT_NAMED_CLASS_MEMBERS(Decipher, Decipher)

  NODE_SET_PROTOTYPE_METHOD(constructor, "init", DecipherInit);
  NODE_SET_PROTOTYPE_METHOD(constructor, "initiv", DecipherInitIv);
  NODE_SET_PROTOTYPE_METHOD(constructor, "update", DecipherUpdate);
  NODE_SET_PROTOTYPE_METHOD(constructor, "setAutoPadding", SetAutoPadding);
  NODE_SET_PROTOTYPE_METHOD(constructor, "final", DecipherFinal);
  NODE_SET_PROTOTYPE_METHOD(constructor, "finaltol", DecipherFinal);  // remove
                                                                      // ?

  END_INIT_NAMED_MEMBERS(Decipher)

  bool DecipherInit(char* cipherType, char* key_buf, int key_buf_len) {
    cipher_ = EVP_get_cipherbyname(cipherType);

    if (!cipher_) {
      fprintf(stderr, "node-crypto : Unknown cipher %s\n", cipherType);
      return false;
    }

    unsigned char key[EVP_MAX_KEY_LENGTH], iv[EVP_MAX_IV_LENGTH];
    int key_len =
        EVP_BytesToKey(cipher_, EVP_md5(), NULL, (unsigned char*)(key_buf),
                       key_buf_len, 1, key, iv);

    EVP_CIPHER_CTX_init(&ctx);
    EVP_CipherInit_ex(&ctx, cipher_, NULL, NULL, NULL, false);
    if (!EVP_CIPHER_CTX_set_key_length(&ctx, key_len)) {
      fprintf(stderr, "node-crypto : Invalid key length %d\n", key_len);
      EVP_CIPHER_CTX_cleanup(&ctx);
      return false;
    }
    EVP_CipherInit_ex(&ctx, NULL, NULL, (unsigned char*)key, (unsigned char*)iv,
                      false);
    initialised_ = true;
    return true;
  }

  bool DecipherInitIv(char* cipherType, char* key, int key_len, char* iv,
                      int iv_len) {
    cipher_ = EVP_get_cipherbyname(cipherType);
    if (!cipher_) {
      fprintf(stderr, "node-crypto : Unknown cipher %s\n", cipherType);
      return false;
    }
    /* OpenSSL versions up to 0.9.8l failed to return the correct
      iv_length (0) for ECB ciphers */
    if (EVP_CIPHER_iv_length(cipher_) != iv_len &&
        !(EVP_CIPHER_mode(cipher_) == EVP_CIPH_ECB_MODE && iv_len == 0)) {
      fprintf(stderr, "node-crypto : Invalid IV length %d\n", iv_len);
      return false;
    }
    EVP_CIPHER_CTX_init(&ctx);
    EVP_CipherInit_ex(&ctx, cipher_, NULL, NULL, NULL, false);
    if (!EVP_CIPHER_CTX_set_key_length(&ctx, key_len)) {
      fprintf(stderr, "node-crypto : Invalid key length %d\n", key_len);
      EVP_CIPHER_CTX_cleanup(&ctx);
      return false;
    }
    EVP_CipherInit_ex(&ctx, NULL, NULL, (unsigned char*)key, (unsigned char*)iv,
                      false);
    initialised_ = true;
    return true;
  }

  int DecipherUpdate(char* data, int len, unsigned char** out, int* out_len) {
    if (!initialised_) {
      *out_len = 0;
      *out = NULL;
      return 0;
    }

    *out_len = len + EVP_CIPHER_CTX_block_size(&ctx);
    *out = new unsigned char[*out_len];

    return EVP_CipherUpdate(&ctx, *out, out_len, (unsigned char*)data, len);
  }

  int SetAutoPadding(bool auto_padding) {
    if (!initialised_) return 0;
    return EVP_CIPHER_CTX_set_padding(&ctx, auto_padding ? 1 : 0);
  }

  // coverity[alloc_arg]
  int DecipherFinal(unsigned char** out, int* out_len) {
    int r;

    if (!initialised_) {
      *out_len = 0;
      *out = NULL;
      return 0;
    }

    *out = new unsigned char[EVP_CIPHER_CTX_block_size(&ctx)];
    r = EVP_CipherFinal_ex(&ctx, *out, out_len);
    EVP_CIPHER_CTX_cleanup(&ctx);
    initialised_ = false;
    return r;
  }

 protected:
  static JS_LOCAL_METHOD(New) {
    Decipher* cipher = new Decipher();
    JS_CLASS_NEW_INSTANCE(obj, Decipher);
    cipher->Wrap(obj);
    RETURN_POINTER(obj);
  }
  JS_METHOD_END

  static JS_LOCAL_METHOD(DecipherInit) {
    Decipher* cipher = ObjectWrap::Unwrap<Decipher>(args.This());

    if (args.Length() <= 1 || !args.IsString(0) ||
        !(args.IsString(1) || Buffer::jxHasInstance(GET_ARG(1), com))) {
      THROW_TYPE_EXCEPTION("Must give cipher-type, key as argument");
    }

    JS_HANDLE_VALUE _item1 = GET_ARG(1);
    ASSERT_IS_BUFFER(_item1);
    ssize_t key_len = BUFFER__LENGTH(_item1);

    if (key_len < 0) {
      THROW_TYPE_EXCEPTION("Bad argument");
    }

    char* key_buf = new char[key_len];
    ssize_t key_written = DecodeWrite(key_buf, key_len, _item1, BINARY);
    assert(key_written == key_len);

    jxcore::JXString cipherType;
    args.GetString(0, &cipherType);

    bool r = cipher->DecipherInit(*cipherType, key_buf, key_len);

    delete[] key_buf;

    if (!r) {
      THROW_EXCEPTION("DecipherInit error");
    }

    RETURN_PARAM(args.This());
  }
  JS_METHOD_END

  static JS_LOCAL_METHOD(DecipherInitIv);
  {
    Decipher* cipher = ObjectWrap::Unwrap<Decipher>(args.This());

    if (args.Length() <= 2 || !args.IsString(0) ||
        !(args.IsString(1) || Buffer::jxHasInstance(GET_ARG(1), com)) ||
        !(args.IsString(2) || Buffer::jxHasInstance(GET_ARG(2), com))) {
      THROW_TYPE_EXCEPTION("Must give cipher-type, key, and iv as argument");
    }

    JS_LOCAL_VALUE _item1 = GET_ARG(1);
    JS_LOCAL_VALUE _item2 = GET_ARG(2);
    ASSERT_IS_BUFFER(_item1);
    ssize_t key_len = BUFFER__LENGTH(_item1);

    if (key_len < 0) {
      THROW_TYPE_EXCEPTION("Bad argument");
    }

    ASSERT_IS_BUFFER(_item2);
    ssize_t iv_len = BUFFER__LENGTH(_item2);

    if (iv_len < 0) {
      THROW_TYPE_EXCEPTION("Bad argument");
    }

    char* key_buf = new char[key_len];
    ssize_t key_written = DecodeWrite(key_buf, key_len, _item1, BINARY);
    assert(key_written == key_len);

    char* iv_buf = new char[iv_len];
    ssize_t iv_written = DecodeWrite(iv_buf, iv_len, _item2, BINARY);
    assert(iv_written == iv_len);

    jxcore::JXString cipherType;
    args.GetString(0, &cipherType);

    bool r =
        cipher->DecipherInitIv(*cipherType, key_buf, key_len, iv_buf, iv_len);

    delete[] key_buf;
    delete[] iv_buf;

    if (!r) {
      THROW_EXCEPTION("DecipherInitIv error");
    }

    RETURN_PARAM(args.This());
  }
  JS_METHOD_END

  static JS_LOCAL_METHOD(DecipherUpdate) {
    Decipher* cipher = ObjectWrap::Unwrap<Decipher>(args.This());

    ASSERT_IS_STRING_OR_BUFFER(GET_ARG(0));

    // Only copy the data if we have to, because it's a string
    unsigned char* out = 0;
    int out_len = 0, r;
    if (args.IsString(0)) {
      JS_HANDLE_STRING string = args.GetAsString(0);

      enum encoding encoding;
      if (!args.IsString(1)) {
        encoding = BINARY;
      } else {
        jxcore::JXString str;
        args.GetString(1, &str);
        encoding = ParseEncoding(*str, str.length(), BINARY);
      }

      if (!StringBytes::IsValidString(string, encoding))
        THROW_TYPE_EXCEPTION("Bad input string");
      size_t buflen = StringBytes::JXStorageSize(string, encoding, false);
      char* buf = new char[buflen];
      size_t written =
          StringBytes::JXWrite(buf, buflen, string, encoding, false);
      r = cipher->DecipherUpdate(buf, written, &out, &out_len);
      delete[] buf;
    } else {
      char* buf = BUFFER__DATA(GET_ARG(0));
      size_t buflen = BUFFER__LENGTH(GET_ARG(0));
      r = cipher->DecipherUpdate(buf, buflen, &out, &out_len);
    }

    if (r == 0) {
      delete[] out;
      ThrowCryptoTypeError(ERR_get_error());
    }

    JS_LOCAL_VALUE outString;
    outString = Encode(out, out_len, BUFFER);

    delete[] out;

    RETURN_PARAM(outString);
  }
  JS_METHOD_END

  static JS_LOCAL_METHOD(SetAutoPadding) {
    Decipher* cipher = ObjectWrap::Unwrap<Decipher>(args.This());

    cipher->SetAutoPadding(args.Length() < 1 || args.GetBoolean(0));
  }
  JS_METHOD_END

  static JS_LOCAL_METHOD(DecipherFinal) {
    Decipher* cipher = ObjectWrap::Unwrap<Decipher>(args.This());

    unsigned char* out_value = NULL;
    int out_len = -1;
    JS_LOCAL_VALUE outString;

    int r = cipher->DecipherFinal(&out_value, &out_len);

    if (out_len <= 0 || r == 0) {
      delete[] out_value;  // allocated even if out_len == 0
      out_value = NULL;
      if (r == 0) ThrowCryptoTypeError(ERR_get_error());
    }

    outString = Encode(out_value, out_len, BUFFER);
    delete[] out_value;
    RETURN_PARAM(outString);
  }
  JS_METHOD_END

  Decipher() : ObjectWrap() { initialised_ = false; }

  ~Decipher() {
    if (initialised_) {
      EVP_CIPHER_CTX_cleanup(&ctx);
    }
  }

 private:
  EVP_CIPHER_CTX ctx;
  const EVP_CIPHER* cipher_;
  bool initialised_;
};

class Hmac : public ObjectWrap {
 public:
  INIT_NAMED_CLASS_MEMBERS(Hmac, Hmac)

  NODE_SET_PROTOTYPE_METHOD(constructor, "init", HmacInit);
  NODE_SET_PROTOTYPE_METHOD(constructor, "update", HmacUpdate);
  NODE_SET_PROTOTYPE_METHOD(constructor, "digest", HmacDigest);

  END_INIT_NAMED_MEMBERS(Hmac)

  bool HmacInit(char* hashType, char* key, int key_len) {
    md = EVP_get_digestbyname(hashType);
    if (!md) {
      fprintf(stderr, "node-crypto : Unknown message digest %s\n", hashType);
      return false;
    }
    HMAC_CTX_init(&ctx);
    if (key_len == 0) {
      HMAC_Init(&ctx, "", 0, md);
    } else {
      HMAC_Init(&ctx, key, key_len, md);
    }
    initialised_ = true;
    return true;
  }

  int HmacUpdate(char* data, int len) {
    if (!initialised_) return 0;
    HMAC_Update(&ctx, (unsigned char*)data, len);
    return 1;
  }

  int HmacDigest(unsigned char** md_value, unsigned int* md_len) {
    if (!initialised_) return 0;
    *md_value = new unsigned char[EVP_MAX_MD_SIZE];
    HMAC_Final(&ctx, *md_value, md_len);
    HMAC_CTX_cleanup(&ctx);
    initialised_ = false;
    return 1;
  }

 protected:
  static JS_LOCAL_METHOD(New) {
    JS_CLASS_NEW_INSTANCE(obj, Hmac);
    Hmac* hmac = new Hmac();
    hmac->Wrap(obj);
    RETURN_POINTER(obj);
  }
  JS_METHOD_END

  static JS_LOCAL_METHOD(HmacInit) {
    Hmac* hmac = ObjectWrap::Unwrap<Hmac>(args.This());

    if (args.Length() == 0 || !args.IsString(0)) {
      THROW_EXCEPTION("Must give hashtype string as argument");
    }

    ASSERT_IS_BUFFER(GET_ARG(1));
    ssize_t buffer_length = BUFFER__LENGTH(GET_ARG(1));

    if (buffer_length < 0) {
      THROW_TYPE_EXCEPTION("Bad argument");
    }

    jxcore::JXString hashType;
    args.GetString(0, &hashType);

    bool r;

    char* buffer_data = BUFFER__DATA(GET_ARG(1));

    r = hmac->HmacInit(*hashType, buffer_data, buffer_length);

    if (!r) {
      THROW_EXCEPTION("hmac error");
    }

    RETURN_PARAM(args.This());
  }
  JS_METHOD_END

  static JS_LOCAL_METHOD(HmacUpdate) {
    Hmac* hmac = ObjectWrap::Unwrap<Hmac>(args.This());

    ASSERT_IS_STRING_OR_BUFFER(GET_ARG(0));

    // Only copy the data if we have to, because it's a string
    int r;
    if (args.IsString(0)) {
      JS_HANDLE_STRING string = args.GetAsString(0);

      enum encoding encoding;
      if (!args.IsString(1)) {
        encoding = BINARY;
      } else {
        jxcore::JXString str;
        args.GetString(1, &str);
        encoding = ParseEncoding(*str, str.length(), BINARY);
      }

      if (!StringBytes::IsValidString(string, encoding))
        THROW_TYPE_EXCEPTION("Bad input string");
      size_t buflen = StringBytes::JXStorageSize(string, encoding, false);
      char* buf = new char[buflen];
      size_t written =
          StringBytes::JXWrite(buf, buflen, string, encoding, false);
      r = hmac->HmacUpdate(buf, written);
      delete[] buf;
    } else {
      char* buf = BUFFER__DATA(GET_ARG(0));
      size_t buflen = BUFFER__LENGTH(GET_ARG(0));
      r = hmac->HmacUpdate(buf, buflen);
    }

    if (!r) {
      THROW_EXCEPTION("HmacUpdate fail");
    }

    RETURN_PARAM(args.This());
  }
  JS_METHOD_END

  static JS_LOCAL_METHOD(HmacDigest) {
    Hmac* hmac = ObjectWrap::Unwrap<Hmac>(args.This());

    enum encoding encoding = BUFFER;
    if (args.Length() >= 1) {
      encoding = ParseEncoding(GET_ARG(0), BUFFER);
    }

    unsigned char* md_value = NULL;
    unsigned int md_len = 0;
    JS_LOCAL_VALUE outString;

    int r = hmac->HmacDigest(&md_value, &md_len);
    if (r == 0) {
      md_value = NULL;
      md_len = 0;
    }

    outString = StringBytes::Encode(reinterpret_cast<const char*>(md_value),
                                    md_len, encoding);

    delete[] md_value;
    RETURN_PARAM(outString);
  }
  JS_METHOD_END

  Hmac() : ObjectWrap() { initialised_ = false; }

  ~Hmac() {
    if (initialised_) {
      HMAC_CTX_cleanup(&ctx);
    }
  }

 private:
  HMAC_CTX ctx;     /* coverity[member_decl] */
  const EVP_MD* md; /* coverity[member_decl] */
  bool initialised_;
};

class Hash : public ObjectWrap {
 public:
  INIT_NAMED_CLASS_MEMBERS(Hash, Hash)

  NODE_SET_PROTOTYPE_METHOD(constructor, "update", HashUpdate);
  NODE_SET_PROTOTYPE_METHOD(constructor, "digest", HashDigest);

  END_INIT_NAMED_MEMBERS(Hash)

  bool HashInit(const char* hashType) {
    md = EVP_get_digestbyname(hashType);
    if (!md) return false;
    EVP_MD_CTX_init(&mdctx);
    EVP_DigestInit_ex(&mdctx, md, NULL);
    initialised_ = true;
    return true;
  }

  int HashUpdate(char* data, int len) {
    if (!initialised_) return 0;
    EVP_DigestUpdate(&mdctx, data, len);
    return 1;
  }

 protected:
  static JS_LOCAL_METHOD(New) {
    if (args.Length() == 0 || !args.IsString(0)) {
      THROW_EXCEPTION("Must give hashtype string as argument");
    }

    jxcore::JXString hashType;
    args.GetString(0, &hashType);

    Hash* hash = new Hash();
    if (!hash->HashInit(*hashType)) {
      delete hash;
      THROW_EXCEPTION("Digest method not supported");
    }

    JS_CLASS_NEW_INSTANCE(obj, Hash);
    hash->Wrap(obj);
    RETURN_POINTER(obj);
  }
  JS_METHOD_END

  static JS_LOCAL_METHOD(HashUpdate) {
    Hash* hash = ObjectWrap::Unwrap<Hash>(args.This());

    ASSERT_IS_STRING_OR_BUFFER(GET_ARG(0));

    // Only copy the data if we have to, because it's a string
    int r;
    if (args.IsString(0)) {
      JS_HANDLE_STRING string = args.GetAsString(0);

      enum encoding encoding;
      if (!args.IsString(1)) {
        encoding = BINARY;
      } else {
        jxcore::JXString str;
        args.GetString(1, &str);
        encoding = ParseEncoding(*str, str.length(), BINARY);
      }

      if (!StringBytes::IsValidString(string, encoding))
        THROW_TYPE_EXCEPTION("Bad input string");
      size_t buflen = StringBytes::JXStorageSize(string, encoding, false);
      char* buf = new char[buflen];
      size_t written =
          StringBytes::JXWrite(buf, buflen, string, encoding, false);
      r = hash->HashUpdate(buf, written);
      delete[] buf;
    } else {
      char* buf = BUFFER__DATA(GET_ARG(0));
      size_t buflen = BUFFER__LENGTH(GET_ARG(0));
      r = hash->HashUpdate(buf, buflen);
    }

    if (!r) {
      THROW_TYPE_EXCEPTION("HashUpdate fail");
    }

    RETURN_PARAM(args.This());
  }
  JS_METHOD_END

  static JS_LOCAL_METHOD(HashDigest) {
    Hash* hash = ObjectWrap::Unwrap<Hash>(args.This());

    if (!hash->initialised_) {
      THROW_EXCEPTION("Not initialized");
    }

    enum encoding encoding = BUFFER;
    if (args.Length() >= 1) {
      encoding = ParseEncoding(JS_VALUE_TO_STRING(GET_ARG(0)), BUFFER);
    }

    unsigned char md_value[EVP_MAX_MD_SIZE];
    unsigned int md_len;

    EVP_DigestFinal_ex(&hash->mdctx, md_value, &md_len);
    EVP_MD_CTX_cleanup(&hash->mdctx);
    hash->initialised_ = false;

    RETURN_PARAM(StringBytes::Encode(reinterpret_cast<const char*>(md_value),
                                     md_len, encoding));
  }
  JS_METHOD_END

  Hash() : ObjectWrap() { initialised_ = false; }

  ~Hash() {
    if (initialised_) {
      EVP_MD_CTX_cleanup(&mdctx);
    }
  }

 private:
  EVP_MD_CTX mdctx; /* coverity[member_decl] */
  const EVP_MD* md; /* coverity[member_decl] */
  bool initialised_;
};

class Sign : public ObjectWrap {
 public:
  INIT_NAMED_CLASS_MEMBERS(Sign, Sign)

  NODE_SET_PROTOTYPE_METHOD(constructor, "init", SignInit);
  NODE_SET_PROTOTYPE_METHOD(constructor, "update", SignUpdate);
  NODE_SET_PROTOTYPE_METHOD(constructor, "sign", SignFinal);

  END_INIT_NAMED_MEMBERS(Sign)

  bool SignInit(const char* signType) {
    md = EVP_get_digestbyname(signType);
    if (!md) {
      printf("Unknown message digest %s\n", signType);
      return false;
    }
    EVP_MD_CTX_init(&mdctx);
    EVP_SignInit_ex(&mdctx, md, NULL);
    initialised_ = true;
    return true;
  }

  int SignUpdate(char* data, int len) {
    if (!initialised_) return 0;
    EVP_SignUpdate(&mdctx, data, len);
    return 1;
  }

  int SignFinal(unsigned char** md_value, unsigned int* md_len, char* key_pem,
                int key_pemLen) {
    if (!initialised_) return 0;

    BIO* bp = NULL;
    EVP_PKEY* pkey;
    bp = BIO_new(BIO_s_mem());
    if (!BIO_write(bp, key_pem, key_pemLen)) return 0;

    pkey = PEM_read_bio_PrivateKey(bp, NULL, NULL, NULL);
    if (pkey == NULL) {
      ERR_print_errors_fp(stderr);
      return 0;
    }

    if (!EVP_SignFinal(&mdctx, *md_value, md_len, pkey)) {
      ERR_print_errors_fp(stderr);
      return 0;
    }
    EVP_MD_CTX_cleanup(&mdctx);
    initialised_ = false;
    EVP_PKEY_free(pkey);
    BIO_free(bp);
    return 1;
  }

 protected:
  static JS_LOCAL_METHOD(New) {
    Sign* sign = new Sign();
    JS_CLASS_NEW_INSTANCE(obj, Sign);
    sign->Wrap(obj);

    RETURN_POINTER(obj);
  }
  JS_METHOD_END

  static JS_LOCAL_METHOD(SignInit) {
    Sign* sign = ObjectWrap::Unwrap<Sign>(args.This());

    if (args.Length() == 0 || !args.IsString(0)) {
      THROW_EXCEPTION("Must give signtype string as argument");
    }

    jxcore::JXString signType;
    args.GetString(0, &signType);

    bool r = sign->SignInit(*signType);

    if (!r) {
      THROW_EXCEPTION("SignInit error");
    }

    RETURN_PARAM(args.This());
  }
  JS_METHOD_END

  static JS_LOCAL_METHOD(SignUpdate) {
    Sign* sign = ObjectWrap::Unwrap<Sign>(args.This());

    ASSERT_IS_STRING_OR_BUFFER(GET_ARG(0));

    // Only copy the data if we have to, because it's a string
    int r;
    if (args.IsString(0)) {
      JS_HANDLE_STRING string = args.GetAsString(0);

      enum encoding encoding;
      if (!args.IsString(1)) {
        encoding = BINARY;
      } else {
        jxcore::JXString str;
        args.GetString(1, &str);
        encoding = ParseEncoding(*str, str.length(), BINARY);
      }

      if (!StringBytes::IsValidString(string, encoding))
        THROW_TYPE_EXCEPTION("Bad input string");
      size_t buflen = StringBytes::JXStorageSize(string, encoding, false);
      char* buf = new char[buflen];
      size_t written =
          StringBytes::JXWrite(buf, buflen, string, encoding, false);
      r = sign->SignUpdate(buf, written);
      delete[] buf;
    } else {
      char* buf = BUFFER__DATA(GET_ARG(0));
      size_t buflen = BUFFER__LENGTH(GET_ARG(0));
      r = sign->SignUpdate(buf, buflen);
    }

    if (!r) {
      THROW_TYPE_EXCEPTION("SignUpdate fail");
    }

    RETURN_PARAM(args.This());
  }
  JS_METHOD_END

  static JS_LOCAL_METHOD(SignFinal) {
    Sign* sign = ObjectWrap::Unwrap<Sign>(args.This());

    unsigned char* md_value;
    unsigned int md_len;
    JS_LOCAL_VALUE outString;

    ASSERT_IS_BUFFER(GET_ARG(0));
    ssize_t len = BUFFER__LENGTH(GET_ARG(0));

    enum encoding encoding = BUFFER;
    if (args.Length() >= 2) {
      encoding = ParseEncoding(args.GetItem(1), BUFFER);
    }

    char* buf = new char[len];
    ssize_t written = DecodeWrite(buf, len, GET_ARG(0), BUFFER);
    assert(written == len);

    md_len = 8192;  // Maximum key size is 8192 bits
    md_value = new unsigned char[md_len];

    int r = sign->SignFinal(&md_value, &md_len, buf, len);
    if (r == 0) {
      delete[] buf;
      delete[] md_value;
      md_value = NULL;
      md_len = r;
      THROW_EXCEPTION("SignFinal error");
    }

    delete[] buf;

    outString = StringBytes::Encode(reinterpret_cast<const char*>(md_value),
                                    md_len, encoding);

    delete[] md_value;
    RETURN_PARAM(outString);
  }
  JS_METHOD_END

  Sign() : ObjectWrap() { initialised_ = false; }

  ~Sign() {
    if (initialised_) {
      EVP_MD_CTX_cleanup(&mdctx);
    }
  }

 private:
  EVP_MD_CTX mdctx; /* coverity[member_decl] */
  const EVP_MD* md; /* coverity[member_decl] */
  bool initialised_;
};

class Verify : public ObjectWrap {
 public:
  INIT_NAMED_CLASS_MEMBERS(Verify, Verify)

  NODE_SET_PROTOTYPE_METHOD(constructor, "init", VerifyInit);
  NODE_SET_PROTOTYPE_METHOD(constructor, "update", VerifyUpdate);
  NODE_SET_PROTOTYPE_METHOD(constructor, "verify", VerifyFinal);

  END_INIT_NAMED_MEMBERS(Verify)

  bool VerifyInit(const char* verifyType) {
    md = EVP_get_digestbyname(verifyType);
    if (!md) {
      fprintf(stderr, "node-crypto : Unknown message digest %s\n", verifyType);
      return false;
    }
    EVP_MD_CTX_init(&mdctx);
    EVP_VerifyInit_ex(&mdctx, md, NULL);
    initialised_ = true;
    return true;
  }

  int VerifyUpdate(char* data, int len) {
    if (!initialised_) return 0;
    EVP_VerifyUpdate(&mdctx, data, len);
    return 1;
  }

  int VerifyFinal(char* key_pem, int key_pemLen, unsigned char* sig,
                  int siglen) {
    if (!initialised_) return 0;

    ClearErrorOnReturn clear_error_on_return;
    (void)&clear_error_on_return;  // Silence compiler warning.

    EVP_PKEY* pkey = NULL;
    BIO* bp = NULL;
    X509* x509 = NULL;
    int r = 0;

    bp = BIO_new(BIO_s_mem());
    if (bp == NULL) {
      ERR_print_errors_fp(stderr);
      return 0;
    }
    if (!BIO_write(bp, key_pem, key_pemLen)) {
      ERR_print_errors_fp(stderr);
      return 0;
    }

    // Check if this is a PKCS#8 or RSA public key before trying as X.509.
    // Split this out into a separate function once we have more than one
    // consumer of public keys.
    if (strncmp(key_pem, PUBLIC_KEY_PFX, PUBLIC_KEY_PFX_LEN) == 0) {
      pkey = PEM_read_bio_PUBKEY(bp, NULL, NULL, NULL);
      if (pkey == NULL) {
        ERR_print_errors_fp(stderr);
        return 0;
      }
    } else if (strncmp(key_pem, PUBRSA_KEY_PFX, PUBRSA_KEY_PFX_LEN) == 0) {
      RSA* rsa = PEM_read_bio_RSAPublicKey(bp, NULL, NULL, NULL);
      if (rsa) {
        pkey = EVP_PKEY_new();
        if (pkey) EVP_PKEY_set1_RSA(pkey, rsa);
        RSA_free(rsa);
      }
      if (pkey == NULL) {
        ERR_print_errors_fp(stderr);
        return 0;
      }
    } else {
      // X.509 fallback
      x509 = PEM_read_bio_X509(bp, NULL, NULL, NULL);
      if (x509 == NULL) {
        ERR_print_errors_fp(stderr);
        return 0;
      }

      pkey = X509_get_pubkey(x509);
      if (pkey == NULL) {
        ERR_print_errors_fp(stderr);
        return 0;
      }
    }

    r = EVP_VerifyFinal(&mdctx, sig, siglen, pkey);

    if (pkey != NULL) EVP_PKEY_free(pkey);
    if (x509 != NULL) X509_free(x509);
    if (bp != NULL) BIO_free(bp);
    EVP_MD_CTX_cleanup(&mdctx);
    initialised_ = false;

    return r;
  }

 protected:
  static JS_LOCAL_METHOD(New) {
    Verify* verify = new Verify();

    JS_CLASS_NEW_INSTANCE(obj, Verify);
    verify->Wrap(obj);
    RETURN_POINTER(obj);
  }
  JS_METHOD_END

  static JS_LOCAL_METHOD(VerifyInit) {
    Verify* verify = ObjectWrap::Unwrap<Verify>(args.This());

    if (args.Length() == 0 || !args.IsString(0)) {
      THROW_EXCEPTION("Must give verifytype string as argument");
    }

    jxcore::JXString verifyType;
    args.GetString(0, &verifyType);

    bool r = verify->VerifyInit(*verifyType);

    if (!r) {
      THROW_EXCEPTION("VerifyInit error");
    }

    RETURN_PARAM(args.This());
  }
  JS_METHOD_END

  static JS_LOCAL_METHOD(VerifyUpdate) {
    Verify* verify = ObjectWrap::Unwrap<Verify>(args.This());

    ASSERT_IS_STRING_OR_BUFFER(GET_ARG(0));

    // Only copy the data if we have to, because it's a string
    int r;
    if (args.IsString(0)) {
      JS_HANDLE_STRING string = args.GetAsString(0);

      enum encoding encoding;
      if (!args.IsString(1)) {
        encoding = BINARY;
      } else {
        jxcore::JXString str;
        args.GetString(1, &str);
        encoding = ParseEncoding(*str, str.length(), BINARY);
      }

      if (!StringBytes::IsValidString(string, encoding))
        THROW_TYPE_EXCEPTION("Bad input string");
      size_t buflen = StringBytes::JXStorageSize(string, encoding, false);
      char* buf = new char[buflen];
      size_t written =
          StringBytes::JXWrite(buf, buflen, string, encoding, false);
      r = verify->VerifyUpdate(buf, written);
      delete[] buf;
    } else {
      char* buf = BUFFER__DATA(GET_ARG(0));
      size_t buflen = BUFFER__LENGTH(GET_ARG(0));
      r = verify->VerifyUpdate(buf, buflen);
    }

    if (!r) {
      THROW_TYPE_EXCEPTION("VerifyUpdate fail");
    }

    RETURN_PARAM(args.This());
  }
  JS_METHOD_END

  static JS_LOCAL_METHOD(VerifyFinal) {
    Verify* verify = ObjectWrap::Unwrap<Verify>(args.This());

    JS_HANDLE_VALUE _item0 = GET_ARG(0);
    ASSERT_IS_BUFFER(_item0);
    ssize_t klen = BUFFER__LENGTH(_item0);

    if (klen < 0) {
      THROW_TYPE_EXCEPTION("Bad argument");
    }

    char* kbuf = new char[klen];
    ssize_t kwritten = DecodeWrite(kbuf, klen, GET_ARG(0), BINARY);
    assert(kwritten == klen);

    JS_HANDLE_VALUE _item1 = GET_ARG(1);
    bool is_buffer = Buffer::jxHasInstance(_item1, com);
    if (!is_buffer && !JS_IS_STRING(_item1)) {
      THROW_TYPE_EXCEPTION("Not a string or buffer");
    }

    // BINARY works for both buffers and binary strings.
    enum encoding encoding = BINARY;
    if (args.Length() >= 3) {
      if (args.IsString(2)) {
        jxcore::JXString str;
        args.GetString(2, &str);
        encoding = ParseEncoding(*str, str.length(), BINARY);
      }
    }

    ssize_t hlen = StringBytes::JXSize(_item1, encoding, is_buffer);

    if (hlen < 0) {
      delete[] kbuf;
      THROW_TYPE_EXCEPTION("Bad argument");
    }

    unsigned char* hbuf = new unsigned char[hlen];
    ssize_t hwritten = StringBytes::JXWrite(reinterpret_cast<char*>(hbuf), hlen,
                                            _item1, encoding, is_buffer);
    assert(hwritten == hlen);

    int r;
    r = verify->VerifyFinal(kbuf, klen, hbuf, hlen);

    delete[] kbuf;
    delete[] hbuf;

    RETURN_PARAM(STD_TO_BOOLEAN(r && r != -1));
  }
  JS_METHOD_END

  Verify() : ObjectWrap() { initialised_ = false; }

  ~Verify() {
    if (initialised_) {
      EVP_MD_CTX_cleanup(&mdctx);
    }
  }

 private:
  EVP_MD_CTX mdctx; /* coverity[member_decl] */
  const EVP_MD* md; /* coverity[member_decl] */
  bool initialised_;
};

class PublicKeyCipher {
public:
  typedef int(*EVP_PKEY_cipher_init_t)(EVP_PKEY_CTX *ctx);
  typedef int(*EVP_PKEY_cipher_t)(EVP_PKEY_CTX *ctx,
    unsigned char *out, size_t *outlen,
    const unsigned char *in, size_t inlen);

  enum Operation {
    kEncrypt,
    kDecrypt
  };

  template <Operation operation,
    EVP_PKEY_cipher_init_t EVP_PKEY_cipher_init,
    EVP_PKEY_cipher_t EVP_PKEY_cipher>
  static bool Cipher(const char* key_pem,
                      int key_pem_len,
                      const char* passphrase,
                      int padding,
                      const unsigned char* data,
                      int len,
                      unsigned char** out,
                      size_t* out_len) {
    EVP_PKEY* pkey = NULL;
    EVP_PKEY_CTX* ctx = NULL;
    BIO* bp = NULL;
    X509* x509 = NULL;
    bool fatal = true;

    bp = BIO_new_mem_buf(const_cast<char*>(key_pem), key_pem_len);
    if (bp == NULL)
      goto exit;

    // Check if this is a PKCS#8 or RSA public key before trying as X.509 and
    // private key.
    if (operation == kEncrypt &&
      strncmp(key_pem, PUBLIC_KEY_PFX, PUBLIC_KEY_PFX_LEN) == 0) {
      pkey = PEM_read_bio_PUBKEY(bp, NULL, NULL, NULL);
      if (pkey == NULL)
        goto exit;
    }
    else if (operation == kEncrypt &&
      strncmp(key_pem, PUBRSA_KEY_PFX, PUBRSA_KEY_PFX_LEN) == 0) {
      RSA* rsa = PEM_read_bio_RSAPublicKey(bp, NULL, NULL, NULL);
      if (rsa) {
        pkey = EVP_PKEY_new();
        if (pkey)
          EVP_PKEY_set1_RSA(pkey, rsa);
        RSA_free(rsa);
      }
      if (pkey == NULL)
        goto exit;
    }
    else if (operation == kEncrypt &&
      strncmp(key_pem, CERTIFICATE_PFX, CERTIFICATE_PFX_LEN) == 0) {
      x509 = PEM_read_bio_X509(bp, NULL, CryptoPemCallback, NULL);
      if (x509 == NULL)
        goto exit;

      pkey = X509_get_pubkey(x509);
      if (pkey == NULL)
        goto exit;
    }
    else {
      pkey = PEM_read_bio_PrivateKey(bp,
        NULL,
        CryptoPemCallback,
        const_cast<char*>(passphrase));
      if (pkey == NULL)
        goto exit;
    }

    ctx = EVP_PKEY_CTX_new(pkey, NULL);
    if (!ctx)
      goto exit;
    if (EVP_PKEY_cipher_init(ctx) <= 0)
      goto exit;
    if (EVP_PKEY_CTX_set_rsa_padding(ctx, padding) <= 0)
      goto exit;

    if (EVP_PKEY_cipher(ctx, NULL, out_len, data, len) <= 0)
      goto exit;

    *out = new unsigned char[*out_len];

    if (EVP_PKEY_cipher(ctx, *out, out_len, data, len) <= 0)
      goto exit;

    fatal = false;

  exit:
    if (pkey != NULL)
      EVP_PKEY_free(pkey);
    if (bp != NULL)
      BIO_free_all(bp);
    if (ctx != NULL)
      EVP_PKEY_CTX_free(ctx);

    return !fatal;
  }

  static JS_LOCAL_METHOD(PrivateDecrypt) {
    return Cipher<kDecrypt, EVP_PKEY_decrypt_init, EVP_PKEY_decrypt>(args);
  }
  JS_METHOD_END

  static JS_LOCAL_METHOD(PublicEncrypt) {
    return Cipher<kEncrypt, EVP_PKEY_encrypt_init, EVP_PKEY_encrypt>(args);
  }
  JS_METHOD_END

  template <Operation operation,
    EVP_PKEY_cipher_init_t EVP_PKEY_cipher_init,
    EVP_PKEY_cipher_t EVP_PKEY_cipher>
  static JS_NATIVE_RETURN_TYPE Cipher(jxcore::PArguments args) {
    JS_ENTER_SCOPE_COM();
    JS_DEFINE_STATE_MARKER(com);

    ASSERT_IS_BUFFER(args.GetItem(0));
    char* kbuf = Buffer::Data(args.GetItem(0));
    ssize_t klen = Buffer::Length(args.GetItem(0));

    ASSERT_IS_BUFFER(args.GetItem(1));
    char* buf = Buffer::Data(args.GetItem(1));
    ssize_t len = Buffer::Length(args.GetItem(1));

    int padding = args.GetItem(2)->Uint32Value();

    jxcore::JXString passphrase;
    args.GetString(3, &passphrase);

    unsigned char* out_value = NULL;
    size_t out_len = 0;

    bool r = Cipher<operation, EVP_PKEY_cipher_init, EVP_PKEY_cipher>(
      kbuf,
      klen,
      args.Length() >= 3 && !args.GetItem(2)->IsNull() ? *passphrase : NULL,
      padding,
      reinterpret_cast<const unsigned char*>(buf),
      len,
      &out_value,
      &out_len);

    if (out_len == 0 || !r) {
      delete[] out_value;
      out_value = NULL;
      out_len = 0;
      if (!r) {
        ThrowCryptoError(ERR_get_error());
      }
    }

    Buffer *buff = Buffer::New(reinterpret_cast<char*>(out_value), out_len, com);
    RETURN_POINTER(JS_TYPE_TO_LOCAL_OBJECT(buff->handle_));
    delete[] out_value;
  }
};


class DiffieHellman : public ObjectWrap {
 public:
  INIT_NAMED_GROUP_CLASS_MEMBERS(DiffieHellman, DiffieHellman,
                                 DiffieHellmanGroup)

  NODE_SET_PROTOTYPE_METHOD(constructor1, "generateKeys", GenerateKeys);
  NODE_SET_PROTOTYPE_METHOD(constructor1, "computeSecret", ComputeSecret);
  NODE_SET_PROTOTYPE_METHOD(constructor1, "getPrime", GetPrime);
  NODE_SET_PROTOTYPE_METHOD(constructor1, "getGenerator", GetGenerator);
  NODE_SET_PROTOTYPE_METHOD(constructor1, "getPublicKey", GetPublicKey);
  NODE_SET_PROTOTYPE_METHOD(constructor1, "getPrivateKey", GetPrivateKey);
  NODE_SET_PROTOTYPE_METHOD(constructor1, "setPublicKey", SetPublicKey);
  NODE_SET_PROTOTYPE_METHOD(constructor1, "setPrivateKey", SetPrivateKey);

  NODE_SET_PROTOTYPE_METHOD(constructor2, "generateKeys", GenerateKeys);
  NODE_SET_PROTOTYPE_METHOD(constructor2, "computeSecret", ComputeSecret);
  NODE_SET_PROTOTYPE_METHOD(constructor2, "getPrime", GetPrime);
  NODE_SET_PROTOTYPE_METHOD(constructor2, "getGenerator", GetGenerator);
  NODE_SET_PROTOTYPE_METHOD(constructor2, "getPublicKey", GetPublicKey);
  NODE_SET_PROTOTYPE_METHOD(constructor2, "getPrivateKey", GetPrivateKey);

  END_INIT_NAMED_GROUP_MEMBERS(DiffieHellman, DiffieHellmanGroup)

  bool Init(int primeLength) {
    dh = DH_new();
    DH_generate_parameters_ex(dh, primeLength, DH_GENERATOR_2, 0);
    bool result = VerifyContext();
    if (!result) return false;
    initialised_ = true;
    return true;
  }

  bool Init(unsigned char* p, int p_len) {
    dh = DH_new();
    dh->p = BN_bin2bn(p, p_len, 0);
    dh->g = BN_new();
    if (!BN_set_word(dh->g, 2)) return false;
    bool result = VerifyContext();
    if (!result) return false;
    initialised_ = true;
    return true;
  }

  bool Init(unsigned char* p, int p_len, unsigned char* g, int g_len) {
    dh = DH_new();
    dh->p = BN_bin2bn(p, p_len, 0);
    dh->g = BN_bin2bn(g, g_len, 0);
    initialised_ = true;
    return true;
  }

 protected:
  static JS_LOCAL_METHOD(DiffieHellmanGroup) {
    DiffieHellman* diffieHellman = new DiffieHellman();

    if (args.Length() != 1 || !args.IsString(0)) {
      THROW_EXCEPTION("No group name given");
    }

    jxcore::JXString group_name;
    args.GetString(0, &group_name);

    modp_group* it = modp_groups;

    while (it->name != NULL) {
      if (!strcasecmp(*group_name, it->name)) break;
      it++;
    }

    if (it->name != NULL) {
      diffieHellman->Init(it->prime, it->prime_size, it->gen, it->gen_size);
    } else {
      THROW_EXCEPTION("Unknown group");
    }

    JS_CLASS_NEW_INSTANCE(obj, DiffieHellmanGroup);
    diffieHellman->Wrap(obj);

    RETURN_POINTER(obj);
  }
  JS_METHOD_END

  static JS_LOCAL_METHOD(New) {
    DiffieHellman* diffieHellman = new DiffieHellman();
    bool initialized = false;

    if (args.Length() > 0) {
      if (args.IsInteger(0)) {
        initialized = diffieHellman->Init(args.GetInt32(0));
      } else {
        initialized = diffieHellman->Init(
            reinterpret_cast<unsigned char*>(BUFFER__DATA(GET_ARG(0))),
            BUFFER__LENGTH(GET_ARG(0)));
      }
    }

    if (!initialized) {
      THROW_EXCEPTION("Initialization failed");
    }

    JS_CLASS_NEW_INSTANCE(obj, DiffieHellman);
    diffieHellman->Wrap(obj);

    RETURN_POINTER(obj);
  }
  JS_METHOD_END

  static JS_LOCAL_METHOD(GenerateKeys) {
    DiffieHellman* diffieHellman =
        ObjectWrap::Unwrap<DiffieHellman>(args.This());

    if (!diffieHellman->initialised_) {
      THROW_EXCEPTION("Not initialized");
    }

    if (!DH_generate_key(diffieHellman->dh)) {
      THROW_EXCEPTION("Key generation failed");
    }

    JS_LOCAL_VALUE outString;

    int dataSize = BN_num_bytes(diffieHellman->dh->pub_key);
    char* data = new char[dataSize];
    BN_bn2bin(diffieHellman->dh->pub_key,
              reinterpret_cast<unsigned char*>(data));

    outString = Encode(data, dataSize, BUFFER);
    delete[] data;

    RETURN_POINTER(outString);
  }
  JS_METHOD_END

  static JS_LOCAL_METHOD(GetPrime) {
    DiffieHellman* diffieHellman =
        ObjectWrap::Unwrap<DiffieHellman>(args.This());

    if (!diffieHellman->initialised_) {
      THROW_EXCEPTION("Not initialized");
    }

    int dataSize = BN_num_bytes(diffieHellman->dh->p);
    char* data = new char[dataSize];
    BN_bn2bin(diffieHellman->dh->p, reinterpret_cast<unsigned char*>(data));

    JS_LOCAL_VALUE outString;

    outString = Encode(data, dataSize, BUFFER);

    delete[] data;

    RETURN_POINTER(outString);
  }
  JS_METHOD_END

  static JS_LOCAL_METHOD(GetGenerator) {
    DiffieHellman* diffieHellman =
        ObjectWrap::Unwrap<DiffieHellman>(args.This());

    if (!diffieHellman->initialised_) {
      THROW_EXCEPTION("Not initialized");
    }

    int dataSize = BN_num_bytes(diffieHellman->dh->g);
    char* data = new char[dataSize];
    BN_bn2bin(diffieHellman->dh->g, reinterpret_cast<unsigned char*>(data));

    JS_LOCAL_VALUE outString;

    outString = Encode(data, dataSize, BUFFER);

    delete[] data;

    RETURN_POINTER(outString);
  }
  JS_METHOD_END

  static JS_LOCAL_METHOD(GetPublicKey) {
    DiffieHellman* diffieHellman =
        ObjectWrap::Unwrap<DiffieHellman>(args.This());

    if (!diffieHellman->initialised_) {
      THROW_EXCEPTION("Not initialized");
    }

    if (diffieHellman->dh->pub_key == NULL) {
      THROW_EXCEPTION("No public key - did you forget to generate one?");
    }

    int dataSize = BN_num_bytes(diffieHellman->dh->pub_key);
    char* data = new char[dataSize];
    BN_bn2bin(diffieHellman->dh->pub_key,
              reinterpret_cast<unsigned char*>(data));

    JS_LOCAL_VALUE outString;

    outString = Encode(data, dataSize, BUFFER);

    delete[] data;

    RETURN_POINTER(outString);
  }
  JS_METHOD_END

  static JS_LOCAL_METHOD(GetPrivateKey) {
    DiffieHellman* diffieHellman =
        ObjectWrap::Unwrap<DiffieHellman>(args.This());

    if (!diffieHellman->initialised_) {
      THROW_EXCEPTION("Not initialized");
    }

    if (diffieHellman->dh->priv_key == NULL) {
      THROW_EXCEPTION("No private key - did you forget to generate one?");
    }

    int dataSize = BN_num_bytes(diffieHellman->dh->priv_key);
    char* data = new char[dataSize];
    BN_bn2bin(diffieHellman->dh->priv_key,
              reinterpret_cast<unsigned char*>(data));

    JS_LOCAL_VALUE outString;

    outString = Encode(data, dataSize, BUFFER);

    delete[] data;

    RETURN_POINTER(outString);
  }
  JS_METHOD_END

  static JS_LOCAL_METHOD(ComputeSecret) {
    DiffieHellman* diffieHellman =
        ObjectWrap::Unwrap<DiffieHellman>(args.This());

    if (!diffieHellman->initialised_) {
      THROW_EXCEPTION("Not initialized");
    }

    ClearErrorOnReturn clear_error_on_return;
    (void)&clear_error_on_return;  // Silence compiler warning.
    BIGNUM* key = 0;

    if (args.Length() == 0) {
      THROW_EXCEPTION("First argument must be other party's public key");
    } else {
      ASSERT_IS_BUFFER(GET_ARG(0));
      key =
          BN_bin2bn(reinterpret_cast<unsigned char*>(BUFFER__DATA(GET_ARG(0))),
                    BUFFER__LENGTH(GET_ARG(0)), 0);
    }

    int dataSize = DH_size(diffieHellman->dh);
    char* data = new char[dataSize];

    int size = DH_compute_key(reinterpret_cast<unsigned char*>(data), key,
                              diffieHellman->dh);

    if (size == -1) {
      int checkResult;
      int checked;

      checked = DH_check_pub_key(diffieHellman->dh, key, &checkResult);
      BN_free(key);
      delete[] data;

      if (!checked) {
        THROW_EXCEPTION("Invalid key");
      } else if (checkResult) {
        if (checkResult & DH_CHECK_PUBKEY_TOO_SMALL) {
          THROW_EXCEPTION("Supplied key is too small");
        } else if (checkResult & DH_CHECK_PUBKEY_TOO_LARGE) {
          THROW_EXCEPTION("Supplied key is too large");
        } else {
          THROW_EXCEPTION("Invalid key");
        }
      } else {
        THROW_EXCEPTION("Invalid key");
      }
    }

    BN_free(key);
    assert(size >= 0);

    // DH_size returns number of bytes in a prime number
    // DH_compute_key returns number of bytes in a remainder of exponent, which
    // may have less bytes than a prime number. Therefore add 0-padding to the
    // allocated buffer.
    if (size != dataSize) {
      assert(dataSize > size);
      memmove(data + dataSize - size, data, size);
      memset(data, 0, dataSize - size);
    }

    JS_LOCAL_VALUE outString;

    outString = Encode(data, dataSize, BUFFER);

    delete[] data;
    RETURN_POINTER(outString);
  }
  JS_METHOD_END

  static JS_LOCAL_METHOD(SetPublicKey) {
    DiffieHellman* diffieHellman =
        ObjectWrap::Unwrap<DiffieHellman>(args.This());

    if (!diffieHellman->initialised_) {
      THROW_EXCEPTION("Not initialized");
    }

    if (args.Length() == 0) {
      THROW_TYPE_EXCEPTION("First argument must be public key");
    } else {
      ASSERT_IS_BUFFER(GET_ARG(0));
      diffieHellman->dh->pub_key =
          BN_bin2bn(reinterpret_cast<unsigned char*>(BUFFER__DATA(GET_ARG(0))),
                    BUFFER__LENGTH(GET_ARG(0)), 0);
    }

    RETURN_PARAM(args.This());
  }
  JS_METHOD_END

  static JS_LOCAL_METHOD(SetPrivateKey) {
    DiffieHellman* diffieHellman =
        ObjectWrap::Unwrap<DiffieHellman>(args.This());

    if (!diffieHellman->initialised_) {
      THROW_EXCEPTION("Not initialized");
    }

    if (args.Length() == 0) {
      THROW_TYPE_EXCEPTION("First argument must be private key");
    } else {
      ASSERT_IS_BUFFER(GET_ARG(0));
      diffieHellman->dh->priv_key =
          BN_bin2bn(reinterpret_cast<unsigned char*>(BUFFER__DATA(GET_ARG(0))),
                    BUFFER__LENGTH(GET_ARG(0)), 0);
    }

    RETURN_PARAM(args.This());
  }
  JS_METHOD_END

  DiffieHellman() : ObjectWrap() {
    initialised_ = false;
    dh = NULL;
  }

  ~DiffieHellman() {
    if (dh != NULL) {
      DH_free(dh);
    }
  }

 private:
  bool VerifyContext() {
    int codes;
    if (!DH_check(dh, &codes)) return false;
    if (codes & DH_CHECK_P_NOT_SAFE_PRIME) return false;
    if (codes & DH_CHECK_P_NOT_PRIME) return false;
    if (codes & DH_UNABLE_TO_CHECK_GENERATOR) return false;
    if (codes & DH_NOT_SUITABLE_GENERATOR) return false;
    return true;
  }

  bool initialised_;
  DH* dh;
};


class ECDH : public ObjectWrap {
public:
  INIT_NAMED_CLASS_MEMBERS(ECDH, ECDH)

  NODE_SET_PROTOTYPE_METHOD(constructor, "generateKeys", GenerateKeys);
  NODE_SET_PROTOTYPE_METHOD(constructor, "computeSecret", ComputeSecret);
  NODE_SET_PROTOTYPE_METHOD(constructor, "getPublicKey", GetPublicKey);
  NODE_SET_PROTOTYPE_METHOD(constructor, "getPrivateKey", GetPrivateKey);
  NODE_SET_PROTOTYPE_METHOD(constructor, "setPublicKey", SetPublicKey);
  NODE_SET_PROTOTYPE_METHOD(constructor, "setPrivateKey", SetPrivateKey);

  END_INIT_NAMED_MEMBERS(ECDH)

  ~ECDH() {
    if (key_ != NULL)
      EC_KEY_free(key_);
    key_ = NULL;
    group_ = NULL;
  }

protected:
  static JS_LOCAL_METHOD(New) {
    // TODO(indutny): Support raw curves?
    assert(args.IsString(0));

    jxcore::JXString curve;
    args.GetString(0, &curve);
    
    int nid = OBJ_sn2nid(*curve);
    if (nid == NID_undef)
      THROW_EXCEPTION("First argument should be a valid curve name");

    EC_KEY* key = EC_KEY_new_by_curve_name(nid);
    if (key == NULL)
      THROW_EXCEPTION("Failed to create EC_KEY using curve name");

    JS_CLASS_NEW_INSTANCE(obj, ECDH);
    ECDH *ecdh = new ECDH(key);
    ecdh->Wrap(obj);
    RETURN_POINTER(obj);
  }
  JS_METHOD_END

  static JS_LOCAL_METHOD(GenerateKeys) {
    ECDH* ecdh = Unwrap<ECDH>(args.This());

    if (!EC_KEY_generate_key(ecdh->key_))
      THROW_EXCEPTION("Failed to generate EC_KEY");

    ecdh->generated_ = true;
  }
  JS_METHOD_END

  static JS_LOCAL_METHOD(ComputeSecret) {
    ASSERT_IS_BUFFER(args.GetItem(0));

    ECDH* ecdh = Unwrap<ECDH>(args.This());

    EC_POINT* pub = ecdh->BufferToPoint(JS_GET_STATE_MARKER(), Buffer::Data(args.GetItem(0)),
      Buffer::Length(args.GetItem(0)));
    if (pub == NULL)
      RETURN();

    // NOTE: field_size is in bits
    int field_size = EC_GROUP_get_degree(ecdh->group_);
    size_t out_len = (field_size + 7) / 8;
    char* out = static_cast<char*>(malloc(out_len));
    assert(out != NULL);

    int r = ECDH_compute_key(out, out_len, pub, ecdh->key_, NULL);
    EC_POINT_free(pub);
    if (!r) {
      free(out);
      THROW_EXCEPTION("Failed to compute ECDH key");
    }

    Buffer *buff = Buffer::New(out, out_len, com);
    free(out);
    RETURN_PARAM(JS_TYPE_TO_LOCAL_OBJECT(buff->handle_));
  }
  JS_METHOD_END

  static JS_LOCAL_METHOD(GetPublicKey) {
    // Conversion form
    assert(args.Length() == 1);

    ECDH* ecdh = Unwrap<ECDH>(args.This());

    if (!ecdh->generated_)
      THROW_EXCEPTION("You should generate ECDH keys first");

    const EC_POINT* pub = EC_KEY_get0_public_key(ecdh->key_);
    if (pub == NULL)
      THROW_EXCEPTION("Failed to get ECDH public key");

    int size;
    point_conversion_form_t form =
      static_cast<point_conversion_form_t>(args.GetUInteger(0));

    size = EC_POINT_point2oct(ecdh->group_, pub, form, NULL, 0, NULL);
    if (size == 0)
      THROW_EXCEPTION("Failed to get public key length");

    unsigned char* out = static_cast<unsigned char*>(malloc(size));
    assert(out != NULL);

    int r = EC_POINT_point2oct(ecdh->group_, pub, form, out, size, NULL);
    if (r != size) {
      free(out);
      THROW_EXCEPTION("Failed to get public key");
    }

    Buffer *buff = Buffer::New(reinterpret_cast<char*>(out), size, com);
    free(out);
    RETURN_PARAM(JS_TYPE_TO_LOCAL_OBJECT(buff->handle_));
  }
  JS_METHOD_END

  static JS_LOCAL_METHOD(GetPrivateKey) {
    ECDH* ecdh = Unwrap<ECDH>(args.This());

    if (!ecdh->generated_)
      THROW_EXCEPTION("You should generate ECDH keys first");

    const BIGNUM* b = EC_KEY_get0_private_key(ecdh->key_);
    if (b == NULL)
      THROW_EXCEPTION("Failed to get ECDH private key");

    int size = BN_num_bytes(b);
    unsigned char* out = static_cast<unsigned char*>(malloc(size));
    assert(out != NULL);

    if (size != BN_bn2bin(b, out)) {
      free(out);
      THROW_EXCEPTION("Failed to convert ECDH private key to Buffer");
    }

    Buffer *buff = Buffer::New(reinterpret_cast<char*>(out), size, com);
    free(out);
    RETURN_PARAM(JS_TYPE_TO_LOCAL_OBJECT(buff->handle_));
  }
  JS_METHOD_END

  static JS_LOCAL_METHOD(SetPublicKey) {
    ECDH* ecdh = Unwrap<ECDH>(args.This());

    ASSERT_IS_BUFFER(args.GetItem(0));

    EC_POINT* pub = ecdh->BufferToPoint(JS_GET_STATE_MARKER(), Buffer::Data(args.GetItem(0)),
      Buffer::Length(args.GetItem(0)));
    if (pub == NULL)
      RETURN();

    int r = EC_KEY_set_public_key(ecdh->key_, pub);
    EC_POINT_free(pub);
    if (!r)
      THROW_EXCEPTION("Failed to convert BN to a private key");
  }
  JS_METHOD_END

  static JS_LOCAL_METHOD(SetPrivateKey) {
    ECDH* ecdh = Unwrap<ECDH>(args.This());

    ASSERT_IS_BUFFER(args.GetItem(0));

    BIGNUM* priv = BN_bin2bn(
      reinterpret_cast<unsigned char*>(Buffer::Data(args.GetItem(0))),
      Buffer::Length(args.GetItem(0)),
      NULL);
    if (priv == NULL)
      THROW_EXCEPTION("Failed to convert Buffer to BN");

    if (!EC_KEY_set_private_key(ecdh->key_, priv))
      THROW_EXCEPTION("Failed to convert BN to a private key");
  }
  JS_METHOD_END

  EC_POINT *BufferToPoint(JS_STATE_MARKER, char* data, size_t len) {
    EC_POINT* pub;
    int r;

    pub = EC_POINT_new(group_);
    if (pub == NULL) {
      THROW_EXCEPTION_NO_RETURN("Failed to allocate EC_POINT for a public key");
      return NULL;
    }

    r = EC_POINT_oct2point(
      group_,
      pub,
      reinterpret_cast<unsigned char*>(data),
      len,
      NULL);
    if (!r) {
      THROW_EXCEPTION_NO_RETURN("Failed to translate Buffer to a EC_POINT");
      goto fatal;
    }

    return pub;

  fatal:
    EC_POINT_free(pub);
    return NULL;
  }

  ECDH(EC_KEY *key) : ObjectWrap(), generated_(false), key_(key), group_(EC_KEY_get0_group(key_)) {
    //MakeWeak<ECDH>(this);
    assert(group_ != NULL);
  }

  bool generated_;
  EC_KEY  *key_;
  const EC_GROUP *group_;
};


struct pbkdf2_req {
  uv_work_t work_req;
  int err;
  char* pass;
  size_t passlen;
  char* salt;
  size_t saltlen;
  size_t iter;
  char* key;
  size_t keylen;
  JS_PERSISTENT_OBJECT obj;
};

void EIO_PBKDF2(pbkdf2_req* req) {
  req->err = PKCS5_PBKDF2_HMAC_SHA1(
      req->pass, req->passlen, (unsigned char*)req->salt, req->saltlen,
      req->iter, req->keylen, (unsigned char*)req->key);
  memset(req->pass, 0, req->passlen);
  memset(req->salt, 0, req->saltlen);
}

void EIO_PBKDF2(uv_work_t* work_req) {
  pbkdf2_req* req = container_of(work_req, pbkdf2_req, work_req);
  EIO_PBKDF2(req);
}

void EIO_PBKDF2After(node::commons* com, pbkdf2_req* req,
                     JS_LOCAL_VALUE argv[2]) {
  JS_DEFINE_STATE_MARKER(com);

  if (req->err) {
    argv[0] = JS_UNDEFINED();
    argv[1] = Encode(req->key, req->keylen, BUFFER);
    memset(req->key, 0, req->keylen);
  } else {
    argv[0] = JS_GET_ERROR_VALUE(
        ENGINE_NS::Exception::Error(STD_TO_STRING("PBKDF2 error")));
    argv[1] = JS_UNDEFINED();
  }
}

void EIO_PBKDF2Cleanup(pbkdf2_req* req) {
  delete[] req->pass;
  delete[] req->salt;
  delete[] req->key;
  JS_CLEAR_PERSISTENT(req->obj);
  delete req;
}

void EIO_PBKDF2After(uv_work_t* work_req, int status) {
  assert(status == 0);
  pbkdf2_req* req = container_of(work_req, pbkdf2_req, work_req);
  JS_ENTER_SCOPE_COM();
  JS_LOCAL_VALUE argv[2];
  JS_LOCAL_OBJECT obj = JS_OBJECT_FROM_PERSISTENT(req->obj);
  EIO_PBKDF2After(com, req, argv);
  MakeCallback(com, obj, JS_PREDEFINED_STRING(ondone), ARRAY_SIZE(argv), argv);
  EIO_PBKDF2Cleanup(req);
}

JS_LOCAL_METHOD(PBKDF2) {
  const char* type_error = NULL;
  char* pass = NULL;
  char* salt = NULL;
  ssize_t passlen = -1;
  ssize_t saltlen = -1;
  ssize_t keylen = -1;
  ssize_t pass_written = -1;
  ssize_t salt_written = -1;
  ssize_t iter = -1;
  pbkdf2_req* req = NULL;

  if (args.Length() != 4 && args.Length() != 5) {
    type_error = "Bad parameter";
    goto err;
  }

  ASSERT_IS_BUFFER(GET_ARG(0));
  passlen = BUFFER__LENGTH(GET_ARG(0));
  if (passlen < 0) {
    type_error = "Bad password";
    goto err;
  }

  pass = new char[passlen];
  pass_written = DecodeWrite(pass, passlen, GET_ARG(0), BINARY);
  assert(pass_written == passlen);

  ASSERT_IS_BUFFER(GET_ARG(1));
  saltlen = BUFFER__LENGTH(GET_ARG(1));
  if (saltlen < 0) {
    type_error = "Bad salt";
    goto err;
  }

  salt = new char[saltlen];
  salt_written = DecodeWrite(salt, saltlen, GET_ARG(1), BINARY);
  assert(salt_written == saltlen);

  if (!args.IsNumber(2)) {
    type_error = "Iterations not a number";
    goto err;
  }

  iter = args.GetInteger(2);
  if (iter < 0) {
    type_error = "Bad iterations";
    goto err;
  }

  if (!args.IsNumber(3)) {
    type_error = "Key length not a number";
    goto err;
  }

  keylen = args.GetInteger(3);
  if (keylen < 0) {
    type_error = "Bad key length";
    goto err;
  }

  req = new pbkdf2_req;
  req->err = 0;
  req->pass = pass;
  req->passlen = passlen;
  req->salt = salt;
  req->saltlen = saltlen;
  req->iter = iter;
  req->key = new char[keylen];
  req->keylen = keylen;

  if (args.IsFunction(4)) {
    JS_NEW_PERSISTENT_OBJECT(req->obj, JS_NEW_EMPTY_OBJECT());
    JS_NAME_SET(JS_OBJECT_FROM_PERSISTENT(req->obj), JS_STRING_ID("ondone"), GET_ARG(4));
    uv_queue_work(com->loop, &req->work_req, EIO_PBKDF2, EIO_PBKDF2After);
    RETURN();
  } else {
    JS_LOCAL_VALUE argv[2];
    EIO_PBKDF2(req);
    EIO_PBKDF2After(com, req, argv);
    EIO_PBKDF2Cleanup(req);
    if (!JS_IS_UNDEFINED(argv[0])) {
      THROW_EXCEPTION_OBJECT(argv[0]);
    }
    RETURN_POINTER(argv[1]);
  }

err:
  delete[] salt;
  delete[] pass;
  THROW_TYPE_EXCEPTION(type_error);
}
JS_METHOD_END

struct RandomBytesRequest {
  ~RandomBytesRequest();
  JS_PERSISTENT_OBJECT obj_;
  unsigned long error_;  // openssl error code or zero
  uv_work_t work_req_;
  size_t size_;
  char* data_;
};

RandomBytesRequest::~RandomBytesRequest() {
  JS_CLEAR_PERSISTENT(obj_);
}

void RandomBytesFree(char* data, void* hint) { delete[] data; }

template <bool pseudoRandom>
void RandomBytesWork(uv_work_t* work_req) {
  RandomBytesRequest* req =
      container_of(work_req, RandomBytesRequest, work_req_);
  int r;

  // Ensure that OpenSSL's PRNG is properly seeded.
  CheckEntropy();

  if (pseudoRandom == true) {
    r = RAND_pseudo_bytes(reinterpret_cast<unsigned char*>(req->data_),
                          req->size_);
  } else {
    r = RAND_bytes(reinterpret_cast<unsigned char*>(req->data_), req->size_);
  }

  // RAND_bytes() returns 0 on error. RAND_pseudo_bytes() returns 0 when the
  // result is not cryptographically strong - but that's not an error.
  if (r == 0 && pseudoRandom == false) {
    req->error_ = ERR_get_error();
  } else if (r == -1) {
    req->error_ = static_cast<unsigned long>(-1);
  }
}

// don't call this function without a valid HandleScope
void RandomBytesCheck(commons* com, RandomBytesRequest* req,
                      JS_LOCAL_VALUE argv[2]) {
  JS_DEFINE_STATE_MARKER(com);

  if (req->error_) {
    char errmsg[256] = "Operation not supported";

    if (req->error_ != (unsigned long)-1)
      ERR_error_string_n(req->error_, errmsg, sizeof errmsg);

    argv[0] =
        JS_GET_ERROR_VALUE(ENGINE_NS::Exception::Error(STD_TO_STRING(errmsg)));
    argv[1] = JS_NULL();
  } else {
    // avoids the malloc + memcpy
    Buffer* buffer =
        Buffer::New(req->data_, req->size_, RandomBytesFree, NULL, com);
    argv[0] = JS_NULL();
    argv[1] = JS_OBJECT_FROM_PERSISTENT(buffer->handle_);
    req->data_ = NULL;
  }
  free(req->data_);
}

void RandomBytesAfter(uv_work_t* work_req, int status) {
  assert(status == 0);
  RandomBytesRequest* req =
      container_of(work_req, RandomBytesRequest, work_req_);
  JS_ENTER_SCOPE_COM();
  JS_LOCAL_VALUE argv[2];
  RandomBytesCheck(com, req, argv);
  MakeCallback(com, JS_OBJECT_FROM_PERSISTENT(req->obj_), JS_PREDEFINED_STRING(ondone), ARRAY_SIZE(argv), argv);
  delete req;
}

template <bool pseudoRandom>
JS_LOCAL_METHOD(RandomBytes) {
  // maybe allow a buffer to write to? cuts down on object creation
  // when generating random data in a loop
  if (!args.IsUnsigned(0)) {
    THROW_TYPE_EXCEPTION("Argument #1 must be number > 0");
  }

  const uint32_t size = args.GetUInteger(0);
  if (size > Buffer::kMaxLength) {
    THROW_TYPE_EXCEPTION("size > Buffer::kMaxLength");
  }

  RandomBytesRequest* req = new RandomBytesRequest();
  req->error_ = 0;
  req->data_ = new char[size];
  req->size_ = size;

  if (args.IsFunction(1)) {
    JS_NEW_EMPTY_PERSISTENT_OBJECT(req->obj_);
    JS_LOCAL_OBJECT objl = JS_OBJECT_FROM_PERSISTENT(req->obj_);
    JS_NAME_SET(objl, JS_STRING_ID("ondone"), GET_ARG(1));
    uv_queue_work(com->loop, &req->work_req_, RandomBytesWork<pseudoRandom>,
                  RandomBytesAfter);

    RETURN_POINTER(objl);
  } else {
    JS_LOCAL_VALUE argv[2];
    RandomBytesWork<pseudoRandom>(&req->work_req_);
    RandomBytesCheck(com, req, argv);
    delete req;

    if (!JS_IS_NULL(argv[0]))
      THROW_EXCEPTION_OBJECT(argv[0]);
    else
      RETURN_POINTER(argv[1]);
  }
}
JS_METHOD_END

JS_LOCAL_METHOD(GetSSLCiphers) {
  SSL_CTX* ctx = SSL_CTX_new(TLSv1_server_method());
  if (ctx == NULL) {
    THROW_EXCEPTION("SSL_CTX_new() failed.");
  }

  SSL* ssl = SSL_new(ctx);
  if (ssl == NULL) {
    SSL_CTX_free(ctx);
    THROW_EXCEPTION("SSL_new() failed.");
  }

  JS_LOCAL_ARRAY arr = JS_NEW_ARRAY();
  STACK_OF(SSL_CIPHER)* ciphers = SSL_get_ciphers(ssl);

  for (int i = 0; i < sk_SSL_CIPHER_num(ciphers); ++i) {
    SSL_CIPHER* cipher = sk_SSL_CIPHER_value(ciphers, i);
    JS_INDEX_SET(arr, i, STD_TO_STRING(SSL_CIPHER_get_name(cipher)));
  }

  SSL_free(ssl);
  SSL_CTX_free(ctx);

  RETURN_POINTER(arr);
}
JS_METHOD_END

template <class TypeName>
static void array_push_back(const TypeName* md, const char* from,
                            const char* to, void* arg) {
  commons* com = node::commons::getInstance();
  JS_DEFINE_STATE_MARKER(com);

  JS_LOCAL_ARRAY& arr = *static_cast<JS_LOCAL_ARRAY*>(arg);
  JS_INDEX_SET(arr, JS_GET_ARRAY_LENGTH(arr), STD_TO_STRING(from));
}

JS_LOCAL_METHOD(GetCiphers) {
  JS_LOCAL_ARRAY arr = JS_NEW_ARRAY();
  EVP_CIPHER_do_all_sorted(array_push_back<EVP_CIPHER>, &arr);
  RETURN_POINTER(arr);
}
JS_METHOD_END

JS_LOCAL_METHOD(GetHashes) {
  JS_LOCAL_ARRAY arr = JS_NEW_ARRAY();
  EVP_MD_do_all_sorted(array_push_back<EVP_MD>, &arr);
  RETURN_POINTER(arr);
}
JS_METHOD_END

DECLARE_CLASS_INITIALIZER(InitCrypto) {
  JS_ENTER_SCOPE_COM();
  JS_DEFINE_STATE_MARKER(com);

  customLock(CSLOCK_CRYPTO);
  if (!commons::ssl_initialized_) {
    commons::ssl_initialized_ = true;
    SSL_library_init();
    OpenSSL_add_all_algorithms();
    OpenSSL_add_all_digests();
    SSL_load_error_strings();
    ERR_load_crypto_strings();

    crypto_lock_init();
    CRYPTO_set_locking_callback(crypto_lock_cb);
    CRYPTO_THREADID_set_callback(crypto_threadid_cb);

// Turn off compression. Saves memory - do it in userland.
#if !defined(OPENSSL_NO_COMP)
    STACK_OF(SSL_COMP)* comp_methods =
#if OPENSSL_VERSION_NUMBER < 0x00908000L
        SSL_COMP_get_compression_method();
#else
        SSL_COMP_get_compression_methods();
#endif
    sk_SSL_COMP_zero(comp_methods);
    assert(sk_SSL_COMP_num(comp_methods) == 0);
#endif
  }
  customUnlock(CSLOCK_CRYPTO);

  SecureContext::Initialize(target);
  Connection::Initialize(target);
  Cipher::Initialize(target);
  Decipher::Initialize(target);
  DiffieHellman::Initialize(target);
  ECDH::Initialize(target);
  Hmac::Initialize(target);
  Hash::Initialize(target);
  Sign::Initialize(target);
  Verify::Initialize(target);

  JS_METHOD_SET(target, "PBKDF2", PBKDF2);
  JS_METHOD_SET(target, "randomBytes", RandomBytes<false>);
  JS_METHOD_SET(target, "pseudoRandomBytes", RandomBytes<true>);
  JS_METHOD_SET(target, "getSSLCiphers", GetSSLCiphers);
  JS_METHOD_SET(target, "getCiphers", GetCiphers);
  JS_METHOD_SET(target, "getHashes", GetHashes);

  JS_METHOD_SET(target, "publicEncrypt", PublicKeyCipher::PublicEncrypt);
  JS_METHOD_SET(target, "privateDecrypt", PublicKeyCipher::PrivateDecrypt);

  JS_NAME_SET(target, JS_STRING_ID("SSL3_ENABLE"), STD_TO_BOOLEAN(SSL3_ENABLE));
  JS_NAME_SET(target, JS_STRING_ID("SSL2_ENABLE"), STD_TO_BOOLEAN(SSL2_ENABLE));
}

}  // namespace crypto
}  // namespace node

NODE_MODULE(node_crypto, node::crypto::InitCrypto)
