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

#ifndef LC_X509_GENERATOR_H
#define LC_X509_GENERATOR_H

#include "lc_dilithium.h"
#include "lc_sphincs.h"
#include "lc_x509_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup X509Gen X.509 Certificate Generate Handling
 *
 * Concept of X.509 certificate generate handling in leancrypto
 *
 * The leancrypto library provides an X.509 generator which can create
 * X.509 certificates. The generator does not enforce any X.509 limitations and
 * thus allows the caller to generate any combination of field offered by the
 * X.509 specification. To appropriately use the X.509 parser, please consider
 * the following rules:
 *
 * 1. The generated X.509 data blob is independent of the original X.509
 *    certificate data structure.
 *
 * 2. The generator does not allocate any memory. All memory MUST be provided
 *    by the caller. Thus, if the caller provides insufficient memory, the
 *    generator will return -EOVERFLOW.
 *
 * 3. Before invoking the X.509 generator, the caller must allocate an
 *    \p lc_x509_certificate data structure (e.g. on stack) and fill it with the
 *    numerous setter functions to add data.
 */

/**
 * @ingroup X509Gen
 * @brief Generate an X.509 certificate
 *
 * The function generates an X.509 data blob from the filled X.509 data
 * structure.
 *
 * @param [in] x509 The data structure that is filled by the caller before this
 *		    invocation using the various setter functions.
 * @param [in,out] data Raw X.509 data blob in DER / BER format - the caller
 *			must provide the memory
 * @param [in,out] avail_datalen Length of the raw X.509 certificate buffer that
 *				 is free (the input value must be equal to the
 * 				 \p data buffer size, the output refers to how
 *				 many bytes are unused)
 *
 * @return 0 on success or < 0 on error
 */
int lc_x509_cert_gen(struct lc_x509_certificate *x509, uint8_t *data,
		     size_t *avail_datalen);

/**
 * @ingroup X509Gen
 * @brief Set the signer Dilithium key pair for certificate generation
 *
 * The referenced keys are used to perform the signature generation operation
 * and to obtain information from the signer.
 *
 * \note The caller must keep the input data available for the lifetime of
 * \p x509.
 *
 * @param [in] x509 Certificate data structure to be filled with the data.
 * @param [in] pk Dilithium public key
 * @param [in] sk Dilithium secret key
 *
 * @return 0 on success or < 0 on error
 */
int lc_x509_cert_set_signer_keypair_dilithium(struct lc_x509_certificate *x509,
				       struct lc_dilithium_pk *pk,
				       struct lc_dilithium_sk *sk);

/**
 * @ingroup X509Gen
 * @brief Set the Dilithium public key to be embedded into the certificate
 *
 * The referenced key is stored in the certificate and protected by the
 * signature.
 *
 * \note The caller must keep the input data available for the lifetime of
 * \p x509.
 *
 * @param [in] x509 Certificate data structure to be filled with the data.
 * @param [in] pk Dilithium public key
 *
 * @return 0 on success or < 0 on error
 */
int lc_x509_cert_set_pubkey_dilithium(
		      struct lc_x509_certificate *x509,
		      struct lc_dilithium_pk *pk);

/**
 * @ingroup X509Gen
 * @brief Set the extended key usage from human readable form
 *
 * The service function can be called repeadetly to set all intended EKU
 * flags.
 *
 * @param [in] cert Certificate data structure to be filled with the data
 * @param [in] name Human readable string (any wrong string will craete the
 *		    list of allowed strings on stdout)
 *
 * @return 0 on success or < 0 on error
 */
int lc_x509_cert_set_eku(struct lc_x509_certificate *cert, const char *name);

/**
 * @ingroup X509Gen
 * @brief Set the extended key usage in integer form
 *
 * @param [in] cert Certificate data structure to be filled with the data
 * @param [in] val EKU value holding the LC_KEY_EKU_* flags
 *
 * @return 0 on success or < 0 on error
 */
int lc_x509_cert_set_eku_val(struct lc_x509_certificate *cert, uint16_t val);

/**
 * @ingroup X509Gen
 * @brief Set the key usage from human readable form
 *
 * The service function can be called repeadetly to set all intended key usage
 * flags.
 *
 * @param [in] cert Certificate data structure to be filled with the data
 * @param [in] name Human readable string (any wrong string will craete the
 *		    list of allowed strings on stdout)
 *
 * @return 0 on success or < 0 on error
 */
int lc_x509_cert_set_keyusage(struct lc_x509_certificate *cert,
			      const char *name);

/**
 * @ingroup X509Gen
 * @brief Set the key usage in integer form
 *
 * @param [in] cert Certificate data structure to be filled with the data
 * @param [out] val key usage value holding the LC_KEY_USAGE_* flags
 *
 * @return 0 on success or < 0 on error
 */
int lc_x509_cert_set_keyusage_val(struct lc_x509_certificate *cert,
				  uint16_t val);

/**
 * @ingroup X509Gen
 * @brief Mark the certificate to bear the basicConstraint CA
 *
 * @param [in] cert Certificate data structure to be filled with the data
 *
 * @return 0 on success or < 0 on error
 */
int lc_x509_cert_set_ca(struct lc_x509_certificate *cert);

/**
 * @ingroup X509Gen
 * @brief Get the SAN DNS name
 *
 * \note The caller must keep the input data available for the lifetime of
 * \p cert.
 *
 * @param [in] cert Certificate data structure to be filled with the data
 * @param [in] san_dns_name SAN DNS name to add to the certificate
 *
 * @return 0 on success or < 0 on error
 */
int lc_x509_cert_set_san_dns(struct lc_x509_certificate *cert,
			     const char *san_dns_name);

/**
 * @ingroup X509Gen
 * @brief Get the SAN IP value
 *
 * \note The caller must keep the input data available for the lifetime of
 * \p cert.
 *
 * @param [in] cert Certificate data structure to be filled with the data
 * @param [in] san_ip Binary representation of IP address
 * @param [in] san_ip_len Length of the IP address buffer
 *
 * @return 0 on success or < 0 on error
 */
int lc_x509_cert_set_san_ip(struct lc_x509_certificate *cert,
			    const uint8_t *san_ip, size_t san_ip_len);

/**
 * @ingroup X509Gen
 * @brief Helper to convert the human IP address value into binary form
 *
 * @param [in] cert Certificate data structure to be filled with the data
 * @param [in] ip_name Caller-provided buffer to fill with human-readable form
 * @param [out] ip Caller-provided buffer of binary representation of IP address
 * @param [in] ip_len Length of the IP address buffer
 *
 * @return 0 on success or < 0 on error
 */
int lc_x509_enc_san_ip(struct lc_x509_certificate *cert, char *ip_name,
		       uint8_t *ip, size_t *ip_len);

/**
 * @ingroup X509Gen
 * @brief Set the SKID value
 *
 * \note The caller must keep the input data available for the lifetime of
 * \p cert.
 *
 * @param [in] cert Certificate data structure to be filled with the data
 * @param [in] skid Binary representation of SKID
 * @param [in] skidlen length of the SKID buffer
 *
 * @return 0 on success or < 0 on error
 */
int lc_x509_cert_set_skid(struct lc_x509_certificate *cert, const uint8_t *skid,
			  size_t skidlen);

/**
 * @ingroup X509Gen
 * @brief Set the AKID value
 *
 * \note The caller must keep the input data available for the lifetime of
 * \p cert.
 *
 * @param [in] cert Certificate data structure to be filled with the data
 * @param [in] akid Binary representation of AKID
 * @param [in] akidlen length of the AKID buffer
 *
 * @return 0 on success or < 0 on error
 */
int lc_x509_cert_set_akid(struct lc_x509_certificate *cert, const uint8_t *akid,
			  size_t akidlen);

/**
 * @ingroup X509Gen
 * @brief Set the valid-from data to the certificate
 *
 * @param [in] cert Certificate data structure to be filled with the data
 * @param [in] time_since_epoch Time in seconds since EPOCH to set
 *
 * @return 0 on success or < 0 on error
 */
int lc_x509_cert_set_valid_from(struct lc_x509_certificate *cert,
				time64_t time_since_epoch);

/**
 * @ingroup X509Gen
 * @brief Set the valid-to data to the certificate
 *
 * @param [in] cert Certificate data structure to be filled with the data
 * @param [in] time_since_epoch Time in seconds since EPOCH to set
 *
 * @return 0 on success or < 0 on error
 */
int lc_x509_cert_set_valid_to(struct lc_x509_certificate *cert,
			      time64_t time_since_epoch);

/**
 * @ingroup X509Gen
 * @brief Set the subject CN field tp the certificate
 *
 * \note The caller must keep the input data available for the lifetime of
 * \p cert.
 *
 * \note The returned pointer may *not* be NULL-terminated which implies that
 * this function returns also the size of the string. Yet, it is a human
 * readable string.
 *
 * @param [in] cert Certificate data structure to be filled with the data
 * @param [in] string Data field to set
 * @param [in] len Length of the data field
 *
 * @return 0 on success or < 0 on error
 */
int lc_x509_cert_set_subject_cn(struct lc_x509_certificate *cert,
				const char *string, size_t len);

/**
 * @ingroup X509Gen
 * @brief Set the subject email field tp the certificate
 *
 * \note The caller must keep the input data available for the lifetime of
 * \p cert.
 *
 * \note The returned pointer may *not* be NULL-terminated which implies that
 * this function returns also the size of the string. Yet, it is a human
 * readable string.
 *
 * @param [in] cert Certificate data structure to be filled with the data
 * @param [in] string Data field to set
 * @param [in] len Length of the data field
 *
 * @return 0 on success or < 0 on error
 */
int lc_x509_cert_set_subject_email(struct lc_x509_certificate *cert,
				   const char *string, size_t len);

/**
 * @ingroup X509Gen
 * @brief Set the subject OU field tp the certificate
 *
 * \note The caller must keep the input data available for the lifetime of
 * \p cert.
 *
 * \note The returned pointer may *not* be NULL-terminated which implies that
 * this function returns also the size of the string. Yet, it is a human
 * readable string.
 *
 * @param [in] cert Certificate data structure to be filled with the data
 * @param [in] string Data field to set
 * @param [in] len Length of the data field
 *
 * @return 0 on success or < 0 on error
 */
int lc_x509_cert_set_subject_ou(struct lc_x509_certificate *cert,
				const char *string, size_t len);

/**
 * @ingroup X509Gen
 * @brief Set the subject O field tp the certificate
 *
 * \note The caller must keep the input data available for the lifetime of
 * \p cert.
 *
 * \note The returned pointer may *not* be NULL-terminated which implies that
 * this function returns also the size of the string. Yet, it is a human
 * readable string.
 *
 * @param [in] cert Certificate data structure to be filled with the data
 * @param [in] string Data field to set
 * @param [in] len Length of the data field
 *
 * @return 0 on success or < 0 on error
 */
int lc_x509_cert_set_subject_o(struct lc_x509_certificate *cert,
			       const char *string, size_t len);

/**
 * @ingroup X509Gen
 * @brief Set the subject ST field tp the certificate
 *
 * \note The caller must keep the input data available for the lifetime of
 * \p cert.
 *
 * \note The returned pointer may *not* be NULL-terminated which implies that
 * this function returns also the size of the string. Yet, it is a human
 * readable string.
 *
 * @param [in] cert Certificate data structure to be filled with the data
 * @param [in] string Data field to set
 * @param [in] len Length of the data field
 *
 * @return 0 on success or < 0 on error
 */
int lc_x509_cert_set_subject_st(struct lc_x509_certificate *cert,
				const char *string, size_t len);

/**
 * @ingroup X509Gen
 * @brief Set the subject C field tp the certificate
 *
 * \note The caller must keep the input data available for the lifetime of
 * \p cert.
 *
 * \note The returned pointer may *not* be NULL-terminated which implies that
 * this function returns also the size of the string. Yet, it is a human
 * readable string.
 *
 * @param [in] cert Certificate data structure to be filled with the data
 * @param [in] string Data field to set
 * @param [in] len Length of the data field
 *
 * @return 0 on success or < 0 on error
 */
int lc_x509_cert_set_subject_c(struct lc_x509_certificate *cert,
			       const char *string, size_t len);

/**
 * @ingroup X509Gen
 * @brief Set the issuer CN field tp the certificate
 *
 * \note The caller must keep the input data available for the lifetime of
 * \p cert.
 *
 * \note The returned pointer may *not* be NULL-terminated which implies that
 * this function returns also the size of the string. Yet, it is a human
 * readable string.
 *
 * @param [in] cert Certificate data structure to be filled with the data
 * @param [in] string Data field to set
 * @param [in] len Length of the data field
 *
 * @return 0 on success or < 0 on error
 */
int lc_x509_cert_set_issuer_cn(struct lc_x509_certificate *cert,
			       const char *string, size_t len);

/**
 * @ingroup X509Gen
 * @brief Set the issuer email field tp the certificate
 *
 * \note The caller must keep the input data available for the lifetime of
 * \p cert.
 *
 * \note The returned pointer may *not* be NULL-terminated which implies that
 * this function returns also the size of the string. Yet, it is a human
 * readable string.
 *
 * @param [in] cert Certificate data structure to be filled with the data
 * @param [in] string Data field to set
 * @param [in] len Length of the data field
 *
 * @return 0 on success or < 0 on error
 */
int lc_x509_cert_set_issuer_email(struct lc_x509_certificate *cert,
				  const char *string, size_t len);

/**
 * @ingroup X509Gen
 * @brief Set the issuer OU field tp the certificate
 *
 * \note The caller must keep the input data available for the lifetime of
 * \p cert.
 *
 * \note The returned pointer may *not* be NULL-terminated which implies that
 * this function returns also the size of the string. Yet, it is a human
 * readable string.
 *
 * @param [in] cert Certificate data structure to be filled with the data
 * @param [in] string Data field to set
 * @param [in] len Length of the data field
 *
 * @return 0 on success or < 0 on error
 */
int lc_x509_cert_set_issuer_ou(struct lc_x509_certificate *cert,
			       const char *string, size_t len);

/**
 * @ingroup X509Gen
 * @brief Set the issuer O field tp the certificate
 *
 * \note The caller must keep the input data available for the lifetime of
 * \p cert.
 *
 * \note The returned pointer may *not* be NULL-terminated which implies that
 * this function returns also the size of the string. Yet, it is a human
 * readable string.
 *
 * @param [in] cert Certificate data structure to be filled with the data
 * @param [in] string Data field to set
 * @param [in] len Length of the data field
 *
 * @return 0 on success or < 0 on error
 */
int lc_x509_cert_set_issuer_o(struct lc_x509_certificate *cert,
			      const char *string, size_t len);

/**
 * @ingroup X509Gen
 * @brief Set the issuer ST field tp the certificate
 *
 * \note The caller must keep the input data available for the lifetime of
 * \p cert.
 *
 * \note The returned pointer may *not* be NULL-terminated which implies that
 * this function returns also the size of the string. Yet, it is a human
 * readable string.
 *
 * @param [in] cert Certificate data structure to be filled with the data
 * @param [in] string Data field to set
 * @param [in] len Length of the data field
 *
 * @return 0 on success or < 0 on error
 */
int lc_x509_cert_set_issuer_st(struct lc_x509_certificate *cert,
			       const char *string, size_t len);

/**
 * @ingroup X509Gen
 * @brief Set the issuer C field tp the certificate
 *
 * \note The caller must keep the input data available for the lifetime of
 * \p cert.
 *
 * \note The returned pointer may *not* be NULL-terminated which implies that
 * this function returns also the size of the string. Yet, it is a human
 * readable string.
 *
 * @param [in] cert Certificate data structure to be filled with the data
 * @param [in] string Data field to set
 * @param [in] len Length of the data field
 *
 * @return 0 on success or < 0 on error
 */
int lc_x509_cert_set_issuer_c(struct lc_x509_certificate *cert,
			      const char *string, size_t len);

/**
 * @ingroup X509Gen
 * @brief Set the serial number of the certificate
 *
 * \note The caller must keep the input data available for the lifetime of
 * \p cert.
 *
 * @param [in] cert Certificate data structure to be filled with the data
 * @param [in] serial Binary representation of serial number
 * @param [in] serial_len Length of the serial number
 *
 * @return 0 on success or < 0 on error
 */
int lc_x509_cert_set_serial(struct lc_x509_certificate *cert,
			    const uint8_t *serial, size_t serial_len);

/**
 * @ingroup X509Gen
 * @brief Helper to convert the human readable name of a public key algorithm to
 *	  its internal representation
 *
 * If there is no match, the function prints out the allowed strings.
 *
 * @param [in] name public key algorithm in human readable form
 * @param [out] pkey_algo leancrypto algorithm reference
 *
 * @return 0 on success or < 0 on error
 */
int lc_x509_pkey_name_to_algorithm(const char *name,
				   enum lc_sig_types *pkey_algo);

#ifdef __cplusplus
}
#endif

#endif /* LC_X509_GENERATOR_H */
