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
#include "math_helper.h"
#include "visibility.h"

#include "sha3_c.h"
#include "sha3_arm_asm.h"
#include "sha3_arm_ce.h"
#include "sha3_arm_neon.h"
#include "sha3_avx2.h"
#include "sha3_avx512.h"
#include "sha3_riscv_asm.h"

#define LC_EXEC_ONE_TEST_512(sha3_impl)                                        \
	if (sha3_impl)                                                         \
	ret += ak_tester_256_512(sha3_impl, #sha3_impl)

#define LC_EXEC_ONE_TEST_512_512(sha3_impl)                                    \
	if (sha3_impl)                                                         \
	ret += ak_tester_512_512(sha3_impl, #sha3_impl)

#define LC_EXEC_ONE_TEST_256(sha3_impl)                                        \
	if (sha3_impl)                                                         \
	ret += ak_tester_256_256(sha3_impl, #sha3_impl)

static void ak_tester_enc(struct lc_aead_ctx *ak, const uint8_t *pt,
			  uint8_t *ct, size_t ptlen)
{
	size_t todo;

	while (ptlen) {
		todo = min_size(ptlen, 3);

		lc_aead_enc_update(ak, pt, ct, todo);
		pt += todo;
		ct += todo;
		ptlen -= todo;
	}
}

static void ak_tester_dec(struct lc_aead_ctx *ak, const uint8_t *ct,
			  uint8_t *pt, size_t ptlen)
{
	size_t todo;

	while (ptlen) {
		todo = min_size(ptlen, 3);

		lc_aead_dec_update(ak, ct, pt, todo);
		pt += todo;
		ct += todo;
		ptlen -= todo;
	}
}

static int ak_tester_one(const struct lc_hash *hash, const uint8_t *pt,
			 size_t ptlen, const uint8_t *iv, size_t ivlen,
			 const uint8_t *aad, size_t aadlen, const uint8_t *key,
			 size_t keylen, const uint8_t *exp_ct,
			 const uint8_t *exp_tag, size_t exp_tag_len)
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
	LC_AK_CTX_ON_STACK(ak, hash);

	/* One shot encryption with pt ptr != ct ptr */
	if (lc_aead_setkey(ak, key, keylen, iv, ivlen))
		return -EFAULT;
	lc_aead_enc_init(ak, aad, aadlen);
	ak_tester_enc(ak, pt, out_enc, ptlen);
	lc_aead_enc_final(ak, tag, exp_tag_len);
	lc_aead_zero(ak);

	ret_checked += lc_compare(out_enc, exp_ct, ptlen,
				  "Ascon Keccak crypt: Encryption, ciphertext");
	ret_checked += lc_compare(tag, exp_tag, exp_tag_len,
				  "Ascon Keccak crypt: Encryption, tag");

	//bin2print(out_enc, ptlen, stderr, "out_enc");
	//bin2print(tag, exp_tag_len, stderr, "tag");

	/* One shot encryption with pt ptr == ct ptr */
	if (lc_ak_alloc(hash, &ak_heap))
		return 1;

	lc_aead_setkey(ak_heap, key, keylen, iv, ivlen);

	memcpy(out_enc, pt, ptlen);
	lc_aead_enc_init(ak_heap, aad, aadlen);
	ak_tester_enc(ak_heap, out_enc, out_enc, ptlen);
	lc_aead_enc_final(ak_heap, tag, exp_tag_len);
	lc_aead_zero_free(ak_heap);

	ret_checked += lc_compare(out_enc, exp_ct, ptlen,
				  "Ascon Keccak: Encryption, ciphertext");
	ret_checked += lc_compare(tag, exp_tag, exp_tag_len,
				  "Ascon Keccak: Encryption, tag");

	/* One shot decryption with pt ptr != ct ptr */
	if (lc_aead_setkey(ak, key, keylen, iv, ivlen))
		return -EFAULT;
	lc_aead_enc_init(ak, aad, aadlen);
	ak_tester_dec(ak, out_enc, out_dec, ptlen);
	ret = lc_aead_enc_final(ak, tag, exp_tag_len);
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
	lc_aead_enc_init(ak, aad, aadlen);
	ak_tester_dec(ak, out_enc, out_dec, ptlen);
	ret = lc_aead_dec_final(ak, tag, exp_tag_len);
	lc_aead_zero(ak);
	if (ret != -EBADMSG)
		return 1;

	return ret_checked;
}

static int ak_tester_256_512(const struct lc_hash *hash, const char *name)
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
		0x02, 0xda, 0xc2, 0x9a, 0xeb, 0x4a, 0xc4, 0xbd, 0x55, 0x5c,
		0x78, 0x1d, 0x5e, 0x55, 0x22, 0x4c, 0xaf, 0xd3, 0x56, 0xb3,
		0xb8, 0xe7, 0x7c, 0x5c, 0x5d, 0xc3, 0xed, 0x29, 0x96, 0x5f,
		0xa5, 0x4a, 0x7a, 0x03, 0x56, 0xdf, 0x57, 0xeb, 0xfd, 0x55,
		0xf3, 0x2d, 0xe1, 0x7a, 0x25, 0x1a, 0x6c, 0xc2, 0x1b, 0xbe,
		0x2f, 0x16, 0xa2, 0x52, 0x39, 0x73, 0x9c, 0xb9, 0x77, 0xa6,
		0x91, 0xa9, 0x8c, 0xa4, 0x9b, 0x55, 0xd7, 0x7d, 0x1f, 0xc2,
		0xca, 0xce, 0x3f, 0x12, 0x44, 0xa0, 0x82, 0x45, 0x58, 0x21,
		0x88, 0x82, 0x2d, 0x49, 0x73, 0x3f, 0x53, 0xe7, 0x99, 0x43,
		0x9f, 0xfd, 0x45, 0x6d, 0x0b, 0x4a, 0x40, 0xad, 0xaf, 0x8c,
		0x82, 0x00, 0x1f, 0x09, 0x0d, 0x83, 0x70, 0x10, 0x80, 0xd0,
		0x5e, 0x27, 0x24, 0x76, 0x28, 0x80, 0x74, 0xae, 0xbb, 0xa6,
		0x8f, 0xea, 0x94, 0x6d, 0x40, 0xdd, 0xaf, 0xe7, 0xb8, 0x48,
		0x61, 0x9e, 0x62, 0x64, 0x84, 0xdd, 0xb2, 0x3e, 0x6d, 0x2d,
		0x27, 0x48, 0xbb, 0x19, 0x6f, 0x17, 0xd3, 0x79, 0x79, 0x0c,
		0x0a, 0xac, 0x34, 0xc7, 0x7d, 0x02, 0xc8, 0xd9, 0x8f, 0xcd,
		0x00, 0xcf, 0x95, 0x25, 0x40, 0xe0, 0x1a, 0x29, 0xcc, 0xa3,
		0xa5, 0xea, 0x00, 0xf3, 0xed, 0xdb, 0x52, 0x26, 0x85, 0x67,
		0x2b, 0x7f, 0x1e, 0x37, 0x6b, 0xdc, 0xeb, 0x42, 0xe9, 0xd0,
		0x0c, 0xb1,
	};
	static const uint8_t exp_tag[] = { 0xd7, 0x15, 0x74, 0x0a, 0xba, 0xca,
					   0x2f, 0xb2, 0xec, 0xee, 0x7c, 0x07,
					   0xf0, 0xa5, 0x8f, 0x53 };

	printf("Ascon Keccak 256/512 crypt ctx %s (%s implementation) len %u, state len %u\n",
	       name, hash == lc_sha3_512_c ? "C" : "accelerated",
	       (unsigned int)LC_AK_CTX_SIZE(hash),
	       (unsigned int)LC_AK_STATE_SIZE);
	return ak_tester_one(hash, in, sizeof(in), iv, sizeof(iv), in,
			     sizeof(in), key, sizeof(key), exp_ct, exp_tag,
			     sizeof(exp_tag));
}

static int ak_tester_512_512(const struct lc_hash *hash, const char *name)
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
		0x9e, 0x9f, 0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
		0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf, 0xb0, 0xb1,
		0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xbb,
		0xbc, 0xbd, 0xbe, 0xbf,
	};
	static const uint8_t iv[] = {
		0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
		0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
	};
	static const uint8_t exp_ct[] = {
		0x0e, 0xcb, 0xc6, 0x4d, 0xa1, 0x69, 0x61, 0xed, 0xc6, 0x86,
		0x61, 0xed, 0x34, 0x08, 0x0d, 0xf4, 0x42, 0x25, 0xd1, 0xe3,
		0xb8, 0x10, 0x83, 0x32, 0x9d, 0x4f, 0x8e, 0x49, 0xc4, 0xf9,
		0x57, 0x37, 0x6d, 0xc6, 0xd7, 0x52, 0x52, 0x4f, 0x76, 0xfe,
		0xd8, 0x7f, 0xc1, 0x47, 0x0f, 0x57, 0xdf, 0x7c, 0xa8, 0x71,
		0xf7, 0x16, 0x8d, 0x53, 0x8c, 0x72, 0x4b, 0x69, 0x8c, 0x8b,
		0x9f, 0xe9, 0xe5, 0xef
	};
	static const uint8_t exp_tag[] = { 0x41, 0xef, 0xd3, 0x17, 0xf2, 0x2f,
					   0x07, 0xdb, 0x5d, 0x29, 0x33, 0xb8,
					   0x8d, 0x4b, 0x26, 0xf0 };

	printf("Ascon Keccak 512/512 crypt ctx %s (%s implementation) len %u, state len %u\n",
	       name, hash == lc_sha3_512_c ? "C" : "accelerated",
	       (unsigned int)LC_AK_CTX_SIZE(hash),
	       (unsigned int)LC_AK_STATE_SIZE);
	return ak_tester_one(hash, in, sizeof(in), iv, sizeof(iv), in,
			     sizeof(in), key, sizeof(key), exp_ct, exp_tag,
			     sizeof(exp_tag));
}

static int ak_tester_256_256(const struct lc_hash *hash, const char *name)
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
		0x98, 0xe1, 0x5c, 0xd7, 0x81, 0xd9, 0x90, 0x9a, 0x63, 0x87,
		0x6f, 0xf8, 0x2a, 0x74, 0x38, 0xa2, 0xc0, 0xbf, 0x1e, 0xe4,
		0x82, 0x50, 0xc0, 0x1d, 0xea, 0x17, 0x30, 0xec, 0xb7, 0xd2,
		0x36, 0xbc, 0x83, 0xd8, 0x8d, 0xa1, 0xf1, 0x7e, 0xe9, 0x6d,
		0x53, 0xb6, 0x48, 0xef, 0x43, 0x85, 0xea, 0x72, 0x6f, 0x51,
		0x3d, 0xb2, 0x35, 0x5d, 0x48, 0x44, 0x77, 0xb7, 0x60, 0x27,
		0x53, 0x9a, 0x74, 0x8e
	};
	static const uint8_t exp_tag[] = {
		0x79, 0xd0, 0x7e, 0x7a, 0xb6, 0x79, 0x7d, 0x14,
		0x0e, 0x6b, 0xe6, 0xe9, 0x64, 0xdb, 0x59, 0x14,
	};

	printf("Ascon Keccak 256/256 crypt ctx %s (%s implementation) len %u, state len %u\n",
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

	LC_EXEC_ONE_TEST_512_512(lc_sha3_512);
	LC_EXEC_ONE_TEST_512_512(lc_sha3_512_c);
	LC_EXEC_ONE_TEST_512_512(lc_sha3_512_arm_asm);
	LC_EXEC_ONE_TEST_512_512(lc_sha3_512_arm_ce);
	LC_EXEC_ONE_TEST_512_512(lc_sha3_512_arm_neon);
	LC_EXEC_ONE_TEST_512_512(lc_sha3_512_avx2);
	LC_EXEC_ONE_TEST_512_512(lc_sha3_512_avx512);
	LC_EXEC_ONE_TEST_512_512(lc_sha3_512_riscv_asm);

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
