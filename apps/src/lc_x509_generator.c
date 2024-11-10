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

#define _GNU_SOURCE
#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "binhexbin.h"
#include "ret_checkers.h"
#include "lc_x509_generator.h"
#include "lc_x509_generator_file_helper.h"
#include "lc_x509_parser.h"

struct lc_x509_key_input_data {
	enum lc_sig_types sig_type;
	union {
		struct lc_dilithium_pk dilithium_pk;
		struct lc_dilithium_ed25519_pk dilithium_ed25519_pk;
		struct lc_sphincs_pk sphincs_pk;
	} pk;
	union {
		struct lc_dilithium_sk dilithium_sk;
		struct lc_dilithium_ed25519_sk dilithium_ed25519_sk;
		struct lc_sphincs_sk sphincs_sk;
	} sk;
};

struct x509_generator_opts {
	struct lc_x509_certificate cert;
	struct lc_x509_certificate signer_cert;
	struct lc_x509_key_input_data key_input_data;
	struct lc_x509_key_input_data signer_key_input_data;
	uint8_t ipaddr[16];
	uint8_t *raw_skid;
	size_t raw_skid_size;
	uint8_t *raw_akid;
	size_t raw_akid_size;
	uint8_t *raw_serial;
	size_t raw_serial_size;
	const char *outfile;
	const char *sk_file;
	const char *pk_file;
	const char *x509_signer_file;
	const char *signer_sk_file;
	uint8_t *signer_data;
	size_t signer_data_len;

	uint8_t *pk_data;
	size_t pk_len;
	uint8_t *sk_data;
	size_t sk_len;
	uint8_t *signer_sk_data;
	size_t signer_sk_len;

	enum lc_sig_types create_keypair_algo;
	enum lc_sig_types in_key_type;
};

static int x509_gen_file(struct x509_generator_opts *opts,
			 const uint8_t *certdata, size_t certdata_len)
{
	FILE *f = NULL;
	size_t written;
	int ret = 0;

	CKNULL(opts->outfile, -EINVAL);

	f = fopen(opts->outfile, "w");
	CKNULL(f, -errno);

	written = fwrite(certdata, 1, certdata_len, f);
	if (written != certdata_len) {
		printf("Writing of X.509 certificate data failed: %zu bytes written, %zu bytes to write\n",
		       written, certdata_len);
		ret = -EFAULT;
		goto out;
	}

out:
	if (f)
		fclose(f);
	return ret;
}

static int x509_gen_cert(struct x509_generator_opts *opts)
{
	struct lc_x509_certificate parsed_x509;
	struct lc_x509_certificate *gcert = &opts->cert;
	uint8_t data[65536] = { 0 };
	size_t avail_datalen = sizeof(data), datalen;
	int ret;

	CKINT(lc_x509_cert_gen(gcert, data, &avail_datalen));
	datalen = sizeof(data) - avail_datalen;

	if (!opts->outfile) {
		bin2print(data, datalen, stdout, "X.509 Certificate");
		goto out;
	}

	CKINT(x509_gen_file(opts, data, datalen));

out:
	lc_x509_cert_clear(&parsed_x509);
	return ret;
}

static void x509_clean_opts(struct x509_generator_opts *opts)
{
	if (!opts)
		return;

	if (opts->raw_skid)
		free(opts->raw_skid);
	if (opts->raw_akid)
		free(opts->raw_akid);
	if (opts->raw_serial)
		free(opts->raw_serial);

	lc_x509_cert_clear(&opts->cert);
	lc_x509_cert_clear(&opts->signer_cert);

	release_data(opts->signer_data, opts->signer_data_len);
	release_data(opts->pk_data, opts->pk_len);
	release_data(opts->sk_data, opts->sk_len);
	release_data(opts->signer_sk_data, opts->signer_sk_len);

	lc_memset_secure(opts, 0, sizeof(*opts));
}

static int x509_enc_eku(struct x509_generator_opts *opts,
			const char *opt_optarg)
{
	struct lc_x509_certificate *cert = &opts->cert;
	unsigned long val;
	char *string;
	int ret;

	val = strtoul(opt_optarg, &string, 10);
	if (val == 0) {
		CKINT(lc_x509_cert_set_eku(cert, string));
	} else if (val < USHRT_MAX) {
		CKINT(lc_x509_cert_set_eku_val(cert, (uint16_t)val));
	} else {
		return -ERANGE;
	}

out:
	return ret;
}

static int x509_enc_keyusage(struct x509_generator_opts *opts,
			     const char *opt_optarg)
{
	struct lc_x509_certificate *cert = &opts->cert;
	unsigned long val;
	char *string;
	int ret;

	val = strtoul(opt_optarg, &string, 10);
	if (val == 0) {
		CKINT_LOG(lc_x509_cert_set_keyusage(cert, string),
			  "Set key usage string %s\n", string);
	} else if (val < USHRT_MAX) {
		CKINT_LOG(lc_x509_cert_set_keyusage_val(cert, (uint16_t)val),
			  "Set key usage value %u\n", (uint16_t)val);
	} else {
		return -ERANGE;
	}

out:
	return ret;
}

static int x509_enc_ca(struct x509_generator_opts *opts)
{
	struct lc_x509_certificate *cert = &opts->cert;

	return lc_x509_cert_set_ca(cert);
}

static int x509_enc_san_dns(struct x509_generator_opts *opts,
			    const char *opt_optarg)
{
	struct lc_x509_certificate *cert = &opts->cert;

	return lc_x509_cert_set_san_dns(cert, opt_optarg);
}

static int x509_enc_san_ip(struct x509_generator_opts *opts, char *opt_optarg)
{
	struct lc_x509_certificate *cert = &opts->cert;
	size_t ip_len = sizeof(opts->ipaddr);
	int ret;

	CKINT(lc_x509_enc_san_ip(cert, opt_optarg, opts->ipaddr, &ip_len));

	CKINT(lc_x509_cert_set_san_ip(cert, opts->ipaddr, ip_len));

out:
	return ret;
}

static int x509_enc_skid(struct x509_generator_opts *opts,
			 const char *opt_optarg)
{
	struct lc_x509_certificate *cert = &opts->cert;
	int ret;

	if (!opt_optarg)
		return -EINVAL;

	CKINT(hex2bin_alloc(opt_optarg, strlen(opt_optarg), &opts->raw_skid,
			    &opts->raw_skid_size));

	CKINT(lc_x509_cert_set_skid(cert, opts->raw_skid, opts->raw_skid_size));

out:
	return ret;
}

static int x509_enc_akid(struct x509_generator_opts *opts,
			 const char *opt_optarg)
{
	struct lc_x509_certificate *cert = &opts->cert;
	int ret;

	if (!opt_optarg)
		return -EINVAL;

	CKINT(hex2bin_alloc(opt_optarg, strlen(opt_optarg), &opts->raw_akid,
			    &opts->raw_akid_size));

	CKINT(lc_x509_cert_set_akid(cert, opts->raw_akid, opts->raw_akid_size));

out:
	return ret;
}

static int x509_enc_valid_from(struct x509_generator_opts *opts,
			       const char *opt_optarg)
{
	struct lc_x509_certificate *cert = &opts->cert;
	unsigned long long val;

	val = strtoull(opt_optarg, NULL, 10);
	if (val == ULLONG_MAX)
		return -ERANGE;

	return lc_x509_cert_set_valid_from(cert, (time64_t)val);
}

static int x509_enc_valid_to(struct x509_generator_opts *opts,
			     const char *opt_optarg)
{
	struct lc_x509_certificate *cert = &opts->cert;
	unsigned long long val;

	val = strtoull(opt_optarg, NULL, 10);
	if (val == ULLONG_MAX)
		return -ERANGE;

	return lc_x509_cert_set_valid_to(cert, (time64_t)val);
}

static int x509_enc_subject_cn(struct x509_generator_opts *opts,
			       const char *opt_optarg)
{
	struct lc_x509_certificate *cert = &opts->cert;

	return lc_x509_cert_set_subject_cn(cert, opt_optarg,
					   strlen(opt_optarg));
}

static int x509_enc_subject_email(struct x509_generator_opts *opts,
				  const char *opt_optarg)
{
	struct lc_x509_certificate *cert = &opts->cert;

	return lc_x509_cert_set_subject_email(cert, opt_optarg,
					      strlen(opt_optarg));
}

static int x509_enc_subject_ou(struct x509_generator_opts *opts,
			       const char *opt_optarg)
{
	struct lc_x509_certificate *cert = &opts->cert;

	return lc_x509_cert_set_subject_ou(cert, opt_optarg,
					   strlen(opt_optarg));
}

static int x509_enc_subject_o(struct x509_generator_opts *opts,
			      const char *opt_optarg)
{
	struct lc_x509_certificate *cert = &opts->cert;

	return lc_x509_cert_set_subject_o(cert, opt_optarg, strlen(opt_optarg));
}

static int x509_enc_subject_st(struct x509_generator_opts *opts,
			       const char *opt_optarg)
{
	struct lc_x509_certificate *cert = &opts->cert;

	return lc_x509_cert_set_subject_st(cert, opt_optarg,
					   strlen(opt_optarg));
}

static int x509_enc_subject_c(struct x509_generator_opts *opts,
			      const char *opt_optarg)
{
	struct lc_x509_certificate *cert = &opts->cert;

	return lc_x509_cert_set_subject_c(cert, opt_optarg, strlen(opt_optarg));
}

static int x509_enc_issuer_cn(struct x509_generator_opts *opts,
			      const char *opt_optarg)
{
	struct lc_x509_certificate *cert = &opts->cert;

	return lc_x509_cert_set_issuer_cn(cert, opt_optarg, strlen(opt_optarg));
}

static int x509_enc_issuer_email(struct x509_generator_opts *opts,
				 const char *opt_optarg)
{
	struct lc_x509_certificate *cert = &opts->cert;

	return lc_x509_cert_set_issuer_email(cert, opt_optarg,
					     strlen(opt_optarg));
}

static int x509_enc_issuer_ou(struct x509_generator_opts *opts,
			      const char *opt_optarg)
{
	struct lc_x509_certificate *cert = &opts->cert;

	return lc_x509_cert_set_issuer_ou(cert, opt_optarg, strlen(opt_optarg));
}

static int x509_enc_issuer_o(struct x509_generator_opts *opts,
			     const char *opt_optarg)
{
	struct lc_x509_certificate *cert = &opts->cert;

	return lc_x509_cert_set_issuer_o(cert, opt_optarg, strlen(opt_optarg));
}

static int x509_enc_issuer_st(struct x509_generator_opts *opts,
			      const char *opt_optarg)
{
	struct lc_x509_certificate *cert = &opts->cert;

	return lc_x509_cert_set_issuer_st(cert, opt_optarg, strlen(opt_optarg));
}

static int x509_enc_issuer_c(struct x509_generator_opts *opts,
			     const char *opt_optarg)
{
	struct lc_x509_certificate *cert = &opts->cert;

	return lc_x509_cert_set_issuer_c(cert, opt_optarg, strlen(opt_optarg));
}

static int x509_enc_serial(struct x509_generator_opts *opts,
			   const char *opt_optarg)
{
	struct lc_x509_certificate *cert = &opts->cert;
	int ret;

	if (!opt_optarg)
		return -EINVAL;

	CKINT(hex2bin_alloc(opt_optarg, strlen(opt_optarg), &opts->raw_serial,
			    &opts->raw_serial_size));

	CKINT(lc_x509_cert_set_serial(cert, opts->raw_serial,
				      opts->raw_serial_size));

out:
	return ret;
}

static int x509_enc_set_signer(struct x509_generator_opts *opts)
{
	struct lc_x509_certificate *gcert = &opts->cert;
	struct lc_x509_key_input_data *signer_key_input_data =
		&opts->signer_key_input_data;
	size_t paramlen = 0, pk_len;
	enum lc_sig_types pkey_type;
	const uint8_t *dparam, *pk_ptr;
	const char *param;
	int ret;

	CKINT_LOG(get_data(opts->x509_signer_file, &opts->signer_data,
			   &opts->signer_data_len),
		  "mmap failure\n");

	CKINT(lc_x509_cert_parse(&opts->signer_cert, opts->signer_data,
				 opts->signer_data_len));

	CKINT(lc_x509_policy_is_ca(&opts->signer_cert));
	if (ret != LC_X509_POL_TRUE)
		printf("WARNING: X.509 signer is no CA!\n");

	/* Set AKID */
	CKINT(lc_x509_cert_get_skid(&opts->signer_cert, &dparam, &paramlen));
	if (!dparam)
		printf("WARNING: X.509 signer has no SKID\n");
	CKINT(lc_x509_cert_set_akid(&opts->cert, dparam, paramlen));

	/* Set issuer */
	CKINT(lc_x509_cert_get_subject_c(&opts->signer_cert, &param,
					 &paramlen));
	CKINT(lc_x509_cert_set_issuer_c(&opts->cert, param, paramlen));

	CKINT(lc_x509_cert_get_subject_st(&opts->signer_cert, &param,
					  &paramlen));
	CKINT(lc_x509_cert_set_issuer_st(&opts->cert, param, paramlen));

	CKINT(lc_x509_cert_get_subject_o(&opts->signer_cert, &param,
					 &paramlen));
	CKINT(lc_x509_cert_set_issuer_o(&opts->cert, param, paramlen));

	CKINT(lc_x509_cert_get_subject_ou(&opts->signer_cert, &param,
					  &paramlen));
	CKINT(lc_x509_cert_set_issuer_ou(&opts->cert, param, paramlen));

	CKINT(lc_x509_cert_get_subject_cn(&opts->signer_cert, &param,
					  &paramlen));
	CKINT(lc_x509_cert_set_issuer_cn(&opts->cert, param, paramlen));

	CKINT(lc_x509_cert_get_subject_email(&opts->signer_cert, &param,
					     &paramlen));
	CKINT(lc_x509_cert_set_issuer_email(&opts->cert, param, paramlen));

	CKINT_LOG(get_data(opts->signer_sk_file, &opts->signer_sk_data,
			   &opts->signer_sk_len),
		  "Signer SK mmap failure\n");

	/* Get the signature type based on the signer key */
	CKINT_LOG(lc_x509_cert_get_pubkey(&opts->signer_cert, &pk_ptr, &pk_len,
					  &pkey_type),
		  "Find X.509 signer public key\n");
	switch (pkey_type) {
	case LC_SIG_DILITHIUM_44:
	case LC_SIG_DILITHIUM_65:
	case LC_SIG_DILITHIUM_87:
		CKINT_LOG(lc_dilithium_pk_load(
				  &signer_key_input_data->pk.dilithium_pk,
				  pk_ptr, pk_len),
			  "Loading X.509 signer public key from certificate\n");
		CKINT_LOG(lc_dilithium_sk_load(
				  &signer_key_input_data->sk.dilithium_sk,
				  opts->signer_sk_data, opts->signer_sk_len),
			  "Loading X.509 signer private key from file\n");
		CKINT_LOG(lc_x509_cert_set_signer_keypair_dilithium(
				  gcert,
				  &signer_key_input_data->pk.dilithium_pk,
				  &signer_key_input_data->sk.dilithium_sk),
			  "Setting X.509 key pair for signing\n");
		break;
	case LC_SIG_SPINCS_SHAKE_128F:
		// CKINT(lc_sphincs_keypair(&key_input_data->pk.sphincs_pk,
		// 			 &key_input_data->sk.sphincs_sk,
		// 			 lc_seeded_rng,
		// 			 LC_SPHINCS_SHAKE_128f));
		// break;
	case LC_SIG_SPINCS_SHAKE_128S:
		// CKINT(lc_sphincs_keypair(&key_input_data->pk.sphincs_pk,
		// 			 &key_input_data->sk.sphincs_sk,
		// 			 lc_seeded_rng,
		// 			 LC_SPHINCS_SHAKE_128s));
		// break;
	case LC_SIG_SPINCS_SHAKE_192F:
		// CKINT(lc_sphincs_keypair(&key_input_data->pk.sphincs_pk,
		// 			 &key_input_data->sk.sphincs_sk,
		// 			 lc_seeded_rng,
		// 			 LC_SPHINCS_SHAKE_192f));
		// break;
	case LC_SIG_SPINCS_SHAKE_192S:
		// CKINT(lc_sphincs_keypair(&key_input_data->pk.sphincs_pk,
		// 			 &key_input_data->sk.sphincs_sk,
		// 			 lc_seeded_rng,
		// 			 LC_SPHINCS_SHAKE_192s));
		// break;
	case LC_SIG_SPINCS_SHAKE_256F:
		// CKINT(lc_sphincs_keypair(&key_input_data->pk.sphincs_pk,
		// 			 &key_input_data->sk.sphincs_sk,
		// 			 lc_seeded_rng,
		// 			 LC_SPHINCS_SHAKE_256f));
		// break;
	case LC_SIG_SPINCS_SHAKE_256S:
		// CKINT(lc_sphincs_keypair(&key_input_data->pk.sphincs_pk,
		// 			 &key_input_data->sk.sphincs_sk,
		// 			 lc_seeded_rng,
		// 			 LC_SPHINCS_SHAKE_256s));
		// break;
	case LC_SIG_DILITHIUM_44_ED25519:
		// CKINT(lc_dilithium_ed25519_keypair(&key_input_data->pk.dilithium_ed25519_pk,
		// 			   &key_input_data->sk.dilithium_ed25519_sk,
		// 			   lc_seeded_rng, LC_DILITHIUM_44));
		// CKINT(lc_dilithium_pk_ptr(&pk_ptr, &pk_len, &key_input_data->pk.dilithium_pk));
		// CKINT(lc_dilithium_sk_ptr(&sk_ptr, &sk_len, &key_input_data->sk.dilithium_sk));
		// break;
	case LC_SIG_DILITHIUM_65_ED25519:
		// CKINT(lc_dilithium_ed25519_keypair(&key_input_data->pk.dilithium_ed25519_pk,
		// 			   &key_input_data->sk.dilithium_ed25519_sk,
		// 			   lc_seeded_rng, LC_DILITHIUM_65));
		// break;
	case LC_SIG_DILITHIUM_87_ED25519:
		// CKINT(lc_dilithium_ed25519_keypair(&key_input_data->pk.dilithium_ed25519_pk,
		// 			   &key_input_data->sk.dilithium_ed25519_sk,
		// 			   lc_seeded_rng, LC_DILITHIUM_87));
		// break;
	case LC_SIG_DILITHIUM_87_ED448:
	case LC_SIG_ECDSA_X963:
	case LC_SIG_ECRDSA_PKCS1:
	case LC_SIG_RSA_PKCS1:
	case LC_SIG_SM2:
	case LC_SIG_UNKNOWN:
	default:
		return -ENOPKG;
	}

out:
	return ret;
}

static int x509_enc_set_pubkey(struct x509_generator_opts *opts)
{
	struct lc_x509_certificate *gcert = &opts->cert;
	struct lc_x509_key_input_data *key_input_data = &opts->key_input_data;
	unsigned int self_signed = !(opts->x509_signer_file);
	int ret = 0;

	if (opts->create_keypair_algo) {
		uint8_t *sk_ptr;
		size_t sk_len;

		switch (opts->create_keypair_algo) {
		case LC_SIG_DILITHIUM_44:
			CKINT(lc_dilithium_keypair(
				&key_input_data->pk.dilithium_pk,
				&key_input_data->sk.dilithium_sk, lc_seeded_rng,
				LC_DILITHIUM_44));
			if (self_signed) {
				CKINT(lc_x509_cert_set_signer_keypair_dilithium(
					gcert, &key_input_data->pk.dilithium_pk,
					&key_input_data->sk.dilithium_sk));
			}
			CKINT(lc_x509_cert_set_pubkey_dilithium(
				gcert, &key_input_data->pk.dilithium_pk));
			CKINT(lc_dilithium_sk_ptr(
				&sk_ptr, &sk_len,
				&key_input_data->sk.dilithium_sk));
			break;
		case LC_SIG_DILITHIUM_65:
			CKINT(lc_dilithium_keypair(
				&key_input_data->pk.dilithium_pk,
				&key_input_data->sk.dilithium_sk, lc_seeded_rng,
				LC_DILITHIUM_65));
			if (self_signed) {
				CKINT(lc_x509_cert_set_signer_keypair_dilithium(
					gcert, &key_input_data->pk.dilithium_pk,
					&key_input_data->sk.dilithium_sk));
			}
			CKINT(lc_x509_cert_set_pubkey_dilithium(
				gcert, &key_input_data->pk.dilithium_pk));
			CKINT(lc_dilithium_sk_ptr(
				&sk_ptr, &sk_len,
				&key_input_data->sk.dilithium_sk));
			break;
		case LC_SIG_DILITHIUM_87:
			CKINT(lc_dilithium_keypair(
				&key_input_data->pk.dilithium_pk,
				&key_input_data->sk.dilithium_sk, lc_seeded_rng,
				LC_DILITHIUM_87));
			if (self_signed) {
				CKINT(lc_x509_cert_set_signer_keypair_dilithium(
					gcert, &key_input_data->pk.dilithium_pk,
					&key_input_data->sk.dilithium_sk));
			}
			CKINT(lc_x509_cert_set_pubkey_dilithium(
				gcert, &key_input_data->pk.dilithium_pk));
			CKINT(lc_dilithium_sk_ptr(
				&sk_ptr, &sk_len,
				&key_input_data->sk.dilithium_sk));
			break;
		case LC_SIG_SPINCS_SHAKE_128F:
			// CKINT(lc_sphincs_keypair(&key_input_data->pk.sphincs_pk,
			// 			 &key_input_data->sk.sphincs_sk,
			// 			 lc_seeded_rng,
			// 			 LC_SPHINCS_SHAKE_128f));
			// break;
		case LC_SIG_SPINCS_SHAKE_128S:
			// CKINT(lc_sphincs_keypair(&key_input_data->pk.sphincs_pk,
			// 			 &key_input_data->sk.sphincs_sk,
			// 			 lc_seeded_rng,
			// 			 LC_SPHINCS_SHAKE_128s));
			// break;
		case LC_SIG_SPINCS_SHAKE_192F:
			// CKINT(lc_sphincs_keypair(&key_input_data->pk.sphincs_pk,
			// 			 &key_input_data->sk.sphincs_sk,
			// 			 lc_seeded_rng,
			// 			 LC_SPHINCS_SHAKE_192f));
			// break;
		case LC_SIG_SPINCS_SHAKE_192S:
			// CKINT(lc_sphincs_keypair(&key_input_data->pk.sphincs_pk,
			// 			 &key_input_data->sk.sphincs_sk,
			// 			 lc_seeded_rng,
			// 			 LC_SPHINCS_SHAKE_192s));
			// break;
		case LC_SIG_SPINCS_SHAKE_256F:
			// CKINT(lc_sphincs_keypair(&key_input_data->pk.sphincs_pk,
			// 			 &key_input_data->sk.sphincs_sk,
			// 			 lc_seeded_rng,
			// 			 LC_SPHINCS_SHAKE_256f));
			// break;
		case LC_SIG_SPINCS_SHAKE_256S:
			// CKINT(lc_sphincs_keypair(&key_input_data->pk.sphincs_pk,
			// 			 &key_input_data->sk.sphincs_sk,
			// 			 lc_seeded_rng,
			// 			 LC_SPHINCS_SHAKE_256s));
			// break;
		case LC_SIG_DILITHIUM_44_ED25519:
			// CKINT(lc_dilithium_ed25519_keypair(&key_input_data->pk.dilithium_ed25519_pk,
			// 			   &key_input_data->sk.dilithium_ed25519_sk,
			// 			   lc_seeded_rng, LC_DILITHIUM_44));
			// CKINT(lc_dilithium_pk_ptr(&pk_ptr, &pk_len, &key_input_data->pk.dilithium_pk));
			// CKINT(lc_dilithium_sk_ptr(&sk_ptr, &sk_len, &key_input_data->sk.dilithium_sk));
			// break;
		case LC_SIG_DILITHIUM_65_ED25519:
			// CKINT(lc_dilithium_ed25519_keypair(&key_input_data->pk.dilithium_ed25519_pk,
			// 			   &key_input_data->sk.dilithium_ed25519_sk,
			// 			   lc_seeded_rng, LC_DILITHIUM_65));
			// break;
		case LC_SIG_DILITHIUM_87_ED25519:
			// CKINT(lc_dilithium_ed25519_keypair(&key_input_data->pk.dilithium_ed25519_pk,
			// 			   &key_input_data->sk.dilithium_ed25519_sk,
			// 			   lc_seeded_rng, LC_DILITHIUM_87));
			// break;
		case LC_SIG_DILITHIUM_87_ED448:
		case LC_SIG_ECDSA_X963:
		case LC_SIG_ECRDSA_PKCS1:
		case LC_SIG_RSA_PKCS1:
		case LC_SIG_SM2:
		case LC_SIG_UNKNOWN:
		default:
			return -ENOPKG;
		}

		CKINT(write_data(opts->sk_file, sk_ptr, sk_len));

	} else {
		CKNULL_LOG(!opts->in_key_type, -EINVAL,
			   "Input key files must be specified with key type\n");

		CKINT_LOG(get_data(opts->pk_file, &opts->pk_data,
				   &opts->pk_len),
			  "PK mmap failure\n");
		if (self_signed) {
			CKINT_LOG(get_data(opts->sk_file, &opts->sk_data,
					   &opts->sk_len),
				  "PK mmap failure\n");
		}

		switch (opts->in_key_type) {
		case LC_SIG_DILITHIUM_44:
		case LC_SIG_DILITHIUM_65:
		case LC_SIG_DILITHIUM_87:
			CKINT(lc_dilithium_pk_load(
				&key_input_data->pk.dilithium_pk, opts->pk_data,
				opts->pk_len));
			CKINT(lc_x509_cert_set_pubkey_dilithium(
				gcert, &key_input_data->pk.dilithium_pk));
			if (self_signed) {
				CKINT(lc_dilithium_sk_load(
					&key_input_data->sk.dilithium_sk,
					opts->sk_data, opts->sk_len));
				CKINT(lc_x509_cert_set_signer_keypair_dilithium(
					gcert, &key_input_data->pk.dilithium_pk,
					&key_input_data->sk.dilithium_sk));
			}
			break;
		case LC_SIG_SPINCS_SHAKE_128F:
			// CKINT(lc_sphincs_keypair(&key_input_data->pk.sphincs_pk,
			// 			 &key_input_data->sk.sphincs_sk,
			// 			 lc_seeded_rng,
			// 			 LC_SPHINCS_SHAKE_128f));
			// break;
		case LC_SIG_SPINCS_SHAKE_128S:
			// CKINT(lc_sphincs_keypair(&key_input_data->pk.sphincs_pk,
			// 			 &key_input_data->sk.sphincs_sk,
			// 			 lc_seeded_rng,
			// 			 LC_SPHINCS_SHAKE_128s));
			// break;
		case LC_SIG_SPINCS_SHAKE_192F:
			// CKINT(lc_sphincs_keypair(&key_input_data->pk.sphincs_pk,
			// 			 &key_input_data->sk.sphincs_sk,
			// 			 lc_seeded_rng,
			// 			 LC_SPHINCS_SHAKE_192f));
			// break;
		case LC_SIG_SPINCS_SHAKE_192S:
			// CKINT(lc_sphincs_keypair(&key_input_data->pk.sphincs_pk,
			// 			 &key_input_data->sk.sphincs_sk,
			// 			 lc_seeded_rng,
			// 			 LC_SPHINCS_SHAKE_192s));
			// break;
		case LC_SIG_SPINCS_SHAKE_256F:
			// CKINT(lc_sphincs_keypair(&key_input_data->pk.sphincs_pk,
			// 			 &key_input_data->sk.sphincs_sk,
			// 			 lc_seeded_rng,
			// 			 LC_SPHINCS_SHAKE_256f));
			// break;
		case LC_SIG_SPINCS_SHAKE_256S:
			// CKINT(lc_sphincs_keypair(&key_input_data->pk.sphincs_pk,
			// 			 &key_input_data->sk.sphincs_sk,
			// 			 lc_seeded_rng,
			// 			 LC_SPHINCS_SHAKE_256s));
			// break;
		case LC_SIG_DILITHIUM_44_ED25519:
			// CKINT(lc_dilithium_ed25519_keypair(&key_input_data->pk.dilithium_ed25519_pk,
			// 			   &key_input_data->sk.dilithium_ed25519_sk,
			// 			   lc_seeded_rng, LC_DILITHIUM_44));
			// CKINT(lc_dilithium_pk_ptr(&pk_ptr, &pk_len, &key_input_data->pk.dilithium_pk));
			// CKINT(lc_dilithium_sk_ptr(&sk_ptr, &sk_len, &key_input_data->sk.dilithium_sk));
			// break;
		case LC_SIG_DILITHIUM_65_ED25519:
			// CKINT(lc_dilithium_ed25519_keypair(&key_input_data->pk.dilithium_ed25519_pk,
			// 			   &key_input_data->sk.dilithium_ed25519_sk,
			// 			   lc_seeded_rng, LC_DILITHIUM_65));
			// break;
		case LC_SIG_DILITHIUM_87_ED25519:
			// CKINT(lc_dilithium_ed25519_keypair(&key_input_data->pk.dilithium_ed25519_pk,
			// 			   &key_input_data->sk.dilithium_ed25519_sk,
			// 			   lc_seeded_rng, LC_DILITHIUM_87));
			// break;
		case LC_SIG_DILITHIUM_87_ED448:
		case LC_SIG_ECDSA_X963:
		case LC_SIG_ECRDSA_PKCS1:
		case LC_SIG_RSA_PKCS1:
		case LC_SIG_SM2:
		case LC_SIG_UNKNOWN:
		default:
			return -ENOPKG;
		}
	}

out:
	return ret;
}

static int x509_enc_crypto_algo(struct x509_generator_opts *opts)
{
	int ret;

	if (!opts->sk_file) {
		printf("A secret key file for the generation the signature is missing!\n");
		return -EINVAL;
	}

	if (!opts->create_keypair_algo && !opts->pk_file) {
		printf("A public key file for the generation the X.509 certificate is missing as no key pair shall be generated!\n");
		return -EINVAL;
	}

	/*
	 * Set the public key
	 */
	CKINT_LOG(x509_enc_set_pubkey(opts),
		  "Setting X.509 public key failed\n");

	/*
	 * Set the signer information
	 */
	if (opts->x509_signer_file) {
		CKINT_LOG(x509_enc_set_signer(opts),
			  "Setting the signer X.509 key data failed\n");
	}

out:
	return ret;
}

static void x509_generator_usage(void)
{
	fprintf(stderr, "\nLeancrypto X.509 Certificate Generator\n");

	fprintf(stderr, "\nUsage:\n");
	fprintf(stderr, "\t[OPTION]\n");

	fprintf(stderr, "\nOptions:\n");
	fprintf(stderr,
		"\t-o --outfile <FILE>\t\tFile to write certificate to\n");
	fprintf(stderr,
		"\t   --sk-file <FILE>\t\tFile with secret key used for signature\n");
	fprintf(stderr, "\t\t\t\t\t\tInput when key is available,\n");
	fprintf(stderr, "\t\t\t\t\t\toutput with --create-keypair\n");
	fprintf(stderr,
		"\t   --pk-file <FILE>\t\tFile with secret key used for signature\n");
	fprintf(stderr, "\t\t\t\t\t\tInput when key is available,\n");
	fprintf(stderr, "\t\t\t\t\t\toutput with --create-keypair\n");
	fprintf(stderr,
		"\t   --key-type <TYPE>\tInput keys are of given type\n");
	fprintf(stderr,
		"\t   --create-keypair <TYPE>\tCreate key pair of given type\n");
	fprintf(stderr,
		"\t   --x509-signer <FILE>\t\tX.509 certificate of signer\n");
	fprintf(stderr,
		"\t\t\t\t\t\tIf not set, create a self-signed certificate\n");
	fprintf(stderr,
		"\t   --signer-sk-file <FILE>\t\tFile with signer secret\n");

	fprintf(stderr,
		"\n\t   --eku <FLAG>\t\t\tSet Extended Key Usage flag\n");
	fprintf(stderr, "\t\t\t\t\t\tQuery available flags with \"?\"\n");

	fprintf(stderr, "\t   --keyusage <FLAG>\t\tSet Key Usage flag\n");
	fprintf(stderr, "\t\t\t\t\t\tQuery available flags with \"?\"\n");

	fprintf(stderr,
		"\t   --ca\t\t\t\tSet CA basic constraint with criticality\n");
	fprintf(stderr, "\t   --san-dns <NAME> \t\tSet SAN DNS name\n");
	fprintf(stderr, "\t   --san-ip <IP> \t\tSet SAN IP name\n");
	fprintf(stderr, "\t   --skid\t\t\tSet SKID (in hex form)\n");
	fprintf(stderr, "\t   --akid\t\t\tSet AKID (in hex form)\n");
	fprintf(stderr, "\t\t\t\t\t\tAKID only used without\n");
	fprintf(stderr, "\t\t\t\t\t\tX.509 signer\n");
	fprintf(stderr, "\t   --valid-from\t\t\tSet start time\n");
	fprintf(stderr, "\t   --valid-to\t\t\tSet end time\n");
	fprintf(stderr,
		"\t   --serial <VALUE>\t\tSet serial numer (in hex form)\n");

	fprintf(stderr, "\n\t   --subject-cn <VALUE>\t\tSet subject CN\n");
	fprintf(stderr, "\t   --subject-email <VALUE>\tSet subject Email\n");
	fprintf(stderr, "\t   --subject-ou <VALUE>\t\tSet subject OU\n");
	fprintf(stderr, "\t   --subject-o <VALUE>\t\tSet subject O\n");
	fprintf(stderr, "\t   --subject-st <VALUE>\t\tSet subject ST\n");
	fprintf(stderr, "\t   --subject-c <VALUE>\t\tSet subject C\n");

	fprintf(stderr,
		"\n\tThe following issuer options are only relevant if no X.509\n");
	fprintf(stderr, "\tsigner is present - they are unused otherwise\n");
	fprintf(stderr, "\t   --issuer-cn <VALUE>\t\tSet issuer CN\n");
	fprintf(stderr, "\t   --issuer-email <VALUE>\tSet issuer Email\n");
	fprintf(stderr, "\t   --issuer-ou <VALUE>\t\tSet issuer OU\n");
	fprintf(stderr, "\t   --issuer-o <VALUE>\t\tSet issuer O\n");
	fprintf(stderr, "\t   --issuer-st <VALUE>\t\tSet issuer ST\n");
	fprintf(stderr, "\t   --issuer-c <VALUE>\t\tSet issuer C\n");

	fprintf(stderr, "\n\t-h  --help\t\t\tPrint this help text\n");
}

int main(int argc, char *argv[])
{
	struct x509_generator_opts parsed_opts = { 0 };
	int ret = 0, opt_index = 0;

	static const char *opts_short = "ho:";
	static const struct option opts[] = { { "help", 0, 0, 'h' },

					      { "outfile", 1, 0, 'o' },
					      { "sk-file", 1, 0, 0 },
					      { "pk-file", 1, 0, 0 },
					      { "key-type", 1, 0, 0 },
					      { "create-keypair", 1, 0, 0 },

					      { "x509-signer", 1, 0, 0 },
					      { "signer-sk-file", 1, 0, 0 },

					      { "eku", 1, 0, 0 },
					      { "keyusage", 1, 0, 0 },

					      { "ca", 0, 0, 0 },
					      { "san-dns", 1, 0, 0 },
					      { "san-ip", 1, 0, 0 },
					      { "skid", 1, 0, 0 },
					      { "akid", 1, 0, 0 },
					      { "valid-from", 1, 0, 0 },
					      { "valid-to", 1, 0, 0 },
					      { "serial", 1, 0, 0 },

					      { "subject-cn", 1, 0, 0 },
					      { "subject-email", 1, 0, 0 },
					      { "subject-ou", 1, 0, 0 },
					      { "subject-o", 1, 0, 0 },
					      { "subject-st", 1, 0, 0 },
					      { "subject-c", 1, 0, 0 },

					      { "issuer-cn", 1, 0, 0 },
					      { "issuer-email", 1, 0, 0 },
					      { "issuer-ou", 1, 0, 0 },
					      { "issuer-o", 1, 0, 0 },
					      { "issuer-st", 1, 0, 0 },
					      { "issuer-c", 1, 0, 0 },

					      { 0, 0, 0, 0 } };

	opterr = 0;
	while (1) {
		int c = getopt_long(argc, argv, opts_short, opts, &opt_index);

		if (-1 == c)
			break;
		switch (c) {
		case 0:
			switch (opt_index) {
			/* help */
			case 0:
				x509_generator_usage();
				goto out;

			/* outfile */
			case 1:
				parsed_opts.outfile = optarg;
				break;
			/* sk-file */
			case 2:
				parsed_opts.sk_file = optarg;
				break;
			/* pk-file */
			case 3:
				parsed_opts.pk_file = optarg;
				break;
			/* key-type */
			case 4:
				CKINT(lc_x509_pkey_name_to_algorithm(
					optarg, &parsed_opts.in_key_type));
				break;
			/* create-keypair */
			case 5:
				CKINT(lc_x509_pkey_name_to_algorithm(
					optarg,
					&parsed_opts.create_keypair_algo));
				break;
			/* x509-signer */
			case 6:
				parsed_opts.x509_signer_file = optarg;
				break;
			/* signer-sk-file */
			case 7:
				parsed_opts.signer_sk_file = optarg;
				break;

			/* eku */
			case 8:
				CKINT_LOG(x509_enc_eku(&parsed_opts, optarg),
					  "Set EKU\n");
				break;
			/* keyusage */
			case 9:
				CKINT_LOG(x509_enc_keyusage(&parsed_opts,
							    optarg),
					  "Set key usage\n");
				break;
			/* ca */
			case 10:
				CKINT_LOG(x509_enc_ca(&parsed_opts),
					  "Set CA\n");
				break;
			/* san-dns */
			case 11:
				CKINT_LOG(x509_enc_san_dns(&parsed_opts,
							   optarg),
					  "Set SAN DNS\n");
				break;
			/* san-ip */
			case 12:
				CKINT_LOG(x509_enc_san_ip(&parsed_opts, optarg),
					  "Set SAN IP\n");
				break;

			/* skid */
			case 13:
				CKINT_LOG(x509_enc_skid(&parsed_opts, optarg),
					  "Set SKID\n");
				break;
			/* akid */
			case 14:
				CKINT_LOG(x509_enc_akid(&parsed_opts, optarg),
					  "Set AKID\n");
				break;
			/* valid-from */
			case 15:
				CKINT_LOG(x509_enc_valid_from(&parsed_opts,
							      optarg),
					  "Set valid from\n");
				break;
			/* valid-to */
			case 16:
				CKINT_LOG(x509_enc_valid_to(&parsed_opts,
							    optarg),
					  "Set valid to\n");
				break;
			/* serial */
			case 17:
				CKINT_LOG(x509_enc_serial(&parsed_opts, optarg),
					  "Set serial\n");
				break;

			/* subject-cn */
			case 18:
				CKINT(x509_enc_subject_cn(&parsed_opts,
							  optarg));
				break;
			/* subject-email */
			case 19:
				CKINT(x509_enc_subject_email(&parsed_opts,
							     optarg));
				break;
			/* subject-ou */
			case 20:
				CKINT(x509_enc_subject_ou(&parsed_opts,
							  optarg));
				break;
			/* subject-o */
			case 21:
				CKINT(x509_enc_subject_o(&parsed_opts, optarg));
				break;
			/* subject-st */
			case 22:
				CKINT(x509_enc_subject_st(&parsed_opts,
							  optarg));
				break;
			/* subject-c */
			case 23:
				CKINT(x509_enc_subject_c(&parsed_opts, optarg));
				break;

			/* issuer-cn */
			case 24:
				CKINT(x509_enc_issuer_cn(&parsed_opts, optarg));
				break;
			/* issuer-email */
			case 25:
				CKINT(x509_enc_issuer_email(&parsed_opts,
							    optarg));
				break;
			/* issuer-ou */
			case 26:
				CKINT(x509_enc_issuer_ou(&parsed_opts, optarg));
				break;
			/* issuer-o */
			case 27:
				CKINT(x509_enc_issuer_o(&parsed_opts, optarg));
				break;
			/* issuer-st */
			case 28:
				CKINT(x509_enc_issuer_st(&parsed_opts, optarg));
				break;
			/* issuer-c */
			case 29:
				CKINT(x509_enc_issuer_c(&parsed_opts, optarg));
				break;
			}
			break;

		case 'o':
			parsed_opts.outfile = optarg;
			break;
		case 'h':
			x509_generator_usage();
			goto out;

		default:
			x509_generator_usage();
			ret = -1;
			goto out;
		}
	}

	CKINT(x509_enc_crypto_algo(&parsed_opts));

	CKINT(x509_gen_cert(&parsed_opts));

out:
	x509_clean_opts(&parsed_opts);
	return -ret;
}
