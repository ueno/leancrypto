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

#include "aes_scr.h"
#include "aes_internal.h"
#include "compare.h"
#include "ext_headers.h"
#include "lc_aes.h"
#include "lc_sym.h"
#include "mode_cbc.h"
#include "visibility.h"
#include "xor.h"

struct lc_sym_state {
	struct lc_mode_state cbc_state;
	struct aes_block_ctx block_ctx;
};

#define LC_AES_CBC_BLOCK_SIZE sizeof(struct lc_sym_state)

static void aes_cbc_encrypt_scr(struct lc_sym_state *ctx, const uint8_t *in,
				uint8_t *out, size_t len)
{
	lc_mode_cbc_c->encrypt(&ctx->cbc_state, in, out, len);
}

static void aes_cbc_decrypt_scr(struct lc_sym_state *ctx, const uint8_t *in,
				uint8_t *out, size_t len)
{
	lc_mode_cbc_c->decrypt(&ctx->cbc_state, in, out, len);
}

static void aes_cbc_init_scr(struct lc_sym_state *ctx)
{
	static int tested = 0;

	mode_cbc_selftest(lc_aes_cbc_scr, &tested, "AES-CBC SCR");
	lc_mode_cbc_c->init(&ctx->cbc_state, lc_aes_scr, &ctx->block_ctx);
}

static int aes_cbc_setkey_scr(struct lc_sym_state *ctx, const uint8_t *key,
			      size_t keylen)
{
	if (!ctx)
		return -EINVAL;
	return lc_mode_cbc_c->setkey(&ctx->cbc_state, key, keylen);
}

static int aes_cbc_setiv_scr(struct lc_sym_state *ctx, const uint8_t *iv,
			     size_t ivlen)
{
	return lc_mode_cbc_c->setiv(&ctx->cbc_state, iv, ivlen);
}

static struct lc_sym _lc_aes_cbc_scr = {
	.init = aes_cbc_init_scr,
	.setkey = aes_cbc_setkey_scr,
	.setiv = aes_cbc_setiv_scr,
	.encrypt = aes_cbc_encrypt_scr,
	.decrypt = aes_cbc_decrypt_scr,
	.statesize = LC_AES_CBC_BLOCK_SIZE,
	.blocksize = AES_BLOCKLEN,
};
LC_INTERFACE_SYMBOL(const struct lc_sym *, lc_aes_cbc_scr) = &_lc_aes_cbc_scr;
