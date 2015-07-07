// Copyright & License details are available under JXCORE_LICENSE file

#include "crypto_extension.h"

#include "openssl/ssl.h"
#include "openssl/rand.h"
#include "openssl/err.h"
#include "openssl/pkcs12.h"

#include "node_buffer.h"

#define EC_CURVE_NAME "secp256k1"
#define EVP_DIGEST_NAME EVP_sha256

#define FAIL(msg)                                                        \
  free_openssl_resources(privkey, x509_cert, cacertstack, pkcs12bundle); \
  THROW_EXCEPTION(msg)

void free_openssl_resources(EVP_PKEY *key, X509 *cert,
                            STACK_OF(X509) * certstack, PKCS12 *pkcs12) {
  if (pkcs12 != NULL) {
    PKCS12_free(pkcs12);
  }
  if (certstack != NULL) {
    sk_X509_free(certstack);
  }
  if (cert != NULL) {
    X509_free(cert);
  }
  if (key != NULL) {
    EVP_PKEY_free(key);
  }
}

STACK_OF(X509) * create_ca_cert_stack(X509 *cert) {
  STACK_OF(X509) * certstack;
  if ((certstack = sk_X509_new_null()) == NULL) {
    return NULL;
  }

  sk_X509_push(certstack, cert);
  return certstack;
}

X509 *create_x509_cert(EVP_PKEY *privkey, char *country, char *organization) {
  X509 *x509;
  x509 = X509_new();

  ASN1_INTEGER_set(X509_get_serialNumber(x509), 0);

  X509_gmtime_adj(X509_get_notBefore(x509), 0);         // current time
  X509_gmtime_adj(X509_get_notAfter(x509), 31536000L);  // valid for 365 days

  X509_set_pubkey(x509, privkey);

  X509_NAME *name;
  name = X509_get_subject_name(
      x509);  // set the name of the issuer to the name of the subject

  X509_NAME_add_entry_by_txt(name, "C", MBSTRING_ASC, (unsigned char *)country,
                             -1, -1, 0);  // country
  X509_NAME_add_entry_by_txt(name, "O", MBSTRING_ASC,
                             (unsigned char *)organization, -1, -1,
                             0);     // organization
  X509_set_issuer_name(x509, name);  // set the issuer name

  int ret = X509_sign(x509, privkey, EVP_sha1());  // using the SHA-1 hashing
                                                   // algorithm to sign the key
                                                   // (MD5 is another option)
  if (!ret) {
    return NULL;
  }
  return x509;
}

EVP_PKEY *create_ec_key() {
  EC_GROUP *ecgroup = NULL;
  EC_KEY *eckey = NULL;
  point_conversion_form_t form = POINT_CONVERSION_UNCOMPRESSED;
  int asn1_flag = OPENSSL_EC_NAMED_CURVE;

  eckey = EC_KEY_new();
  ecgroup = EC_GROUP_new_by_curve_name(OBJ_sn2nid(EC_CURVE_NAME));
  EC_GROUP_set_asn1_flag(ecgroup, asn1_flag);
  EC_GROUP_set_point_conversion_form(ecgroup, form);
  EC_KEY_set_group(eckey, ecgroup);
  EC_KEY_generate_key(eckey);

  EVP_PKEY *evpKey = EVP_PKEY_new();
  if (EVP_PKEY_set1_EC_KEY(evpKey, eckey) != 1) {
    return NULL;
  }

  return evpKey;
}

namespace node {

///// API that gets called by JS code to generate HKDF
JS_METHOD(CryptoExtensionWrap, GenerateHKDF) {
  // TODO EVP_DIGEST_NAME can be optional
  const EVP_MD *(*md_func)(void) = EVP_DIGEST_NAME;
  uint8_t *buf;
  JS_LOCAL_VALUE ret_val;

  // Validate input parameters.
  if (args.Length() < 4) {
    THROW_EXCEPTION(
        "too few argument, expected arguments are (int, string / buffer, "
        "string / buffer, "
        "string / buffer)");
  }

  if (!args.IsUnsigned(0)) {
    THROW_EXCEPTION("first parameter needs to be a positive number");
  }

  int bytesToGenerate = args.GetInteger(0);

#define FAIL_NO_STRING(x, y)                                              \
  JS_HANDLE_STRING arg##y = args.GetAsString(y);                          \
  if (!JS_IS_STRING(arg##y) && !node::Buffer::jxHasInstance(arg##y, com)) \
    THROW_EXCEPTION(#x " is expected to be a string or buffer.");         \
  char *x;                                                                \
  int x##Length;                                                          \
  jxcore::JXString x##y;                                                  \
  if (JS_IS_STRING(arg##y)) {                                             \
    args.GetString(y, &x##y);                                             \
    x##Length = x##y.length();                                            \
    x = *x##y;                                                            \
  } else {                                                                \
    x = BUFFER__DATA(arg##y);                                             \
    x##Length = BUFFER__LENGTH(arg##y);                                   \
  }                                                                       \
  if (x##Length == 0) {                                                   \
    THROW_EXCEPTION(#x " cannot be an empty string");                     \
  }

  FAIL_NO_STRING(publicKeyString, 1)
  FAIL_NO_STRING(saltArray, 2)
  FAIL_NO_STRING(digestBuffer, 3)

#undef FAIL_NO_STRING

  if (!(buf = (uint8_t *)malloc(256))) {
    THROW_EXCEPTION("failed to create memory for output buffer");
  }

  uint8_t *out_key = buf;
  size_t out_len = bytesToGenerate;
  const EVP_MD *digest = md_func();
  const uint8_t *secret = (unsigned char *)publicKeyString;
  size_t secret_len = publicKeyStringLength;
  const uint8_t *salt = (unsigned char *)saltArray;
  size_t salt_len = saltArrayLength;
  const uint8_t *info = (unsigned char *)digestBuffer;
  size_t info_len = digestBufferLength;

  /* https://tools.ietf.org/html/rfc5869#section-2.2 */
  const size_t digest_len = EVP_MD_size(digest);
  uint8_t prk[EVP_MAX_MD_SIZE], previous[EVP_MAX_MD_SIZE];
  size_t n, done = 0;
  unsigned i, prk_len;
  HMAC_CTX hmac;

  /* If salt is not given, HashLength zeros are used. However, HMAC does that
   * internally already so we can ignore it.
   */

  /* Expand key material to desired length. */
  n = (out_len + digest_len - 1) / digest_len;
  if (out_len + digest_len < out_len || n > 255) {
    THROW_EXCEPTION("length check failed");
  }

  HMAC_CTX_init(&hmac);

  /* Extract input keying material into pseudorandom key |prk|. */
  if (HMAC(digest, salt, salt_len, secret, secret_len, prk, &prk_len) == NULL) {
    HMAC_CTX_cleanup(&hmac);
    THROW_EXCEPTION("creation of pseudorandom key failed");
  }

  if (prk_len != digest_len) {
    HMAC_CTX_cleanup(&hmac);
    THROW_EXCEPTION("pseudorandom key length is incorrect");
  }

  if (!HMAC_Init_ex(&hmac, prk, prk_len, digest, NULL)) {
    HMAC_CTX_cleanup(&hmac);
    THROW_EXCEPTION("hmac initialization failed");
  }

  for (i = 0; i < n; i++) {
    uint8_t ctr = i + 1;
    size_t todo;

    if (i != 0 && (!HMAC_Init_ex(&hmac, NULL, 0, NULL, NULL) ||
                   !HMAC_Update(&hmac, previous, digest_len))) {
      HMAC_CTX_cleanup(&hmac);
      THROW_EXCEPTION("first set of HMAC library calls failed");
    }

    if (!HMAC_Update(&hmac, info, info_len) || !~HMAC_Update(&hmac, &ctr, 1) ||
        !HMAC_Final(&hmac, previous, NULL)) {
      HMAC_CTX_cleanup(&hmac);
      THROW_EXCEPTION("second set of HMAC library calls failed");
    }

    todo = digest_len;
    if (done + todo > out_len) {
      todo = out_len - done;
    }

    memcpy(out_key + done, previous, todo);
    done += todo;
  }

  HMAC_CTX_cleanup(&hmac);

  node::Buffer *buff = node::Buffer::New((char *)buf, done, com);
  JS_LOCAL_OBJECT hval = JS_VALUE_TO_OBJECT(buff->handle_);

  RETURN_PARAM(hval);
}
JS_METHOD_END

///// API that gets called by JS code to create a PKCS12 bundle
JS_METHOD(CryptoExtensionWrap, CreateBundle) {
  EVP_PKEY *privkey = NULL;
  X509 *x509_cert = NULL;
  STACK_OF(X509) *cacertstack = NULL;
  PKCS12 *pkcs12bundle = NULL;

  BIO *wbio = NULL;
  char *pkcs12out = NULL;
  int size;
  char *tempptr;

#define FAIL_NO_STRING(x, y)                                               \
  if (!args.IsString(y))                                                   \
    THROW_EXCEPTION(#x " is expected to be a string. (" #y ". argument)"); \
  jxcore::JXString x;                                                      \
  args.GetString(y, &x);                                                   \
  if (x.length() == 0)                                                     \
  THROW_EXCEPTION(#x " can not be empty. (" #y ". argument)")

  FAIL_NO_STRING(password, 0);
  FAIL_NO_STRING(certname, 1);
  FAIL_NO_STRING(country, 2);
  FAIL_NO_STRING(organization, 3);

#undef FAIL_NO_STRING

  privkey = create_ec_key();
  if (privkey == NULL) {
    FAIL("create_ec_key failed.");
  }

  x509_cert =
      create_x509_cert(privkey, (char *)*country, (char *)*organization);
  if (x509_cert == NULL) {
    FAIL("create_x509_cert failed.");
  }

  cacertstack = create_ca_cert_stack(x509_cert);
  if (cacertstack == NULL) {
    FAIL("create_ca_cert_stack failed.");
  }

  if ((pkcs12bundle = PKCS12_new()) == NULL) {
    FAIL("PKCS12_new failed.");
  }

  pkcs12bundle = PKCS12_create(*password,    // certbundle access password
                               *certname,    // friendly certname
                               privkey,      // the certificate private key
                               x509_cert,    // the main certificate
                               cacertstack,  // stack of CA cert chain
                               0,            // int nid_key (default 3DES)
                               0,            // int nid_cert (40bitRC2)
                               0,            // int iter (default 2048)
                               0,            // int mac_iter (default 1)
                               0             // int keytype (default no flag)
                               );
  if (pkcs12bundle == NULL) {
    FAIL("PKCS12_create failed.\n");
  }

  if (!(wbio = BIO_new(BIO_s_mem()))) {
    FAIL("BIO init failed.\n");
  }

  i2d_PKCS12_bio(wbio, pkcs12bundle); /* der encode it */

  BIO_flush(wbio);
  size = BIO_get_mem_data(wbio, &tempptr);

  if (!(pkcs12out = (char *)malloc(size + 1))) {
    FAIL("Failed to create memory for pkcs12 buffer\n");
  }

  memcpy(pkcs12out, tempptr, size);
  pkcs12out[size] = '\0';

  free_openssl_resources(privkey, x509_cert, cacertstack, pkcs12bundle);

  node::Buffer *buff = node::Buffer::New(pkcs12out, size, com);
  JS_LOCAL_OBJECT hval = JS_VALUE_TO_OBJECT(buff->handle_);
  RETURN_PARAM(hval);
}
JS_METHOD_END

///// API that gets called by JS code to extract the public key from a PKCS12
/// bundle
JS_METHOD(CryptoExtensionWrap, ExtractPublicKey) {
  PKCS12 *pkcs12bundle = NULL;
  EVP_PKEY *privkey = NULL;
  EVP_PKEY *pubKey = NULL;
  X509 *x509_cert = NULL;
  STACK_OF(X509) *cacertstack = NULL;

  BIO *rbio;
  BUF_MEM *bm;
  BIO *wbio = NULL;
  char *pubkeyout = NULL;
  int size;
  char *tempptr;

  // Validate input parameters.
  if (args.Length() < 2 || !args.IsString(0)) {
    THROW_EXCEPTION(
        "expected arguments (string password, buffer/string pkcs12-bundle)");
  }

  // Extract input parameters.
  jxcore::JXString password;
  args.GetString(0, &password);

#define FAIL_NO_STRING(x, y)                                              \
  JS_HANDLE_STRING arg##y = args.GetAsString(y);                          \
  if (!JS_IS_STRING(arg##y) && !node::Buffer::jxHasInstance(arg##y, com)) \
    THROW_EXCEPTION(#x " is expected to be a string or buffer. (" #y      \
                       ". argument)");                                    \
  char *x;                                                                \
  int x##_length;                                                         \
  jxcore::JXString x##y;                                                  \
  if (JS_IS_STRING(arg##y)) {                                             \
    args.GetString(y, &x##y);                                             \
    if (x##y.length() == 0) {                                             \
      THROW_EXCEPTION(#x " can not be empty. (" #y ". argument)");        \
    }                                                                     \
    x##_length = x##y.length();                                           \
    x = *x##y;                                                            \
  } else {                                                                \
    if (BUFFER__LENGTH(arg##y) == 0) {                                    \
      THROW_EXCEPTION(#x " can not be empty. (" #y ". argument)");        \
    }                                                                     \
    x##_length = BUFFER__LENGTH(arg##y);                                  \
    x = BUFFER__DATA(arg##y);                                             \
  }

  FAIL_NO_STRING(pkcs12in, 1)

#undef FAIL_NO_STRING

  if (password.length() == 0) {
    THROW_EXCEPTION("password can not be empty");
  }

  if (!(rbio = BIO_new(BIO_s_mem()))) {
    FAIL("Failed to create memory for BIO");
  }

  if (!(bm = BUF_MEM_new())) {
    FAIL("Failed to create memory for BUF_MEM");
  }

  size = pkcs12in_length;
  if (!BUF_MEM_grow(bm, size)) {
    FAIL("Failed to grow memory for BUF_MEM");
  }

  memcpy(bm->data, pkcs12in, size);
  BIO_set_mem_buf(rbio, bm, 0 /*not used*/);

  d2i_PKCS12_bio(rbio, &pkcs12bundle);

  if (pkcs12bundle == NULL) {
    FAIL("Failed to fill the PKCS12 bundle");
  }

  if (!PKCS12_parse(pkcs12bundle, *password, &privkey, &x509_cert,
                    &cacertstack)) {
    FAIL("Failed to parse the PKCS12 bundle");
  }

  if (x509_cert == NULL) {
    FAIL("Failed to extract the certificate\n");
  }

  pubKey = X509_get_pubkey(x509_cert);
  if (pubKey == NULL) {
    FAIL("Failed to extract public-key from certificate\n");
  }

  if (!(wbio = BIO_new(BIO_s_mem()))) {
    FAIL("Failed to create memory to save public-key\n");
  }

  if (!PEM_write_bio_PUBKEY(wbio, pubKey)) {
    FAIL("Error writing public key data in PEM format\n");
  }

  BIO_flush(wbio);
  size = BIO_get_mem_data(wbio, &tempptr);

  if (!(pubkeyout = (char *)malloc(size + 1))) {
    FAIL("Failed to create memory for public-key buffer\n");
  }

  memcpy(pubkeyout, tempptr, size);
  pubkeyout[size] = '\0';

  free_openssl_resources(privkey, x509_cert, cacertstack, pkcs12bundle);

  node::Buffer *buff = node::Buffer::New(pubkeyout, size, com);
  JS_LOCAL_OBJECT hval = JS_VALUE_TO_OBJECT(buff->handle_);
  RETURN_PARAM(hval);
}
JS_METHOD_END

}  // namespace node

NODE_MODULE(node_crypto_extension_wrap, node::CryptoExtensionWrap::Initialize)