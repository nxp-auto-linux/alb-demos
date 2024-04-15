// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright 2024 NXP
 */

#include <inttypes.h>
#include <util.h>
#include <string.h>

#include <tee_internal_api.h>
#include <hse_embed_handle_ta.h>

static const uint8_t hse_magic[] = {
	0xCA, 0xBF, 0x00, 0x1C, 0xCA, 0xBF, 0x54, 0x1C,
	0x39, 0x78, 0x04, 0xA3, 0x5A, 0x11, 0x24, 0x9F,
	0x64, 0xEE, 0x89, 0x23, 0x90, 0x8C, 0xF5, 0x64,
	0x9A, 0x8C, 0x7E, 0x35
};

#define MAGIC_SIZE	ARRAY_SIZE(hse_magic)

struct hse_embed_handle {
	uint8_t magic[MAGIC_SIZE];
	uint32_t handle;
} __attribute__((packed));

static inline void hse_embed_handle_init(struct hse_embed_handle *buff,
					 uint32_t handle)
{
	memcpy(buff->magic, hse_magic, MAGIC_SIZE);
	buff->handle = handle;
}

struct hse_sess {
	TEE_ObjectHandle pub_key;
	TEE_ObjectHandle priv_key;
};

static TEE_Result ta2tee_key_type(uint32_t ta_key_type,
				  uint32_t *tee_key_type)
{
	switch (ta_key_type) {
	case TA_HSE_EMBED_HANDLE_RSA_PUBKEY:
		*tee_key_type = TEE_TYPE_RSA_PUBLIC_KEY;
		break;

	case TA_HSE_EMBED_HANDLE_RSA_PRIVKEY:
		*tee_key_type = TEE_TYPE_RSA_KEYPAIR;
		break;

	case TA_HSE_EMBED_HANDLE_ECC_PRIVKEY:
		*tee_key_type = TEE_TYPE_ECDSA_KEYPAIR;
		break;

	case TA_HSE_EMBED_HANDLE_ECC_PUBKEY:
		*tee_key_type = TEE_TYPE_ECDSA_PUBLIC_KEY;
		break;

	case TA_HSE_EMBED_HANDLE_AES_KEY:
		*tee_key_type = TEE_TYPE_AES;
		break;

	default:
		return TEE_ERROR_BAD_PARAMETERS;
	}

	return TEE_SUCCESS;
}

static TEE_Result allocate_tee_obj(uint32_t ta_key_type, TEE_ObjectHandle *key)
{
	TEE_Result res;
	uint32_t embed_handle_bitsize = sizeof(struct hse_embed_handle) * 8;
	uint32_t tee_key_type;

	res = ta2tee_key_type(ta_key_type, &tee_key_type);
	if (res != TEE_SUCCESS) {
		EMSG("Invalid key type: %u", ta_key_type);
		return res;
	}

	/*
	 * Allocate a new object based on its TEE type. The 256 bit size of the
	 * struct hse_embed_handle should be in the range of minimum and maximum
	 * object size of every TEE Object Key type.
	 */
	res = TEE_AllocateTransientObject(tee_key_type, embed_handle_bitsize,
					  key);
	if (res != TEE_SUCCESS)
		EMSG("TEE_AllocateTransientObject failed, 0x%x", res);

	return res;
}

static TEE_Result populate_rsakey(struct hse_sess *state, TEE_ObjectHandle key,
			   struct hse_embed_handle hse_handle,
			   uint32_t ta_key_type)
{
	TEE_Result res;
	const uint32_t embed_handle_size = sizeof(hse_handle);
	uint8_t dummy_exp[embed_handle_size];
	TEE_Attribute attr[3];
	uint8_t attr_count = 0;

	/*
	 * Only the modulus must be populated with the actual embedded handle
	 * buffer. The private & public exponents are dummy buffers as the TEE
	 * Internal API requires these attributes to be part of the private/
	 * public key.
	 */
	TEE_InitRefAttribute(&attr[attr_count++], TEE_ATTR_RSA_MODULUS,
			     &hse_handle, embed_handle_size);
	TEE_InitRefAttribute(&attr[attr_count++], TEE_ATTR_RSA_PUBLIC_EXPONENT,
			     dummy_exp, embed_handle_size);
	if (ta_key_type == TA_HSE_EMBED_HANDLE_RSA_PRIVKEY)
		TEE_InitRefAttribute(&attr[attr_count++],
				     TEE_ATTR_RSA_PRIVATE_EXPONENT,
				     dummy_exp, embed_handle_size);

	res = TEE_PopulateTransientObject(key, attr, attr_count);
	if (res != TEE_SUCCESS) {
		EMSG("TEE_PopulateTransientObject failed, 0x%x", res);
		return res;
	}

	if (ta_key_type == TA_HSE_EMBED_HANDLE_RSA_PRIVKEY) {
		TEE_FreeTransientObject(state->priv_key);
		state->priv_key = key;
	} else {
		TEE_FreeTransientObject(state->pub_key);
		state->pub_key = key;
	}

	return res;
}

static TEE_Result populate_ecckey(struct hse_sess *state, TEE_ObjectHandle key,
			   struct hse_embed_handle hse_handle,
			   uint32_t ta_key_type)
{
	TEE_Result res;
	const uint32_t embed_handle_size = sizeof(hse_handle);
	uint8_t dummy_point[embed_handle_size];
	TEE_Attribute attr[4];
	uint8_t attr_count = 0;

	/*
	 * Only the public value x must be populated with the actual
	 * embedded handle buffer. The public value y and the private value 
	 * are dummy buffers as the TEE Internal API requires these attributes
	 * to be part of the private/public key.
	 */
	TEE_InitValueAttribute(&attr[attr_count++], TEE_ATTR_ECC_CURVE, TEE_ECC_CURVE_NIST_P192, 0);
	TEE_InitRefAttribute(&attr[attr_count++], TEE_ATTR_ECC_PUBLIC_VALUE_X,
			     &hse_handle, embed_handle_size);
	TEE_InitRefAttribute(&attr[attr_count++], TEE_ATTR_ECC_PUBLIC_VALUE_Y,
			     dummy_point, embed_handle_size);
	if (ta_key_type == TA_HSE_EMBED_HANDLE_ECC_PRIVKEY)
		TEE_InitRefAttribute(&attr[attr_count++],
				     TEE_ATTR_ECC_PRIVATE_VALUE,
				     dummy_point, embed_handle_size);

	res = TEE_PopulateTransientObject(key, attr, attr_count);
	if (res != TEE_SUCCESS) {
		EMSG("TEE_PopulateTransientObject failed, 0x%x", res);
		return res;
	}

	if (ta_key_type == TA_HSE_EMBED_HANDLE_ECC_PRIVKEY) {
		TEE_FreeTransientObject(state->priv_key);
		state->priv_key = key;
	} else {
		TEE_FreeTransientObject(state->pub_key);
		state->pub_key = key;
	}

	return res;
}

static TEE_Result populate_aeskey(struct hse_sess *state, TEE_ObjectHandle key,
			   struct hse_embed_handle hse_handle)
{
	TEE_Result res;
	const uint32_t embed_handle_size = sizeof(hse_handle);
	TEE_Attribute attr;

	TEE_InitRefAttribute(&attr, TEE_ATTR_SECRET_VALUE,
			     &hse_handle, embed_handle_size);

	res = TEE_PopulateTransientObject(key, &attr, 1);
	if (res != TEE_SUCCESS)
		EMSG("TEE_PopulateTransientObject failed, 0x%x", res);

	TEE_FreeTransientObject(state->priv_key);
	state->priv_key = key;
	state->pub_key = TEE_HANDLE_NULL;

	return res;
}

static TEE_Result cmd_set_key(struct hse_sess *state, uint32_t pt,
				  TEE_Param params[TEE_NUM_PARAMS])
{
	TEE_Result res;
	const uint32_t exp_pt = TEE_PARAM_TYPES(TEE_PARAM_TYPE_VALUE_INPUT,
						TEE_PARAM_TYPE_NONE,
						TEE_PARAM_TYPE_NONE,
						TEE_PARAM_TYPE_NONE);
	uint32_t ta_key_type, hse_key_handle;
	TEE_ObjectHandle key;
	struct hse_embed_handle embed_handle;

	if (pt != exp_pt)
		return TEE_ERROR_BAD_PARAMETERS;

	ta_key_type = params[0].value.a;
	res = allocate_tee_obj(ta_key_type, &key);
	if (res != TEE_SUCCESS)
		return res;

	hse_key_handle = params[0].value.b;
	hse_embed_handle_init(&embed_handle, hse_key_handle);

	switch (ta_key_type) {
	case TA_HSE_EMBED_HANDLE_RSA_PUBKEY:
	case TA_HSE_EMBED_HANDLE_RSA_PRIVKEY:
		res = populate_rsakey(state, key, embed_handle, ta_key_type);
		break;

	case TA_HSE_EMBED_HANDLE_ECC_PUBKEY:
	case TA_HSE_EMBED_HANDLE_ECC_PRIVKEY:
		res = populate_ecckey(state, key, embed_handle, ta_key_type);
		break;

	case TA_HSE_EMBED_HANDLE_AES_KEY:
		res = populate_aeskey(state, key, embed_handle);
		break;

	default:
		res = TEE_ERROR_BAD_PARAMETERS;
	}

	return res;

}

static TEE_Result init_tee_op(TEE_OperationHandle *op, TEE_ObjectHandle key,
			      uint32_t key_size, uint32_t alg, uint32_t mode)
{
	TEE_Result res;

	res = TEE_AllocateOperation(op, alg, mode, key_size);
	if (res != TEE_SUCCESS) {
		EMSG("TEE_AllocateOperation(TEE_MODE_ENCRYPT, %#" PRIx32 ", %" PRId32 "): %#"
		     PRIx32, alg, key_size, res);
		return res;
	}

	res = TEE_SetOperationKey(*op, key);
	if (res) {
		EMSG("TEE_SetOperationKey: %#" PRIx32, res);
		TEE_FreeOperation(*op);
	}

	return res;
}

static TEE_Result cmd_cipher_op(struct hse_sess *state, uint32_t pt,
			       TEE_Param params[TEE_NUM_PARAMS], uint32_t mode)
{
	TEE_Result res;
	TEE_ObjectHandle key = state->priv_key;
	TEE_ObjectInfo key_info;
	TEE_OperationHandle op;
	const void *inbuf;
	void *outbuf;
	size_t inbuf_len, outbuf_len;
	const uint32_t exp_pt = TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INPUT,
						TEE_PARAM_TYPE_MEMREF_OUTPUT,
						TEE_PARAM_TYPE_NONE,
						TEE_PARAM_TYPE_NONE);

	if (pt != exp_pt)
		return TEE_ERROR_BAD_PARAMETERS;

	if (!key)
		return TEE_ERROR_BAD_STATE;

	res = TEE_GetObjectInfo1(key, &key_info);
	if (res) {
		EMSG("TEE_GetObjectInfo1: %#" PRIx32, res);
		return res;
	}

	if (key_info.objectType != TEE_TYPE_AES)
		return TEE_ERROR_BAD_STATE;

	res = init_tee_op(&op, key, key_info.maxObjectSize,
			  TEE_ALG_AES_ECB_NOPAD, mode);
	if (res != TEE_SUCCESS)
		return res;

	inbuf = params[0].memref.buffer;
	inbuf_len = params[0].memref.size;
	outbuf = params[1].memref.buffer;
	outbuf_len = params[1].memref.size;

	if (outbuf_len < inbuf_len)
		return TEE_ERROR_BAD_PARAMETERS;

	TEE_CipherInit(op, NULL, 0);
	res = TEE_CipherUpdate(op, inbuf, inbuf_len, outbuf, &outbuf_len);
	if (res != TEE_SUCCESS)
		EMSG("TEE_CipherUpdate(%" PRId64 ", %" PRId64 "): %#"
		     PRIx32, inbuf_len, outbuf_len, res);

	params[1].memref.size = outbuf_len;

	TEE_FreeOperation(op);
	return res;
}

static TEE_Result cmd_asymm_enc_dec_op(struct hse_sess *state, uint32_t pt,
			       	       TEE_Param params[TEE_NUM_PARAMS],
				       uint32_t mode)
{
	TEE_Result res;
	TEE_ObjectHandle key;
	TEE_ObjectInfo key_info;
	TEE_OperationHandle op;
	const void *inbuf;
	void *outbuf;
	size_t inbuf_len, outbuf_len;
	const uint32_t exp_pt = TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INPUT,
						TEE_PARAM_TYPE_MEMREF_OUTPUT,
						TEE_PARAM_TYPE_NONE,
						TEE_PARAM_TYPE_NONE);

	if (pt != exp_pt)
		return TEE_ERROR_BAD_PARAMETERS;

	if (mode == TEE_MODE_ENCRYPT)
		key = state->pub_key;
	else
		key = state->priv_key;

	if (!key)
		return TEE_ERROR_BAD_STATE;

	res = TEE_GetObjectInfo1(key, &key_info);
	if (res) {
		EMSG("TEE_GetObjectInfo1: %#" PRIx32, res);
		return res;
	}

	if (key_info.objectType != TEE_TYPE_RSA_KEYPAIR &&
	    key_info.objectType != TEE_TYPE_RSA_PUBLIC_KEY)
		return TEE_ERROR_BAD_STATE;

	res = init_tee_op(&op, key, key_info.maxObjectSize,
			  TEE_ALG_RSA_NOPAD, mode);
	if (res != TEE_SUCCESS)
		return res;

	inbuf = params[0].memref.buffer;
	inbuf_len = params[0].memref.size;
	outbuf = params[1].memref.buffer;
	outbuf_len = params[1].memref.size;

	if (mode == TEE_MODE_ENCRYPT)
		res = TEE_AsymmetricEncrypt(op, NULL, 0, inbuf, inbuf_len, outbuf,
				    &outbuf_len);
	else
		res = TEE_AsymmetricDecrypt(op, NULL, 0, inbuf, inbuf_len, outbuf,
				    &outbuf_len);
	if (res != TEE_SUCCESS)
		EMSG("TEE_AsymmetricEncrypt(%" PRId64 ", %" PRId64 "): %#"
		     PRIx32, inbuf_len, outbuf_len, res);

	params[1].memref.size = outbuf_len;

	TEE_FreeOperation(op);
	return res;
}

static TEE_Result cmd_sign_verify_op(struct hse_sess *state, uint32_t pt,
				     TEE_Param params[TEE_NUM_PARAMS],
				     uint32_t mode)
{
	TEE_Result res;
	TEE_ObjectHandle key;
	TEE_ObjectInfo key_info;
	TEE_OperationHandle op;
	const void *inbuf;
	void *outbuf;
	size_t inbuf_len, outbuf_len;
	uint32_t alg;
	const uint32_t exp_pt = TEE_PARAM_TYPES(TEE_PARAM_TYPE_MEMREF_INPUT,
						TEE_PARAM_TYPE_MEMREF_OUTPUT,
						TEE_PARAM_TYPE_NONE,
						TEE_PARAM_TYPE_NONE);

	if (pt != exp_pt)
		return TEE_ERROR_BAD_PARAMETERS;

	if (mode == TEE_MODE_SIGN)
		key = state->priv_key;
	else
		key = state->pub_key;

	if (!key)
		return TEE_ERROR_BAD_STATE;

	res = TEE_GetObjectInfo1(key, &key_info);
	if (res) {
		EMSG("TEE_GetObjectInfo1: %#" PRIx32, res);
		return res;
	}

	switch (key_info.objectType) {
	case TEE_TYPE_RSA_KEYPAIR:
	case TEE_TYPE_RSA_PUBLIC_KEY:
		alg = TEE_ALG_RSASSA_PKCS1_V1_5_SHA1;
		break;
	case TEE_TYPE_ECDSA_KEYPAIR:
	case TEE_TYPE_ECDSA_PUBLIC_KEY:
		alg = TEE_ALG_ECDSA_SHA1;
		break;

	default:
		return TEE_ERROR_BAD_STATE;
	}

	/*
	 * key_info.maxObjectSize provided as the third argument would not work
	 * because the key object's max size is 256 bits and ECC P-192 operation
	 * would only allow key objects with a maximum size of 192 bits. Provide
	 * the objectSize instead.
	 */
	res = init_tee_op(&op, key, key_info.objectSize,
			  alg, mode);
	if (res != TEE_SUCCESS)
		return res;

	inbuf = params[0].memref.buffer;
	inbuf_len = params[0].memref.size;
	outbuf = params[1].memref.buffer;
	outbuf_len = params[1].memref.size;

	if (mode == TEE_MODE_SIGN) {
		res = TEE_AsymmetricSignDigest(op, NULL, 0, inbuf, inbuf_len, outbuf,
					       &outbuf_len);
		params[1].memref.size = outbuf_len;
	} else {
		res = TEE_AsymmetricVerifyDigest(op, NULL, 0, inbuf, inbuf_len, outbuf,
				    		 outbuf_len);
	}

	if (res != TEE_SUCCESS)
		EMSG("TEE_AsymmetricSignDigest(%" PRId64 ", %" PRId64 "): %#"
		     PRIx32, inbuf_len, outbuf_len, res);

	TEE_FreeOperation(op);
	return res;
}

TEE_Result TA_CreateEntryPoint(void)
{
	/* Nothing to do */
	return TEE_SUCCESS;
}

void TA_DestroyEntryPoint(void)
{
	/* Nothing to do */
}

TEE_Result TA_OpenSessionEntryPoint(uint32_t __unused param_types,
				    TEE_Param __unused params[4],
				    void **session)
{
	struct hse_sess *state;

	/*
	 * Allocate and init state for the session.
	 */
	state = TEE_Malloc(sizeof(*state), 0);
	if (!state)
		return TEE_ERROR_OUT_OF_MEMORY;

	state->pub_key = TEE_HANDLE_NULL;
	state->priv_key = TEE_HANDLE_NULL;

	*session = state;

	return TEE_SUCCESS;
}

void TA_CloseSessionEntryPoint(void *session)
{
	struct hse_sess *state = session;

	TEE_FreeTransientObject(state->pub_key);
	TEE_FreeTransientObject(state->priv_key);
	TEE_Free(state);
}

TEE_Result TA_InvokeCommandEntryPoint(void *session, uint32_t cmd,
				      uint32_t param_types,
				      TEE_Param params[TEE_NUM_PARAMS])
{
	switch (cmd) {
	case TA_HSE_EMBED_HANDLE_CMD_SET_KEY:
		return cmd_set_key(session, param_types, params);
	case TA_HSE_EMBED_HANDLE_CMD_CIPHER_ENCRYPT:
		return cmd_cipher_op(session, param_types, params,
				    TEE_MODE_ENCRYPT);
	case TA_HSE_EMBED_HANDLE_CMD_CIPHER_DECRYPT:
		return cmd_cipher_op(session, param_types, params,
				    TEE_MODE_DECRYPT);
	case TA_HSE_EMBED_HANDLE_CMD_ASYMM_ENCRYPT:
		return cmd_asymm_enc_dec_op(session, param_types, params,
				    	 TEE_MODE_ENCRYPT);
	case TA_HSE_EMBED_HANDLE_CMD_ASYMM_DECRYPT:
		return cmd_asymm_enc_dec_op(session, param_types, params,
					    TEE_MODE_DECRYPT);
	case TA_HSE_EMBED_HANDLE_CMD_SIGN:
		return cmd_sign_verify_op(session, param_types, params,
					  TEE_MODE_SIGN);
	case TA_HSE_EMBED_HANDLE_CMD_VERIFY:
		return cmd_sign_verify_op(session, param_types, params,
					  TEE_MODE_VERIFY);
	default:
		EMSG("Command ID %#" PRIx32 " is not supported", cmd);
		return TEE_ERROR_NOT_SUPPORTED;
	}
}
