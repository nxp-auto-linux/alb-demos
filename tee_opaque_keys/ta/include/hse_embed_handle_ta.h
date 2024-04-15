/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright 2024 NXP
 */


#ifndef __HSE_EMBED_HANDLE_TA_H__
#define __HSE_EMBED_HANDLE_TA_H__

/* UUID of the embed handle example trusted application */
#define TA_HSE_EMBED_HANDLE_UUID \
	{ 0x5fe52085, 0x5121, 0x4cdf, \
		{ 0xb8, 0x00, 0x49, 0x64, 0xb8, 0xdd, 0x0c, 0x94} }

#define TA_HSE_EMBED_HANDLE_RSA_PUBKEY		0
#define TA_HSE_EMBED_HANDLE_RSA_PRIVKEY		1
#define TA_HSE_EMBED_HANDLE_ECC_PUBKEY		2
#define TA_HSE_EMBED_HANDLE_ECC_PRIVKEY		3
#define TA_HSE_EMBED_HANDLE_AES_KEY		4

/*
 * TA_HSE_EMBED_HANDLE_CMD_SET_KEY - Sets the key for a cryptographic operation
 * in	params[0].value.a: TA_HSE_EMBED_HANDLE_*
 * in	params[0].value.b: HSE Key Handle
 */
#define TA_HSE_EMBED_HANDLE_CMD_SET_KEY			0

/*
 * TA_HSE_EMBED_HANDLE_CMD_CIPHER_ENCRYPT - Encrypts the plaintext using AES-ECB
 * in	params[0].memref  plaintext
 * out	params[1].memref  ciphertext
 */
#define TA_HSE_EMBED_HANDLE_CMD_CIPHER_ENCRYPT		1

/*
 * TA_HSE_EMBED_HANDLE_CMD_CIPHER_DECRYPT - Decrypts the ciphertext using AES-ECB
 * in	params[0].memref  ciphertext
 * out	params[1].memref  plaintext
 */
#define TA_HSE_EMBED_HANDLE_CMD_CIPHER_DECRYPT		2

/*
 * TA_HSE_EMBED_HANDLE_CMD_ASYMM_ENCRYPT - Encrypts the plaintext using RSA_NOPAD
 * in	params[0].memref: plaintext
 * out	params[1].memref  ciphertext
 */
#define TA_HSE_EMBED_HANDLE_CMD_ASYMM_ENCRYPT		3

/*
 * TA_HSE_EMBED_HANDLE_CMD_ASYMM_DECRYPT - Decrypts the ciphertext using RSA_NOPAD
 * in	params[0].memref  ciphertext
 * out	params[1].memref  plaintext
 */
#define TA_HSE_EMBED_HANDLE_CMD_ASYMM_DECRYPT		4

/*
 * TA_HSE_EMBED_HANDLE_CMD_SIGN - Generates a signature of the provided digest using ECDSA-SHA1
 * in	params[0].memref  digest
 * out	params[1].memref  signature
 */
#define TA_HSE_EMBED_HANDLE_CMD_SIGN			5

/*
 * TA_HSE_EMBED_HANDLE_CMD_VERIFY - Verifies the provided signature using ECDSA-SHA1
 * in	params[0].memref  digest
 * in	params[1].memref  signature
 */
#define TA_HSE_EMBED_HANDLE_CMD_VERIFY			6

#endif /* __ACIPHER_TA_H */
