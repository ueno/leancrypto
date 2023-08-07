/*
 * Copyright (C) 2020 - 2023, Stephan Mueller <smueller@chronox.de>
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
#include "lc_cshake.h"
#include "lc_sha3.h"
#include "visibility.h"

#include "sha3_c.h"
#include "sha3_arm_asm.h"
#include "sha3_arm_ce.h"
#include "sha3_arm_neon.h"
#include "sha3_avx2.h"
#include "sha3_avx512.h"
#include "sha3_riscv_asm.h"

#define LC_EXEC_ONE_TEST(sha3_impl)                                            \
	if (sha3_impl)                                                         \
	ret += _shake_256_tester(sha3_impl, #sha3_impl)

static int _shake_256_tester(const struct lc_hash *shake_256, const char *name)
{
	static const uint8_t msg1[] = { 0x6C, 0x9E, 0xC8, 0x5C, 0xBA, 0xBA,
					0x62, 0xF5, 0xBC, 0xFE, 0xA1, 0x9E,
					0xB9, 0xC9, 0x20, 0x52, 0xD8, 0xFF,
					0x18, 0x81, 0x52, 0xE9, 0x61, 0xC1,
					0xEC, 0x5C, 0x75, 0xBF, 0xC3, 0xC9,
					0x1C, 0x8D };
	static const uint8_t exp1[] = { 0x7d, 0x6a, 0x09, 0x6e, 0x13, 0x66,
					0x1d, 0x9d, 0x0e, 0xca, 0xf5, 0x38,
					0x30, 0xa1, 0x92, 0x87, 0xe0, 0xb3,
					0x6e, 0xce, 0x48, 0x82, 0xeb, 0x58,
					0x0b, 0x78, 0x5c, 0x1d, 0xef, 0x2d,
					0xe5, 0xaa, 0x6c };
	uint8_t act1[sizeof(exp1)];
	static const uint8_t msg2[] = { 0x69, 0xF9, 0x02, 0x27, 0x27, 0xA7,
					0x5B, 0xB9, 0xDA, 0xCC, 0xBA, 0x6C,
					0x66, 0x9E, 0x52, 0x88, 0x96, 0xBF,
					0xEF, 0x8D, 0xFD, 0xD1, 0xBD, 0x7E,
					0x42, 0xAA, 0x3B, 0x22, 0x71, 0xE4,
					0x79, 0xC5 };
	static const uint8_t exp2[] = {
		0x96, 0x06, 0x3c, 0xe6, 0xd3, 0x4f, 0x7c, 0x82, 0x9a, 0x3d,
		0xea, 0xff, 0x48, 0x9d, 0xc2, 0x5d, 0x53, 0x34, 0xc1, 0x7b,
		0x87, 0x03, 0x77, 0x25, 0x04, 0x14, 0x9b, 0xee, 0xd2, 0x2a,
		0x20, 0x87, 0x36, 0x8f, 0xe3, 0xdd, 0x38, 0xc2, 0xfe, 0xcf,
		0x94, 0x6c, 0x5a, 0x7d, 0x89, 0xf9, 0x85, 0x8e, 0xd8, 0xd1,
		0xf9, 0x74, 0x81, 0x76, 0x8e, 0x90, 0xc4, 0xf2, 0xae, 0xa1,
		0xd3, 0x2e, 0x2f, 0xd3, 0xed, 0x71, 0x05, 0xfc, 0x60, 0x91,
		0x20, 0x46, 0xfd, 0x38, 0x79, 0x0b, 0x1e, 0xe2, 0x83, 0x02,
		0x25, 0xa0, 0x13, 0x9b, 0x08, 0xd9, 0x26, 0x50, 0x32, 0x31,
		0x51, 0x5c, 0x63, 0xba, 0x04, 0xec, 0x19, 0x64, 0x0c, 0x54,
		0xfd, 0xaa, 0x27, 0x60, 0x40, 0xd6, 0x2a, 0x1c, 0xe1, 0xfe,
		0x3d, 0x15, 0xa2, 0xc5, 0xf8, 0x6d, 0x03, 0xf4, 0x01, 0xd7,
		0xfa, 0x06, 0x54, 0x9b, 0xad, 0x44, 0xf9, 0x87, 0xc8, 0x60,
		0x80, 0xdf, 0x37, 0xab, 0x52, 0xb9, 0x45, 0x7d, 0xeb, 0x97,
		0xca, 0x55, 0x2d, 0x78, 0x19, 0xa5, 0xb3, 0x26, 0xb3, 0xf7,
		0x14, 0xba, 0x51, 0x3e, 0x68, 0x90, 0x70, 0x1b, 0xf5, 0x41,
		0xa1, 0x39, 0x02, 0xf5, 0xdd, 0x87, 0x8e, 0xa2, 0x2c, 0x04,
		0x90, 0x1c, 0xaf, 0x10, 0xba, 0x97, 0x92, 0x0c, 0x70, 0xa5,
		0xbc, 0xec, 0xee, 0xd8, 0xce, 0xe4, 0x0e, 0xab, 0xa4, 0xf0,
		0x41, 0xe4, 0x6b, 0xfd, 0x52, 0x40, 0x4a, 0x89, 0x99, 0xaa,
		0x96, 0xc4, 0x07, 0x5b, 0x67, 0x4c, 0x39, 0x50, 0x9b, 0xca,
		0x5d, 0x82, 0x29, 0x96, 0x48, 0xf8, 0x56, 0xe3, 0xbf, 0x92,
		0x04, 0xba, 0x8b, 0x03, 0xca, 0x65, 0x68, 0xa1, 0x24, 0x08,
		0xfd, 0x8d, 0xa9, 0xc9, 0x0b, 0x93, 0xcb, 0x1e, 0x55, 0xb5,
		0x8b, 0x95, 0xcb, 0x61, 0x41, 0xa1, 0x3d, 0xa1, 0xf6, 0x51,
		0x69, 0x80, 0xf0, 0x95, 0xfa, 0xc4, 0x64, 0xa8, 0xcb, 0x7e,
		0x47, 0xc0, 0x8a, 0xaf, 0xb0, 0xad, 0xc1, 0xc9, 0x30, 0x1a,
		0x96, 0xe1, 0x72, 0x57, 0x3e, 0x6f, 0x01, 0x47, 0x31, 0x93,
		0x9f, 0x00, 0x05, 0x59, 0xb7, 0xb4, 0x66, 0x6b, 0xab, 0xe3,
		0x9d, 0x80, 0x61, 0xbd, 0x58, 0x4e, 0xa2, 0xad, 0x7f, 0x12,
		0xbf, 0xc8, 0x69, 0xbc, 0x7b, 0xa2, 0x5b, 0x81, 0x4d, 0x48,
		0xce, 0x91, 0x4b, 0x6d, 0xb2, 0x92, 0x1d, 0xf2, 0x01, 0x19,
		0xe2, 0x34, 0x93, 0xbb, 0x05, 0xff, 0x44, 0x46, 0xe0, 0xc5
	};
	uint8_t act2[sizeof(exp2)];
	int ret;
	LC_HASH_CTX_ON_STACK(ctx, shake_256);
	LC_SHAKE_256_CTX_ON_STACK(shake256_stack);
	LC_HASH_CTX_ON_STACK(cctx, shake_256);

	printf("hash ctx %s (%s implementation) len %lu\n", name,
	       shake_256 == lc_shake256_c ? "C" : "accelerated",
	       LC_HASH_CTX_SIZE(shake_256));

	lc_hash_init(ctx);
	lc_hash_update(ctx, msg1, sizeof(msg1));
	lc_hash_set_digestsize(ctx, sizeof(act1));
	lc_hash_final(ctx, act1);
	ret = lc_compare(act1, exp1, sizeof(act1), "SHAKE256 1");
	lc_hash_zero(ctx);

	if (ret)
		return ret;

	lc_hash_init(shake256_stack);
	lc_hash_update(shake256_stack, msg1, sizeof(msg1));
	lc_hash_set_digestsize(shake256_stack, sizeof(act1));
	lc_hash_final(shake256_stack, act1);
	ret = lc_compare(act1, exp1, sizeof(act1), "SHAKE256 1 - separate ctx");
	lc_hash_zero(shake256_stack);
	if (ret)
		return ret;

	lc_hash_init(ctx);
	lc_hash_update(ctx, msg2, sizeof(msg2));
	lc_hash_set_digestsize(ctx, sizeof(act2));
	lc_hash_final(ctx, act2);
	ret = lc_compare(act2, exp2, sizeof(act2), "SHAKE256 2");
	lc_hash_zero(ctx);

	if (ret)
		return ret;

	/* Check if cSHAKE falls back to SHAKE */
	lc_cshake_init(cctx, NULL, 0, NULL, 0);
	lc_hash_update(cctx, msg2, sizeof(msg2));
	lc_hash_set_digestsize(cctx, sizeof(act2));
	lc_hash_final(cctx, act2);
	ret = lc_compare(act2, exp2, sizeof(act2), "SHAKE256 3");
	lc_hash_zero(cctx);

	return ret;
}

static int shake256_tester(void)
{
	int ret = 0;

	LC_EXEC_ONE_TEST(lc_shake256);
	LC_EXEC_ONE_TEST(lc_shake256_c);
	LC_EXEC_ONE_TEST(lc_shake256_arm_asm);
	LC_EXEC_ONE_TEST(lc_shake256_arm_ce);
	LC_EXEC_ONE_TEST(lc_shake256_arm_neon);
	LC_EXEC_ONE_TEST(lc_shake256_avx2);
	LC_EXEC_ONE_TEST(lc_shake256_avx512);
	LC_EXEC_ONE_TEST(lc_shake256_riscv_asm);

	return ret;
}

LC_TEST_FUNC(int, main, int argc, char *argv[])
{
	(void)argc;
	(void)argv;
	return shake256_tester();
}
