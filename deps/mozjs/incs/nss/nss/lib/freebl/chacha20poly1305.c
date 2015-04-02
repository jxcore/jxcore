/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifdef FREEBL_NO_DEPEND
#include "stubs.h"
#endif

#include <string.h>
#include <stdio.h>

#include "seccomon.h"
#include "secerr.h"
#include "blapit.h"
#include "poly1305/poly1305.h"
#include "chacha20/chacha20.h"
#include "chacha20poly1305.h"

/* Poly1305Do writes the Poly1305 authenticator of the given additional data
 * and ciphertext to |out|. */
static void
Poly1305Do(unsigned char *out,
	   const unsigned char *ad, unsigned int adLen,
	   const unsigned char *ciphertext, unsigned int ciphertextLen,
	   const unsigned char key[32])
{
    poly1305_state state;
    unsigned int j;
    unsigned char lengthBytes[8];
    unsigned int i;

    Poly1305Init(&state, key);
    j = adLen;
    for (i = 0; i < sizeof(lengthBytes); i++) {
	lengthBytes[i] = j;
	j >>= 8;
    }
    Poly1305Update(&state, ad, adLen);
    Poly1305Update(&state, lengthBytes, sizeof(lengthBytes));
    j = ciphertextLen;
    for (i = 0; i < sizeof(lengthBytes); i++) {
	lengthBytes[i] = j;
	j >>= 8;
    }
    Poly1305Update(&state, ciphertext, ciphertextLen);
    Poly1305Update(&state, lengthBytes, sizeof(lengthBytes));
    Poly1305Finish(&state, out);
}

SECStatus
ChaCha20Poly1305_InitContext(ChaCha20Poly1305Context *ctx,
			     const unsigned char *key, unsigned int keyLen,
			     unsigned int tagLen)
{
    if (keyLen != 32) {
	PORT_SetError(SEC_ERROR_BAD_KEY);
	return SECFailure;
    }
    if (tagLen == 0 || tagLen > 16) {
	PORT_SetError(SEC_ERROR_INPUT_LEN);
	return SECFailure;
    }

    memcpy(ctx->key, key, sizeof(ctx->key));
    ctx->tagLen = tagLen;

    return SECSuccess;
}

ChaCha20Poly1305Context *
ChaCha20Poly1305_CreateContext(const unsigned char *key, unsigned int keyLen,
			       unsigned int tagLen)
{
    ChaCha20Poly1305Context *ctx;

    ctx = PORT_New(ChaCha20Poly1305Context);
    if (ctx == NULL) {
	return NULL;
    }

    if (ChaCha20Poly1305_InitContext(ctx, key, keyLen, tagLen) != SECSuccess) {
	PORT_Free(ctx);
	ctx = NULL;
    }

    return ctx;
}

void
ChaCha20Poly1305_DestroyContext(ChaCha20Poly1305Context *ctx, PRBool freeit)
{
    memset(ctx, 0, sizeof(*ctx));
    if (freeit) {
	PORT_Free(ctx);
    }
}

SECStatus
ChaCha20Poly1305_Seal(const ChaCha20Poly1305Context *ctx,
		      unsigned char *output, unsigned int *outputLen,
		      unsigned int maxOutputLen,
		      const unsigned char *input, unsigned int inputLen,
		      const unsigned char *nonce, unsigned int nonceLen,
		      const unsigned char *ad, unsigned int adLen)
{
    unsigned char block[64];
    unsigned char tag[16];

    if (nonceLen != 8) {
	PORT_SetError(SEC_ERROR_INPUT_LEN);
	return SECFailure;
    }
    *outputLen = inputLen + ctx->tagLen;
    if (maxOutputLen < *outputLen) {
	PORT_SetError(SEC_ERROR_OUTPUT_LEN);
	return SECFailure;
    }

    memset(block, 0, sizeof(block));
    // Generate a block of keystream. The first 32 bytes will be the poly1305
    // key. The remainder of the block is discarded.
    ChaCha20XOR(block, block, sizeof(block), ctx->key, nonce, 0);
    ChaCha20XOR(output, input, inputLen, ctx->key, nonce, 1);

    Poly1305Do(tag, ad, adLen, output, inputLen, block);
    memcpy(output + inputLen, tag, ctx->tagLen);

    return SECSuccess;
}

SECStatus
ChaCha20Poly1305_Open(const ChaCha20Poly1305Context *ctx,
		      unsigned char *output, unsigned int *outputLen,
		      unsigned int maxOutputLen,
		      const unsigned char *input, unsigned int inputLen,
		      const unsigned char *nonce, unsigned int nonceLen,
		      const unsigned char *ad, unsigned int adLen)
{
    unsigned char block[64];
    unsigned char tag[16];

    if (nonceLen != 8) {
	PORT_SetError(SEC_ERROR_INPUT_LEN);
	return SECFailure;
    }
    if (inputLen < ctx->tagLen) {
	PORT_SetError(SEC_ERROR_INPUT_LEN);
	return SECFailure;
    }
    *outputLen = inputLen - ctx->tagLen;
    if (maxOutputLen < *outputLen) {
	PORT_SetError(SEC_ERROR_OUTPUT_LEN);
	return SECFailure;
    }

    memset(block, 0, sizeof(block));
    // Generate a block of keystream. The first 32 bytes will be the poly1305
    // key. The remainder of the block is discarded.
    ChaCha20XOR(block, block, sizeof(block), ctx->key, nonce, 0);
    Poly1305Do(tag, ad, adLen, input, inputLen - ctx->tagLen, block);
    if (NSS_SecureMemcmp(tag, &input[inputLen - ctx->tagLen], ctx->tagLen) != 0) {
	PORT_SetError(SEC_ERROR_BAD_DATA);
	return SECFailure;
    }

    ChaCha20XOR(output, input, inputLen - ctx->tagLen, ctx->key, nonce, 1);

    return SECSuccess;
}
