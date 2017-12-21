/**********************************************************************
 * Copyright (C) 2014 Intel Corporation. All rights reserved.

 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at

 * http://www.apache.org/licenses/LICENSE-2.0

 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 **********************************************************************/


#ifndef __DRM_WV_MOD_LIB__
#define __DRM_WV_MOD_LIB__


#include <inttypes.h>
#include <stdbool.h>
#include "drm_wv_mod_lib_error.h"


//
// Macro for converting a SEP message parameter length in bytes so
// that the value is aligned to a four byte boundary. The maximum
// SEP message parameter length in bytes is required to be word
// aligned.
//
#define BYTES_TO_4BYTE_BLOCKS(x) ( ( (x) + 3 ) / 4 )
#define SEP_MSG_PARAM_MAX_LEN(x) ( BYTES_TO_4BYTE_BLOCKS(x) * 4 )

//
// Macro for calculating the maximum number of bytes for a byte array in the
// IA-to-Chaabi SEP message after the SEP message parameters. Calculation is
// done by passing in the number of non byte array SEP message parameters.
//
#define SEP_MSG_MAX_BYTE_ARRAY_SIZE_IN_BYTES(params) ( SEP_MESSAGE_MAX_SIZE - ( (params) * sizeof( DxUint32_t ) ) )

//
// Flags for if data being written to SEP message shared memory is a byte array
// or parameters.
//
#define SEP_MESSAGE_BYTE_ARRAY ( DX_TRUE )
#define SEP_MESSAGE_PARAMS     ( DX_FALSE )

//
// Do not use secure DMA when making Fastcall SEP driver calls.
//
enum
{
    FASTCALL_NOT_SECURE_DMA = 0,
    FASTCALL_SECURE_DMA     = 1
};

//
// DMA Data Control Block (DCB) will not be used with secure token Fastcall SEP
// driver calls.
//
enum
{
    NO_FASTCALL_DCB = 0
};


//
// Modular Widevine DRM key object data type.
//
typedef struct drm_wv_mod_key_object
{
	const uint8_t *pKey_id;
	size_t        key_id_size_in_bytes;
	const uint8_t *pKey_data_iv;
	const uint8_t *pKey_data;
	size_t        key_data_size_in_bytes;
	const uint8_t *pKey_control_iv;
	const uint8_t *pKey_control;
} drm_wv_mod_key_object_t;


//
// Modular Widevine key refresh object data type.
//
typedef struct drm_wv_mod_key_refresh_object
{
	const uint8_t *pKey_id;
	size_t        key_id_size_in_bytes;
	const uint8_t *pKey_control_iv;
	const uint8_t *pKey_control;
} drm_wv_mod_key_refresh_object_t;


//
// Modular Widevine DRM buffer type data type.
//
typedef enum drm_wv_mod_buffer_type
{
	DRM_WV_MOD_BUFFERTYPE_CLEAR,
	DRM_WV_MOD_BUFFERTYPE_SECURE,
	DRM_WV_MOD_BUFFERTYPE_DIRECT
} drm_wv_mod_buffer_type_t;


//
// Modular Widevine DRM buffer description data type.
//
typedef struct drm_wv_mod_dest_buffer_desc
{
	drm_wv_mod_buffer_type_t type;
	union
	{
		struct
		{ // type == DRM_WV_MOD_BUFFERTYPE_CLEAR
			uint8_t *pAddress;
			size_t max_size_in_bytes;
		} clear;

		struct
		{ // type == DRM_WV_MOD_BUFFERTYPE_SECURE
			size_t imr_byte_offset;
			size_t max_size_in_bytes;
		} secure;

		struct
		{ // type == DRM_WV_MOD_BUFFERTYPE_DIRECT
			bool is_video;
		} direct;
	} buffer;
} drm_wv_mod_dest_buffer_desc_t;


//
// Modular Widevine DRM sub sample bit flags.
//
typedef enum drm_wv_mod_decrypt_subsample_flags
{
	DRM_WV_MOD_NEITHER_FIRST_NOR_LAST_SUB_SAMPLE = 0,
	DRM_WV_MOD_FIRST_SUB_SAMPLE                  = ( 1 << 0 ),
	DRM_WV_MOD_LAST_SUB_SAMPLE                   = ( 1 << 1 ),
	DRM_WV_MOD_BOTH_FIRST_AND_LAST_SUB_SAMPLE    = ( DRM_WV_MOD_FIRST_SUB_SAMPLE | \
	                                                 DRM_WV_MOD_LAST_SUB_SAMPLE )
} drm_wv_mod_decrypt_subsample_flags_t;


//
// Modular Widevine algorithms for generic encryption, decryption, signing, and verifying.
//
typedef enum drm_wv_mod_algorithm
{
	DRM_WV_MOD_AES_CBC_128_NO_PADDING = 0,
	HMAC_SHA256                       = 1
} drm_wv_mod_algorithm_t;


//
// Modular Widevine Load Keys encrypted MAC keys size in bytes.
//
enum
{
	WMLK_ENC_MAC_KEYS_SIZE_IN_BYTES = 64
};

//
// DRM_WV_MOD_NULL_POINTER_TOKEN - SEP message value that indicates
//	KeyObject pointer is NULL
//
enum
{
	DRM_WV_MOD_NULL_POINTER_TOKEN = ((uint32_t)(-1))
};
enum
{
	DRM_WV_MAXIMUM_USER_CONTEXT_SIZE = (8*1024)
};

enum drm_wv_mod_rsa_padding_scheme
{
	DRM_WV_MOD_RSA_PADDING_SCHEME_RSASSA_PSS_SHA1 = 1,
	DRM_WV_MOD_RSA_PADDING_SCHEME_RSASSA_PKCS1v15,
	DRM_WV_MOD_RSA_PADDING_SCHEME_MAX_VALUE
};

/**
 * @brief Initializes cryptographic hardware.
 *
 * @return DRM_WV_MOD_SUCCESS The cryptographic hardware was initialized.
 */
drm_wv_mod_result_t drm_wv_mod_initialize( void );


/**
 * @brief Closes the cryptographic hardware.
 *
 * @return DRM_WV_MOD_SUCCESS The cryptographic hardware was closed.
 */
drm_wv_mod_result_t drm_wv_mod_terminate( void );


/**
 * @brief Opens a new security engine context.
 *
 * @param[out] pSession_id Pointer to return buffer for the new session context ID.
 *
 * @return DRM_WV_MOD_SUCCESS Opening a new security engine context succeeded.
 */
drm_wv_mod_result_t drm_wv_mod_open_session( uint32_t * const pSession_id );


/**
 * @brief Closes an existing security engine context.
 *
 * @param[in] session_id ID number of an open session.
 *
 * @return DRM_WV_MOD_SUCCESS Closing the session succeeded.
 */
drm_wv_mod_result_t drm_wv_mod_close_session( const uint32_t session_id );


/**
 * @brief Generates keys for handling signing and content key decryption.
 *
 * @param[in] session_id ID number of an open session.
 *
 * @param[in] pMac_key_context Pointer to buffer with context data for HMAC
 *            generation key.
 *
 * @param[in] mac_key_context_size_in_bytes Length of the pMac_key_context
 *            buffer in bytes.
 *
 * @param[in] pEnc_key_context Pointer to buffer with context data for
 *            generating encryption key.
 *
 * @param[in] enc_key_context_size_in_bytes Length of the pEnc_key_context
 *            buffer in bytes.
 *
 * @return DRM_WV_MOD_SUCCESS Generating the keys succeeded.
 */
drm_wv_mod_result_t drm_wv_mod_generate_derived_keys( const uint32_t session_id,
                                                      const uint8_t * const pMac_key_context,
                                                      const uint32_t mac_key_context_size_in_bytes,
                                                      const uint8_t * const pEnc_key_context,
                                                      const uint32_t enc_key_context_size_in_bytes );


/**
 * @brief Generates a nonce to be used with drm_wv_mod_LoadKeys().
 *
 * @param[in] session_id ID number of an open session.
 *
 * @param[out] pNonce Pointer to the return buffer for the nonce.
 *
 * @return DRM_WV_MOD_SUCCESS Generating a nonce succeeded.
 *
 * The last four nonces that are generated with this API are also stored in
 * secure storage.
 */
drm_wv_mod_result_t drm_wv_mod_generate_nonce( const uint32_t session_id,
                                               uint32_t * const pNonce );


/**
 * @brief Generates a SHA-2 256 HMAC signature of a message.
 *
 * @param[in] session_id ID number of an open session.
 *
 * @param[in] pMessage Pointer to the message to generate a signature of.
 *
 * @param[in] message_size_in_bytes Length of the message in bytes.
 *
 * @param[out] pSignature Pointer to a buffer with the returned SHA-2 256 HMAC
 *             signature of the message.
 *
 * @param[in|out] pSignature_size_in_bytes Pointer to a buffer that on input
 *                contains the size of the return signature buffer in bytes and
 *                on output contains the size of the returned signature.
 *
 * @return DRM_WV_MOD_SUCCESS Generating the signature succeeded.
 *
 * The API drm_wv_mod_GenerateDerivedKeys() must be called first to create the
 * key used for the HMAC.
 */
drm_wv_mod_result_t drm_wv_mod_generate_signature( const uint32_t session_id,
                                                   const uint8_t * const pMessage,
                                                   const size_t message_size_in_bytes,
                                                   uint8_t * const pSignature,
                                                   size_t * const pSignature_size_in_bytes );


/**
 * @brief Loads a set of keys for decryption within the session.
 *
 * @param[in] session_id ID number of an open session.
 *
 * @param[in] pMessage Pointer to the message to be verified.
 *
 * @param[in] message_size_in_bytes Length of the message in bytes.
 *
 * @param[in] pSignature SHA-2 256 HMAC signature of the message.
 *
 * @param[in] signature_size_in_bytes Length of the signature in bytes.
 *
 * @param[in] pEnc_mac_keys_iv Pointer to buffer containing 128-bit IV for
 *            decrypting new keys.
 *
 * @param[in] pEnc_mac_keys Pointer to buffer containing encrypted keys.
 *
 * @param[in] num_keys Number of keys present.
 *
 * @param[in] pKey_array Pointer to a buffer containing a set of keys to
 *            install.
 *
 * @return DRM_WV_MOD_SUCCESS Loading the keys succeeded.
 *
 * Size of pEnc_mac_keys buffer is defined in the
 * WMLK_ENC_MAC_KEYS_SIZE_IN_BYTES constant.
 */
drm_wv_mod_result_t drm_wv_mod_load_keys( const uint32_t session_id,
                                          const uint8_t * const pMessage,
                                          const size_t message_size_in_bytes,
                                          const uint8_t * const pSignature,
                                          const size_t signature_size_in_bytes,
                                          const uint8_t * const pEnc_mac_keys_iv,
                                          const uint8_t * const pEnc_mac_keys,
                                          const size_t num_keys,
                                          const drm_wv_mod_key_object_t * const pKey_array );


/**
 * @brief Updates an existing set of keys for continuing decryption in the
 * current session.
 *
 * @param[in] session_id ID number of an open session.
 *
 * @param[in] pMessage Pointer to the message to be verified.
 *
 * @param[in] message_size_in_bytes Length of the message in bytes.
 *
 * @param[in] pSignature SHA-2 256 HMAC signature of the message.
 *
 * @param[in] signature_size_in_bytes Length of the signature in bytes.
 *
 * @param[in] num_keys Number of keys present.
 *
 * @param[in] pKey_array Pointer to a buffer containing a set of keys to install.
 *
 * @return DRM_WV_MOD_SUCCESS Refreshing the keys succeeded.
 */
drm_wv_mod_result_t drm_wv_mod_refresh_keys( const uint32_t session_id,
                                             const uint8_t * const pMessage,
                                             const size_t message_size_in_bytes,
                                             const uint8_t* pSignature,
                                             const size_t signature_size_in_bytes,
                                             const size_t num_keys,
                                             const drm_wv_mod_key_refresh_object_t * const pKey_array );


/**
 * @brief Select a content key and install it in the hardware key ladder for
 *        subsequent decryption operations in this session.
 *
 * @param[in] session_id ID number of an open session.
 *
 * @param[in] pKey_id Pointer to the key ID.
 *
 * @param[in] key_id_size_in_bytes Length of the key ID in bytes.
 *
 * @return DRM_WV_MOD_SUCCESS Selecting a key succeeded.
 */
drm_wv_mod_result_t drm_wv_mod_select_key( const uint32_t session_id,
                                           const uint8_t * const pKey_id,
                                           const size_t key_id_size_in_bytes );


/**
 * @brief Select a content key and install it in the hardware key ladder for
 *        subsequent decryption operations in this session.
 *
 * @param[in] session_id ID number of an open session.
 *
 * @param[in] pData_addr Pointer to the buffer with data to decrypt.
 *
 * @param[in] data_size_in_bytes Length of the data buffer in bytes.
 *
 * @param[in] is_encrypted Boolean flag signifying if the data in the buffer is
 *            encrypted or not.
 *
 * @param[in] pIv Pointer to a buffer containing the 128-bit IV for decrypting
 *            ciphertext data.
 *
 * @param[in] block_offset_in_bytes Decryption block byte offset.
 *
 * @param[in] pOut_buffer Pointer to a buffer for the return decrypted data.
 *
 * @param[in] subsample_flags Bit flags for type of subsample.
 *
 * @return DRM_WV_MOD_SUCCESS Decryption succeeded.
 *
 * If is_encrypted is set to true then the data buffer contains encrypted
 * data. If the flag is false then only the pData_addr and data_size_in_bytes
 * parameters are used (the pIv and block_offset parameters are ignored).
 *
 * On CTP, the block_offset and subsample_flags parameters will be ignored.
 */
drm_wv_mod_result_t drm_wv_mod_decrypt_ctr( const uint32_t session_id,
                                            const uint8_t * const pData_addr,
                                            const size_t data_size_in_bytes,
                                            const bool is_encrypted,
                                            const uint8_t * const pIv,
                                            size_t block_offset_in_bytes,
                                            const drm_wv_mod_dest_buffer_desc_t * const pOut_buffer,
                                            drm_wv_mod_decrypt_subsample_flags_t subsample_flags );


/**
 * @brief Verifies an RSA provisioning response is valid and corresponds to the
 *        previous provisioning request by checking the nonce.
 *
 * @param[in] session_id ID number of an open session.
 *
 * @param[in] pMessage Pointer to buffer containing message to be verified.
 *
 * @param[in] message_size_in_bytes Length of the message in bytes.
 *
 * @param[in] pSignature Pointer to buffer containing SHA-2 256 HMAC signature
 *            of the message.
 *
 * @param[in] signature_size_in_bytes Length of the signature in bytes.
 *
 * @param[in] pNonce Pointer to buffer containing the nonce provided in the
 *            provisioning response.
 *
 * @param[in] pEnc_rsa_key Pointer to a buffer containing encrypted device
 *            private RSA key received from the provisioning server.
 *
 * @param[in] enc_rsa_key_size_in_bytes Length of the encrypted RSA key in
 *            bytes.
 *
 * @param[in] pEnc_rsa_key_iv Pointer to buffer containing 128-bit IV for
 *            decrypting RSA key.
 *
 * @param[out] pWrapped_rsa_key Pointer to a buffer containing returned
 *             wrapped encrypted RSA key.
 *
 * @param[in|out] pWrapped_rsa_key_size_in_bytes Pointer to a buffer whose
 *                input value is the size of the encrypted RSA key return
 *                buffer in bytes and whose output value is the length of the
 *                returned encrypted RSA key in bytes.
 *
 * @return DRM_WV_MOD_SUCCESS Rewrapping the device RSA key succeeded.
 */
drm_wv_mod_result_t drm_wv_mod_rewrap_device_rsa_key( const uint32_t session_id,
                                                      const uint8_t * const pMessage,
                                                      const size_t message_size_in_bytes,
                                                      const uint8_t * const pSignature,
                                                      const size_t signature_size_in_bytes,
                                                      const uint32_t * const pNonce,
                                                      const uint8_t * const pEnc_rsa_key,
                                                      const size_t enc_rsa_key_size_in_bytes,
                                                      const uint8_t * const pEnc_rsa_key_iv,
                                                      uint8_t * const pWrapped_rsa_key,
                                                      size_t * const pWrapped_rsa_key_size_in_bytes );


/**
 * @brief Loads a wrapped RSA private key to secure memory for use by this
 *        session in future calls to drm_wv_mod_generate_signature().
 *
 * @param[in] session_id ID number of an open session.
 *
 * @param[in] pWrapped_rsa_key Pointer to buffer containing wrapped device RSA
 *            key stored on the device generated by
 *            drm_wv_mod_rewrap_device_rsa_key().
 *
 * @param[in] wrapped_rsa_key_size_in_bytes Length of the wrapped RSA key in
 *            bytes.
 *
 * @return DRM_WV_MOD_SUCCESS Loading the wrapped RSA key succeeded.
 */
drm_wv_mod_result_t drm_wv_mod_load_device_rsa_key( const uint32_t session_id,
                                                    const uint8_t * const pWrapped_rsa_key,
                                                    const size_t wrapped_rsa_key_size_in_bytes );


/**
 * @brief Sign messages using the device private RSA key.
 *
 * @param[in] session_id ID number of an open session.
 *
 * @param[in] pMessage Pointer to buffer containing the message to be signed.
 *
 * @param[in] message_size_in_bytes Length of the message in bytes.
 *
 * @param[out] pSignature Pointer to return buffer containing the returned
 *             signature of the message signed with the device private RSA key.
 *
 * @param[in|out] pSignature_size_in_bytes Pointer to a buffer whose input
 *                value is the size of the signature return buffer in bytes and
 *                whose output value is the length of the returned message
 *                signature in bytes.
 *
 * @return DRM_WV_MOD_SUCCESS Signing the message succeeded.
 */
drm_wv_mod_result_t drm_wv_mod_generate_rsa_signature( const uint32_t session_id,
                                                       const uint8_t * const pMessage,
                                                       const size_t message_size_in_bytes,
                                                       uint8_t * const pSignature,
                                                       size_t * const pSignature_size_in_bytes );


/**
 * @brief Sign messages using the device private RSA key for supporting modular
 * widevine version 9 or later specification
 *
 * @param[in] session_id ID number of an open session.
 *
 * @param[in] pMessage Pointer to buffer containing the message to be signed.
 *
 * @param[in] message_size_in_bytes Length of the message in bytes.
 *
 * @param[out] pSignature Pointer to return buffer containing the returned
 *             signature of the message signed with the device private RSA key.
 *
 * @param[in|out] pSignature_size_in_bytes Pointer to a buffer whose input
 *                value is the size of the signature return buffer in bytes and
 *                whose output value is the length of the returned message
 *                signature in bytes.
 *
 * @param[in] padding_scheme Padding scheme that FW should use for input data.
 *
 * @return DRM_WV_MOD_SUCCESS Signing the message succeeded.
 */
drm_wv_mod_result_t drm_wv_mod_v9_generate_rsa_signature( const uint32_t session_id,
                                                          const uint8_t * const pMessage,
                                                          const size_t message_size_in_bytes,
                                                          uint8_t * const pSignature,
                                                          size_t * const pSignature_size_in_bytes,
                                                          const enum drm_wv_mod_rsa_padding_scheme padding_scheme );

/**
 * @brief Generates three secondary keys, server mac_key, client mac_key and
 *        encrypt_key, for handling signing and content key decryption under
 *        the license server protocol for AES CTR mode.
 *
 * @param[in] session_id ID number of an open session.
 *
 * @param[in] pEnc_session_key Pointer to buffer containing the session key
 *            encrypted with the device RSA key.
 *
 * @param[in] enc_session_key_size_in_bytes Length of the encrypted session key in bytes.
 *
 * @param[out] pMac_key_context Pointer to return buffer containing the context
 *             data for computing the HMAC generation key.
 *
 * @param[in] mac_key_context_size_in_bytes Length of the HMAC generation key
 *            context data in bytes.
 *
 * @param[in] pEnc_key_context Pointer to return buffer containing the context
 *            data for computing the encryption key.
 *
 * @param[in] enc_key_context_size_in_bytes Length of the encryption key
 *            context data in bytes.
 *
 * @return DRM_WV_MOD_SUCCESS The secondary keys generation succeeded.
 */
drm_wv_mod_result_t drm_wv_mod_derive_keys_from_session_key( const uint32_t session_id,
                                                             const uint8_t * const pEnc_session_key,
                                                             const size_t enc_session_key_size_in_bytes,
                                                             const uint8_t * const pMac_key_context,
                                                             const size_t mac_key_context_size_in_bytes,
                                                             const uint8_t * const pEnc_key_context,
                                                             const size_t enc_key_context_size_in_bytes );


/**
 * @brief Generic encryption of a buffer of data using the current key.
 *
 * @param[in] session_id ID number of an open session.
 *
 * @param[in] pIn_buffer Pointer to buffer containing the plaintext data to
 *            encrypt.
 *
 * @param[in] buffer_size_in_bytes Length of the plaintext data buffer in bytes
 *            (encryption algorithm may restrict this size to a multiple of the
 *            algorithm's block size).
 *
 * @param[in] pIv Pointer to buffer containing 128-bit IV for encryption.
 *
 * @param[in] algorithm Specifies which encryption algorithm to use.
 *
 * @param[out] pOut_buffer Pointer to return buffer containing the encrypted
 *             ciphertext data. The size of this buffer must be at least the
 *             same size as the input buffer.
 *
 * @return DRM_WV_MOD_SUCCESS Generic encryption succeeded.
 */
drm_wv_mod_result_t drm_wv_mod_generic_encrypt( const uint32_t session_id,
                                                const uint8_t * const pIn_buffer,
                                                const size_t buffer_size_in_bytes,
                                                const uint8_t * const pIv,
                                                const drm_wv_mod_algorithm_t algorithm,
                                                uint8_t * const pOut_buffer );


/**
 * @brief Generic decryption of a buffer of data using the current key.
 *
 * @param[in] session_id ID number of an open session.
 *
 * @param[in] pIn_buffer Pointer to buffer containing the ciphertext data to
 *            decrypt.
 *
 * @param[in] buffer_size_in_bytes Length of the ciphertext data buffer in bytes
 *            (decryption algorithm may restrict this size to a multiple of the
 *            algorithm's block size).
 *
 * @param[in] pIv Pointer to buffer containing 128-bit IV for decryption.
 *
 * @param[in] algorithm Specifies which decryption algorithm to use.
 *
 * @param[out] pOut_buffer Pointer to return buffer containing the decrypted
 *             plaintext data.
 *
 * @return DRM_WV_MOD_SUCCESS Generic decryption succeeded.
 */
drm_wv_mod_result_t drm_wv_mod_generic_decrypt( const uint32_t session_id,
                                                const uint8_t * const pIn_buffer,
                                                const size_t buffer_size_in_bytes,
                                                const uint8_t * const pIv,
                                                const drm_wv_mod_algorithm_t algorithm,
                                                uint8_t * const pOut_buffer );


/**
 * @brief Generic signing of a buffer of unsigned data using the current key.
 *
 * @param[in] session_id ID number of an open session.
 *
 * @param[in] pIn_buffer Pointer to buffer containing the unsigned data to sign.
 *
 * @param[in] buffer_size_in_bytes Length of the unsigned data buffer in bytes.
 *
 * @param[in] algorithm Specifies which signing algorithm to use.
 *
 * @param[out] pSignature Pointer to return buffer containing the signature of the data.
 *
 * @param[in|out] pSignature_size_in_bytes Pointer to a buffer whose input
 *                value is the size of the signature return buffer in bytes and
 *                whose output value is the length of the returned message
 *                signature in bytes.
 *
 * @return DRM_WV_MOD_SUCCESS Generic signing succeeded.
 */
drm_wv_mod_result_t drm_wv_mod_generic_sign( const uint32_t session_id,
                                             const uint8_t * const pIn_buffer,
                                             const size_t buffer_size_in_bytes,
                                             const drm_wv_mod_algorithm_t algorithm,
                                             uint8_t * const pSignature,
                                             size_t * const pSignature_size_in_bytes );


/**
 * @brief Generic verification of a buffer of signed data using the current key.
 *
 * @param[in] session_id ID number of an open session.
 *
 * @param[in] pIn_buffer Pointer to buffer containing the signed data to verify.
 *
 * @param[in] buffer_size_in_bytes Length of the signed data buffer in bytes.
 *
 * @param[in] algorithm Specifies which verifying algorithm to use.
 *
 * @param[in] pSignature Pointer to buffer containing the signature for
 *            verification.
 *
 * @param[in] signature_size_in_bytes Size of the buffer containing the
 *            signature in bytes.
 *
 * @return DRM_WV_MOD_SUCCESS Generic verification succeeded.
 */
drm_wv_mod_result_t drm_wv_mod_generic_verify( const uint32_t session_id,
                                               const uint8_t * const pIn_buffer,
                                               const size_t buffer_size_in_bytes,
                                               const drm_wv_mod_algorithm_t algorithm,
                                               const uint8_t * const pSignature,
                                               const size_t signature_size_in_bytes );


/**
 * @brief Returns parsed NALU headers.
 *
 * @param[in] session_id ID number of an open session.
 *
 * @param[in] imr_byte_offset Byte offset in the IMR of the NALUs.
 *
 * @param[in] frame_size_in_bytes Size of the frame in bytes.
 *
 * @param[in] pHeader_byte_offsets Pointer to a buffer of NALU header byte
 *            offsets.
 *
 * @param[in] number_of_header_byte_offsets Number of NALU header byte offsets.
 *
 * @param[out] pNalu_header_buffer Pointer to a buffer for returning the parsed
 *             NALU headers.
 *
 * @param[in|out] pNalu_header_buffer_size_in_bytes Pointer to a buffer whose
 *                input value is the size of the NALU return buffer in bytes and
 *                whose output value is the length of the returned NALU headers
 *                in bytes.
 *
 * @return DRM_WV_MOD_SUCCESS Returning NALU headers succeeded.
 */
drm_wv_mod_result_t drm_wv_mod_return_nalu_headers( const uint32_t session_id,
                                                    const uint32_t imr_byte_offset,
                                                    const uint32_t frame_size_in_bytes,
                                                    const uint32_t * const pHeader_byte_offsets,
                                                    const uint32_t number_of_header_byte_offsets,
                                                    uint8_t * const pNalu_header_buffer,
                                                    uint32_t * const pNalu_header_buffer_size_in_bytes );


/**
 * @brief Enables a modular Widevine session for playback.
 *
 * @param[in] session_id ID number of an open session.
 *
 * @return DRM_WV_MOD_SUCCESS Start playback succeeded.
 */
drm_wv_mod_result_t drm_wv_mod_start_playback( const uint32_t session_id );


/**
 * @brief Disables a modular Widevine session for playback.
 *
 * @param[in] session_id ID number of an open session.
 *
 * @return DRM_WV_MOD_SUCCESS End playback succeeded.
 */
drm_wv_mod_result_t drm_wv_mod_end_playback( const uint32_t session_id );

// TODO: Documentation
drm_wv_mod_result_t drm_wv_mod_v9_load_keys( const uint32_t session_id,
								  const uint8_t * const pMessage,
								  const size_t message_size_in_bytes,
								  const uint8_t * const pSignature,
								  const size_t signature_size_in_bytes,
								  const uint8_t * const pEnc_mac_keys_iv,
								  const uint8_t * const pEnc_mac_keys,
								  const size_t num_keys,
								  const drm_wv_mod_key_object_t * const pKey_array,
								  const uint8_t * const pPst,
								  const uint32_t pst_size_in_bytes );

// TODO: Documentation
drm_wv_mod_result_t drm_wv_mod_load_usage_table(const uint8_t *const usage_table_data,
                                     uint32_t *const data_size);

// TODO: Documentation
drm_wv_mod_result_t drm_wv_mod_update_usage_table(uint8_t *const usage_table_data,
                                       const uint32_t data_size,
                                       uint8_t *const is_updated);

// TODO: Documentation
drm_wv_mod_result_t drm_wv_mod_deactivate_usage_entry(const uint8_t *const pst,
                                           uint32_t pst_length);

// TODO: Documentation
drm_wv_mod_result_t drm_wv_mod_report_usage(uint32_t session_id,
                                 const uint8_t *const pst,
                                 uint32_t pst_length,
                                 uint8_t *const pst_report_buffer,
                                 uint32_t *const pst_report_buffer_length);

// TODO: Documentation
drm_wv_mod_result_t drm_wv_mod_delete_usage_entry(uint32_t session_id,
                                       const uint8_t *const pst,
                                       uint32_t pst_length,
                                       const uint8_t *const msg,
                                       uint32_t msg_length,
                                       const uint8_t *const signature,
                                       uint32_t signature_length);

// TODO: Documentation
drm_wv_mod_result_t drm_wv_mod_delete_usage_table(void);


#endif /* __DRM_WV_MOD_LIB__ */

