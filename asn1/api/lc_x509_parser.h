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

#ifndef LC_X509_PARSER_H
#define LC_X509_PARSER_H

#include "ext_headers.h"
#include "lc_x509_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup X509 X.509 Certificate Parsing Handling
 *
 * Concept of X.509 certificate parsing handling in leancrypto
 *
 * The leancrypto library provides an X.509 parser which can read and understand
 * X.509 certificates. To appropriately use the X.509 parser, please consider
 * the following rules:
 *
 * 1. The parser interprets the provided X.509 data blob and fills a data
 *    structure which allows immediate access to the certificate properties
 *    by the leancrypto code. The data structure \p x509_certificate is provided
 *    as part of the official header file. But it is NOT considered to be an
 *    API. I.e. member variables or the structure format may change between
 *    versions of leancrypto without announcement. The reason for providing the
 *    data structure in the official header file is to support stack-only
 *    clients.
 *
 * 2. The parser fills the data structure with pointers into the original X.509
 *    data blob. The caller MUST keep the original X.509 data blob at the same
 *    location for the life time of the associated instance of the
 *    \p x509_certificate data structure.
 *
 * 3. The X.509 parser API call only interprets and parses the X.509 data blob.
 *    It does NOT enforce any kind of restrictions or policies. The caller
 *    MUST use the provided X.509 policy API to enforce policies on the given
 *    certificate.
 */

/**
 * @ingroup X509
 * @brief Clear the resources used by the X.509 certificate parsing state
 *
 * @param [in] cert Certificate structure to be cleared
 */
void lc_x509_cert_clear(struct lc_x509_certificate *cert);

/**
 * @ingroup X509
 * @brief Parse an X.509 certificate
 *
 * The function parses an X.509 data buffer into a data structure that allows
 * accessing the various data points of the certificate.
 *
 * \note The \p cert data structure will contain pointers to the \p data
 * buffer. I.e. the certificate parsing analyzes \p data and finds all relevant
 * data in the raw X.509 data blob. The caller MUST therefore keep the
 * \p data pointer constant as long as the \p cert pointer is valid.
 *
 * \note This function only loads and parses the certificate into the data
 * structure to allow leancrypto to immediately access the information. This
 * function call does not validate the certificate (except for a self-signed
 * signature). Thus, the caller MUST apply the X.509 policy check functions
 * to validate the certificate considering that the loading of the certificate
 * has no information about the use case.
 *
 * @param [in,out] cert The data structure that is filled with all parameters
 *			from the X.509 certificate data buffer. The buffer must
 *			have been allocated by the caller. It is permissible
 *			to keep it on the stack.
 * @param [in] data Raw X.509 data blob in DER / BER format
 * @param [in] datalen Length of the raw X.509 certificate buffer
 *
 * @return 0 on success or < 0 on error
 */
int lc_x509_cert_parse(struct lc_x509_certificate *cert, const uint8_t *data,
		       size_t datalen);

/**
 * @ingroup X509
 * @brief Get a reference of the public key data
 *
 * The service function returns a pointer to the public key data in the
 * certificate.
 *
 * \note The returned pointers have the same life time as \p cert.
 *
 * @param [in] cert X.509 certificate from which the public is to be obtained
 * @param [out] pk X.509 public key buffer reference (may be NULL)
 * @param [out] pk_size Size of the public key (may be NULL)
 * @param [out] key_type Type of the public key (may be NULL)
 *
 * @return 0 on success or < 0 on error
 */
int lc_x509_cert_get_pubkey(const struct lc_x509_certificate *cert,
			    const uint8_t **pk, size_t *pk_size,
			    enum lc_sig_types *key_type);

/**
 * @ingroup X509
 * @brief Get the extended key usage in human readable form
 *
 * The service function returns an array of EKU names the certificate contains
 *
 * @param [in] cert X.509 certificate from which the data is to be obtained
 * @param [out] eku_names Reference to an array of strings
 * @param [out] num_eku Number of returned EKU strings
 *
 * @return 0 on success or < 0 on error
 */
int lc_x509_cert_get_eku(const struct lc_x509_certificate *cert,
			 const char ***eku_names, unsigned int *num_eku);

/**
 * @ingroup X509
 * @brief Get the extended key usage in integer form
 *
 * @param [in] cert X.509 certificate from which the data is to be obtained
 * @param [out] val EKU value holding the LC_KEY_EKU_* flags
 *
 * @return 0 on success or < 0 on error
 */
int lc_x509_cert_get_eku_val(const struct lc_x509_certificate *cert,
			     uint16_t *val);

/**
 * @ingroup X509
 * @brief Get the key usage in human readable form
 *
 * The service function returns an array of key usage names the certificate
 * contains
 *
 * @param [in] cert X.509 certificate from which the data is to be obtained
 * @param [out] keyusage_names Reference to an array of strings
 * @param [out] num_keyusage Number of returned key usage strings
 *
 * @return 0 on success or < 0 on error
 */
int lc_x509_cert_get_keyusage(const struct lc_x509_certificate *cert,
			      const char ***keyusage_names,
			      unsigned int *num_keyusage);

/**
 * @ingroup X509
 * @brief Get the key usage in integer form
 *
 * @param [in] cert X.509 certificate from which the data is to be obtained
 * @param [out] val key usage value holding the LC_KEY_USAGE_* flags
 *
 * @return 0 on success or < 0 on error
 */
int lc_x509_cert_get_keyusage_val(const struct lc_x509_certificate *cert,
				  uint16_t *val);

/**
 * @ingroup X509
 * @brief Get the SAN DNS name
 *
 * \note The \p san_dns_name may *not* be NULL-terminated which implies that
 * this function returns also the size of the string. Yet, it is a human
 * readable string.
 *
 * \note The returned pointers have the same life time as \p cert.
 *
 * @param [in] cert X.509 certificate from which the data is to be obtained
 * @param [out] san_dns_name Reference to the SAN DNS name
 * @param [out] san_dns_len Length of the SAN DNS name
 *
 * @return 0 on success or < 0 on error
 */
int lc_x509_cert_get_san_dns(const struct lc_x509_certificate *cert,
			     const char **san_dns_name, size_t *san_dns_len);

/**
 * @ingroup X509
 * @brief Get the SAN IP value
 *
 * \note The returned pointers have the same life time as \p cert.
 *
 * @param [in] cert X.509 certificate from which the data is to be obtained
 * @param [out] san_ip Binary representation of IP address
 * @param [out] san_ip_len Length of the SAN IP address
 *
 * @return 0 on success or < 0 on error
 */
int lc_x509_cert_get_san_ip(const struct lc_x509_certificate *cert,
			    const uint8_t **san_ip, size_t *san_ip_len);

/**
 * @ingroup X509
 * @brief Helper to convert the binary IP address value into human-readable form
 *
 * @param [in] cert X.509 certificate from which the data is to be obtained
 * @param [in] ip Binary representation of IP address
 * @param [in] ip_len Length of the IP address
 * @param [out] ip_name Caller-provided buffer to fill with human-readable form
 * @param [in] ip_name_len Size of the ip_name buffer that can be filled
 *
 * @return 0 on success or < 0 on error
 */
int lc_x509_dec_san_ip(const struct lc_x509_certificate *cert,
		       const uint8_t *ip, size_t ip_len, char *ip_name,
		       size_t ip_name_len);

/**
 * @ingroup X509
 * @brief Get the SKID value
 *
 * \note The returned pointers have the same life time as \p cert.
 *
 * @param [in] cert X.509 certificate from which the data is to be obtained
 * @param [out] skid Binary representation of SKID
 * @param [out] skidlen length of the SKID buffer
 *
 * @return 0 on success or < 0 on error
 */
int lc_x509_cert_get_skid(const struct lc_x509_certificate *cert,
			  const uint8_t **skid, size_t *skidlen);

/**
 * @ingroup X509
 * @brief Get the AKID value
 *
 * \note The returned pointers have the same life time as \p cert.
 *
 * @param [in] cert X.509 certificate from which the data is to be obtained
 * @param [out] akid Binary representation of AKID
 * @param [out] akidlen length of the AKID buffer
 *
 * @return 0 on success or < 0 on error
 */
int lc_x509_cert_get_akid(const struct lc_x509_certificate *cert,
			  const uint8_t **akid, size_t *akidlen);

/**
 * @ingroup X509
 * @brief Get the valid-from data from the certificate
 *
 * The returned time data is an integer giving the data in seconds since EPOCH.
 *
 * @param [in] cert X.509 certificate from which the data is to be obtained
 * @param [out] time_since_epoch Time in seconds since EPOCH
 *
 * @return 0 on success or < 0 on error
 */
int lc_x509_cert_get_valid_from(const struct lc_x509_certificate *cert,
				time64_t *time_since_epoch);

/**
 * @ingroup X509
 * @brief Get the valid-to data from the certificate
 *
 * The returned time data is an integer giving the data in seconds since EPOCH.
 *
 * @param [in] cert X.509 certificate from which the data is to be obtained
 * @param [out] time_since_epoch Time in seconds since EPOCH
 *
 * @return 0 on success or < 0 on error
 */
int lc_x509_cert_get_valid_to(const struct lc_x509_certificate *cert,
			      time64_t *time_since_epoch);

/**
 * @ingroup X509
 * @brief Get the subject CN field from the certificate
 *
 * \note The returned pointers have the same life time as \p cert.
 *
 * \note The returned pointer may *not* be NULL-terminated which implies that
 * this function returns also the size of the string. Yet, it is a human
 * readable string.
 *
 * @param [in] cert X.509 certificate from which the data is to be obtained
 * @param [out] string Reference to data field
 * @param [out] string_len Length of the data field
 *
 * @return 0 on success or < 0 on error
 */
int lc_x509_cert_get_subject_cn(const struct lc_x509_certificate *cert,
				const char **string, size_t *string_len);

/**
 * @ingroup X509
 * @brief Get the subject email field from the certificate
 *
 * \note The returned pointers have the same life time as \p cert.
 *
 * \note The returned pointer may *not* be NULL-terminated which implies that
 * this function returns also the size of the string. Yet, it is a human
 * readable string.
 *
 * @param [in] cert X.509 certificate from which the data is to be obtained
 * @param [out] string Reference to data field
 * @param [out] string_len Length of the data field
 *
 * @return 0 on success or < 0 on error
 */
int lc_x509_cert_get_subject_email(const struct lc_x509_certificate *cert,
				   const char **string, size_t *string_len);

/**
 * @ingroup X509
 * @brief Get the subject OU field from the certificate
 *
 * \note The returned pointers have the same life time as \p cert.
 *
 * \note The returned pointer may *not* be NULL-terminated which implies that
 * this function returns also the size of the string. Yet, it is a human
 * readable string.
 *
 * @param [in] cert X.509 certificate from which the data is to be obtained
 * @param [out] string Reference to data field
 * @param [out] string_len Length of the data field
 *
 * @return 0 on success or < 0 on error
 */
int lc_x509_cert_get_subject_ou(const struct lc_x509_certificate *cert,
				const char **string, size_t *string_len);

/**
 * @ingroup X509
 * @brief Get the subject O field from the certificate
 *
 * \note The returned pointers have the same life time as \p cert.
 *
 * \note The returned pointer may *not* be NULL-terminated which implies that
 * this function returns also the size of the string. Yet, it is a human
 * readable string.
 *
 * @param [in] cert X.509 certificate from which the data is to be obtained
 * @param [out] string Reference to data field
 * @param [out] string_len Length of the data field
 *
 * @return 0 on success or < 0 on error
 */
int lc_x509_cert_get_subject_o(const struct lc_x509_certificate *cert,
			       const char **string, size_t *string_len);

/**
 * @ingroup X509
 * @brief Get the subject ST field from the certificate
 *
 * \note The returned pointers have the same life time as \p cert.
 *
 * \note The returned pointer may *not* be NULL-terminated which implies that
 * this function returns also the size of the string. Yet, it is a human
 * readable string.
 *
 * @param [in] cert X.509 certificate from which the data is to be obtained
 * @param [out] string Reference to data field
 * @param [out] string_len Length of the data field
 *
 * @return 0 on success or < 0 on error
 */
int lc_x509_cert_get_subject_st(const struct lc_x509_certificate *cert,
				const char **string, size_t *string_len);

/**
 * @ingroup X509
 * @brief Get the subject C field from the certificate
 *
 * \note The returned pointers have the same life time as \p cert.
 *
 * \note The returned pointer may *not* be NULL-terminated which implies that
 * this function returns also the size of the string. Yet, it is a human
 * readable string.
 *
 * @param [in] cert X.509 certificate from which the data is to be obtained
 * @param [out] string Reference to data field
 * @param [out] string_len Length of the data field
 *
 * @return 0 on success or < 0 on error
 */
int lc_x509_cert_get_subject_c(const struct lc_x509_certificate *cert,
			       const char **string, size_t *string_len);

/**
 * @ingroup X509
 * @brief Get the issuer CN field from the certificate
 *
 * \note The returned pointers have the same life time as \p cert.
 *
 * \note The returned pointer may *not* be NULL-terminated which implies that
 * this function returns also the size of the string. Yet, it is a human
 * readable string.
 *
 * @param [in] cert X.509 certificate from which the data is to be obtained
 * @param [out] string Reference to data field
 * @param [out] string_len Length of the data field
 *
 * @return 0 on success or < 0 on error
 */
int lc_x509_cert_get_issuer_cn(const struct lc_x509_certificate *cert,
			       const char **string, size_t *string_len);

/**
 * @ingroup X509
 * @brief Get the issuer email field from the certificate
 *
 * \note The returned pointers have the same life time as \p cert.
 *
 * \note The returned pointer may *not* be NULL-terminated which implies that
 * this function returns also the size of the string. Yet, it is a human
 * readable string.
 *
 * @param [in] cert X.509 certificate from which the data is to be obtained
 * @param [out] string Reference to data field
 * @param [out] string_len Length of the data field
 *
 * @return 0 on success or < 0 on error
 */
int lc_x509_cert_get_issuer_email(const struct lc_x509_certificate *cert,
				  const char **string, size_t *string_len);

/**
 * @ingroup X509
 * @brief Get the issuer OU field from the certificate
 *
 * \note The returned pointers have the same life time as \p cert.
 *
 * \note The returned pointer may *not* be NULL-terminated which implies that
 * this function returns also the size of the string. Yet, it is a human
 * readable string.
 *
 * @param [in] cert X.509 certificate from which the data is to be obtained
 * @param [out] string Reference to data field
 * @param [out] string_len Length of the data field
 *
 * @return 0 on success or < 0 on error
 */
int lc_x509_cert_get_issuer_ou(const struct lc_x509_certificate *cert,
			       const char **string, size_t *string_len);

/**
 * @ingroup X509
 * @brief Get the issuer O field from the certificate
 *
 * \note The returned pointers have the same life time as \p cert.
 *
 * \note The returned pointer may *not* be NULL-terminated which implies that
 * this function returns also the size of the string. Yet, it is a human
 * readable string.
 *
 * @param [in] cert X.509 certificate from which the data is to be obtained
 * @param [out] string Reference to data field
 * @param [out] string_len Length of the data field
 *
 * @return 0 on success or < 0 on error
 */
int lc_x509_cert_get_issuer_o(const struct lc_x509_certificate *cert,
			      const char **string, size_t *string_len);

/**
 * @ingroup X509
 * @brief Get the issuer ST field from the certificate
 *
 * \note The returned pointers have the same life time as \p cert.
 *
 * \note The returned pointer may *not* be NULL-terminated which implies that
 * this function returns also the size of the string. Yet, it is a human
 * readable string.
 *
 * @param [in] cert X.509 certificate from which the data is to be obtained
 * @param [out] string Reference to data field
 * @param [out] string_len Length of the data field
 *
 * @return 0 on success or < 0 on error
 */
int lc_x509_cert_get_issuer_st(const struct lc_x509_certificate *cert,
			       const char **string, size_t *string_len);

/**
 * @ingroup X509
 * @brief Get the issuer C field from the certificate
 *
 * \note The returned pointers have the same life time as \p cert.
 *
 * \note The returned pointer may *not* be NULL-terminated which implies that
 * this function returns also the size of the string. Yet, it is a human
 * readable string.
 *
 * @param [in] cert X.509 certificate from which the data is to be obtained
 * @param [out] string Reference to data field
 * @param [out] string_len Length of the data field
 *
 * @return 0 on success or < 0 on error
 */
int lc_x509_cert_get_issuer_c(const struct lc_x509_certificate *cert,
			      const char **string, size_t *string_len);

/**
 * @ingroup X509
 * @brief Get the serial number of the certificate
 *
 * \note The returned pointers have the same life time as \p cert.
 *
 * @param [in] cert X.509 certificate from which the data is to be obtained
 * @param [out] serial Binary representation of serial number
 * @param [out] serial_len Length of the serial number
 *
 * @return 0 on success or < 0 on error
 */
int lc_x509_cert_get_serial(const struct lc_x509_certificate *cert,
			    const uint8_t **serial, size_t *serial_len);

/******************************************************************************
 * X.509 Certificate policy service functions
 ******************************************************************************/

/** X.509 Policy checks: returns True or False, or a POSIX error */
typedef int x509_pol_ret_t /* __attribute__((warn_unused_result)) */;

/** X.509 Policy checks: "True" result */
#define LC_X509_POL_TRUE 1

/** X.509 Policy checks: "False" result */
#define LC_X509_POL_FALSE 0

/**
 * @ingroup X509
 * @brief Is the given certificate a CA certificate (root or intermediate)?
 *
 * @param [in] cert Reference to the certificate
 *
 * @return < 0 on error, LC_X509_POL_TRUE or LC_X509_POL_FALSE
 */
x509_pol_ret_t lc_x509_policy_is_ca(const struct lc_x509_certificate *cert);

/**
 * @ingroup X509
 * @brief Can the given certificate validate CRLs?
 *
 * @param [in] cert Reference to the certificate
 *
 * @return < 0 on error, LC_X509_POL_TRUE or LC_X509_POL_FALSE
 */
x509_pol_ret_t
lc_x509_policy_can_validate_crls(const struct lc_x509_certificate *cert);

/**
 * @ingroup X509
 * @brief Is the given certificate a self-signed certificate?
 *
 * @param [in] cert Reference to the certificate
 *
 * @return < 0 on error, LC_X509_POL_TRUE or LC_X509_POL_FALSE
 */

x509_pol_ret_t
lc_x509_policy_is_selfsigned(const struct lc_x509_certificate *cert);

/**
 * @ingroup X509
 * @brief Is the given certificate a root CA certificate?
 *
 * @param [in] cert Reference to the certificate
 *
 * @return < 0 on error, LC_X509_POL_TRUE or LC_X509_POL_FALSE
 */
x509_pol_ret_t
lc_x509_policy_is_root_ca(const struct lc_x509_certificate *cert);

/**
 * @ingroup X509
 * @brief Does the given AKID match the certificate AKID?
 *
 * @param [in] cert Reference to the certificate
 * @param [in] reference_akid AKID in binary format to be matched
 * @param [in] reference_akid_len length of AKID binary buffer
 *
 * @return < 0 on error, LC_X509_POL_TRUE or LC_X509_POL_FALSE
 */
x509_pol_ret_t lc_x509_policy_match_akid(const struct lc_x509_certificate *cert,
					 const uint8_t *reference_akid,
					 size_t reference_akid_len);

/**
 * @ingroup X509
 * @brief Does the given SKID match the certificate SKID?
 *
 * @param [in] cert Reference to the certificate
 * @param [in] reference_skid SKID in binary format to be matched
 * @param [in] reference_skid_len length of SKID binary buffer
 *
 * @return < 0 on error, LC_X509_POL_TRUE or LC_X509_POL_FALSE
 */
x509_pol_ret_t lc_x509_policy_match_skid(const struct lc_x509_certificate *cert,
					 const uint8_t *reference_skid,
					 size_t reference_skid_len);

/**
 * @ingroup X509
 * @brief Check if set of required key usage flags are present
 *
 * @param [in] cert Reference to the certificate
 * @param [in] required_key_usage flags field with the bits set that the
 *				  certificate must contain
 *
 * @return < 0 on error, LC_X509_POL_TRUE or LC_X509_POL_FALSE
 */
x509_pol_ret_t
lc_x509_policy_match_key_usage(const struct lc_x509_certificate *cert,
			       uint16_t required_key_usage);

/**
 * @ingroup X509
 * @brief Check if set of required extended key usage flags are present
 *
 * @param [in] cert Reference to the certificate
 * @param [in] required_eku flags field with the bits set that the certificate
 *			    must contain
 *
 * @return < 0 on error, LC_X509_POL_TRUE or LC_X509_POL_FALSE
 */
x509_pol_ret_t
lc_x509_policy_match_extended_key_usage(const struct lc_x509_certificate *cert,
					uint16_t required_eku);

/**
 * @ingroup X509
 * @brief Check if the given time falls within the range of the certificate
 * validity time.
 *
 * @param [in] cert Reference to the certificate
 * @param [in] current_time Time value to verify - this time is given in seconds
 *			    since EPOCH, e.g. by the POSIX service function
 *			    `time`.
 *
 * @return < 0 on error, LC_X509_POL_TRUE or LC_X509_POL_FALSE
 */

x509_pol_ret_t lc_x509_policy_time_valid(const struct lc_x509_certificate *cert,
					 time64_t current_time);

/**
 * @ingroup X509
 * @brief Check if certificate is valid
 *
 * This check validates all RFC5280 constraints for a conforming certificate.
 *
 * @param [in] cert Reference to the certificate
 *
 * @return < 0 on error, LC_X509_POL_TRUE or LC_X509_POL_FALSE
 */
x509_pol_ret_t
lc_x509_policy_cert_valid(const struct lc_x509_certificate *cert);

/**
 * @brief X509
 * @brief Verification of an X.509 certificate against a public key
 *
 * This function performs the signature verification of the signature associated
 * with an X.509 certificate against the public key provided by the caller.
 * In addition, it performs all validity checks required as part of the
 * verification operation, including the validity time enforcement. Only if all
 * checks pass, the certificate is considered to be validated.
 *
 * @param [in] pkey Public key to check the certificate against
 * @param [in] cert Reference to the certificate to be validated
 * @param [in] flags Flags for the verification process (currently unused)
 *
 * @return 0 on success, < 0 on error
 */
int lc_x509_policy_cert_verify(const struct lc_public_key *pkey,
			       const struct lc_x509_certificate *cert,
			       uint64_t flags);

#ifdef __cplusplus
}
#endif

#endif /* LC_X509_PARSER_H */
