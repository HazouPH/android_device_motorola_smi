////////////////////////////////////////////////////////////////////////////
// rril_OEM.h
//
// Copyright 2009 Intrinsyc Software International, Inc.  All rights reserved.
// Patents pending in the United States of America and other jurisdictions.
//
//
// Description:
//    Header providing structs and defines used in the OEM
//
/////////////////////////////////////////////////////////////////////////////

#ifndef RRIL_RRIL_OEM_H
#define RRIL_RRIL_OEM_H

typedef struct s_request_data
{
    char szCmd1[1024];           // AT command buffer to be sent to modem
    char szCmd2[1024];           // AT command buffer to send after cmd1 returns if populated

    unsigned long uiTimeout;    // Time to wait for response in milliseconds
    unsigned long uiRetries;    // If command fails, retry this many times.
    bool fForceParse;           // Always calls associated parse function

    void* pContextData;         // Point it to any object you will need during
                                // the parsing process that otherwise is not available.
                                // Can be left NULL if not required

    unsigned int cbContextData; // Size in bytes of the context data object

    void* pContextData2;        // Additional Context Data pointer
                                // Point it to any object you will need during
                                // the parsing process that otherwise is not available.
                                // Can be left NULL if not required

    unsigned int cbContextData2; // Size in bytes of the context data object
} REQUEST_DATA;

typedef struct s_response_data
{
    char* szResponse;           // AT response string from modem
    void* pData;                // Point to blob of memory containing response expected by
                                // upper layers

    unsigned long uiDataSize;   // Size of response blob

    unsigned long uiChannel;    // Channel the response was received on

    void* pContextData;         // Points to object given in request phase or NULL
                                // if not supplied

    unsigned int cbContextData; // Size in bytes of the context data object

    void* pContextData2;        // Additional Context Data pointer
                                // Points to object given in request phase or NULL
                                // if not supplied

    unsigned int cbContextData2;// Size in bytes of the context data object

    unsigned int uiResultCode;  // Result code for the AT command sent

    unsigned int uiErrorCode;   // CME/CMS error codes returned for the AT command sent
                                //  or general error code.
} RESPONSE_DATA;

typedef struct s_post_cmd_handler_data
{
    unsigned int uiChannel;         // Channel the command was sent on

    void* pRilToken;                // Pointer to the RIL request token

    void* pContextData;             // Points to object given in request phase or NULL
                                    // if not supplied

    unsigned int uiContextDataSize; // Size in bytes of the context data object

    int requestId;                  // Corresponding RIL request ID

    unsigned int uiResultCode;      // Result code for the AT command sent

    unsigned int uiErrorCode;       // CME/CMS error codes returned for the AT command sent or
                                    //  general error code.

    void* pData;                    // Point to blob of memory containing response expected by
                                    // upper layers

    unsigned int uiDataSize;        // Size of response blob
} POST_CMD_HANDLER_DATA;

#endif
