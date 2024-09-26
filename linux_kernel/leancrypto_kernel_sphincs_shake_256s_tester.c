// SPDX-License-Identifier: GPL-2.0 OR BSD-2-Clause
/*
 * Copyright (C) 2024, Stephan Mueller <smueller@chronox.de>
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

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <crypto/akcipher.h>
#include <linux/module.h>
#include <linux/scatterlist.h>
#include <linux/version.h>

#ifdef LC_SPHINCS_TYPE_128F
#define LC_SPHINCS_IMPL_NAME "sphincs-shake-128f-sha3-256-leancrypto"
#include "../slh-dsa/api/sphincs_type.h"
#include "lc_sphincs_shake_128f.h"
#include "../slh-dsa/tests/sphincs_tester_vectors_shake_128f.h"
#elif defined(LC_SPHINCS_TYPE_128S)
#define LC_SPHINCS_IMPL_NAME "sphincs-shake-128s-sha3-256-leancrypto"
#include "../slh-dsa/api/sphincs_type.h"
#include "lc_sphincs_shake_128s.h"
#include "../slh-dsa/tests/sphincs_tester_vectors_shake_128s.h"
#elif defined(LC_SPHINCS_TYPE_192F)
#define LC_SPHINCS_IMPL_NAME "sphincs-shake-192f-sha3-384-leancrypto"
#include "../slh-dsa/api/sphincs_type.h"
#include "lc_sphincs_shake_192f.h"
#include "../slh-dsa/tests/sphincs_tester_vectors_shake_192f.h"
#elif defined(LC_SPHINCS_TYPE_192S)
#define LC_SPHINCS_IMPL_NAME "sphincs-shake-192s-sha3-384-leancrypto"
#include "../slh-dsa/api/sphincs_type.h"
#include "lc_sphincs_shake_192s.h"
#include "../slh-dsa/tests/sphincs_tester_vectors_shake_192s.h"
#elif defined(LC_SPHINCS_TYPE_256F)
#define LC_SPHINCS_IMPL_NAME "sphincs-shake-256f-sha3-512-leancrypto"
#include "../slh-dsa/api/sphincs_type.h"
#include "lc_sphincs_shake_256f.h"
#include "../slh-dsa/tests/sphincs_tester_vectors_shake_256f.h"
#else
#define LC_SPHINCS_IMPL_NAME "sphincs-shake-256s-sha3-512-leancrypto"
#include "../slh-dsa/api/sphincs_type.h"
#include "lc_sphincs_shake_256s.h"
#include "../slh-dsa/tests/sphincs_tester_vectors_shake_256s.h"
#endif

/*
 * kzfree was renamed to kfree_sensitive in 5.9
 */
#undef free_zero
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 9, 0)
#define free_zero(x) kfree_sensitive(x)
#else
#define free_zero(x) kzfree(x)
#endif

struct lc_tcrypt_res {
	struct completion completion;
	int err;
};

/* tie all data structures together */
struct lc_akcipher_def {
	struct crypto_akcipher *tfm;
	struct akcipher_request *req;
	struct lc_tcrypt_res result;
};

static void lc_akcipher_wait(struct lc_akcipher_def *akcipher, int rc)
{
	init_completion(&akcipher->result.completion);

	switch (rc) {
	case 0:
		break;
	case -EINPROGRESS:
	case -EBUSY:
		wait_for_completion(&akcipher->result.completion);
		rc = akcipher->result.err;
		if (!akcipher->result.err) {
			reinit_completion(&akcipher->result.completion);
		}
		break;
	default:
		pr_info("akcipher cipher operation returned with %d result"
			" %d\n",
			rc, akcipher->result.err);
		break;
	}
}

static int lc_test_sigver(const char *algname,
			  const struct lc_sphincs_test *vector)
{
	struct crypto_akcipher *tfm = NULL;
	struct lc_akcipher_def akcipher;
	struct akcipher_request *req = NULL;
	struct scatterlist src, dst;
	int err = -ENOMEM;

	tfm = crypto_alloc_akcipher(algname, 0, 0);
	if (IS_ERR(tfm)) {
		pr_info("could not allocate akcipher handle for %s %ld\n",
			algname, PTR_ERR(tfm));
		return PTR_ERR(tfm);
	}

	req = akcipher_request_alloc(tfm, GFP_KERNEL);
	if (!req)
		goto err;

	err = crypto_akcipher_set_pub_key(tfm, vector->pk, sizeof(vector->pk));
	if (err)
		goto err;

	pr_info("input datasize: %u\n", crypto_akcipher_maxsize(tfm));

	akcipher.tfm = tfm;
	akcipher.req = req;

	sg_init_one(&src, vector->sig, sizeof(vector->sig));
	sg_init_one(&dst, vector->msg, sizeof(vector->msg));

	akcipher_request_set_crypt(req, &src, &dst, sizeof(vector->sig),
				   sizeof(vector->msg));

	err = crypto_akcipher_verify(req);

	lc_akcipher_wait(&akcipher, err);

	pr_info("Signature verification result %d\n", err);

err:
	if (req)
		akcipher_request_free(req);
	if (tfm)
		crypto_free_akcipher(tfm);
	return err;
}

static int lc_test_siggen(const char *algname, struct lc_sphincs_test *vector)
{
	struct crypto_akcipher *tfm = NULL;
	struct lc_akcipher_def akcipher;
	struct akcipher_request *req = NULL;
	struct scatterlist src, dst;
	int err = -ENOMEM;

	tfm = crypto_alloc_akcipher(algname, 0, 0);
	if (IS_ERR(tfm)) {
		pr_info("could not allocate akcipher handle for %s %ld\n",
			algname, PTR_ERR(tfm));
		return PTR_ERR(tfm);
	}

	req = akcipher_request_alloc(tfm, GFP_KERNEL);
	if (!req)
		goto err;

	err = crypto_akcipher_set_priv_key(tfm, vector->sk, sizeof(vector->sk));
	if (err)
		goto err;

	pr_info("output datasize: %u\n", crypto_akcipher_maxsize(tfm));
	if (sizeof(vector->sig) < crypto_akcipher_maxsize(tfm)) {
		err = -EOVERFLOW;
		goto err;
	}

	akcipher.tfm = tfm;
	akcipher.req = req;

	sg_init_one(&src, vector->msg, sizeof(vector->msg));
	sg_init_one(&dst, vector->sig, crypto_akcipher_maxsize(tfm));
	akcipher_request_set_crypt(req, &src, &dst, sizeof(vector->msg),
				   crypto_akcipher_maxsize(tfm));

	err = crypto_akcipher_sign(req);
	lc_akcipher_wait(&akcipher, err);

	pr_info("Sphincs signature generation result %d\n", err);

err:
	if (req)
		akcipher_request_free(req);
	if (tfm)
		crypto_free_akcipher(tfm);
	return err;
}

static int lc_sphincs_tester(void)
{
	static const struct lc_sphincs_test *vector = &tests[0];
	struct lc_sphincs_test *v = NULL;
	int ret;

	v = kmalloc(sizeof(*v), GFP_KERNEL);
	if (!v) {
		ret = -ENOMEM;
		goto out;
	}
	/*
	 * Copy the data from the r/o buffer as otherwise it cannot be mapped
	 * by the SG-handling logic.
	 */
	memcpy(v->seed, vector->seed, sizeof(v->seed));
	memcpy(v->msg, vector->msg, sizeof(v->msg));
	memcpy(v->pk, vector->pk, sizeof(v->pk));
	memcpy(v->sk, vector->sk, sizeof(v->sk));

	ret = lc_test_siggen(LC_SPHINCS_IMPL_NAME, v);
	if (ret) {
		pr_err("Generation of signature failed\n");
		goto out;
	}

	ret = lc_test_sigver(LC_SPHINCS_IMPL_NAME, v);
	if (ret) {
		pr_err("Verification of generated signature failed\n");
		goto out;
	}

	/*
	 * memcmp is not possible as we use lc_seeded_rng causing a different
	 * signature for each invocation. When using NULL as the RNG context
	 * in sig gen, this can be enabled.
	 */
#if 0
	if (memcmp(sig, vector->sig, siglen)) {
		char hex[2 * siglen];

		pr_err("Calculated signature does not match expected signature\n");

		memset(hex, 0, sizeof(hex));
		bin2hex(hex, sig, siglen);
		pr_err("hex string: %s\n", hex);

		ret = -EINVAL;
		goto out;
	}
#endif

	pr_info("Sphincs " LC_SPHINCS_IMPL_NAME
		" invocation via kernel crypto API succeeded\n");

out:
	if (v)
		free_zero(v);
	return ret;
}

static int __init leancrypto_kernel_sphincs_test_init(void)
{
	return lc_sphincs_tester();
}

static void __exit leancrypto_kernel_sphincs_test_exit(void)
{
}

module_init(leancrypto_kernel_sphincs_test_init);
module_exit(leancrypto_kernel_sphincs_test_exit);

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Stephan Mueller <smueller@chronox.de>");
MODULE_DESCRIPTION(
	"Kernel module leancrypto_kernel_sphincs_test for implementation " LC_SPHINCS_IMPL_NAME);
