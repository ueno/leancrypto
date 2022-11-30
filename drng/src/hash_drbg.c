/* SP800-90A Hash DRBG generic code
 *
 * Copyright Stephan Mueller <smueller@chronox.de>, 2022
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

#include "bitshift_be.h"
#include "lc_hash_drbg.h"
#include "visibility.h"

/***************************************************************
 * Hash invocations requested by DRBG
 ***************************************************************/

static void drbg_hash(struct lc_drbg_hash_state *drbg,
		      uint8_t *outval, const struct lc_drbg_string *in)
{
	lc_hash_init(&drbg->hash_ctx);
	for (; in != NULL; in = in->next)
		      lc_hash_update(&drbg->hash_ctx, in->buf, in->len);
	lc_hash_final(&drbg->hash_ctx, outval);
}

/******************************************************************
 * Hash DRBG callback functions
 ******************************************************************/

/*
 * Increment buffer
 *
 * @dst buffer to increment
 * @add value to add
 */
static void drbg_add_buf(uint8_t *dst, size_t dstlen,
			 const uint8_t *add, size_t addlen)
{
	/* implied: dstlen > addlen */
	uint8_t *dstptr;
	const uint8_t *addptr;
	unsigned int remainder = 0;
	size_t len = addlen;

	dstptr = dst + (dstlen-1);
	addptr = add + (addlen-1);
	while (len) {
		remainder += *dstptr + *addptr;
		*dstptr = remainder & 0xff;
		remainder >>= 8;
		len--; dstptr--; addptr--;
	}
	len = dstlen - addlen;
	while (len && remainder > 0) {
		remainder = *dstptr + 1;
		*dstptr = remainder & 0xff;
		remainder >>= 8;
		len--; dstptr--;
	}
}

/*
 * scratchpad usage: as drbg_hash_update and drbg_hash_df are used
 * interlinked, the scratchpad is used as follows:
 * drbg_hash_update
 *	start: drbg->scratchpad
 *	length: DRBG_STATELEN
 * drbg_hash_df:
 *	start: drbg->scratchpad + DRBG_STATELEN
 *	length: DRBG_BLOCKLEN
 *
 * drbg_hash_process_addtl uses the scratchpad, but fully completes
 * before either of the functions mentioned before are invoked. Therefore,
 * drbg_hash_process_addtl does not need to be specifically considered.
 */

/* Derivation Function for Hash DRBG as defined in 10.4.1 */
static void drbg_hash_df(struct lc_drbg_hash_state *drbg,
			 uint8_t *outval, size_t outlen,
			 struct lc_drbg_string *entropylist)
{
	size_t len = 0;
	unsigned char input[5];
	unsigned char *tmp = drbg->scratchpad + LC_DRBG_HASH_STATELEN;
	struct lc_drbg_string data;

	/* 10.3.1 step 3 */
	input[0] = 1;
	/* Cast is appropriate as outlen is never larger than 2^16. */
	be32_to_ptr(&input[1], (uint32_t)(outlen * 8));

	/* 10.4.1 step 3.1 -- concatenation of data for input into hash */
	lc_drbg_string_fill(&data, input, 5);
	data.next = entropylist;

	/* 10.4.1 step 4 */
	while (len < outlen) {
		size_t blocklen = 0;

		/* 10.3.1 step 4.1 */
		drbg_hash(drbg, tmp, &data);
		/* 10.3.1 step 4.2 */
		input[0]++;
		blocklen = (LC_DRBG_HASH_BLOCKLEN < (outlen - len)) ?
			    LC_DRBG_HASH_BLOCKLEN : (outlen - len);
		memcpy(outval + len, tmp, blocklen);
		len += blocklen;
	}

	memset(tmp, 0, LC_DRBG_HASH_BLOCKLEN);
}

/* update function for Hash DRBG as defined in 10.1.1.2 / 10.1.1.3 */
static void drbg_hash_update(struct lc_drbg_hash_state *drbg,
			     struct lc_drbg_string *seed)
{
	struct lc_drbg_string data1, data2;
	uint8_t *V = drbg->scratchpad;
	uint8_t prefix = DRBG_PREFIX1;

	if (drbg->seeded) {
		/* 10.1.1.3 step 1 */
		memcpy(V, drbg->V, LC_DRBG_HASH_STATELEN);
		lc_drbg_string_fill(&data1, &prefix, 1);
		lc_drbg_string_fill(&data2, V, LC_DRBG_HASH_STATELEN);
		data1.next = &data2;
		data2.next = seed;
	} else {
		      lc_drbg_string_fill(&data1, seed->buf, seed->len);
		data1.next = seed->next;
	}

	/* 10.1.1.2 / 10.1.1.3 step 2 and 3 */
	drbg_hash_df(drbg, drbg->V, LC_DRBG_HASH_STATELEN, &data1);

	/* 10.1.1.2 / 10.1.1.3 step 4  */
	prefix = DRBG_PREFIX0;
	lc_drbg_string_fill(&data1, &prefix, 1);
	lc_drbg_string_fill(&data2, drbg->V, LC_DRBG_HASH_STATELEN);
	data1.next = &data2;
	/* 10.1.1.2 / 10.1.1.3 step 4 */
	drbg_hash_df(drbg, drbg->C, LC_DRBG_HASH_STATELEN, &data1);

	memset(drbg->scratchpad, 0, LC_DRBG_HASH_STATELEN);
}

/* processing of additional information string for Hash DRBG */
static void drbg_hash_process_addtl(struct lc_drbg_hash_state *drbg,
				    struct lc_drbg_string *addtl)
{
	struct lc_drbg_string data1, data2;
	uint8_t prefix = DRBG_PREFIX2;

	/* 10.1.1.4 step 2 */
	if (!addtl || addtl->len == 0)
		return;

	/* 10.1.1.4 step 2a */
	lc_drbg_string_fill(&data1, &prefix, 1);
	lc_drbg_string_fill(&data2, drbg->V, LC_DRBG_HASH_STATELEN);
	data1.next = &data2;
	data2.next = addtl;
	addtl->next = NULL;
	drbg_hash(drbg, drbg->scratchpad, &data1);

	/* 10.1.1.4 step 2b */
	drbg_add_buf(drbg->V, LC_DRBG_HASH_STATELEN,
		     drbg->scratchpad, LC_DRBG_HASH_BLOCKLEN);

	memset(drbg->scratchpad, 0, LC_DRBG_HASH_BLOCKLEN);
}

/* Hashgen defined in 10.1.1.4 */
static void drbg_hash_hashgen(struct lc_drbg_hash_state *drbg,
			      uint8_t *buf, size_t buflen)
{
	struct lc_drbg_string data;
	size_t len = 0;
	uint8_t *src = drbg->scratchpad;
	uint8_t *dst = drbg->scratchpad + LC_DRBG_HASH_STATELEN;
	uint8_t prefix = DRBG_PREFIX1;

	/* 10.1.1.4 step hashgen 2 */
	memcpy(src, drbg->V, LC_DRBG_HASH_STATELEN);
	lc_drbg_string_fill(&data, src, LC_DRBG_HASH_STATELEN);

	while (len < buflen) {
		size_t outlen = 0;

		/* 10.1.1.4 step hashgen 4.1 */
		drbg_hash(drbg, dst, &data);
		outlen = (LC_DRBG_HASH_BLOCKLEN < (buflen - len)) ?
			  LC_DRBG_HASH_BLOCKLEN : (buflen - len);

		/* 10.1.1.4 step hashgen 4.2 */
		memcpy(buf + len, dst, outlen);
		len += outlen;
		/* 10.1.1.4 hashgen step 4.3 */
		if (len < buflen)
			drbg_add_buf(src, LC_DRBG_HASH_STATELEN, &prefix, 1);
	}

	memset(drbg->scratchpad, 0,
	       (LC_DRBG_HASH_STATELEN + LC_DRBG_HASH_BLOCKLEN));
}

/* generate function for Hash DRBG as defined in  10.1.1.4 */
static void drbg_hash_generate_internal(struct lc_drbg_hash_state *drbg,
					uint8_t *buf, size_t buflen,
					struct lc_drbg_string *addtl)
{
	struct lc_drbg_string data1, data2;
	uint8_t req[8], prefix = DRBG_PREFIX3;

	drbg->reseed_ctr++;

	/* 10.1.1.4 step 2 */
	drbg_hash_process_addtl(drbg, addtl);

	/* 10.1.1.4 step 3 */
	drbg_hash_hashgen(drbg, buf, buflen);

	/* this is the value H as documented in 10.1.1.4 */
	/* 10.1.1.4 step 4 */
	lc_drbg_string_fill(&data1, &prefix, 1);
	lc_drbg_string_fill(&data2, drbg->V, LC_DRBG_HASH_STATELEN);
	data1.next = &data2;
	drbg_hash(drbg, drbg->scratchpad, &data1);

	/* 10.1.1.4 step 5 */
	drbg_add_buf(drbg->V, LC_DRBG_HASH_STATELEN,
		     drbg->scratchpad, LC_DRBG_HASH_BLOCKLEN);
	drbg_add_buf(drbg->V, LC_DRBG_HASH_STATELEN,
		     drbg->C, LC_DRBG_HASH_STATELEN);
	be64_to_ptr(req, drbg->reseed_ctr);
	drbg_add_buf(drbg->V, LC_DRBG_HASH_STATELEN, req, sizeof(req));

	memset(drbg->scratchpad, 0, LC_DRBG_HASH_BLOCKLEN);
}

static int
lc_drbg_hash_generate(void *_state,
		      const uint8_t *addtl_input, size_t addtl_input_len,
		      uint8_t *out, size_t outlen)
{
	struct lc_drbg_hash_state *drbg_hash = _state;
	struct lc_drbg_string addtl_data;
	struct lc_drbg_string *addtl = NULL;

	if (!drbg_hash)
		return -EINVAL;

	if (outlen > lc_drbg_max_request_bytes())
		return -EINVAL;

	if (addtl_input_len > lc_drbg_max_addtl())
		return -EINVAL;

	if (addtl_input_len && addtl_input) {
		lc_drbg_string_fill(&addtl_data, addtl_input, addtl_input_len);
		addtl = &addtl_data;
	}
	drbg_hash_generate_internal(drbg_hash, out, outlen, addtl);

	return 0;
}

static int
lc_drbg_hash_seed(void *_state,
		  const uint8_t *seedbuf, size_t seedlen,
		  const uint8_t *persbuf, size_t perslen)
{
	struct lc_drbg_hash_state *drbg_hash = _state;
	struct lc_drbg_string seed;
	struct lc_drbg_string pers;

	if (!drbg_hash)
		return -EINVAL;

	/* 9.1 / 9.2 / 9.3.1 step 3 */
	if (persbuf && perslen > (lc_drbg_max_addtl()))
		return -EINVAL;

	if (!seedbuf || !seedlen)
		return -EINVAL;
	lc_drbg_string_fill(&seed, seedbuf, seedlen);

	/*
	 * concatenation of entropy with personalization str / addtl input)
	 * the variable pers is directly handed in by the caller, so check its
	 * contents whether it is appropriate
	 */
	if (persbuf && perslen) {
		lc_drbg_string_fill(&pers, persbuf, perslen);
		seed.next = &pers;
	}

	drbg_hash_update(drbg_hash, &seed);
	drbg_hash->seeded = 1;

	/*
	 * 10.1.1.2 / 10.1.1.3 step 5 - set reseed counter to 0 instead of 1
	 * as the drbg_generate function increments it before the generate
	 * operation.
	 */
	drbg_hash->reseed_ctr = 0;

	return 0;
}

static void lc_drbg_hash_zero(void *_state)
{
	struct lc_drbg_hash_state *drbg_hash = _state;

	if (!drbg_hash)
		return;

	drbg_hash->reseed_ctr = 0;
	drbg_hash->seeded = 0;
	lc_memset_secure(drbg_hash->hash_state, 0,
			 sizeof(drbg_hash->hash_state));
	lc_memset_secure(drbg_hash->V, 0, sizeof(drbg_hash->V));
	lc_memset_secure(drbg_hash->C, 0, sizeof(drbg_hash->C));
	lc_memset_secure(drbg_hash->scratchpad, 0,
			 sizeof(drbg_hash->scratchpad));
}

LC_INTERFACE_FUNCTION(
int, lc_drbg_hash_alloc, struct lc_rng_ctx **drbg)
{
	struct lc_rng_ctx *out_state = NULL;
	int ret;

	if (!drbg)
		return -EINVAL;

	ret = lc_alloc_aligned((void *)&out_state, LC_HASH_COMMON_ALIGNMENT,
			       LC_DRBG_HASH_CTX_SIZE);
	if (ret)
		return -ret;

	/* prevent paging out of the memory state to swap space */
	ret = mlock(out_state, sizeof(*out_state));
	if (ret && errno != EPERM && errno != EAGAIN) {
		int errsv = errno;

		lc_free(out_state);
		return -errsv;
	}

	LC_DRBG_HASH_RNG_CTX(out_state);

	lc_drbg_hash_zero(out_state->rng_state);

	*drbg = out_state;

	return 0;
}

LC_INTERFACE_FUNCTION(
int, lc_drbg_hash_healthcheck_sanity, struct lc_rng_ctx *drbg)
{
	unsigned char buf[16] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
	size_t max_addtllen, max_request_bytes;
	ssize_t len = 0;
	int ret = -EFAULT;

	if (!drbg)
		return -EINVAL;

	/*
	 * if the following tests fail, it is likely that there is a buffer
	 * overflow as buf is much smaller than the requested or provided
	 * string lengths -- in case the error handling does not succeed
	 * we may get an OOPS. And we want to get an OOPS as this is a
	 * grave bug.
	 */

	max_addtllen = lc_drbg_max_addtl();
	max_request_bytes = lc_drbg_max_request_bytes();

	/* overflow addtllen with additonal info string */
	len = lc_rng_generate(drbg, buf, max_addtllen + 1, buf, sizeof(buf));
	if (len >= 0)
		goto out;

	/* overflow max_bits */
	len = lc_rng_generate(drbg, NULL, 0, buf, (max_request_bytes + 1));
	if (len >= 0)
		goto out;

	/* overflow max addtllen with personalization string */
	len = lc_rng_generate(NULL, NULL, 0, buf, sizeof(buf));
	if (len >= 0)
		goto out;

	ret = 0;

out:
	lc_rng_zero(drbg);
	return ret;
}

static const struct lc_rng _lc_hash_drbg = {
	.generate	= lc_drbg_hash_generate,
	.seed		= lc_drbg_hash_seed,
	.zero		= lc_drbg_hash_zero,
};
LC_INTERFACE_SYMBOL(const struct lc_rng *, lc_hash_drbg) = &_lc_hash_drbg;
