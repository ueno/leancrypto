/*
 * Copyright (C) 2022 - 2024, Stephan Mueller <smueller@chronox.de>
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

#include "compare.h"
#include "ext_headers.h"
#include "lc_ascon_keccak.h"
#include "visibility.h"

#include "sha3_c.h"
#include "sha3_arm_asm.h"
#include "sha3_arm_ce.h"
#include "sha3_arm_neon.h"
#include "sha3_avx2.h"
#include "sha3_avx512.h"
#include "sha3_riscv_asm.h"

#define LC_EXEC_ONE_TEST_512(sha3_impl)                                        \
	if (sha3_impl) {                                                       \
		ret += ak_tester_512(sha3_impl, #sha3_impl);                   \
		ret += ak_tester_512_large_iv_tag(sha3_impl, #sha3_impl);      \
	}

#define LC_EXEC_ONE_TEST_256(sha3_impl)                                        \
	if (sha3_impl) {                                                       \
		ret += ak_tester_256(sha3_impl, #sha3_impl);                   \
		ret += ak_tester_256_large_iv_tag(sha3_impl, #sha3_impl);      \
	}

static int ak_tester_one(const struct lc_hash *hash, const uint8_t *pt,
			 size_t ptlen, const uint8_t *iv, size_t ivlen,
			 const uint8_t *aad, size_t aadlen, const uint8_t *key,
			 size_t keylen, const uint8_t *exp_ct,
			 const uint8_t *exp_tag, uint8_t exp_tag_len)
{
	struct lc_aead_ctx *ak_heap = NULL;
	ssize_t ret;
	int ret_checked = 0;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wvla"
	uint8_t out_enc[ptlen];
	uint8_t out_dec[ptlen];
	uint8_t tag[exp_tag_len];
#pragma GCC diagnostic pop
	LC_AK_CTX_ON_STACK_TAGLEN(ak, hash, exp_tag_len);

	/* One shot encryption with pt ptr != ct ptr */
	ret = lc_aead_setkey(ak, key, keylen, iv, ivlen);
	if (ret) {
		printf("AEAD setkey failed: %zd\n", ret);
		return (int)ret;
	}

	lc_aead_encrypt(ak, pt, out_enc, ptlen, aad, aadlen, tag, exp_tag_len);
	lc_aead_zero(ak);

	ret_checked += lc_compare(out_enc, exp_ct, ptlen,
				  "Ascon Keccak crypt: Encryption, ciphertext");
	ret_checked += lc_compare(tag, exp_tag, exp_tag_len,
				  "Ascon Keccak crypt: Encryption, tag");

	//bin2print(out_enc, ptlen, stderr, "out_enc");
	//bin2print(tag, exp_tag_len, stderr, "tag");

	/* One shot encryption with pt ptr == ct ptr */
	if (lc_ak_alloc_taglen(hash, exp_tag_len, &ak_heap))
		return 1;

	lc_aead_setkey(ak_heap, key, keylen, iv, ivlen);

	memcpy(out_enc, pt, ptlen);
	lc_aead_encrypt(ak_heap, out_enc, out_enc, ptlen, aad, aadlen, tag,
			exp_tag_len);
	lc_aead_zero_free(ak_heap);

	ret_checked += lc_compare(out_enc, exp_ct, ptlen,
				  "Ascon Keccak: Encryption, ciphertext");
	ret_checked += lc_compare(tag, exp_tag, exp_tag_len,
				  "Ascon Keccak: Encryption, tag");

	/* One shot decryption with pt ptr != ct ptr */
	if (lc_aead_setkey(ak, key, keylen, iv, ivlen))
		return -EFAULT;
	ret = lc_aead_decrypt(ak, out_enc, out_dec, ptlen, aad, aadlen, tag,
			      exp_tag_len);
	//bin2print(out_dec, ptlen, stderr, "out_enc");
	lc_aead_zero(ak);
	if (ret < 0)
		return 1;

	ret_checked += lc_compare(out_dec, pt, ptlen,
				  "Ascon Keccak crypt: Decryption, plaintext");

	/* Check authentication error */
	if (lc_aead_setkey(ak, key, keylen, iv, ivlen))
		return -EFAULT;

	out_enc[0] = (uint8_t)((out_enc[0] + 1) & 0xff);
	ret = lc_aead_decrypt(ak, out_enc, out_dec, ptlen, aad, aadlen, tag,
			      exp_tag_len);
	lc_aead_zero(ak);
	if (ret != -EBADMSG)
		return 1;

	return ret_checked;
}

static int ak_tester_512_large_iv_tag(const struct lc_hash *hash,
				      const char *name)
{
	static const uint8_t in[] = {
		0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09,
		0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13,
		0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d,
		0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
		0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30, 0x31,
		0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b,
		0x3c, 0x3d, 0x3e, 0x3f, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05,
		0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
		0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19,
		0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23,
		0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d,
		0x2e, 0x2f, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
		0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f, 0x00, 0x01,
		0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b,
		0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15,
		0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
		0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29,
		0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30, 0x31, 0x32, 0x33,
		0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d,
		0x3e, 0x3f,
	};
	static const uint8_t key[] = {
		0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09,
		0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13,
		0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d,
		0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
		0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30, 0x31,
		0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b,
		0x3c, 0x3d, 0x3e, 0x3f
	};
	static const uint8_t iv[] = {
		0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29,
		0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x20, 0x21, 0x22, 0x23,
		0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d,
		0x2e, 0x2f, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
		0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x20, 0x21,
		0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b,
		0x2c, 0x2d, 0x2e, 0x2f,
	};
	static const uint8_t exp_ct[] = {
		0x9c, 0x2f, 0xa3, 0xcf, 0xac, 0x8a, 0xfe, 0x69, 0xbe, 0xe3,
		0x87, 0x4f, 0xcf, 0x07, 0xe2, 0x57, 0x4c, 0x42, 0xae, 0x3d,
		0xf7, 0xb0, 0xf1, 0xb5, 0xb3, 0x0e, 0xf2, 0xcf, 0x0d, 0x73,
		0x34, 0x26, 0x15, 0xe0, 0x1d, 0x97, 0xf9, 0x18, 0xf3, 0x37,
		0x59, 0x3f, 0x95, 0x54, 0x44, 0x8b, 0xb6, 0x49, 0x2d, 0xbf,
		0xbd, 0xfb, 0x83, 0xb9, 0xf1, 0x17, 0x69, 0xe8, 0x2f, 0x83,
		0x4c, 0x5b, 0x9e, 0x23, 0x29, 0x87, 0xfc, 0x5a, 0x84, 0xc6,
		0x7d, 0xc3, 0xd2, 0x5a, 0x00, 0x7f, 0x28, 0x3d, 0xa0, 0xb8,
		0xcd, 0xe5, 0x18, 0xbc, 0x80, 0xa9, 0x00, 0x8d, 0xa5, 0x15,
		0x7b, 0xdf, 0xda, 0x31, 0x10, 0xa5, 0x62, 0xc2, 0x0e, 0x2e,
		0xd3, 0x2f, 0x0d, 0x50, 0x7f, 0x06, 0xcb, 0xcc, 0x5c, 0x81,
		0xce, 0xe9, 0x8a, 0xe0, 0x5c, 0xe0, 0x12, 0xbe, 0xcc, 0xc2,
		0x5b, 0x8f, 0x72, 0x6e, 0x33, 0xf6, 0xde, 0x46, 0xfb, 0xb4,
		0x7e, 0xc1, 0xf9, 0x26, 0xe1, 0x44, 0x3b, 0x86, 0x4c, 0x54,
		0x47, 0x5c, 0x95, 0x84, 0xe8, 0x8b, 0xf1, 0xb9, 0x3a, 0x2a,
		0x45, 0x6e, 0x06, 0xdc, 0x65, 0x32, 0x9e, 0xac, 0xb5, 0xe1,
		0xf8, 0x64, 0x58, 0xbf, 0x5e, 0x89, 0xa7, 0x98, 0xa0, 0x84,
		0xed, 0x1b, 0x51, 0x88, 0x4e, 0x47, 0xeb, 0x8b, 0x02, 0xf8,
		0xc1, 0x75, 0x8e, 0x49, 0xe8, 0x53, 0x9d, 0xbc, 0xc4, 0xee,
		0xc1, 0x17
	};
	static const uint8_t exp_tag[] = {
		0x21, 0xc0, 0xa4, 0x8f, 0x7c, 0x97, 0x66, 0x53, 0x76, 0xc5,
		0x99, 0xbe, 0xb0, 0x03, 0xc6, 0xba, 0x1d, 0x7e, 0x57, 0x75,
		0x47, 0x8d, 0x22, 0x96, 0x36, 0xf5, 0x76, 0x31, 0x16, 0x3f,
		0x80, 0x59, 0xc6, 0xc7, 0x49, 0xc9, 0x17, 0x73, 0x5d, 0xa2,
		0xdb, 0x5c, 0xeb, 0xd7, 0x04, 0x4a, 0xa7, 0xeb, 0x97, 0x5f,
		0x40, 0x05, 0x51, 0xbc, 0xa8, 0xcd, 0xac, 0x35, 0xe9, 0x20,
		0xf0, 0x29, 0xe9, 0x61
	};

	printf("Ascon Keccak 512 crypt ctx %s (%s implementation) len %u, state len %u\n",
	       name, hash == lc_sha3_512_c ? "C" : "accelerated",
	       (unsigned int)LC_AK_CTX_SIZE(hash),
	       (unsigned int)LC_AK_STATE_SIZE);
	return ak_tester_one(hash, in, sizeof(in), iv, sizeof(iv), in,
			     sizeof(in), key, sizeof(key), exp_ct, exp_tag,
			     sizeof(exp_tag));
}

static int ak_tester_512(const struct lc_hash *hash, const char *name)
{
	static const uint8_t in[] = {
		0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09,
		0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13,
		0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d,
		0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
		0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30, 0x31,
		0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b,
		0x3c, 0x3d, 0x3e, 0x3f, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05,
		0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
		0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19,
		0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23,
		0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d,
		0x2e, 0x2f, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
		0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f, 0x00, 0x01,
		0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b,
		0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15,
		0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
		0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29,
		0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30, 0x31, 0x32, 0x33,
		0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d,
		0x3e, 0x3f,
	};
	static const uint8_t key[] = {
		0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09,
		0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13,
		0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d,
		0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
		0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30, 0x31,
		0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b,
		0x3c, 0x3d, 0x3e, 0x3f
	};
	static const uint8_t iv[] = {
		0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
		0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
	};
	static const uint8_t exp_ct[] = {
		0x8c, 0x24, 0x59, 0xe1, 0x01, 0xc6, 0xed, 0x9b, 0xa4, 0xd4,
		0x8b, 0x65, 0x8f, 0x43, 0xc6, 0xc9, 0x1f, 0x71, 0x4b, 0x40,
		0xe4, 0x30, 0x61, 0xae, 0x60, 0x21, 0x94, 0x59, 0x8b, 0x4e,
		0xc9, 0xce, 0xf0, 0x33, 0xc5, 0xa9, 0x7c, 0x3b, 0x37, 0x9d,
		0x14, 0x61, 0xa9, 0x68, 0xdc, 0xa6, 0x5a, 0x56, 0x6b, 0x84,
		0xa7, 0xc2, 0x6d, 0x5d, 0x70, 0xef, 0xeb, 0x5a, 0xae, 0xb1,
		0xdc, 0x40, 0xb2, 0x3e, 0x23, 0xdc, 0xd0, 0x77, 0x79, 0xef,
		0x38, 0x14, 0xbf, 0x3e, 0x99, 0x4b, 0xf4, 0x3c, 0x4c, 0xf4,
		0x08, 0x4e, 0xff, 0x2f, 0x0f, 0xc1, 0x7d, 0x46, 0x2c, 0xa8,
		0xae, 0xec, 0x72, 0x7f, 0xef, 0xc4, 0x9d, 0x15, 0xf3, 0x3a,
		0xb3, 0x65, 0x6e, 0xd8, 0xc1, 0xd9, 0x7f, 0x96, 0x97, 0x59,
		0xed, 0x7a, 0x9b, 0xc6, 0xfc, 0x3d, 0xc8, 0xac, 0xb1, 0xac,
		0x8b, 0x66, 0xb4, 0x96, 0x20, 0x53, 0x41, 0x33, 0xb0, 0x75,
		0xd8, 0x26, 0x5f, 0xca, 0x9f, 0xa6, 0x92, 0x3e, 0x3d, 0x8e,
		0xed, 0x3e, 0x1d, 0x43, 0xe4, 0x29, 0xcf, 0xa7, 0x57, 0x08,
		0x1f, 0x93, 0x2b, 0x64, 0x65, 0x45, 0x74, 0xb9, 0xeb, 0xe6,
		0xfb, 0x1a, 0x6a, 0x36, 0x9d, 0x00, 0x18, 0xe0, 0xbd, 0xb2,
		0x46, 0x9b, 0xfe, 0xce, 0xe6, 0x83, 0xcf, 0x73, 0xfa, 0xc1,
		0xe9, 0x23, 0x9c, 0x75, 0x9e, 0x20, 0x6a, 0x32, 0x11, 0x60,
		0x6e, 0x84
	};
	static const uint8_t exp_tag[] = { 0x8f, 0x44, 0x00, 0xbc, 0x1d, 0x01,
					   0x87, 0x59, 0x2b, 0x1d, 0xa7, 0x7f,
					   0x3e, 0x21, 0x7c, 0x76 };

	printf("Ascon Keccak 512 crypt ctx %s (%s implementation) len %u, state len %u\n",
	       name, hash == lc_sha3_512_c ? "C" : "accelerated",
	       (unsigned int)LC_AK_CTX_SIZE(hash),
	       (unsigned int)LC_AK_STATE_SIZE);
	return ak_tester_one(hash, in, sizeof(in), iv, sizeof(iv), in,
			     sizeof(in), key, sizeof(key), exp_ct, exp_tag,
			     sizeof(exp_tag));
}

static int ak_tester_256(const struct lc_hash *hash, const char *name)
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
		0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
		0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f,
		0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97,
		0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f,
	};
	static const uint8_t iv[] = {
		0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
		0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
	};
	static const uint8_t exp_ct[] = {
		0x39, 0xb5, 0x1b, 0xc6, 0x72, 0xab, 0x1a, 0xcb, 0x49, 0x4c,
		0xe8, 0x66, 0x53, 0x3c, 0x10, 0x7f, 0xed, 0xde, 0xb3, 0x0c,
		0x46, 0x6a, 0xb8, 0x98, 0xc0, 0x40, 0x2b, 0xbc, 0x56, 0x41,
		0xfa, 0x9f, 0x56, 0xb4, 0xf6, 0xce, 0x29, 0x67, 0xa2, 0x80,
		0x44, 0xed, 0x7c, 0x07, 0x07, 0xd4, 0x66, 0x94, 0x9f, 0xe8,
		0x6d, 0xa5, 0x60, 0x6d, 0xa9, 0xb9, 0x06, 0x0d, 0x00, 0xd8,
		0x7b, 0xba, 0xff, 0x59
	};
	static const uint8_t exp_tag[] = { 0xc7, 0xe7, 0x58, 0x28, 0xe5, 0x76,
					   0x7c, 0xc7, 0xfa, 0xdf, 0x16, 0xa3,
					   0x54, 0x2c, 0x0e, 0x11 };

	printf("Ascon Keccak 256 crypt ctx %s (%s implementation) len %u, state len %u\n",
	       name, hash == lc_sha3_256_c ? "C" : "accelerated",
	       (unsigned int)LC_AK_CTX_SIZE(hash),
	       (unsigned int)LC_AK_STATE_SIZE);
	return ak_tester_one(hash, in, sizeof(in), iv, sizeof(iv), in,
			     sizeof(in), key, sizeof(key), exp_ct, exp_tag,
			     sizeof(exp_tag));
}

static int ak_tester_256_large_iv_tag(const struct lc_hash *hash,
				      const char *name)
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
		0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
		0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f,
		0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97,
		0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f,
	};
	static const uint8_t iv[] = {
		0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
		0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
		0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
		0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
	};
	static const uint8_t exp_ct[] = {
		0x6f, 0x7b, 0xf2, 0x4e, 0x59, 0xff, 0x1b, 0xf2, 0xd6, 0x37,
		0xcb, 0x31, 0x52, 0x70, 0x33, 0x3a, 0x8b, 0x9a, 0xe1, 0x1f,
		0x38, 0xee, 0xf4, 0xb7, 0x14, 0x7e, 0xad, 0x0f, 0xbf, 0xd5,
		0x68, 0xe9, 0xde, 0x4b, 0xcf, 0xf7, 0xf5, 0x51, 0xf4, 0x28,
		0xd9, 0x91, 0x1f, 0x8a, 0xcc, 0xcd, 0x60, 0x8d, 0x94, 0xe4,
		0xc8, 0x8e, 0x3d, 0x28, 0xed, 0xed, 0x97, 0xe1, 0xa3, 0xdd,
		0x54, 0x9b, 0x2b, 0x72
	};
	static const uint8_t exp_tag[] = { 0x17, 0xba, 0xb9, 0x61, 0x49, 0xeb,
					   0x30, 0xe9, 0x66, 0xc7, 0x80, 0x50,
					   0xf4, 0xe3, 0xb4, 0xba, 0xbb, 0x22,
					   0xaa, 0xca, 0x19, 0x18, 0x61, 0x3a,
					   0xe7, 0xc3, 0xea, 0xa1, 0xa8, 0x03,
					   0xce, 0x36 };

	printf("Ascon Keccak 256 crypt ctx %s (%s implementation) len %u, state len %u\n",
	       name, hash == lc_sha3_256_c ? "C" : "accelerated",
	       (unsigned int)LC_AK_CTX_SIZE(hash),
	       (unsigned int)LC_AK_STATE_SIZE);
	return ak_tester_one(hash, in, sizeof(in), iv, sizeof(iv), in,
			     sizeof(in), key, sizeof(key), exp_ct, exp_tag,
			     sizeof(exp_tag));
}

LC_TEST_FUNC(int, main, int argc, char *argv[])
{
	int ret = 0;
	(void)argc;
	(void)argv;

	LC_EXEC_ONE_TEST_512(lc_sha3_512);
	LC_EXEC_ONE_TEST_512(lc_sha3_512_c);
	LC_EXEC_ONE_TEST_512(lc_sha3_512_arm_asm);
	LC_EXEC_ONE_TEST_512(lc_sha3_512_arm_ce);
	LC_EXEC_ONE_TEST_512(lc_sha3_512_arm_neon);
	LC_EXEC_ONE_TEST_512(lc_sha3_512_avx2);
	LC_EXEC_ONE_TEST_512(lc_sha3_512_avx512);
	LC_EXEC_ONE_TEST_512(lc_sha3_512_riscv_asm);

	LC_EXEC_ONE_TEST_256(lc_sha3_256);
	LC_EXEC_ONE_TEST_256(lc_sha3_256_c);
	LC_EXEC_ONE_TEST_256(lc_sha3_256_arm_asm);
	LC_EXEC_ONE_TEST_256(lc_sha3_256_arm_ce);
	LC_EXEC_ONE_TEST_256(lc_sha3_256_arm_neon);
	LC_EXEC_ONE_TEST_256(lc_sha3_256_avx2);
	LC_EXEC_ONE_TEST_256(lc_sha3_256_avx512);
	LC_EXEC_ONE_TEST_256(lc_sha3_256_riscv_asm);

	return ret;
}
