/*
 * Automatically generated by asn1_compiler.  Do not edit
 *
 * ASN.1 parser for x509
 */
#pragma once
#include "asn1_encoder.h"
#include "asn1_decoder.h"

// clang-format off
extern const struct asn1_encoder x509_encoder;
extern const struct asn1_decoder x509_decoder;

extern int x509_attribute_value_continue_enc(void *, uint8_t *, size_t *);
extern int x509_attribute_value_continue(void *, size_t, unsigned char, const uint8_t *, size_t);
extern int x509_extension_OID_enc(void *, uint8_t *, size_t *);
extern int x509_extension_OID(void *, size_t, unsigned char, const uint8_t *, size_t);
extern int x509_extension_continue_enc(void *, uint8_t *, size_t *);
extern int x509_extension_continue(void *, size_t, unsigned char, const uint8_t *, size_t);
extern int x509_extension_critical_enc(void *, uint8_t *, size_t *);
extern int x509_extension_critical(void *, size_t, unsigned char, const uint8_t *, size_t);
extern int x509_extract_attribute_name_segment_enc(void *, uint8_t *, size_t *);
extern int x509_extract_attribute_name_segment(void *, size_t, unsigned char, const uint8_t *, size_t);
extern int x509_extract_key_data_enc(void *, uint8_t *, size_t *);
extern int x509_extract_key_data(void *, size_t, unsigned char, const uint8_t *, size_t);
extern int x509_note_algorithm_OID_enc(void *, uint8_t *, size_t *);
extern int x509_note_algorithm_OID(void *, size_t, unsigned char, const uint8_t *, size_t);
extern int x509_note_attribute_type_OID_enc(void *, uint8_t *, size_t *);
extern int x509_note_attribute_type_OID(void *, size_t, unsigned char, const uint8_t *, size_t);
extern int x509_note_issuer_enc(void *, uint8_t *, size_t *);
extern int x509_note_issuer(void *, size_t, unsigned char, const uint8_t *, size_t);
extern int x509_note_not_after_enc(void *, uint8_t *, size_t *);
extern int x509_note_not_after(void *, size_t, unsigned char, const uint8_t *, size_t);
extern int x509_note_not_before_enc(void *, uint8_t *, size_t *);
extern int x509_note_not_before(void *, size_t, unsigned char, const uint8_t *, size_t);
extern int x509_note_params_enc(void *, uint8_t *, size_t *);
extern int x509_note_params(void *, size_t, unsigned char, const uint8_t *, size_t);
extern int x509_note_serial_enc(void *, uint8_t *, size_t *);
extern int x509_note_serial(void *, size_t, unsigned char, const uint8_t *, size_t);
extern int x509_note_sig_algo_enc(void *, uint8_t *, size_t *);
extern int x509_note_sig_algo(void *, size_t, unsigned char, const uint8_t *, size_t);
extern int x509_note_signature_enc(void *, uint8_t *, size_t *);
extern int x509_note_signature(void *, size_t, unsigned char, const uint8_t *, size_t);
extern int x509_note_subject_enc(void *, uint8_t *, size_t *);
extern int x509_note_subject(void *, size_t, unsigned char, const uint8_t *, size_t);
extern int x509_note_tbs_certificate_enc(void *, uint8_t *, size_t *);
extern int x509_note_tbs_certificate(void *, size_t, unsigned char, const uint8_t *, size_t);
extern int x509_process_extension_enc(void *, uint8_t *, size_t *);
extern int x509_process_extension(void *, size_t, unsigned char, const uint8_t *, size_t);
extern int x509_set_gen_time_enc(void *, uint8_t *, size_t *);
extern int x509_set_gen_time(void *, size_t, unsigned char, const uint8_t *, size_t);
extern int x509_set_uct_time_enc(void *, uint8_t *, size_t *);
extern int x509_set_uct_time(void *, size_t, unsigned char, const uint8_t *, size_t);
extern int x509_signature_algorithm_enc(void *, uint8_t *, size_t *);
extern int x509_signature_algorithm(void *, size_t, unsigned char, const uint8_t *, size_t);
extern int x509_version_enc(void *, uint8_t *, size_t *);
extern int x509_version(void *, size_t, unsigned char, const uint8_t *, size_t);
// clang-format on
