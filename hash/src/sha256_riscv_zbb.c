/* Generic SHA-256 implementation
 *
 * Copyright (C) 2025, Stephan Mueller <smueller@chronox.de>
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

#include "asm/riscv64/sha2-256-riscv.h"

#include "bitshift.h"
#include "sha256_riscv_zbb.h"
#include "sha2_common.h"
#include "visibility.h"

static void sha256_update_riscv_zbb(void *_state, const uint8_t *in,
				    size_t inlen)
{
	struct lc_sha256_state *ctx = _state;

	sha256_update(ctx, in, inlen, sha256_block_data_order_riscv_zbb);
}

static void sha256_final_riscv_zbb(void *_state, uint8_t *digest)
{
	struct lc_sha256_state *ctx = _state;

	if (!ctx)
		return;

	sha256_final(_state, digest, sha256_block_data_order_riscv_zbb);
}

static const struct lc_hash _sha256_riscv_zbb = {
	.init = sha256_init,
	.update = sha256_update_riscv_zbb,
	.final = sha256_final_riscv_zbb,
	.set_digestsize = NULL,
	.get_digestsize = sha256_get_digestsize,
	.sponge_permutation = NULL,
	.sponge_add_bytes = NULL,
	.sponge_extract_bytes = NULL,
	.sponge_newstate = NULL,
	.sponge_rate = LC_SHA256_SIZE_BLOCK,
	.statesize = sizeof(struct lc_sha256_state),
};

LC_INTERFACE_SYMBOL(const struct lc_hash *,
		    lc_sha256_riscv_zbb) = &_sha256_riscv_zbb;
