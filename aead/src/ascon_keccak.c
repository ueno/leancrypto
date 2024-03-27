/*
 * Copyright (C) 2024, Stephan Mueller <smueller@chronox.de>
 *
 * License: see LICENSE file in root directory
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE, ALL OF
 * WHICH ARE HEREBY DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF NOT ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 */

#include "alignment.h"
#include "bitshift.h"
#include "compare.h"
#include "ext_headers.h"
#include "lc_ascon_keccak.h"
#include "lc_memcmp_secure.h"
#include "lc_memory_support.h"
#include "math_helper.h"
#include "visibility.h"
#include "xor.h"

/*
 * Algorithms with 256 bit security strength based on the fact that the
 * capacity is 512 bits or larger.
 *
 *                       ---- Bit size of ----- Rounds
 *                       Key Nonce Tag DataBlock pa pb
 *                                     Rate
 * Ascon-Keccak 512/512  512 128   128  576      24 24
 * Ascon-Keccak 256/512  256 128   128  576      24 24
 * Ascon-Keccak 256/256  256 128   128 1088      24 24
 *
 * Note, the tag is allowed also to be larger, up to the size of the capacity.
 */
#define LC_AEAD_AK_SHA3_512_512_INIT 0x0200024000180018
#define LC_AEAD_AK_SHA3_256_512_INIT 0x0100024000180018
#define LC_AEAD_AK_SHA3_256_256_INIT 0x0100044000180018

static void lc_ak_selftest(int *tested, const char *impl)
{
	static const uint8_t in[] = {
		0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09,
		0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13,
		0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d,
		0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
		0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30, 0x31,
		0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b,
		0x3c, 0x3d, 0x3e, 0x3f,
	};
	static const uint8_t key[] = {
		0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89,
		0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f, 0x90, 0x91, 0x92, 0x93,
		0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d,
		0x9e, 0x9f,
	};
	static const uint8_t iv[] = {
		0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29,
		0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
	};
	static const uint8_t exp_ct[] = {
		0x66, 0xeb, 0x20, 0xf0, 0x33, 0xf0, 0x12, 0xe0, 0x5a, 0xe3,
		0x41, 0x0f, 0xf0, 0xf4, 0xe2, 0x86, 0xa7, 0xec, 0xd6, 0x5a,
		0x91, 0x74, 0x62, 0x04, 0xcd, 0x4c, 0xb7, 0xf5, 0x65, 0x68,
		0x98, 0x03, 0x1a, 0xf2, 0x49, 0xcf, 0x9f, 0x09, 0x86, 0x0c,
		0xe4, 0xda, 0xaa, 0xfa, 0xf0, 0x74, 0xc5, 0x4c, 0x87, 0x66,
		0x49, 0x53, 0x45, 0x1a, 0x63, 0x3f, 0x57, 0xfa, 0x5c, 0x7b,
		0x4b, 0xbe, 0xa1, 0xca
	};
	static const uint8_t exp_tag[] = {
		0x03, 0x3b, 0x2d, 0x50, 0x90, 0x16, 0x72, 0x4f, 0x36, 0xf4,
		0xe3, 0x0a, 0xfd, 0x58, 0x5c, 0x65
	};
	uint8_t act_ct[sizeof(exp_ct)] __align(sizeof(uint32_t));
	uint8_t act_tag[sizeof(exp_tag)] __align(sizeof(uint32_t));
	char status[35];

	LC_SELFTEST_RUN(tested);

	LC_AK_CTX_ON_STACK(ak, lc_sha3_256);

	assert(!lc_aead_setkey(ak, key, sizeof(key), iv, sizeof(iv)));
	lc_aead_encrypt(ak, in, act_ct, sizeof(in), in, sizeof(in), act_tag,
			sizeof(act_tag));
	snprintf(status, sizeof(status), "%s encrypt", impl);
	lc_compare_selftest(act_ct, exp_ct, sizeof(exp_ct), status);
	lc_compare_selftest(act_tag, exp_tag, sizeof(exp_tag), status);
	lc_aead_zero(ak);

	assert(!lc_aead_setkey(ak, key, sizeof(key), iv, sizeof(iv)));
	assert(!lc_aead_decrypt(ak, act_ct, act_ct, sizeof(act_ct), in,
				sizeof(in), act_tag, sizeof(act_tag)));

	snprintf(status, sizeof(status), "%s decrypt", impl);
	lc_compare_selftest(act_ct, in, sizeof(in), status);
	lc_aead_zero(ak);
}

/**
 * @brief Set the key for the encyption or decryption operation
 *
 * @param ak [in] Ascon-Keccak crypt cipher handle
 * @param key [in] Buffer with key
 * @param keylen [in] Length of key buffer
 * @param iv [in] initialization vector to be used
 * @param ivlen [in] length of initialization vector
 *
 * The algorithm supports a key of arbitrary size. The only requirement is that
 * the same key is used for decryption as for encryption.
 */
static int lc_ak_setkey(void *state, const uint8_t *key, size_t keylen,
			const uint8_t *iv, size_t ivlen)
{
	struct lc_ak_cryptor *ak = state;
	const struct lc_hash *hash = ak->hash;
	unsigned int i;
	static int tested = 0;

	lc_ak_selftest(&tested, "Asacon Keccak AEAD");

	memset(state, 0, LC_SHA3_STATE_SIZE);

	/* INIT || 0* || key || iv */
	switch (hash->rate) {
	case 0x240 / 8:
		if (ivlen != 16)
			return -EINVAL;

		switch (keylen) {
		case 32:
			ak->keccak_state[0] = LC_AEAD_AK_SHA3_256_512_INIT;
			break;
		case 64:
			ak->keccak_state[0] = LC_AEAD_AK_SHA3_512_512_INIT;
			break;
		default:
			return -EINVAL;
		}

		for (i = 1;
		     i < (LC_SHA3_STATE_WORDS - (16 + 32) / sizeof(uint64_t));
		     i++)
			ak->keccak_state[i] = 0;

		memcpy(ak->key, key, keylen);

		break;
	case 0x440 / 8:
		if (ivlen != 16 || keylen != 32)
			return -EINVAL;
		ak->keccak_state[0] = LC_AEAD_AK_SHA3_256_256_INIT;
		for (i = 1;
		     i < (LC_SHA3_STATE_WORDS - (16 + 16) / sizeof(uint64_t));
		     i++)
			ak->keccak_state[i] = 0;

		memcpy(ak->key, key, keylen);
		memset(ak->key + 16, 0, 16);
		break;
	default:
		return -EINVAL;
	}

	lc_keccak_add_bytes(hash, ak->keccak_state, key,
			    (unsigned int)(LC_SHA3_STATE_SIZE - ivlen - keylen),
			    (unsigned int)keylen);
	lc_keccak_add_bytes(hash, ak->keccak_state, iv,
			    (unsigned int)(LC_SHA3_STATE_SIZE - ivlen),
			    (unsigned int)ivlen);

	/* Keccak */
	lc_keccak(hash, state);

	/* XOR key to last part of state */
	lc_keccak_add_bytes(hash, ak->keccak_state, key,
			    (unsigned int)(LC_SHA3_STATE_SIZE - keylen),
			    (unsigned int)keylen);

	return 0;
}

static void lc_ak_add_padbyte(struct lc_ak_cryptor *ak, size_t offset)
{
	const struct lc_hash *hash = ak->hash;
	static const uint8_t pad_data = 0x80;

	/*
	 * The data was exactly a multiple of the rate -> permute before adding
	 * the padding byte.
	 */
	if (offset == hash->rate)
		lc_keccak(hash, ak->keccak_state);

	lc_keccak_add_bytes(hash, ak->keccak_state, &pad_data,
			    (unsigned int)offset, 1);
}

static void lc_ak_aad(struct lc_ak_cryptor *ak, const uint8_t *aad,
		      size_t aadlen)
{
	const struct lc_hash *hash = ak->hash;
	size_t todo = 0;
	static const uint8_t pad_trail = 0x01;

	/* Authenticated Data - Insert into rate */
	while (aadlen) {
		todo = min_size(aadlen, hash->rate);
		lc_keccak_add_bytes(hash, ak->keccak_state, aad, 0,
				    (unsigned int)todo);

		aadlen -= todo;

		/* Insert the trailing 1 */
		if (!aadlen)
			lc_ak_add_padbyte(ak, todo);

		lc_keccak(hash, ak->keccak_state);
	}

	/* Add pad_trail bit */
	lc_keccak_add_bytes(hash, ak->keccak_state, &pad_trail,
			    LC_SHA3_STATE_SIZE - 1, 1);
}

static void lc_ak_finalization(struct lc_ak_cryptor *ak, uint8_t *tag,
			       size_t taglen)
{
	const struct lc_hash *hash = ak->hash;

	/* Finalization - Insert key into capacity */
	lc_keccak_add_bytes(hash, ak->keccak_state, ak->key, hash->rate, 32);

	lc_keccak(hash, ak->keccak_state);

	/* Finalization - Insert key into capacity */
	lc_keccak_add_bytes(hash, ak->keccak_state, ak->key, hash->rate, 32);

	/* Finalization - Extract tag from capacity */
	lc_keccak_extract_bytes(hash, ak->keccak_state, tag, hash->rate,
				(unsigned int)taglen);
}

static void lc_ak_encrypt(void *state, const uint8_t *plaintext,
			  uint8_t *ciphertext, size_t datalen,
			  const uint8_t *aad, size_t aadlen, uint8_t *tag,
			  size_t taglen)
{
	struct lc_ak_cryptor *ak = state;
	const struct lc_hash *hash = ak->hash;
	size_t todo = 0;

	if (taglen > (LC_SHA3_STATE_SIZE - hash->rate) || taglen < 16)
		return;

	/* Authenticated Data */
	lc_ak_aad(ak, aad, aadlen);

	/* Plaintext - Insert into rate */
	while (datalen) {
		todo = min_size(datalen, hash->rate);

		lc_keccak_add_bytes(hash, ak->keccak_state, plaintext, 0,
				    (unsigned int)todo);

		lc_keccak_extract_bytes(hash, ak->keccak_state, ciphertext, 0,
					(unsigned int)todo);

		datalen -= todo;

		/* Apply Keccak for all rounds other than the last one */
		if (datalen) {
			lc_keccak(hash, ak->keccak_state);
			plaintext += todo;
			ciphertext += todo;
		} else {
			lc_ak_add_padbyte(ak, todo);
		}
	}

	/* Finalization */
	lc_ak_finalization(ak, tag, taglen);
}

static int lc_ak_decrypt(void *state, const uint8_t *ciphertext,
			 uint8_t *plaintext, size_t datalen, const uint8_t *aad,
			 size_t aadlen, const uint8_t *tag, size_t taglen)
{
	struct lc_ak_cryptor *ak = state;
	const struct lc_hash *hash = ak->hash;
	uint8_t calctag[16] __align(sizeof(uint64_t));
	uint8_t tmp_pt[136] __align(sizeof(uint64_t)) = { 0 };
	uint8_t *calctag_p = calctag, *pt_p = plaintext;
	size_t todo;
	int ret, zero_tmp = 0;

	/* If we have an in-place cipher operation, we need a tmp-buffer */
	if (plaintext == ciphertext) {
		pt_p = tmp_pt;
		zero_tmp = 1;
	}

	if (taglen > sizeof(calctag)) {
		ret = lc_alloc_aligned((void **)&calctag_p,
				       LC_MEM_COMMON_ALIGNMENT, taglen);
		if (ret)
			return -ret;
	}

	/* Authenticated Data - Insert into rate */
	lc_ak_aad(ak, aad, aadlen);

	/* Plaintext - Insert into rate */
	while (datalen) {
		todo = min_size(datalen, hash->rate);
		lc_keccak_extract_bytes(hash, ak->keccak_state, pt_p, 0,
					(unsigned int)todo);

		datalen -= todo;

		/*
		 * Replace state with ciphertext and apply Keccak for all
		 * rounds other than the last one.
		 */
		if (datalen) {
			lc_keccak_newstate(hash, ak->keccak_state, ciphertext,
					   0, todo);
			lc_keccak(hash, ak->keccak_state);

			/*
			 * Perform XOR operation here to ensure decryption in
			 * place.
			 */
			if (!zero_tmp) {
				xor_64(pt_p, ciphertext, todo);
				pt_p += todo;
			} else {
				xor_64_3(plaintext, pt_p, ciphertext, todo);
				plaintext += todo;
			}

			ciphertext += todo;
		} else {
			if (!zero_tmp) {
				xor_64(pt_p, ciphertext, todo);
				lc_keccak_add_bytes(hash, ak->keccak_state,
						    pt_p, 0,
						    (unsigned int)todo);
			} else {
				xor_64_3(plaintext, pt_p, ciphertext, todo);
				lc_keccak_add_bytes(hash, ak->keccak_state,
						    plaintext, 0,
						    (unsigned int)todo);
			}

			lc_ak_add_padbyte(ak, todo);
		}
	}

	/* Finalization */
	lc_ak_finalization(ak, calctag_p, taglen);

	ret = (lc_memcmp_secure(calctag_p, taglen, tag, taglen) ? -EBADMSG : 0);
	lc_memset_secure(calctag_p, 0, taglen);
	if (taglen > sizeof(calctag))
		lc_free(calctag_p);

	if (zero_tmp)
		lc_memset_secure(tmp_pt, 0, sizeof(tmp_pt));

	return ret;
}

LC_INTERFACE_FUNCTION(int, lc_ak_alloc, const struct lc_hash *hash,
		      struct lc_aead_ctx **ctx)
{
	struct lc_aead_ctx *tmp = NULL;
	int ret;

	ret = lc_alloc_aligned((void **)&tmp, LC_ASCON_KECCAK_ALIGNMENT,
			       LC_AK_CTX_SIZE(hash));
	if (ret)
		return -ret;

	LC_AK_SET_CTX(tmp, hash);

	*ctx = tmp;

	return 0;
}

static void lc_ak_zero(void *state)
{
	struct lc_ak_cryptor *ak = state;

	lc_memset_secure((uint8_t *)ak->keccak_state, 0,
			 sizeof(ak->keccak_state));
	lc_memset_secure((uint8_t *)ak->key, 0, sizeof(ak->key));
}

struct lc_aead _lc_ascon_keccak_aead = {
	.setkey = lc_ak_setkey,
	.encrypt = lc_ak_encrypt,
	.enc_update = NULL,
	.enc_final = NULL,
	.decrypt = lc_ak_decrypt,
	.dec_update = NULL,
	.dec_final = NULL,
	.zero = lc_ak_zero };
LC_INTERFACE_SYMBOL(const struct lc_aead *, lc_ascon_keccak_aead) =
	&_lc_ascon_keccak_aead;
