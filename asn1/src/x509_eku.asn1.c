
/*
 * Automatically generated by asn1_compiler.  Do not edit
 *
 * ASN.1 parser for x509_eku
 */
#include "asn1_ber_bytecode.h"
#include "x509_eku.asn1.h"

// clang-format off

enum x509_eku_actions {
	ACT_x509_eku = 0,
	NR__x509_eku_actions = 1
};

static const asn1_action_t x509_eku_action_table[NR__x509_eku_actions] = {
	[   0] = x509_eku,
};

static const asn1_action_enc_t x509_eku_action_table_enc[NR__x509_eku_actions] = {
	[   0] = x509_eku_enc,
};

static const unsigned char x509_eku_machine[] = {
	// ExtKeyUsageSyntax
	[   0] = ASN1_OP_MATCH,
	[   1] = _tag(UNIV, CONS, SEQ),
	// KeyPurposeId
	[   2] =  ASN1_OP_MATCH_ACT,
	[   3] =  _tag(UNIV, PRIM, OID),
	[   4] =  _action(ACT_x509_eku),
	[   5] = ASN1_OP_END_SEQ_OF,
	[   6] = _jump_target(2),
	[   7] = ASN1_OP_COMPLETE,
};

const struct asn1_decoder x509_eku_decoder = {
	.machine = x509_eku_machine,
	.machlen = sizeof(x509_eku_machine),
	.actions = x509_eku_action_table,
};

const struct asn1_encoder x509_eku_encoder = {
	.machine = x509_eku_machine,
	.machlen = sizeof(x509_eku_machine),
	.actions = x509_eku_action_table_enc,
};

// clang-format on
