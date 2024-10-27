/*
 * Automatically generated by asn1_compiler.  Do not edit
 *
 * ASN.1 parser for x509_skid
 */
#include "asn1_ber_bytecode.h"
#include "x509_skid.asn1.h"

enum x509_skid_actions { ACT_x509_skid = 0, NR__x509_skid_actions = 1 };

static const asn1_action_t x509_skid_action_table[NR__x509_skid_actions] = {
	[0] = x509_skid,
};

static const unsigned char x509_skid_machine[] = {
	// SubjectKeyIdentifier
	// KeyIdentifier
	[0] = ASN1_OP_MATCH_ACT,
	[1] = _tag(UNIV, PRIM, OTS),
	[2] = _action(ACT_x509_skid),
	[3] = ASN1_OP_COMPLETE,
};

const struct asn1_decoder x509_skid_decoder = {
	.machine = x509_skid_machine,
	.machlen = sizeof(x509_skid_machine),
	.actions = x509_skid_action_table,
};
