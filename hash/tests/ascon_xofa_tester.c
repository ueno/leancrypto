/*
 * Copyright (C) 2020 - 2024, Stephan Mueller <smueller@chronox.de>
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
#include "lc_ascon_hash.h"
#include "visibility.h"

#include "ascon_avx512.h"
#include "ascon_arm_neon.h"
#include "ascon_c.h"

#define LC_EXEC_ONE_TEST(ascon_impl)                                           \
	if (ascon_impl)                                                        \
	ret += ascon_xofa_tester(ascon_impl, #ascon_impl)

static int ascon_xofa_tester(const struct lc_hash *ascon, const char *name)
{
	/*
	 * Vector 5 from genkat_crypto_hash_asconxofav12_ref generated by code
	 * https://github.com/ascon/ascon-c
	 */
	static const uint8_t msg[] = { 0x00, 0x01, 0x02, 0x03 };
	static const uint8_t exp[] = { 0xE2, 0xFE, 0xE1, 0x11, 0xA8, 0xE4, 0xB6,
				       0x22, 0x46, 0x2F, 0x89, 0x7D, 0xA4, 0x8C,
				       0x02, 0xB8, 0x07, 0xCA, 0xDD, 0xC2, 0x80,
				       0x17, 0x18, 0x6D, 0xC8, 0x56, 0xD8, 0xCF,
				       0x3D, 0xC2, 0x02, 0x48 };
	uint8_t act[LC_ASCON_HASH_DIGESTSIZE];
	int ret;
	LC_ASCON_CTX_ON_STACK(ascon_stack, ascon);

	printf("hash ctx %s (%s implementation) len %u\n", name,
	       ascon == lc_ascon_xofa_c ? "C" : "accelerated",
	       (unsigned int)LC_HASH_CTX_SIZE(ascon));

	lc_xof(lc_ascon_xofa, msg, sizeof(msg), act, sizeof(act));
	ret = lc_compare(act, exp, sizeof(act), "Ascon XOFa");

	lc_hash_init(ascon_stack);
	lc_hash_update(ascon_stack, msg, sizeof(msg));
	lc_hash_set_digestsize(ascon_stack, sizeof(act));
	lc_hash_final(ascon_stack, act);
	lc_hash_zero(ascon_stack);
	ret += lc_compare(act, exp, sizeof(act), "Ascon XOFa stack");

	return ret;
}

LC_TEST_FUNC(int, main, int argc, char *argv[])
{
	int ret = 0;
	(void)argc;
	(void)argv;

	LC_EXEC_ONE_TEST(lc_ascon_xofa);
	LC_EXEC_ONE_TEST(lc_ascon_xofa_c);
	LC_EXEC_ONE_TEST(lc_ascon_xofa_avx512);
	LC_EXEC_ONE_TEST(lc_ascon_xofa_arm_neon);

	return ret;
}
