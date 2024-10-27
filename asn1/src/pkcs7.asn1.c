/*
 * Automatically generated by asn1_compiler.  Do not edit
 *
 * ASN.1 parser for pkcs7
 */
#include "asn1_ber_bytecode.h"
#include "pkcs7.asn1.h"

enum pkcs7_actions {
	ACT_pkcs7_check_content_type = 0,
	ACT_pkcs7_extract_cert = 1,
	ACT_pkcs7_note_OID = 2,
	ACT_pkcs7_note_certificate_list = 3,
	ACT_pkcs7_note_content = 4,
	ACT_pkcs7_note_data = 5,
	ACT_pkcs7_note_signed_info = 6,
	ACT_pkcs7_note_signeddata_version = 7,
	ACT_pkcs7_note_signerinfo_version = 8,
	ACT_pkcs7_sig_note_authenticated_attr = 9,
	ACT_pkcs7_sig_note_digest_algo = 10,
	ACT_pkcs7_sig_note_issuer = 11,
	ACT_pkcs7_sig_note_pkey_algo = 12,
	ACT_pkcs7_sig_note_serial = 13,
	ACT_pkcs7_sig_note_set_of_authattrs = 14,
	ACT_pkcs7_sig_note_signature = 15,
	ACT_pkcs7_sig_note_skid = 16,
	NR__pkcs7_actions = 17
};

static const asn1_action_t pkcs7_action_table[NR__pkcs7_actions] = {
	[   0] = pkcs7_check_content_type,
	[   1] = pkcs7_extract_cert,
	[   2] = pkcs7_note_OID,
	[   3] = pkcs7_note_certificate_list,
	[   4] = pkcs7_note_content,
	[   5] = pkcs7_note_data,
	[   6] = pkcs7_note_signed_info,
	[   7] = pkcs7_note_signeddata_version,
	[   8] = pkcs7_note_signerinfo_version,
	[   9] = pkcs7_sig_note_authenticated_attr,
	[  10] = pkcs7_sig_note_digest_algo,
	[  11] = pkcs7_sig_note_issuer,
	[  12] = pkcs7_sig_note_pkey_algo,
	[  13] = pkcs7_sig_note_serial,
	[  14] = pkcs7_sig_note_set_of_authattrs,
	[  15] = pkcs7_sig_note_signature,
	[  16] = pkcs7_sig_note_skid,
};

static const unsigned char pkcs7_machine[] = {
	// PKCS7ContentInfo
	[   0] = ASN1_OP_MATCH,
	[   1] = _tag(UNIV, CONS, SEQ),
	// ContentType
	[   2] =  ASN1_OP_MATCH_ACT,
	[   3] =  _tag(UNIV, PRIM, OID),
	[   4] =  _action(ACT_pkcs7_note_OID),
	[   5] =  ASN1_OP_ACT,
	[   6] =  _action(ACT_pkcs7_check_content_type),
	[   7] =  ASN1_OP_MATCH_JUMP_OR_SKIP,		// content
	[   8] =  _tagn(CONT, CONS,  0),
	[   9] =  _jump_target(12),
	[  10] = ASN1_OP_END_SEQ,
	[  11] = ASN1_OP_COMPLETE,

	// SignedData
	[  12] =  ASN1_OP_MATCH,
	[  13] =  _tag(UNIV, CONS, SEQ),
	[  14] =   ASN1_OP_MATCH_ACT,		// version
	[  15] =   _tag(UNIV, PRIM, INT),
	[  16] =   _action(ACT_pkcs7_note_signeddata_version),
	// DigestAlgorithmIdentifiers
	[  17] =   ASN1_OP_MATCH_JUMP_OR_SKIP,		// daSet
	[  18] =   _tag(UNIV, CONS, SET),
	[  19] =   _jump_target(61),
	[  20] =   ASN1_OP_COND_MATCH_JUMP_OR_SKIP,		// daSequence
	[  21] =   _tag(UNIV, CONS, SEQ),
	[  22] =   _jump_target(67),
	[  23] =   ASN1_OP_COND_FAIL,
	// ContentInfo
	[  24] =   ASN1_OP_MATCH,
	[  25] =   _tag(UNIV, CONS, SEQ),
	// ContentType
	[  26] =    ASN1_OP_MATCH_ACT,
	[  27] =    _tag(UNIV, PRIM, OID),
	[  28] =    _action(ACT_pkcs7_note_OID),
	[  29] =    ASN1_OP_ACT,
	[  30] =    _action(ACT_pkcs7_note_OID),
	[  31] =    ASN1_OP_MATCH_JUMP_OR_SKIP,		// content
	[  32] =    _tagn(CONT, CONS,  0),
	[  33] =    _jump_target(73),
	[  34] =   ASN1_OP_END_SEQ,
	[  35] =   ASN1_OP_ACT,
	[  36] =   _action(ACT_pkcs7_note_content),
	// ExtendedCertificatesAndCertificates
	[  37] =   ASN1_OP_MATCH_JUMP_OR_SKIP,		// certSet
	[  38] =   _tagn(CONT, CONS,  0),
	[  39] =   _jump_target(77),		// --> ExtendedCertificatesAndCertificates
	// Certificates
	[  40] =   ASN1_OP_COND_MATCH_JUMP_OR_SKIP,		// certSequence
	[  41] =   _tagn(CONT, CONS,  2),
	[  42] =   _jump_target(85),		// --> Certificates
	[  43] =   ASN1_OP_ACT,
	[  44] =   _action(ACT_pkcs7_note_certificate_list),
	// CertificateRevocationLists
	[  45] =   ASN1_OP_MATCH_JUMP_OR_SKIP,		// crlSet
	[  46] =   _tagn(CONT, CONS,  1),
	[  47] =   _jump_target(90),		// --> CertificateRevocationLists
	// CRLSequence
	[  48] =   ASN1_OP_COND_MATCH_JUMP_OR_SKIP,		// crlSequence
	[  49] =   _tagn(CONT, CONS,  3),
	[  50] =   _jump_target(96),		// --> CRLSequence
	// SignerInfos
	[  51] =   ASN1_OP_MATCH_JUMP_OR_SKIP,		// siSet
	[  52] =   _tag(UNIV, CONS, SET),
	[  53] =   _jump_target(102),
	[  54] =   ASN1_OP_COND_MATCH_JUMP_OR_SKIP,		// siSequence
	[  55] =   _tag(UNIV, CONS, SEQ),
	[  56] =   _jump_target(108),
	[  57] =   ASN1_OP_COND_FAIL,
	[  58] =  ASN1_OP_END_SEQ,
	[  59] = ASN1_OP_END_SEQ,
	[  60] = ASN1_OP_RETURN,

	// DigestAlgorithmIdentifier
	[  61] =  ASN1_OP_MATCH_JUMP,
	[  62] =  _tag(UNIV, CONS, SEQ),
	[  63] =  _jump_target(114),		// --> DigestAlgorithmIdentifier
	[  64] = ASN1_OP_END_SET_OF,
	[  65] = _jump_target(61),
	[  66] = ASN1_OP_RETURN,

	// DigestAlgorithmIdentifier
	[  67] =  ASN1_OP_MATCH_JUMP,
	[  68] =  _tag(UNIV, CONS, SEQ),
	[  69] =  _jump_target(114),		// --> DigestAlgorithmIdentifier
	[  70] = ASN1_OP_END_SEQ_OF,
	[  71] = _jump_target(67),
	[  72] = ASN1_OP_RETURN,

	// Data
	[  73] =  ASN1_OP_MATCH_ANY_ACT,
	[  74] =  _action(ACT_pkcs7_note_data),
	[  75] = ASN1_OP_END_SEQ,
	[  76] = ASN1_OP_RETURN,

	// ExtendedCertificateOrCertificate
	// Certificate
	[  77] =  ASN1_OP_MATCH_ANY_ACT_OR_SKIP,
	[  78] =  _action(ACT_pkcs7_extract_cert),
	// ExtendedCertificate
	// Certificate
	[  79] =  ASN1_OP_COND_MATCH_ANY_ACT_OR_SKIP,
	[  80] =  _action(ACT_pkcs7_extract_cert),
	[  81] =  ASN1_OP_COND_FAIL,
	[  82] = ASN1_OP_END_SET_OF,
	[  83] = _jump_target(77),
	[  84] = ASN1_OP_RETURN,

	// Certificate
	[  85] =  ASN1_OP_MATCH_ANY_ACT,
	[  86] =  _action(ACT_pkcs7_extract_cert),
	[  87] = ASN1_OP_END_SEQ_OF,
	[  88] = _jump_target(85),
	[  89] = ASN1_OP_RETURN,

	// CertificateList
	[  90] =  ASN1_OP_MATCH_JUMP,
	[  91] =  _tag(UNIV, CONS, SEQ),
	[  92] =  _jump_target(120),		// --> CertificateList
	[  93] = ASN1_OP_END_SET_OF,
	[  94] = _jump_target(90),
	[  95] = ASN1_OP_RETURN,

	// CertificateList
	[  96] =  ASN1_OP_MATCH_JUMP,
	[  97] =  _tag(UNIV, CONS, SEQ),
	[  98] =  _jump_target(120),		// --> CertificateList
	[  99] = ASN1_OP_END_SEQ_OF,
	[ 100] = _jump_target(96),
	[ 101] = ASN1_OP_RETURN,

	// SignerInfo
	[ 102] =  ASN1_OP_MATCH_JUMP,
	[ 103] =  _tag(UNIV, CONS, SEQ),
	[ 104] =  _jump_target(125),		// --> SignerInfo
	[ 105] = ASN1_OP_END_SET_OF,
	[ 106] = _jump_target(102),
	[ 107] = ASN1_OP_RETURN,

	// SignerInfo
	[ 108] =  ASN1_OP_MATCH_JUMP,
	[ 109] =  _tag(UNIV, CONS, SEQ),
	[ 110] =  _jump_target(125),		// --> SignerInfo
	[ 111] = ASN1_OP_END_SEQ_OF,
	[ 112] = _jump_target(108),
	[ 113] = ASN1_OP_RETURN,

	[ 114] =  ASN1_OP_MATCH_ACT,		// algorithm
	[ 115] =  _tag(UNIV, PRIM, OID),
	[ 116] =  _action(ACT_pkcs7_note_OID),
	[ 117] =  ASN1_OP_MATCH_ANY_OR_SKIP,		// parameters
	[ 118] = ASN1_OP_END_SEQ,
	[ 119] = ASN1_OP_RETURN,

	// Certificate
	[ 120] =  ASN1_OP_MATCH_ANY_ACT,
	[ 121] =  _action(ACT_pkcs7_extract_cert),
	[ 122] = ASN1_OP_END_SEQ_OF,
	[ 123] = _jump_target(120),
	[ 124] = ASN1_OP_RETURN,

	[ 125] =  ASN1_OP_MATCH_ACT,		// version
	[ 126] =  _tag(UNIV, PRIM, INT),
	[ 127] =  _action(ACT_pkcs7_note_signerinfo_version),
	// SignerIdentifier
	// IssuerAndSerialNumber
	[ 128] =  ASN1_OP_MATCH_JUMP_OR_SKIP,		// issuerAndSerialNumber
	[ 129] =  _tag(UNIV, CONS, SEQ),
	[ 130] =  _jump_target(169),		// --> IssuerAndSerialNumber
	// SubjectKeyIdentifier
	[ 131] =  ASN1_OP_COND_MATCH_ACT_OR_SKIP,		// subjectKeyIdentifier
	[ 132] =  _tagn(CONT, PRIM,  0),
	[ 133] =  _action(ACT_pkcs7_sig_note_skid),
	[ 134] =  ASN1_OP_COND_FAIL,
	// DigestAlgorithmIdentifier
	[ 135] =  ASN1_OP_MATCH_JUMP,
	[ 136] =  _tag(UNIV, CONS, SEQ),
	[ 137] =  _jump_target(114),		// --> DigestAlgorithmIdentifier
	[ 138] =  ASN1_OP_ACT,
	[ 139] =  _action(ACT_pkcs7_sig_note_digest_algo),
	// SetOfAuthenticatedAttribute
	[ 140] =  ASN1_OP_MATCH_JUMP_OR_SKIP,		// aaSet
	[ 141] =  _tagn(CONT, CONS,  0),
	[ 142] =  _jump_target(192),		// --> SetOfAuthenticatedAttribute
	[ 143] =  ASN1_OP_MAYBE_ACT,
	[ 144] =  _action(ACT_pkcs7_sig_note_set_of_authattrs),
	[ 145] =  ASN1_OP_COND_MATCH_JUMP_OR_SKIP,		// aaSequence
	[ 146] =  _tagn(CONT, CONS,  2),
	[ 147] =  _jump_target(198),
	// DigestEncryptionAlgorithmIdentifier
	[ 148] =  ASN1_OP_MATCH,
	[ 149] =  _tag(UNIV, CONS, SEQ),
	[ 150] =   ASN1_OP_MATCH_ACT,		// algorithm
	[ 151] =   _tag(UNIV, PRIM, OID),
	[ 152] =   _action(ACT_pkcs7_note_OID),
	[ 153] =   ASN1_OP_MATCH_ANY_OR_SKIP,		// parameters
	[ 154] =  ASN1_OP_END_SEQ,
	[ 155] =  ASN1_OP_ACT,
	[ 156] =  _action(ACT_pkcs7_sig_note_pkey_algo),
	// EncryptedDigest
	[ 157] =  ASN1_OP_MATCH_ACT,
	[ 158] =  _tag(UNIV, PRIM, OTS),
	[ 159] =  _action(ACT_pkcs7_sig_note_signature),
	[ 160] =  ASN1_OP_MATCH_JUMP_OR_SKIP,		// uaSet
	[ 161] =  _tagn(CONT, CONS,  1),
	[ 162] =  _jump_target(207),
	[ 163] =  ASN1_OP_COND_MATCH_JUMP_OR_SKIP,		// uaSequence
	[ 164] =  _tagn(CONT, CONS,  3),
	[ 165] =  _jump_target(213),
	[ 166] = ASN1_OP_END_SEQ_ACT,
	[ 167] = _action(ACT_pkcs7_note_signed_info),
	[ 168] = ASN1_OP_RETURN,

	// Name
	[ 169] =  ASN1_OP_MATCH,
	[ 170] =  _tag(UNIV, CONS, SEQ),
	// RelativeDistinguishedName
	[ 171] =   ASN1_OP_MATCH,
	[ 172] =   _tag(UNIV, CONS, SET),
	// AttributeValueAssertion
	[ 173] =    ASN1_OP_MATCH,
	[ 174] =    _tag(UNIV, CONS, SEQ),
	[ 175] =     ASN1_OP_MATCH_ACT,		// attributeType
	[ 176] =     _tag(UNIV, PRIM, OID),
	[ 177] =     _action(ACT_pkcs7_note_OID),
	[ 178] =     ASN1_OP_MATCH_ANY,		// attributeValue
	[ 179] =    ASN1_OP_END_SEQ,
	[ 180] =   ASN1_OP_END_SET_OF,
	[ 181] =   _jump_target(173),
	[ 182] =  ASN1_OP_END_SEQ_OF,
	[ 183] =  _jump_target(171),
	[ 184] =  ASN1_OP_ACT,
	[ 185] =  _action(ACT_pkcs7_sig_note_issuer),
	// CertificateSerialNumber
	[ 186] =  ASN1_OP_MATCH,
	[ 187] =  _tag(UNIV, PRIM, INT),
	[ 188] =  ASN1_OP_ACT,
	[ 189] =  _action(ACT_pkcs7_sig_note_serial),
	[ 190] = ASN1_OP_END_SEQ,
	[ 191] = ASN1_OP_RETURN,

	// AuthenticatedAttribute
	[ 192] =  ASN1_OP_MATCH_JUMP,
	[ 193] =  _tag(UNIV, CONS, SEQ),
	[ 194] =  _jump_target(219),		// --> AuthenticatedAttribute
	[ 195] = ASN1_OP_END_SET_OF,
	[ 196] = _jump_target(192),
	[ 197] = ASN1_OP_RETURN,

	[ 198] =  ASN1_OP_MATCH,		// aaSequence
	[ 199] =  _tag(UNIV, CONS, SEQ),
	// AuthenticatedAttribute
	[ 200] =   ASN1_OP_MATCH_JUMP,
	[ 201] =   _tag(UNIV, CONS, SEQ),
	[ 202] =   _jump_target(219),		// --> AuthenticatedAttribute
	[ 203] =  ASN1_OP_END_SEQ_OF,
	[ 204] =  _jump_target(200),
	[ 205] = ASN1_OP_END_SEQ,
	[ 206] = ASN1_OP_RETURN,

	// UnauthenticatedAttribute
	[ 207] =  ASN1_OP_MATCH_JUMP,
	[ 208] =  _tag(UNIV, CONS, SEQ),
	[ 209] =  _jump_target(230),		// --> UnauthenticatedAttribute
	[ 210] = ASN1_OP_END_SET_OF,
	[ 211] = _jump_target(207),
	[ 212] = ASN1_OP_RETURN,

	// UnauthenticatedAttribute
	[ 213] =  ASN1_OP_MATCH_JUMP,
	[ 214] =  _tag(UNIV, CONS, SEQ),
	[ 215] =  _jump_target(230),		// --> UnauthenticatedAttribute
	[ 216] = ASN1_OP_END_SEQ_OF,
	[ 217] = _jump_target(213),
	[ 218] = ASN1_OP_RETURN,

	[ 219] =  ASN1_OP_MATCH_ACT,		// type
	[ 220] =  _tag(UNIV, PRIM, OID),
	[ 221] =  _action(ACT_pkcs7_note_OID),
	[ 222] =  ASN1_OP_MATCH,		// values
	[ 223] =  _tag(UNIV, CONS, SET),
	[ 224] =   ASN1_OP_MATCH_ANY_ACT,
	[ 225] =   _action(ACT_pkcs7_sig_note_authenticated_attr),
	[ 226] =  ASN1_OP_END_SET_OF,
	[ 227] =  _jump_target(224),
	[ 228] = ASN1_OP_END_SEQ,
	[ 229] = ASN1_OP_RETURN,

	[ 230] =  ASN1_OP_MATCH,		// type
	[ 231] =  _tag(UNIV, PRIM, OID),
	[ 232] =  ASN1_OP_MATCH,		// values
	[ 233] =  _tag(UNIV, CONS, SET),
	[ 234] =   ASN1_OP_MATCH_ANY,
	[ 235] =  ASN1_OP_END_SET_OF,
	[ 236] =  _jump_target(234),
	[ 237] = ASN1_OP_END_SEQ,
	[ 238] = ASN1_OP_RETURN,
};

const struct asn1_decoder pkcs7_decoder = {
	.machine = pkcs7_machine,
	.machlen = sizeof(pkcs7_machine),
	.actions = pkcs7_action_table,
};
