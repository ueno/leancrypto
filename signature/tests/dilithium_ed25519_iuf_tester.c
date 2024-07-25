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

#include "dilithium_tester.h"
#include "ret_checkers.h"
#include "selftest_rng.h"
#include "small_stack_support.h"
#include "visibility.h"

static int dilithium_ed25519_tester(struct lc_dilithium_ed25519_ctx *ctx,
				    int failcheck)
{
	struct workspace {
		struct lc_dilithium_ed25519_sk sk;
		struct lc_dilithium_ed25519_pk pk;
		struct lc_dilithium_ed25519_sig sig;
	};
	static const uint8_t msg[] = { 0x00, 0x01, 0x02 };
	static const uint8_t msg2[] = { 0x00, 0x01, 0x03 };
	int ret;
	LC_DECLARE_MEM(ws, struct workspace, sizeof(uint64_t));
	LC_SELFTEST_DRNG_CTX_ON_STACK(selftest_rng);

	CKINT(lc_dilithium_ed25519_keypair(&ws->pk, &ws->sk, selftest_rng));
	CKINT(lc_dilithium_ed25519_sign_init(ctx, &ws->sk));
	CKINT(lc_dilithium_ed25519_sign_update(ctx, &msg[0], 1));
	CKINT(lc_dilithium_ed25519_sign_update(ctx, &msg[1], 1));
	CKINT(lc_dilithium_ed25519_sign_update(ctx, &msg[2], 1));
	CKINT(lc_dilithium_ed25519_sign_final(&ws->sig, ctx, &ws->sk,
					      selftest_rng));
	CKINT_LOG(lc_dilithium_ed25519_verify(&ws->sig, msg, sizeof(msg), &ws->pk),
		  "Sign IUF, Verify one-shot\n");

	CKINT(lc_dilithium_ed25519_keypair(&ws->pk, &ws->sk, selftest_rng));
	CKINT(lc_dilithium_ed25519_sign(&ws->sig, msg, sizeof(msg), &ws->sk,
					selftest_rng));
	CKINT(lc_dilithium_ed25519_verify_init(ctx, &ws->pk));
	CKINT(lc_dilithium_ed25519_verify_update(ctx, &msg[0], 1));
	CKINT(lc_dilithium_ed25519_verify_update(ctx, &msg[1], 1));
	CKINT(lc_dilithium_ed25519_verify_update(ctx, &msg[2], 1));
	CKINT_LOG(lc_dilithium_ed25519_verify_final(&ws->sig, ctx, &ws->pk),
		  "Sign one-shot, Verify IUF\n");

	CKINT(lc_dilithium_ed25519_keypair(&ws->pk, &ws->sk, selftest_rng));
	CKINT(lc_dilithium_ed25519_sign_init(ctx, &ws->sk));
	CKINT(lc_dilithium_ed25519_sign_update(ctx, &msg[0], 1));
	CKINT(lc_dilithium_ed25519_sign_update(ctx, &msg[1], 1));
	CKINT(lc_dilithium_ed25519_sign_update(ctx, &msg[2], 1));
	CKINT(lc_dilithium_ed25519_sign_final(&ws->sig, ctx, &ws->sk,
					      selftest_rng));
	CKINT(lc_dilithium_ed25519_verify_init(ctx, &ws->pk));
	CKINT(lc_dilithium_ed25519_verify_update(ctx, &msg[0], 1));
	CKINT(lc_dilithium_ed25519_verify_update(ctx, &msg[1], 1));
	CKINT(lc_dilithium_ed25519_verify_update(ctx, &msg[2], 1));
	CKINT_LOG(lc_dilithium_ed25519_verify_final(&ws->sig, ctx, &ws->pk),
		  "Sign IUF, Verify IUF\n");

	if (!failcheck)
		goto out;

	/* modify msg */
	CKINT(lc_dilithium_ed25519_verify_init(ctx, &ws->pk));
	CKINT(lc_dilithium_ed25519_verify_update(ctx, &msg2[0], 1));
	CKINT(lc_dilithium_ed25519_verify_update(ctx, &msg2[1], 1));
	CKINT(lc_dilithium_ed25519_verify_update(ctx, &msg2[2], 1));
	if (lc_dilithium_ed25519_verify_final(&ws->sig, ctx, &ws->pk) !=
	    -EBADMSG) {
		ret = 1;
		goto out;
	}

	/* modify Dilithium key */
	ws->pk.pk.pk[0] = (uint8_t)((ws->pk.pk.pk[0] + 0x01) & 0xff);
	CKINT(lc_dilithium_ed25519_verify_init(ctx, &ws->pk));
	CKINT(lc_dilithium_ed25519_verify_update(ctx, &msg[0], 1));
	CKINT(lc_dilithium_ed25519_verify_update(ctx, &msg[1], 1));
	CKINT(lc_dilithium_ed25519_verify_update(ctx, &msg[2], 1));
	if (lc_dilithium_ed25519_verify_final(&ws->sig, ctx, &ws->pk) !=
	    -EBADMSG) {
		ret = 1;
		goto out;
	}
	ws->pk.pk.pk[0] = (uint8_t)((ws->pk.pk.pk[0] - 0x01) & 0xff);

	/* modify ED25519 key */
	ws->pk.pk_ed25519.pk[0] =
		(uint8_t)((ws->pk.pk_ed25519.pk[0] + 0x01) & 0xff);
	CKINT(lc_dilithium_ed25519_verify_init(ctx, &ws->pk));
	CKINT(lc_dilithium_ed25519_verify_update(ctx, &msg[0], 1));
	CKINT(lc_dilithium_ed25519_verify_update(ctx, &msg[1], 1));
	CKINT(lc_dilithium_ed25519_verify_update(ctx, &msg[2], 1));
	ret = lc_dilithium_ed25519_verify_final(&ws->sig, ctx, &ws->pk);
	if (ret != -EBADMSG && ret != -EINVAL) {
		ret = 1;
		goto out;
	}
	ws->pk.pk_ed25519.pk[0] =
		(uint8_t)((ws->pk.pk_ed25519.pk[0] - 0x01) & 0xff);

	ret = 0;

out:
	LC_RELEASE_MEM(ws);
	return ret;
}

LC_TEST_FUNC(int, main, int argc, char *argv[])
{
	size_t count;
	int ret = 0;
	LC_DILITHIUM_ED25519_CTX_ON_STACK(ctx_stack);

	(void)argv;

	if (argc != 2) {
		struct lc_dilithium_ed25519_ctx *ctx_heap;

		CKINT(lc_dilithium_ed25519_ctx_alloc(&ctx_heap));

		ret += dilithium_ed25519_tester(ctx_stack, 1);
		lc_dilithium_ed25519_ctx_zero(ctx_stack);

		ret += dilithium_ed25519_tester(ctx_heap, 1);
		lc_dilithium_ed25519_ctx_zero_free(ctx_heap);

		return ret;
	}

	for (count = 0; count < 10000; count++)
		ret += dilithium_ed25519_tester(ctx_stack, 0);

out:
	return ret;
}
