// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2024 NXP
 */

#include <err.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* OP-TEE TEE client API (built by optee_client) */
#include <tee_client_api.h>

/* For the UUID (found in the TA's h-file(s)) */
#include <hse_embed_handle_ta.h>
#include <pta_hse_kp.h>

/* HSE Key groups are part of the NVM Catalog */
#define HSE_AES_KEY_GROUP	8
#define HSE_AES_KEY_SLOT	1

#define HSE_RSA_PAIR_GROUP	6
#define HSE_RSA_PAIR_SLOT	0

#define HSE_RSA_PUB_GROUP	5
#define HSE_RSA_PUB_SLOT	0

#define HSE_ECC_PAIR_GROUP	3
#define HSE_ECC_PAIR_SLOT	0

#define HSE_ECC_PUB_GROUP	4
#define HSE_ECC_PUB_SLOT	0

#define PROVISION_KEYS_NUM	3

#define KEY_INFO_INIT(group, slot, type, flags) \
	{ \
		.key_handle = (GET_KEY_HANDLE(HSE_KEY_CATALOG_ID_NVM, \
					     (group), (slot))), \
		.key_type = (type), \
		.key_flags = (flags), \
	}

/* 2048-bit RSA Keys */
static uint8_t rsa_pub_mod[] = {
	0xce, 0xe8, 0x27, 0x64, 0xe3, 0x87, 0xa3, 0x08, 0x40, 0xf2, 0x1a, 0x0d,
	0x17, 0x7a, 0x34, 0x41, 0xc3, 0x06, 0x82, 0x6e, 0xce, 0xd0, 0x56, 0xda,
	0x7b, 0x61, 0x49, 0xd1, 0xb6, 0x69, 0xfc, 0x26, 0x10, 0x13, 0x3a, 0xb4,
	0x98, 0xd5, 0xbf, 0x31, 0x31, 0xf9, 0x11, 0x0b, 0xd1, 0x6f, 0x8f, 0x5c,
	0x2e, 0xea, 0x91, 0x5e, 0x56, 0xf0, 0x2e, 0x55, 0x6f, 0x41, 0x29, 0x61,
	0xa5, 0x9c, 0x0f, 0xe9, 0x37, 0x9a, 0x4f, 0x3d, 0xe5, 0x8d, 0x53, 0x47,
	0xb6, 0x59, 0xa9, 0xc9, 0x31, 0xbe, 0xc2, 0x63, 0xb0, 0x21, 0xa9, 0x18,
	0x99, 0x77, 0xdf, 0x57, 0xb2, 0x6a, 0xa3, 0x78, 0x41, 0x58, 0x26, 0xbe,
	0xcb, 0x6e, 0xfe, 0x24, 0x38, 0x82, 0xb5, 0xe8, 0x86, 0xac, 0xf7, 0x77,
	0x41, 0xfe, 0x2e, 0x14, 0x97, 0x2a, 0x14, 0x77, 0xf3, 0xef, 0xc0, 0x03,
	0x80, 0x91, 0xb6, 0x05, 0x1c, 0x00, 0x39, 0x64, 0x96, 0x92, 0xb3, 0x08,
	0x31, 0xa2, 0xff, 0xe4, 0x0d, 0x76, 0xb2, 0xd1, 0xe1, 0xaf, 0x10, 0xa9,
	0xa2, 0x2e, 0x1c, 0xa2, 0xdb, 0x26, 0x42, 0x32, 0xa2, 0xaf, 0x86, 0xc4,
	0xf3, 0xaf, 0x5b, 0x9d, 0x07, 0x48, 0x0b, 0x5f, 0x8f, 0xc7, 0x33, 0x1d,
	0x5a, 0x18, 0x52, 0x0f, 0x3d, 0xc0, 0x5e, 0xb0, 0x1b, 0x82, 0xab, 0x36,
	0x5b, 0x8d, 0xb2, 0xa5, 0x7e, 0x0a, 0x80, 0x6f, 0x73, 0xb5, 0x0c, 0xd2,
	0x30, 0x12, 0xe0, 0x04, 0x74, 0xf7, 0xf3, 0x9a, 0xbb, 0xae, 0x76, 0xaf,
	0xb4, 0xe5, 0x3a, 0xa6, 0x4d, 0x84, 0x27, 0x6d, 0x7a, 0x8d, 0x2b, 0xd6,
	0xda, 0xcb, 0x95, 0x0e, 0x77, 0xd5, 0x2d, 0xa4, 0x5f, 0xa6, 0x0f, 0x4c,
	0xbe, 0xa0, 0xcb, 0x15, 0x5b, 0x85, 0x1b, 0xde, 0xc0, 0x1a, 0x83, 0xa4,
	0x7e, 0x91, 0x2a, 0xfb, 0x0d, 0x63, 0x37, 0x09, 0x1b, 0x4f, 0x7d, 0x7f,
	0xb3, 0x96, 0xa0, 0xb7
};
static uint8_t rsa_pub_exp[] = {
	0x01, 0x00, 0x01
};
static uint8_t rsa_priv_exp[] = {
	0x65, 0x15, 0x82, 0x79, 0x85, 0xb7, 0xb4, 0x6c, 0xc9, 0xdd, 0xfd, 0xe0,
	0x3b, 0x82, 0x16, 0x99, 0xb2, 0x40, 0x31, 0xe5, 0x14, 0xe1, 0x30, 0x2d,
	0x99, 0x6d, 0x94, 0x75, 0x02, 0x5b, 0x91, 0x36, 0xbf, 0x4f, 0xc5, 0xb4,
	0x89, 0x9e, 0x93, 0xc3, 0xe3, 0x18, 0x3d, 0xbd, 0xba, 0x5a, 0x78, 0x7d,
	0x9e, 0xe6, 0x36, 0x93, 0x12, 0x82, 0x51, 0x00, 0xa8, 0x53, 0xfb, 0x75,
	0x9b, 0x45, 0x3d, 0x78, 0xc1, 0x13, 0x80, 0x77, 0x4a, 0xf8, 0xda, 0xa1,
	0x04, 0x73, 0x95, 0xc1, 0x97, 0xae, 0x0c, 0xa6, 0x4b, 0x5c, 0x86, 0x92,
	0x60, 0x17, 0xf2, 0x63, 0x93, 0xe5, 0x09, 0x6c, 0x2c, 0xf0, 0x76, 0x2d,
	0x6c, 0x92, 0x13, 0x53, 0x7a, 0xd0, 0x51, 0x33, 0x02, 0xe8, 0xea, 0x15,
	0xb7, 0x86, 0x08, 0xd9, 0x42, 0x68, 0xea, 0x8b, 0x18, 0x00, 0x1f, 0x9d,
	0x16, 0x69, 0x19, 0xb6, 0x31, 0x51, 0xe2, 0xb7, 0x62, 0xd4, 0xef, 0xb5,
	0x3f, 0xaa, 0xc1, 0x2b, 0x5a, 0xfb, 0x90, 0xf3, 0xd3, 0x66, 0x9e, 0xfa,
	0x6c, 0xc0, 0x1e, 0x0d, 0x20, 0xdc, 0x64, 0x22, 0x3e, 0xd8, 0xc0, 0xdf,
	0x7b, 0x06, 0x00, 0x06, 0xa8, 0xc0, 0xa3, 0xb3, 0x15, 0xb4, 0x56, 0x5a,
	0x8a, 0xe2, 0xfb, 0xf6, 0x36, 0x0a, 0xa0, 0x3e, 0x0b, 0xfb, 0x2b, 0x7a,
	0x03, 0xdb, 0xf8, 0xc8, 0x7d, 0x78, 0xac, 0x0e, 0x43, 0x1e, 0xed, 0x17,
	0x91, 0x5b, 0xc9, 0x27, 0xdd, 0xec, 0x2f, 0x6f, 0x92, 0x2a, 0x38, 0x11,
	0x7c, 0xf9, 0xe5, 0x50, 0x7b, 0x72, 0xff, 0xb4, 0x00, 0x1d, 0xce, 0x06,
	0xf9, 0x86, 0x85, 0x27, 0x5f, 0xe3, 0x60, 0x24, 0x29, 0xe9, 0xac, 0x94,
	0x2f, 0xb0, 0x41, 0xb2, 0x09, 0xf8, 0x7b, 0x33, 0x62, 0x96, 0xcb, 0xaf,
	0xca, 0x20, 0xa1, 0x38, 0xec, 0x34, 0xfa, 0x96, 0xf6, 0xd3, 0x3b, 0xcd,
	0x8f, 0xc2, 0xa7, 0xf1
};

/* P-192 ECC Keys */
static uint8_t ecc_private[] = {
	0x00, 0x17, 0x89, 0x99, 0x49, 0xd0, 0x2b, 0x55, 0xf9, 0x55, 0x68, 0x46,
	0x41, 0x1c, 0xc9, 0xde, 0x51, 0x2c, 0x6f, 0x16, 0xec, 0xde, 0xb1, 0xc4
};
static uint8_t ecc_pub_x[] = {
	0x14, 0xf6, 0x97, 0x38, 0x59, 0x96, 0x89, 0xf5, 0x70, 0x6a, 0xb7, 0x13,
	0x43, 0xbe, 0xcc, 0x88, 0x6e, 0xf1, 0x56, 0x9a, 0x2d, 0x11, 0x37, 0xfe
};
static uint8_t ecc_pub_y[] = {
	0x0c, 0xf5, 0xa4, 0x33, 0x90, 0x9e, 0x33, 0x21, 0x7f, 0xb4, 0xdf, 0x6b,
	0x95, 0x93, 0xf7, 0x1d, 0x43, 0xfb, 0x1c, 0x2a, 0x56, 0x53, 0xb7, 0x63
};

struct hse_ctx {
	TEEC_Context ctx;
	TEEC_Session sess;
};

static void teec_err(TEEC_Result res, uint32_t eo, const char *str)
{
	printf("[ERROR] %s: %#" PRIx32 " (error origin %#" PRIx32 ")\n", str, res, eo);
	errx(1, "%s: %#" PRIx32 " (error origin %#" PRIx32 ")", str, res, eo);
}

#define PRINT_OK	 printf("... OK!\n")

static TEEC_Result provision_cmd(TEEC_Session *sess,
				 uint8_t *keys[PROVISION_KEYS_NUM],
				 uint16_t lenghts[PROVISION_KEYS_NUM],
				 struct hse_kp_info key_info)
{
	TEEC_Operation op = {0};
	size_t i;
	TEEC_Result res;
	uint32_t eo;

	if (!sess || !keys || !lenghts)
		return TEEC_ERROR_BAD_PARAMETERS;

	op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT,
					 TEEC_MEMREF_TEMP_INPUT,
					 TEEC_MEMREF_TEMP_INPUT,
					 TEEC_MEMREF_TEMP_INPUT);

	op.params[0].tmpref.buffer = &key_info;
	op.params[0].tmpref.size = sizeof(key_info);

	for (i = 0; i < PROVISION_KEYS_NUM; i++) {
		op.params[i + 1].tmpref.buffer = keys[i];
		op.params[i + 1].tmpref.size = lenghts[i];
	}

	res = TEEC_InvokeCommand(sess, PTA_CMD_KEY_PROVISION,
				 &op, &eo);
	if (res != TEEC_SUCCESS)
		teec_err(res, eo, "TEEC_InvokeCommand(PTA_CMD_KEY_PROVISION) failed");

	return res;
}

static TEEC_Result erase_cmd(TEEC_Session *sess,
			     uint32_t hse_key_handle)
{
	TEEC_Operation op = {0};
	TEEC_Result res;
	uint32_t eo;

	if (!sess)
		return TEEC_ERROR_BAD_PARAMETERS;

	op.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT,
					 TEEC_MEMREF_TEMP_INPUT,
					 TEEC_MEMREF_TEMP_INPUT,
					 TEEC_MEMREF_TEMP_INPUT);

	op.params[0].value.a = hse_key_handle;

	res = TEEC_InvokeCommand(sess, PTA_CMD_KEY_ERASE,
				 &op, &eo);
	if (res != TEEC_SUCCESS)
		teec_err(res, eo, "TEEC_InvokeCommand(PTA_CMD_KEY_ERASE) failed");

	return res;
}

static TEEC_Result import_aes_key(TEEC_Session *sess)
{
	struct hse_kp_info key_info;
	uint16_t key_flags;
	/* Choose a valid AES key length: 24, 28 or 32 */
	uint8_t aes_key[32];
	uint8_t *provision_keys_arr[PROVISION_KEYS_NUM] = {
		NULL, NULL, aes_key
	};
	uint16_t provision_keys_lengths[PROVISION_KEYS_NUM] = {
		0, 0, sizeof(aes_key)
	};

	if (!sess)
		return TEEC_ERROR_BAD_PARAMETERS;

	/* Use bit-wise OR to add more than one flag */
	key_flags = HSE_KF_USAGE_ENCRYPT | HSE_KF_USAGE_DECRYPT;
	key_info = (struct hse_kp_info)
		KEY_INFO_INIT(HSE_AES_KEY_GROUP, HSE_AES_KEY_SLOT,
			      HSE_KEY_TYPE_AES, key_flags);

	memset(aes_key, 0xAA, 32); /* set a dummy value */

	return provision_cmd(sess, provision_keys_arr, provision_keys_lengths,
			     key_info);
}

static TEEC_Result import_rsa_keypair(TEEC_Session *sess)
{
	struct hse_kp_info key_info;
	uint16_t key_flags;
	uint8_t *provision_keys_arr[PROVISION_KEYS_NUM] = {
		rsa_pub_mod, rsa_pub_exp, rsa_priv_exp};
	uint16_t provision_keys_lengths[PROVISION_KEYS_NUM] = {
		sizeof(rsa_pub_mod), sizeof(rsa_pub_exp), sizeof(rsa_priv_exp)};

	if (!sess)
		return TEEC_ERROR_BAD_PARAMETERS;

	key_flags = HSE_KF_USAGE_DECRYPT;
	key_info = (struct hse_kp_info)
		KEY_INFO_INIT(HSE_RSA_PAIR_GROUP, HSE_RSA_PAIR_SLOT,
			      HSE_KEY_TYPE_RSA_PAIR, key_flags);

	return provision_cmd(sess, provision_keys_arr, provision_keys_lengths,
			     key_info);
}

static TEEC_Result import_rsa_pubkey(TEEC_Session *sess)
{
	struct hse_kp_info key_info;
	uint16_t key_flags;
	uint8_t *provision_keys_arr[PROVISION_KEYS_NUM] = {
		rsa_pub_mod, rsa_pub_exp, NULL};
	uint16_t provision_keys_lengths[PROVISION_KEYS_NUM] = {
		sizeof(rsa_pub_mod), sizeof(rsa_pub_exp), 0};

	if (!sess)
		return TEEC_ERROR_BAD_PARAMETERS;

	key_flags = HSE_KF_USAGE_ENCRYPT;
	key_info = (struct hse_kp_info)
		KEY_INFO_INIT(HSE_RSA_PUB_GROUP, HSE_RSA_PUB_SLOT,
			      HSE_KEY_TYPE_RSA_PUB, key_flags);

	return provision_cmd(sess, provision_keys_arr, provision_keys_lengths,
			     key_info);
}

static TEEC_Result import_ecc_keypair(TEEC_Session *sess)
{
	struct hse_kp_info key_info;
	uint16_t key_flags;
	uint16_t ecc_point_size = sizeof(ecc_private);
	uint8_t ecc_public_xy[2 * ecc_point_size];
	uint8_t *provision_keys_arr[PROVISION_KEYS_NUM] = {
		ecc_public_xy, NULL, ecc_private};
	uint16_t provision_keys_lengths[PROVISION_KEYS_NUM] = {
		2 * ecc_point_size, 0, ecc_point_size};

	if (!sess)
		return TEEC_ERROR_BAD_PARAMETERS;

	/*
	 * The provision command expects the public point in the format X||Y
	 * put in the first parameter
	 */
	memcpy(ecc_public_xy, ecc_pub_x, ecc_point_size);
	memcpy(ecc_public_xy + ecc_point_size, ecc_pub_y, ecc_point_size);

	key_flags = HSE_KF_USAGE_SIGN;
	key_info = (struct hse_kp_info)
		KEY_INFO_INIT(HSE_ECC_PAIR_GROUP, HSE_ECC_PAIR_SLOT,
			      HSE_KEY_TYPE_ECC_PAIR, key_flags);

	return provision_cmd(sess, provision_keys_arr, provision_keys_lengths,
			     key_info);
}

static TEEC_Result import_ecc_pubkey(TEEC_Session *sess)
{
	struct hse_kp_info key_info;
	uint16_t key_flags;
	uint16_t ecc_point_size = sizeof(ecc_private);
	uint8_t ecc_public_xy[2 * ecc_point_size];
	uint8_t *provision_keys_arr[PROVISION_KEYS_NUM] = {
		ecc_public_xy, NULL, NULL};
	uint16_t provision_keys_lengths[PROVISION_KEYS_NUM] = {
		2 * ecc_point_size, 0, 0};

	if (!sess)
		return TEEC_ERROR_BAD_PARAMETERS;

	/*
	 * The provision command expects the public point in the format X||Y
	 * put in the first parameter
	 */
	memcpy(ecc_public_xy, ecc_pub_x, ecc_point_size);
	memcpy(ecc_public_xy + ecc_point_size, ecc_pub_y, ecc_point_size);

	key_flags = HSE_KF_USAGE_VERIFY;
	key_info = (struct hse_kp_info)
		KEY_INFO_INIT(HSE_ECC_PUB_GROUP, HSE_ECC_PUB_SLOT,
			      HSE_KEY_TYPE_ECC_PUB, key_flags);

	return provision_cmd(sess, provision_keys_arr, provision_keys_lengths,
			     key_info);
}

static TEEC_Result set_operation_key(TEEC_Session *sess, uint8_t key_type,
				     uint8_t group, uint8_t slot)
{
	TEEC_Result res;
	TEEC_Operation op = {0};
	uint32_t eo;

	if (!sess)
		return TEEC_ERROR_BAD_PARAMETERS;

	op.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT, TEEC_NONE,
					 TEEC_NONE, TEEC_NONE);
	op.params[0].value.a = key_type;
	op.params[0].value.b = GET_KEY_HANDLE(HSE_KEY_CATALOG_ID_NVM,
					      group, slot);

	res = TEEC_InvokeCommand(sess, TA_HSE_EMBED_HANDLE_CMD_SET_KEY, &op, &eo);

	if (res != TEEC_SUCCESS)
		teec_err(res, eo, "Set operation key failed");

	return res;
}

static TEEC_Result asymm_cmd(TEEC_Session *sess, uint8_t cmd,
				 uint8_t *input, size_t input_size,
				 uint8_t *output, size_t *output_size)
{
	TEEC_Operation op = {0};
	TEEC_Result res;
	uint32_t eo;

	if (!sess)
		return TEEC_ERROR_BAD_PARAMETERS;

	if (cmd != TA_HSE_EMBED_HANDLE_CMD_ASYMM_ENCRYPT &&
	    cmd != TA_HSE_EMBED_HANDLE_CMD_ASYMM_DECRYPT)
		return TEEC_ERROR_BAD_PARAMETERS;

	if (!input || !input_size || !output || !output_size)
		return TEEC_ERROR_BAD_PARAMETERS;

	op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT,
					 TEEC_MEMREF_TEMP_OUTPUT,
					 TEEC_NONE, TEEC_NONE);
	op.params[0].tmpref.buffer = input;
	op.params[0].tmpref.size = input_size;
	op.params[1].tmpref.buffer = output;
	op.params[1].tmpref.size = *output_size;

	res = TEEC_InvokeCommand(sess, cmd, &op, &eo);
	if (res != TEEC_SUCCESS)
		teec_err(res, eo, "Asymmetric encyption/decryption failed");

	*output_size = op.params[1].tmpref.size;

	return res;
}

static TEEC_Result cipher_command(TEEC_Session *sess, uint8_t cmd,
				 uint8_t *input, size_t input_size,
				 uint8_t *output, size_t *output_size)
{
	TEEC_Operation op = {0};
	TEEC_Result res;
	uint32_t eo;

	if (!sess)
		return TEEC_ERROR_BAD_PARAMETERS;

	if (cmd != TA_HSE_EMBED_HANDLE_CMD_CIPHER_DECRYPT &&
	    cmd != TA_HSE_EMBED_HANDLE_CMD_CIPHER_ENCRYPT)
		return TEEC_ERROR_BAD_PARAMETERS;

	if (!input || !input_size || !output || !output_size)
		return TEEC_ERROR_BAD_PARAMETERS;

	op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT,
					 TEEC_MEMREF_TEMP_OUTPUT,
					 TEEC_NONE, TEEC_NONE);
	op.params[0].tmpref.buffer = input;
	op.params[0].tmpref.size = input_size;
	op.params[1].tmpref.buffer = output;
	op.params[1].tmpref.size = *output_size;

	res = TEEC_InvokeCommand(sess, cmd, &op, &eo);
	if (res != TEEC_SUCCESS)
		teec_err(res, eo, "Symmetric cipher failed");

	*output_size = op.params[1].tmpref.size;

	return res;
}

static TEEC_Result sign_command(TEEC_Session *sess, uint8_t cmd,
				 uint8_t *digest, size_t digest_len,
				 uint8_t *signature, size_t *signature_len)
{
	TEEC_Operation op = {0};
	TEEC_Result res;
	uint32_t eo;

	if (!sess)
		return TEEC_ERROR_BAD_PARAMETERS;

	if (cmd != TA_HSE_EMBED_HANDLE_CMD_SIGN &&
	    cmd != TA_HSE_EMBED_HANDLE_CMD_VERIFY)
		return TEEC_ERROR_BAD_PARAMETERS;

	if (!digest || !digest_len || !signature || !signature_len)
		return TEEC_ERROR_BAD_PARAMETERS;

	op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INPUT,
					 TEEC_MEMREF_TEMP_OUTPUT,
					 TEEC_NONE, TEEC_NONE);
	op.params[0].tmpref.buffer = digest;
	op.params[0].tmpref.size = digest_len;
	op.params[1].tmpref.buffer = signature;
	op.params[1].tmpref.size = *signature_len;

	res = TEEC_InvokeCommand(sess, cmd, &op, &eo);
	if (res != TEEC_SUCCESS)
		teec_err(res, eo, "ECC Sign/Verify failed");

	*signature_len = op.params[1].tmpref.size;

	return res;
}

static TEEC_Result import_hse_keys(struct hse_ctx *ctx)
{
	TEEC_UUID uuid = PTA_HSE_KP_UUID;
	TEEC_Session *sess = &ctx->sess;
	uint32_t eo;
	TEEC_Result res;

	/* Open a session with the HSE Key Provisioning PTA */
	res = TEEC_OpenSession(&ctx->ctx, &ctx->sess, &uuid,
			       TEEC_LOGIN_PUBLIC, NULL, NULL, &eo);
	if (res != TEEC_SUCCESS)
		teec_err(res, eo, "TEEC_Opensession failed");

	printf("Importing keys into HSE NVM Catalog:\n");

	printf("AES Key");
	res = import_aes_key(sess);
	if (res != TEEC_SUCCESS)
		return res;
	PRINT_OK;

	printf("RSA Key Pair");
	res = import_rsa_keypair(sess);
	if (res != TEEC_SUCCESS)
		return res;
	PRINT_OK;

	printf("RSA Public Key");
	res = import_rsa_pubkey(sess);
	if (res != TEEC_SUCCESS)
		return res;
	PRINT_OK;

	printf("ECC Key Pair");
	res = import_ecc_keypair(sess);
	if (res != TEEC_SUCCESS)
		return res;
	PRINT_OK;

	printf("ECC Public Key");
	res = import_ecc_pubkey(sess);
	if (res != TEEC_SUCCESS)
		return res;
	PRINT_OK;

	printf("All keys were successfully imported!\n\n");

	TEEC_CloseSession(&ctx->sess);

	return TEEC_SUCCESS;
}

static TEEC_Result erase_hse_keys(struct hse_ctx *ctx)
{
	TEEC_UUID uuid = PTA_HSE_KP_UUID;
	TEEC_Session *sess = &ctx->sess;
	uint32_t eo, hse_key_handle;
	TEEC_Result res;

	/* Open a session with the HSE Key Provisioning PTA */
	res = TEEC_OpenSession(&ctx->ctx, &ctx->sess, &uuid,
			       TEEC_LOGIN_PUBLIC, NULL, NULL, &eo);
	if (res != TEEC_SUCCESS)
		teec_err(res, eo, "TEEC_Opensession failed");

	printf("Erasing keys from HSE NVM Catalog:\n");

	printf("AES Key");
	hse_key_handle = GET_KEY_HANDLE(HSE_KEY_CATALOG_ID_NVM,
					HSE_AES_KEY_GROUP, HSE_AES_KEY_SLOT);
	res = erase_cmd(sess, hse_key_handle);
	if (res != TEEC_SUCCESS)
		return res;
	PRINT_OK;

	printf("RSA Key Pair");
	hse_key_handle = GET_KEY_HANDLE(HSE_KEY_CATALOG_ID_NVM,
					HSE_RSA_PAIR_GROUP, HSE_RSA_PAIR_SLOT);
	res = erase_cmd(sess, hse_key_handle);
	if (res != TEEC_SUCCESS)
		return res;
	PRINT_OK;

	printf("RSA Public Key");
	hse_key_handle = GET_KEY_HANDLE(HSE_KEY_CATALOG_ID_NVM,
					HSE_RSA_PUB_GROUP, HSE_RSA_PUB_SLOT);
	res = erase_cmd(sess, hse_key_handle);
	if (res != TEEC_SUCCESS)
		return res;
	PRINT_OK;

	printf("ECC Key Pair");
	hse_key_handle = GET_KEY_HANDLE(HSE_KEY_CATALOG_ID_NVM,
					HSE_ECC_PAIR_GROUP, HSE_ECC_PAIR_SLOT);
	res = erase_cmd(sess, hse_key_handle);
	if (res != TEEC_SUCCESS)
		return res;
	PRINT_OK;

	printf("ECC Public Key");
	hse_key_handle = GET_KEY_HANDLE(HSE_KEY_CATALOG_ID_NVM,
					HSE_ECC_PUB_GROUP, HSE_ECC_PUB_SLOT);
	res = erase_cmd(sess, hse_key_handle);
	if (res != TEEC_SUCCESS)
		return res;
	PRINT_OK;

	printf("All keys were successfully erased!\n\n");

	TEEC_CloseSession(&ctx->sess);

	return TEEC_SUCCESS;
}

static TEEC_Result symm_cipher_operations(struct hse_ctx *ctx)
{
	TEEC_Result res;
	uint32_t eo;
	TEEC_UUID uuid = TA_HSE_EMBED_HANDLE_UUID;
	TEEC_Session *sess = &ctx->sess;

	/* The AES Block size is equal to 16 bytes */
	uint8_t in_vect[16] = {0xAB}; /* use a dummy value */
	uint8_t out_vect[16] = {0};
	uint8_t out_enc_vect[16] = {0};
	size_t out_size = sizeof(out_vect), in_size = sizeof(in_vect),
	       out_enc_size = sizeof(out_enc_vect);

	/* Open a session with the HSE Key Provisioning PTA */
	res = TEEC_OpenSession(&ctx->ctx, &ctx->sess, &uuid,
			       TEEC_LOGIN_PUBLIC, NULL, NULL, &eo);
	if (res != TEEC_SUCCESS)
		teec_err(res, eo, "TEEC_Opensession failed");

	printf("Setting Embedded AES key for the encryption "\
	       "operation");
	res = set_operation_key(sess, TA_HSE_EMBED_HANDLE_AES_KEY,
				HSE_AES_KEY_GROUP, HSE_AES_KEY_SLOT);
	if (res != TEEC_SUCCESS)
		return res;
	PRINT_OK;

	printf("Executing AES encryption");
	res = cipher_command(sess, TA_HSE_EMBED_HANDLE_CMD_CIPHER_ENCRYPT,
			     in_vect, in_size,
			     out_enc_vect, &out_enc_size);
	if (res != TEEC_SUCCESS)
		return res;
	PRINT_OK;

	printf("Executing AES decryption");
	res = cipher_command(sess, TA_HSE_EMBED_HANDLE_CMD_CIPHER_DECRYPT,
			     out_enc_vect, out_enc_size,
			     out_vect, &out_size);
	if (res != TEEC_SUCCESS)
		return res;
	PRINT_OK;

	if (in_size != out_size) {
		printf("[ERROR] Size of the decrypted output does not match "\
		       "with the input's size\n");
		return TEEC_ERROR_BAD_FORMAT;
	}

	if (memcmp(in_vect, out_vect, in_size) != 0) {
		printf("[ERROR] The contents of the decrypted output do not "\
		       "match with the input's\n");
		return TEEC_ERROR_BAD_FORMAT;
	}

	printf("Cipher operations using the hse embedded "\
	       "key handles were successful!\n\n");

	TEEC_CloseSession(&ctx->sess);

	return TEEC_SUCCESS;
}

static TEEC_Result rsa_cipher_operations(struct hse_ctx *ctx)
{
	TEEC_Result res;
	uint32_t eo;
	TEEC_UUID uuid = TA_HSE_EMBED_HANDLE_UUID;
	TEEC_Session *sess = &ctx->sess;

	/*
	 * The TA will perform RSA NO-PAD encryption so the input must be
	 * less or equal to the size of the public modulus
	 */
	uint8_t in_vect[sizeof(rsa_pub_mod)] = {0xAB};

	/* The OP-TEE RSA maximum accepted size for the public modulus is
	 * 512 bytes (4096 bits). Thus, the encryption output cannot exceed
	 * this limit.
	 */
	uint8_t out_vect[512] = {0};
	uint8_t out_enc_vect[512] = {0};
	size_t out_size = sizeof(out_vect), in_size = sizeof(in_vect),
	       out_enc_size = sizeof(out_enc_vect);

	/* Open a session with the HSE Key Provisioning PTA */
	res = TEEC_OpenSession(&ctx->ctx, &ctx->sess, &uuid,
			       TEEC_LOGIN_PUBLIC, NULL, NULL, &eo);
	if (res != TEEC_SUCCESS)
		teec_err(res, eo, "TEEC_Opensession failed");

	printf("Setting Embedded RSA Public key for the encryption "\
	       "operation");
	res = set_operation_key(sess, TA_HSE_EMBED_HANDLE_RSA_PUBKEY,
				HSE_RSA_PUB_GROUP, HSE_RSA_PUB_SLOT);
	if (res != TEEC_SUCCESS)
		return res;
	PRINT_OK;

	printf("Executing RSA encryption");
	res = asymm_cmd(sess, TA_HSE_EMBED_HANDLE_CMD_ASYMM_ENCRYPT,
			    in_vect, in_size,
			    out_enc_vect, &out_enc_size);
	if (res != TEEC_SUCCESS)
		return res;
	PRINT_OK;

	printf("Setting Embedded RSA Private key for the decryption "\
	       "operation");
	res = set_operation_key(sess, TA_HSE_EMBED_HANDLE_RSA_PRIVKEY,
				HSE_RSA_PAIR_GROUP, HSE_RSA_PAIR_SLOT);
	if (res != TEEC_SUCCESS)
		return res;
	PRINT_OK;

	printf("Executing RSA decryption");
	res = asymm_cmd(sess, TA_HSE_EMBED_HANDLE_CMD_ASYMM_DECRYPT,
			   out_enc_vect, out_enc_size,
			   out_vect, &out_size);
	if (res != TEEC_SUCCESS)
		return res;
	PRINT_OK;

	if (in_size != out_size) {
		printf("[ERROR] Size of the decrypted output does not match"\
		       "with the input's size\n");
		return TEEC_ERROR_BAD_FORMAT;
	}

	if (memcmp(in_vect, out_vect, in_size) != 0) {
		printf("[ERROR] The contents of the decrypted output do not "\
		       "match with the input's\n");
		return TEEC_ERROR_BAD_FORMAT;
	}

	printf("Asymmetric encryption & decryption using the hse embedded "\
	       "key handles was successful\n\n");

	TEEC_CloseSession(&ctx->sess);

	return TEEC_SUCCESS;
}

static TEEC_Result ecc_sign_operations(struct hse_ctx *ctx)
{
	TEEC_Result res;
	uint32_t eo;
	TEEC_UUID uuid = TA_HSE_EMBED_HANDLE_UUID;
	TEEC_Session *sess = &ctx->sess;

	/*
	 * The P-192 ECDSA operation expects a SHA-1 hashed message
	 * (20 byte length digest)
	 */
	uint8_t digest[20] = {0xAB};
	uint8_t signature[2 * sizeof(ecc_private)] = {0};
	size_t digest_len = sizeof(digest), signature_len = sizeof(signature);

	/* Open a session with the HSE Key Provisioning PTA */
	res = TEEC_OpenSession(&ctx->ctx, &ctx->sess, &uuid,
			       TEEC_LOGIN_PUBLIC, NULL, NULL, &eo);
	if (res != TEEC_SUCCESS)
		teec_err(res, eo, "TEEC_Opensession failed");

	printf("Setting ECC Private Key for sign "\
	       "operation");
	res = set_operation_key(sess, TA_HSE_EMBED_HANDLE_ECC_PRIVKEY,
				HSE_ECC_PAIR_GROUP, HSE_ECC_PAIR_SLOT);
	if (res != TEEC_SUCCESS)
		return res;
	PRINT_OK;

	printf("Executing ECC Sign");
	res = sign_command(sess, TA_HSE_EMBED_HANDLE_CMD_SIGN,
			   digest, digest_len,
			   signature, &signature_len);
	if (res != TEEC_SUCCESS)
		return res;
	PRINT_OK;

	printf("Setting ECC Public Key for verification "\
	       "operation");
	res = set_operation_key(sess, TA_HSE_EMBED_HANDLE_ECC_PUBKEY,
				HSE_ECC_PUB_GROUP, HSE_ECC_PUB_SLOT);
	if (res != TEEC_SUCCESS)
		return res;
	PRINT_OK;

	printf("Executing ECC Verify");
	res = sign_command(sess, TA_HSE_EMBED_HANDLE_CMD_VERIFY,
			     digest, digest_len,
			     signature, &signature_len);
	if (res != TEEC_SUCCESS)
		return res;
	PRINT_OK;

	printf("ECC sign & verify using the hse embedded "\
	       "key handles was successful\n\n");

	TEEC_CloseSession(&ctx->sess);

	return TEEC_SUCCESS;
}

void usage(char *progname)
{
	printf("\nImport plain keys into NVM Catalog; use those keys for "\
	       "crypto operations and erase them afterwards\n");
	printf("\n");
	printf("Usage:\n");
	printf("\n");
	printf("%s [-h] [-i] [-o] [-e]\n", progname);
	printf("\n");
	printf("\t-h:\tdisplay this help string\n");
	printf("\t-i:\tonly import the keys in NVM Catalog\n");
	printf("\t-o:\tonly perform the symmetric/asymmetric cipher & "\
	       "sign/verify operations\n");
	printf("\t-e:\tonly erase the keys from the NVM Catalog\n");
}

int main(int argc, char *argv[])
{
	int c;
	bool iflag = false, oflag = false, eflag = false;
	struct hse_ctx ctx;
	TEEC_Result res;

	while ((c = getopt(argc, argv, "hioe")) != -1) {
		switch (c) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 'i':
			iflag = true;
			break;
		case 'o':
			oflag = true;
			break;
		case 'e':
			eflag = true;
			break;
		default:
			printf("Unknown error\n");
			usage(argv[0]);
			return -1;
		}
	}

	/* Initialize a context connecting us to the TEE */
	res = TEEC_InitializeContext(NULL, &ctx.ctx);
	if (res != TEEC_SUCCESS)
		teec_err(res, 0, "TEEC_InitializeContext failed");

	if (!iflag && !oflag && !eflag) {
		iflag = true;
		oflag = true;
		eflag = true;
	}

	if (iflag) {
		res = import_hse_keys(&ctx);
		if (res)
			goto out;
	}

	if (oflag) {
		res = symm_cipher_operations(&ctx);
		if (res)
			goto out;

		res = rsa_cipher_operations(&ctx);
		if (res)
			goto out;

		res = ecc_sign_operations(&ctx);
		if (res)
			goto out;
	}

	if (eflag) {
		res = erase_hse_keys(&ctx);
		if (res)
			goto out;
	}
out:
	TEEC_FinalizeContext(&ctx.ctx);

	if (res)
		return -1;
	return 0;
}
