/**********************************************************************
 * Copyright (C) 2011 Intel Corporation. All rights reserved.

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

/*
 * Jayant.Mangalampalli@intel.com
 */
#ifndef __DRM_H__
#define __DRM_H__

#include <inttypes.h>
#include "sepdrm-common.h"


//
// Maximum size in bytes of the provisioning input key data.
//
#define FKP_MAX_KEY_SIZE_IN_BYTES 32


//
// Secure clock transaction ID (TID) size in bytes.
//
#define DRM_TID_SIZE 16

//
// defines for Drm_LicAcq_ECC256()
//
#define DRM_ECC256_SIGNATURE_SIZE 64


//
// Bitmasks for the return flags for Drm_SecureClock_GetClockValue().
//
#define DRM_SECURE_CLOCK_FLAG_RESET     ( 1 << 0 )
#define DRM_SECURE_CLOCK_FLAG_REFRESH   ( 1 << 1 )
#define DRM_SECURE_CLOCK_FLAG_NO_DELTAS ( 1 << 2 )


//
// Maximum number of bytes for an audio or video data DMA. This reflects the
// limits of the IA to Chaabi DMA and Chaabi IMR allocation.
//
#define MAX_DMA_DATA_SIZE_IN_BYTES ( 4 * 1024 * 1024 )


/*
 * The size of an AES Initialization Vector counter in bytes.
 */
#define AES_IV_COUNTER_SIZE_IN_BYTES 16


#define SEC_MAX_NUM_SLICES      (64)

#define PLAYREADY_SEED_KEY_BLOB_LENGTH (64)
#define PLAYREADY_MODEL_KEY_BLOB_LENGTH (1024)
#define WIDEVINE_KEYBOX_DEVICE_ID_SIZE (32)
#define KEYBOX_DEVKEY_SIZE		(16)
#define WIDEVINE_KEYBOX_KEY_DATA_SIZE (72)

#define SEP_SHARED_AREA_PHYSADDRESS_OFFSET (11)

typedef enum {
	SEC_DEVICE_SIGN_KEY = 4, // the key container has device signing key
	SEC_DEVICE_ENC_KEY, // the key container has device encryption key
	WideVine_KeyStore_ID_DEVICE_KEY = 9,
	SEC_PRSEED_KEY = 0x10,
	// the key container has PR Seed key
	// the PLAYREADY SEED key its key length is 16 in its container
	// it has PLAYREADY_SEED_KEY_BLOB_LENGTH
	SEC_MODEL_KEY,
	// the key container has model keys
	// the PLAYREAD MODEL key, its key length is 976 in its container
	// it has PLAYREADY_MODEL_KEY_BLOB_LENGTH
} Key_id_t;


#define key_enumeration_type_t Key_id_t

typedef enum {
	SEC_PLAINTEXT,		// the key container is in the clear
	SEC_INTEL_GKEK,		// the key container is wrapped with Intel's GKEK
	SEC_CUSTOMER_GKEK,	// the key container is wrapped with a customer's GKEK
	SEC_INTERNAL_KEY_STORE, // use a previously loaded internal key
	SEC_OEM_SEED, 		// use an OEM seed hidden in the SEC
} sec_encryption_type_t;

/* context for backward compatibility sake */
typedef struct {
	uint8_t reserved;
} sec_dev_context_t;

//
// Field key provision key container structure.
//
typedef struct
{
	uint8_t  keyType;
	uint8_t  cryptRights;
	uint8_t  signRights;
	uint8_t  flags;
	uint32_t keySizeInBytes;
	uint32_t keyId; //Key_id_t: keyId size is 4 bytes, keyId is a type of Key_id_t
	uint32_t reserved;
	uint8_t  keyData[];
} fkp_key_container_t;


//
// Secure clock time of day data type.
// day_of_month: starts with 1.
// month: 0 = January.
// year: Epoch is 70 (i.e., 1970). Maximum value is 138 (i.e., 2038).
//
typedef struct
{
	uint8_t sec;
	uint8_t min;
	uint8_t hour;
	uint8_t day_of_week;
	uint8_t day_of_month;
	uint8_t month;
	uint8_t year;
	uint8_t padding;
} time_of_day_t;


//
// Secure clock server response data type.
//
typedef struct
{
	uint8_t       tid[ DRM_TID_SIZE ];
	time_of_day_t current_time;
	time_of_day_t refresh_time;
	uint8_t       signature[ 256 ];
} Drm_SecureClock_ServerResponse_t;

/* Widevine structs */
typedef struct {
	uint32_t imr_offset; // Where this NALU resides
	uint32_t nalu_size; // size of full NALU
	uint32_t data_size; // size of data returned
	uint8_t data[];
} DRM_WV_NALU;

typedef struct {
	uint32_t num_nalu; // Number of NALUs returned
	DRM_WV_NALU nalu[]; // NALUs returned
} DRM_WV_NALU_HEADERS;
/* End Widevine structs */

/*
 * DRM Schemes
 */
typedef enum {
	DRM_SCHEME_Netflix,
	DRM_SCHEME_Widevine,
} drm_scheme_t;


typedef struct _frame_meta_data
{
	uint32_t num_slices;
	uint8_t  ivcounter[16];
	uint8_t  *pinput_frame_buffer;
	uint32_t frame_data_size;

	struct SLICE_INFO
	{
		uint32_t slicedata_size;
		uint32_t sliceheader_size;
		uint8_t *sliceheader;
	} slice_info[SEC_MAX_NUM_SLICES];
} SEC_FRAME_META_DATA;


/*
 * This structure is used to provide necessary information for Widevine audio
 * ciphertext decryption.
 * 
 * The members are:
 *
 *   iv                     - AES initialization vector.
 *   input_ciphertext_size  - Input ciphertext data size in bytes.
 *   p_input_ciphertext     - Pointer to the input ciphertext data buffer.
 *   output_plaintext_size  - Output plaintext data size in bytes.
 *   p_output_plaintext     - Pointer to the output plaintext data buffer.
 */
typedef struct
{
	uint8_t  iv[ AES_IV_COUNTER_SIZE_IN_BYTES ];
	uint32_t input_ciphertext_size;
	uint8_t  *p_input_ciphertext;
	uint32_t output_plaintext_size;
	uint8_t  *p_output_plaintext;
} DRM_WV_AUDIO_DATA;


/*
 * This structure is used to provide necessary information for Widevine video
 * ciphertext decryption.
 * 
 * The members are:
 *
 *   iv                     - AES initialization vector.
 *   input_ciphertext_size  - Input ciphertext data size in bytes.
 *   p_input_ciphertext     - Pointer to the input ciphertext data.
 */
typedef struct
{
	uint8_t  iv[ AES_IV_COUNTER_SIZE_IN_BYTES ];
	uint32_t input_ciphertext_size;
	uint8_t  *p_input_ciphertext;
} DRM_WV_VIDEO_CIPHER;


#define DRM_ECC256_PUBKEYLEN	64
#define DRM_ECC256_PRIVKEYLEN	32
#define DRM_KEYPAIR_KCSIZE	160
typedef struct {
	uint32_t encrypted_prkey_length; //reserved for future use, now returned 0
	uint8_t pubKey[DRM_ECC256_PUBKEYLEN];
	uint8_t encrypted_privateKey[DRM_KEYPAIR_KCSIZE];
	//1st 80 bytes in encrypted_privateKey are currently used, 2nd 80 for future use
} DeviceKeyPair_t, DeviceKeyPair[2];
//DeviceKeyPair[0] is the encrpyt key and DeviceKeyPair[1] is the sign key.

typedef struct {
	uint32_t IMR_size;
	uint32_t IMR_Num_Buckets;
	uint32_t IMR_Bucket_Size;
	uint32_t reserved[13];
} DRM_PLATFORM_CAPS;
#define DRM_PLATCAP_IED		0x01
#define DRM_PLATCAP_IMR		0x02
#define DRM_PLATCAP_EPID	0x04
#define DRM_PLATCAP_HDCP	0x08


/*
 * DRM Library Initialization
 * Description:
 * 	Initializes the SEP driver for DRM library use.
 *
 * Return:
 *   NF_SUCCESSFUL           - The SEP driver is initialized.
 *   NF_FAIL_NOT_INITIALIZED - The SEP driver is not initialized.
 */
sec_result_t Drm_Library_Init( void );

/*
 * Set Widevine Asset Key
 */
sec_result_t Drm_WV_SetEntitlementKey(uint8_t*pEMMKey, uint32_t EMMKeyLen);

/*
 * Derive Widevine Control Word
 */
sec_result_t Drm_WV_DeriveControlWord(uint8_t *pCW, uint32_t CWLen, uint32_t *pCWFlags);

/*
 * Get Widevine Keybox infomation
 * Description:
 *	Retrieve the encypted kbox data and decrypt the encrypted kbox data
 *
 * Parameters:
 * Parameter Name		In/Out		Description
 * KeyData	Out		Pointer to KeyData field in KeyBox
 * KeyDataSize	In/Out		KeyData size (72 bytes)
 * DeviceId	Out		Pointer to DeviceId field in KeyBox
 * DeviceIdSize	In/Out		DeviceId size (32 bytes)
*/
sec_result_t Drm_WV_GetKeyBoxInfo(uint8_t *KeyData, uint32_t *KeyDataSize,
				  uint8_t *DeviceId, uint32_t *DeviceIdSize);

/*
 * Secure Clock Transaction
 * Description:
 * 	Chaabi generates a transaction ID and copies it to the buffer pointed to
 *        by the pointer pTid. The buffer pointed to by pTid must be
 *        DRM_TID_SIZE bytes in size.
 *
 *      This function returns NF_SUCCESSFUL if generating the transaction ID was
 *      successful.
 */
sec_result_t Drm_SecureClock_StartTransaction( uint8_t *pTid );


/*
 * Secure Clock Parse And Verify
 * Description:
 * 	Pointer to the secure clock server response is passed in. Chaabi
 *        verifies the response and returns the verification status.
 *
 *      Return value of NF_SUCCESSFUL means the server response was successfully
 *      verified.
 */
sec_result_t Drm_SecureClock_ParseAndVerify( uint8_t *pResponse, uint32_t responseSizeInBytes );


/*
 * Secure Clock Get Current Value
 * Description:
 * 	pClock is set to the current server clock value in Chaabi as the number
 *        of seconds since the epoch (1970-Jan-01 00:00:00 UTC).
 *
 *      pFlags is set to the flag values corresponding to the current clock
 *        state in Chaabi. These flags are not mutually exclusive. If pFlags
 *        is zero then the secure clock time is good. If any of the flags
 *        are set then the secure clock needs to be reset.
 *
 *        The following bitmasks can be used to tell the state of the server
 *        clock:
 *
 *        DRM_SECURE_CLOCK_FLAG_RESET     : Clock went through a reset
 *        DRM_SECURE_CLOCK_FLAG_REFRESH   : Past the recommended refresh state
 *        DRM_SECURE_CLOCK_FLAG_NO_DELTAS : Deltas not stored
 *
 *      Return value of NF_SUCCESSFUL means the server clock time was
 *      successfully read.
 */
sec_result_t Drm_SecureClock_GetClockValue( uint32_t *pClock, uint32_t *pFlags );


/*
 * @brief Writes random bytes into buffer
 * @param pBuffer Pointer to a memory buffer to write the random byte values into
 * @param numberOfBytes Number of random value bytes to write
 * @return DRM_SUCCESSFUL The random value bytes were successfully written
 */
sec_result_t Drm_GetRandom( uint8_t * const pRandom,
                            const uint32_t bufLen );


/*
 * Bind
 *
 *    PlayReady CEK extraction and validation.
 *
 * Wenjie Zhang's sep_ext_processlicresponse() function, which should be
 * sec_ext_pr_bind() in the Netflix Chaabi APIs specification.
 */
sec_result_t sep_ext_processlicresponse(uint8_t * xmr,int len,uint8_t *encxmr, int *encxmrlen);


/*
 * Sign challenge
 *
 *   PlayReady license acquisition step by generating license challenge to be
 *   sent to the license server.
 *
 * Wenjie Zhang's sep_ext_sign_licensechallenge() function, which should be
 * sec_ext_ecc256_sign() in the Netflix Chaabi APIs specification.
 */
sec_result_t sep_ext_sign_licensechallenge(uint8_t * la,unsigned int lalen,uint8_t * signnode,unsigned int signnodelen,uint8_t * signature,unsigned int *signaturelen);


/*
 * Provision device key:
 * Description:
 * 	Provision Device key to Chaabi using OEM pre-shared key.
 *	sec_dev_context can be either NULL or NOT NULL
 *      key_encryption_type support AES encription
 *      outputlength must not less than inputlength
 */

sec_result_t sec_ext_provision_key(sec_dev_context_t *sec_dev_context,
				   sec_encryption_type_t key_encryption_type,
				   unsigned int key_id,
				   uint8_t *input_key_container,
				   uint8_t *rewrapped_key_container);


/*
 * Load device key
 * Description:
 * 	Load device key into Chaabi context.
 * sec_dev_context	In	[= NULL] Device context not used by chaabi APIs
 * key_encryption_type	In	[=NULL] Need not be used for chaabi encrypted keys
 * key_id		In	[=NULL] Need not be used for chaabi encrypted keys
* key_container		In	Pointer to key container with keys to be loaded
* 			into chaabi's secure memory. Key container itself indicates
*			the key ID and other details needed by chaabi to decrypt the key.
*			The container is of type fkp_key_container_t.
* Public_key	Out	[= NULL] Pointer to public key in case the key is asymmetric. Ignored for now.
*/
sec_result_t sec_ext_load_key (sec_dev_context_t *sec_dev_context, sec_encryption_type_t key_encryption_type,
			key_enumeration_type_t  key_id, uint8_t *key_container, uint8_t *public_key);


/*
 * Invalidate Key
 * Description:
 * 	Invalidate the current key in Chaabi context
 * sec_dev_context [In]	[= NULL] Device context not used by chaabi APIs
 * key_id	[In]	The Key ID to be invalidated from chaabi's internal memory.
 */
sec_result_t sec_ext_invalidate_key (sec_dev_context_t *sec_dev_context,
							unsigned int key_id);

/*
 * Device KeyPair
 * Description:
 *	Generate Device Keypair
 */
sec_result_t sec_ext_gen_keypair(sec_dev_context_t *sec_dev_context,
				 uint8_t *keyBlob, uint32_t *blog_length);

/*
 * Device Certificate
 * Description:
 *	Generate Device Certificate
 */
sec_result_t sec_ext_gen_dev_cert(sec_dev_context_t *sec_dev_context,
				  uint8_t *devCert, uint32_t cert_length,
				  uint8_t *signature, uint32_t *sign_length);

/*
 * Generate Challange
 * Description:
 * 	Request chaabi to generate challange that can be verified by Netflix server
 */
sec_result_t sec_ext_pr_generate_challenge(uint32_t sessionID,
					   uint32_t headerSize,
					   uint8_t* header,
					   uint32_t challengeSize,
					   uint8_t* challenge);

/*
 * Process Response
 * Description:
 * 	Request chaabi to process a server-sent response for the chaabi generated challange.
 */
sec_result_t sec_ext_pr_process_response(uint32_t sessionID,
					 uint8_t* response,
					 uint32_t responseSize);

/*
 * Check Video Path
 * Description:
 * 	Check if secure Video patch is active and the number of active sessions
 * 	TODO: What is the format? Is this needed?
 */
sec_result_t check_video_path(uint32_t sessionID);


sec_result_t Drm_LicAcq_ECC256(uint8_t *challenge, uint32_t challenge_size,
			       uint8_t ECC256Signature[DRM_ECC256_SIGNATURE_SIZE]);
sec_result_t Drm_LicenseAcq_SignChallenge(uint8_t *laNode, unsigned int laNodeLen,
					  uint8_t *signNode,
					  unsigned int signNodeLen,
					  uint8_t *signature,
					  unsigned int *signLen);
sec_result_t Drm_PR_CreateSession(uint8_t *pXmr, uint32_t xmrSize,
				  uint32_t *pIMROffset,
				  uint32_t *pIMRBufferSize,
				  uint32_t *pSessionID);

sec_result_t Drm_WV_CreateSession(uint32_t *pIMROffset,
				  uint32_t *pIMRBufferSize,
				  uint32_t *pSessionID);

sec_result_t Drm_DestroySession(uint32_t sessionID);


/*
 * Keeps an active DRM session from timing out
 *
 * Parameters:
 *	sessionID	: handle to DRM session
 */
sec_result_t Drm_KeepAlive(uint32_t sessionID, uint32_t *timeout);

/*
 * Query secure platform capabilities
 *
 * Parameters:
 *	pPlatCap	: bitfield specifying which capabilities to query
 *	pCapArray	: query results
 */
sec_result_t Drm_QueryPlatformCapabilities(uint32_t *pPlatCap,
					   DRM_PLATFORM_CAPS *pCapArray);

/*
 * @brief Decrypts Netflix video ciphertext data
 * @param sessionID DRM Session ID number
 * @param pMetaData Pointer to the video meta data
 * @param decryptIMROffset IMR byte offset for video plaintext data
 *
 * The pointer pMetaData points to the video meta data structure that contains
 * information about the video frame, such as input and output buffer pointer,
 * sizes, and data buffers.
 */
sec_result_t Drm_Reader_DecryptFrame( uint32_t sessionID, SEC_FRAME_META_DATA *pMetaData, uint32_t decryptIMROffset);


/*
 * @brief Pauses the playback of a video decryption session.
 * @param sessionID The ID number of the session to pause playback.
 * @return DRM_SUCCESSFUL The video decryption session was paused.
 */
sec_result_t Drm_Playback_Pause(uint32_t sessionID);


/*
 * @brief Resumes the playback of a paused video decryption session.
 * @param sessionID The ID number of the session to resume playback.
 * @return DRM_SUCCESSFUL The video decryption session was resumed.
 */
sec_result_t Drm_Playback_Resume(uint32_t sessionID);


/*
 * @brief Decrypts Widevine encrypted audio data
 * @param sessionID The ID number of the session to resume playback.
 * @param pAudioInfo Pointer to a buffer containing Widevine audio information
 * @return DRM_SUCCESSFUL The Widevine audio ciphertext was decrypted
 */
sec_result_t Drm_WV_DecryptAudio( const uint32_t sessionID,
                                  DRM_WV_AUDIO_DATA * const pAudioInfo );


/*
 * @brief Decrypts Widevine video ciphertext data into the IMR decryption buffer
 * @param sessionID DRM Session ID number
 * @param pVideoData Pointer to the Widevine video data
 * @param decryptIMROffset IMR byte offset for video plaintext data
 */
sec_result_t Drm_WV_DecryptVideo( const uint32_t sessionID,
                                  const DRM_WV_VIDEO_CIPHER * const pVideoInfo,
                                  const uint32_t decryptIMROffset );


sec_result_t Drm_WV_ReturnNALUHeaders(uint32_t sessionID, uint32_t imrOffset,
				      uint32_t frameSize, uint8_t *pNaluHdrBuf,
				      uint32_t *pNaluBufLength);


#endif //__DRM_H__
