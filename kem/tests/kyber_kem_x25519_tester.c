/*
 * Copyright (C) 2023, Stephan Mueller <smueller@chronox.de>
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

#include "ext_headers.h"

#include "compare.h"
#include "kyber_internal.h"
#include "ret_checkers.h"
#include "small_stack_support.h"
#include "selftest_rng.h"
#include "visibility.h"

static int kyber_kem_double_tester(void)
{
#if LC_KYBER_K == 2
	static const uint8_t kyber_pk_exp[] = {
		0x5a, 0xb0, 0xa7, 0xbf, 0xc6, 0x23, 0xd4, 0x2b,
		0x06, 0x65, 0x54, 0x26, 0x82, 0xa7, 0xb4, 0xa3,
		0xb7, 0xa3, 0x83, 0x77, 0x5b, 0x3b, 0x64, 0xcc,
		0xae, 0x40, 0x70, 0xe1, 0x65, 0x05, 0x96, 0x91
	};
	static const uint8_t kyber_sk_exp[] = {
		0x3c, 0xbc, 0x53, 0x8d, 0xa4, 0x54, 0x2c, 0xea,
		0x83, 0xa2, 0x19, 0x6a, 0x1d, 0x2c, 0x2f, 0x6e,
		0xdb, 0xce, 0xd0, 0x32, 0xb2, 0xf1, 0xc1, 0x13,
		0xd4, 0x90, 0x01, 0xc2, 0x81, 0x80, 0xa2, 0x93
	};
	static const uint8_t x25519_pk_exp[] = {
		0xe5, 0x6f, 0x17, 0x57, 0x67, 0x40, 0xce, 0x2a,
		0x32, 0xfc, 0x51, 0x45, 0x03, 0x01, 0x45, 0xcf,
		0xb9, 0x7e, 0x63, 0xe0, 0xe4, 0x1d, 0x35, 0x42,
		0x74, 0xa0, 0x79, 0xd3, 0xe6, 0xfb, 0x2e, 0x15
	};
	static const uint8_t x25519_sk_exp[] = {
		0x35, 0xb8, 0xcc, 0x87, 0x3c, 0x23, 0xdc, 0x62,
		0xb8, 0xd2, 0x60, 0x16, 0x9a, 0xfa, 0x2f, 0x75,
		0xab, 0x91, 0x6a, 0x58, 0xd9, 0x74, 0x91, 0x88,
		0x35, 0xd2, 0x5e, 0x6a, 0x43, 0x50, 0x85, 0xb2
	};
	static const uint8_t ss_exp[] = {
		0x2b, 0x04, 0xa0, 0xd7, 0xc2, 0x94, 0x36, 0xb5, 0xe9, 0x0e,
		0xed, 0x43, 0x3d, 0x0c, 0xd4, 0x44, 0x8b, 0x22, 0x04, 0xe6,
		0x88, 0x8b, 0xfd, 0x4f, 0xe7, 0xd7, 0x2f, 0xc1, 0xd0, 0x3e,
		0xf9, 0x60, 0x0a, 0xac, 0x7b, 0xaa, 0x2b, 0xe2, 0x49, 0x5c,
		0x93, 0x46, 0xc3, 0xd4, 0xa6, 0x42, 0x08, 0x77, 0x86, 0x7d,
		0xa4, 0x0c, 0x30, 0x0a, 0xfd, 0xe7, 0xaf, 0x24, 0x81, 0xaf,
		0x08, 0xe1, 0xcc, 0x38, 0x7e, 0x20, 0x7d
	};
#elif LC_KYBER_K == 3
	static const uint8_t kyber_pk_exp[] = {
		0x1b, 0xc3, 0x31, 0xb6, 0x59, 0xa6, 0x1a, 0x04,
		0x88, 0x3d, 0x0c, 0x5e, 0xbb, 0xc0, 0x77, 0x27,
		0x54, 0xa4, 0xc3, 0x3b, 0x6a, 0x90, 0xe5, 0x2e,
		0x06, 0x78, 0xce, 0x06, 0xa0, 0x45, 0x3b, 0xa8
	};
	static const uint8_t kyber_sk_exp[] = {
		0x24, 0xc5, 0x9d, 0x1c, 0x76, 0x03, 0xe7, 0xb7,
		0x4b, 0xc7, 0xaa, 0x1b, 0xc2, 0xcb, 0x3a, 0x21,
		0x4b, 0x3c, 0xfa, 0xeb, 0xb6, 0x3b, 0xd8, 0x5b,
		0x65, 0x40, 0x84, 0x27, 0xc4, 0x98, 0xba, 0x39
	};
	static const uint8_t x25519_pk_exp[] = {
		0xe5, 0x6f, 0x17, 0x57, 0x67, 0x40, 0xce, 0x2a,
		0x32, 0xfc, 0x51, 0x45, 0x03, 0x01, 0x45, 0xcf,
		0xb9, 0x7e, 0x63, 0xe0, 0xe4, 0x1d, 0x35, 0x42,
		0x74, 0xa0, 0x79, 0xd3, 0xe6, 0xfb, 0x2e, 0x15
	};
	static const uint8_t x25519_sk_exp[] = {
		0x35, 0xb8, 0xcc, 0x87, 0x3c, 0x23, 0xdc, 0x62,
		0xb8, 0xd2, 0x60, 0x16, 0x9a, 0xfa, 0x2f, 0x75,
		0xab, 0x91, 0x6a, 0x58, 0xd9, 0x74, 0x91, 0x88,
		0x35, 0xd2, 0x5e, 0x6a, 0x43, 0x50, 0x85, 0xb2
	};
	static const uint8_t ss_exp[] = {
		0xba, 0x3b, 0xec, 0xfe, 0x03, 0xda, 0xf5, 0x1e, 0x70, 0x44,
		0x8a, 0x6c, 0xbc, 0x28, 0x0b, 0xd6, 0xd1, 0xf2, 0x64, 0xc8,
		0xcc, 0x34, 0xad, 0x0f, 0xba, 0x6f, 0x3d, 0xf7, 0x8c, 0xa8,
		0xaa, 0xec, 0x70, 0x8c, 0x21, 0x03, 0x69, 0xc4, 0x80, 0x15,
		0x83, 0xed, 0xde, 0xfa, 0x40, 0x3a, 0x27, 0xed, 0x9e, 0xd2,
		0x37, 0x17, 0x89, 0xb3, 0x29, 0x1b, 0x5c, 0x33, 0x5b, 0x5d,
		0xb3, 0x4d, 0xae, 0x6a, 0x46, 0x9b, 0xfe
	};
#elif LC_KYBER_K == 4
	static const uint8_t kyber_pk_exp[] = {
		0x78, 0x0c, 0x72, 0x5a, 0x25, 0x15, 0x1f, 0x1b,
		0x09, 0xe9, 0xc7, 0x1d, 0x26, 0x97, 0x5d, 0xd6,
		0x52, 0x20, 0xe0, 0x59, 0xca, 0x52, 0x6b, 0x6e,
		0x6f, 0x87, 0x4a, 0xa5, 0xe8, 0xb6, 0x22, 0x97
	};
	static const uint8_t kyber_sk_exp[] = {
		0x24, 0xc5, 0x9d, 0x1c, 0x76, 0x03, 0xe7, 0xb7,
		0x4b, 0xc7, 0xaa, 0x1b, 0xc2, 0xcb, 0x3a, 0x21,
		0x4b, 0x3c, 0xfa, 0xeb, 0xb6, 0x3b, 0xd8, 0x5b,
		0x65, 0x40, 0x84, 0x27, 0xc4, 0x98, 0xba, 0x39
	};
	static const uint8_t x25519_pk_exp[] = {
		0xe5, 0x6f, 0x17, 0x57, 0x67, 0x40, 0xce, 0x2a,
		0x32, 0xfc, 0x51, 0x45, 0x03, 0x01, 0x45, 0xcf,
		0xb9, 0x7e, 0x63, 0xe0, 0xe4, 0x1d, 0x35, 0x42,
		0x74, 0xa0, 0x79, 0xd3, 0xe6, 0xfb, 0x2e, 0x15
	};
	static const uint8_t x25519_sk_exp[] = {
		0x35, 0xb8, 0xcc, 0x87, 0x3c, 0x23, 0xdc, 0x62,
		0xb8, 0xd2, 0x60, 0x16, 0x9a, 0xfa, 0x2f, 0x75,
		0xab, 0x91, 0x6a, 0x58, 0xd9, 0x74, 0x91, 0x88,
		0x35, 0xd2, 0x5e, 0x6a, 0x43, 0x50, 0x85, 0xb2
	};
	static const uint8_t ss_exp[] = {
		0xea, 0x09, 0x60, 0xb5, 0xb7, 0xad, 0x78, 0x41, 0x73, 0x3e,
		0xc6, 0x11, 0x61, 0x3b, 0x76, 0x9e, 0x2c, 0x68, 0xd9, 0x11,
		0x52, 0x31, 0xaf, 0xef, 0x96, 0x3a, 0x7f, 0x3a, 0x13, 0x9d,
		0x8d, 0x9e, 0x4e, 0x99, 0x85, 0xf0, 0x8c, 0x68, 0x24, 0x3e,
		0xa5, 0x56, 0x0a, 0xc7, 0x28, 0x9b, 0x3e, 0x80, 0x49, 0xc2,
		0xb9, 0xbb, 0xce, 0x29, 0x6d, 0x1f, 0xf7, 0xcf, 0xc6, 0xc9,
		0x84, 0xbe, 0x9e, 0xff, 0x9b, 0x60, 0x64
	};
#endif
	struct workspace {
		struct lc_kyber_x25519_pk pk;
		struct lc_kyber_x25519_sk sk;
		struct lc_kyber_x25519_ct ct;
		uint8_t ss1[sizeof(ss_exp)], ss2[sizeof(ss_exp)];
	};
	int ret, rc = 0;
	LC_DECLARE_MEM(ws, struct workspace, sizeof(uint64_t));
	LC_SELFTEST_DRNG_CTX_ON_STACK(selftest_rng);

	CKINT(lc_kyber_x25519_keypair(&ws->pk, &ws->sk, selftest_rng));

	rc += lc_compare(ws->pk.pk.pk, kyber_pk_exp, sizeof(kyber_pk_exp),
			 "Kyber pk keygen\n");
	rc += lc_compare(ws->sk.sk.sk, kyber_sk_exp, sizeof(kyber_sk_exp),
			 "Kyber sk keygen\n");
	rc += lc_compare(ws->pk.pk_x25519.pk, x25519_pk_exp,
			 sizeof(x25519_pk_exp), "X25519 pk keygen\n");
	rc += lc_compare(ws->sk.sk_x25519.sk, x25519_sk_exp,
			 sizeof(x25519_sk_exp), "X25519 sk keygen\n");

	CKINT(lc_kyber_x25519_enc_kdf_internal(
		&ws->ct, ws->ss1, sizeof(ws->ss1), &ws->pk, selftest_rng));

	CKINT(lc_kyber_x25519_dec_kdf(ws->ss2, sizeof(ws->ss2), &ws->ct,
				      &ws->sk));

	rc += lc_compare(ws->ss1, ss_exp, sizeof(ss_exp),
			 "Kyber X25519 SS generation\n");
	rc += lc_compare(ws->ss1, ws->ss2, sizeof(ws->ss2),
			 "Kyber X25519 SS comparison\n");

out:
	LC_RELEASE_MEM(ws);
	return ret ? ret : rc;
}

LC_TEST_FUNC(int, main, int argc, char *argv[])
{
	int ret = 0;

	(void)argc;
	(void)argv;

	ret += kyber_kem_double_tester();

	return ret;
}
