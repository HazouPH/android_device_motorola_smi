////////////////////////////////////////////////////////////////////////////
// te_xmm2230.h
//
// Copyright (C) Intel 2014.
//
//
// Description:
//    Overlay for the IMC 2230 modem
//
/////////////////////////////////////////////////////////////////////////////

#ifndef RRIL_TE_XMM2230_H
#define RRIL_TE_XMM2230_H

#include "te_xmm6260.h"
#include "rril.h"
#include "channel_data.h"

class CEvent;
class CInitializer;

class CTE_XMM2230 : public CTE_XMM6260
{
public:

    CTE_XMM2230(CTE& cte);
    virtual ~CTE_XMM2230();

private:

    CTE_XMM2230();

    //  Prevent assignment: Declared but not implemented.
    CTE_XMM2230(const CTE_XMM2230& rhs);  // Copy Constructor
    CTE_XMM2230& operator=(const CTE_XMM2230& rhs);  //  Assignment operator

public:
    // modem overrides
    virtual char* GetUnlockInitCommands(UINT32 uiChannelType);

    virtual CInitializer* GetInitializer();

    // RIL_REQUEST_BASEBAND_VERSION
    virtual RIL_RESULT_CODE CoreBasebandVersion(REQUEST_DATA& rReqData,
            void* pData, UINT32 uiDataSize);
    virtual RIL_RESULT_CODE ParseBasebandVersion(RESPONSE_DATA& rRspData);

    // RIL_REQUEST_SIM_TRANSMIT_BASIC
    virtual RIL_RESULT_CODE CoreSimTransmitBasic(REQUEST_DATA& rReqData,
            void* pData,
            UINT32 uiDataSize);

    // RIL_REQUEST_SIM_OPEN_CHANNEL
        virtual RIL_RESULT_CODE CoreSimOpenChannel(REQUEST_DATA& rReqData,
            void* pData,
            UINT32 uiDataSize);

    // RIL_REQUEST_SIM_CLOSE_CHANNEL
    virtual RIL_RESULT_CODE CoreSimCloseChannel(REQUEST_DATA& rReqData,
            void* pData,
            UINT32 uiDataSize);

    // RIL_REQUEST_SIM_TRANSMIT_CHANNEL
    virtual RIL_RESULT_CODE CoreSimTransmitChannel(REQUEST_DATA& rReqData,
            void* pData,
            UINT32 uiDataSize);

protected:
    virtual const char* GetRegistrationInitString();
    virtual const char* GetCsRegistrationReadString();
    virtual const char* GetScreenOnString();
};

#endif
