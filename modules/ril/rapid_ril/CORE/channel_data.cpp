////////////////////////////////////////////////////////////////////////////
// channel_Data.cpp
//
// Copyright 2005-2007 Intrinsyc Software International, Inc.  All rights reserved.
// Patents pending in the United States of America and other jurisdictions.
//
//
// Description:
//    Provides implementations for helper functions used
//    to facilitate the use of multiple data channels.
//    GPRS/UMTS data (1st primary context)
//
/////////////////////////////////////////////////////////////////////////////

#include <stdio.h>

// This is for socket-related calls.
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <linux/if_ether.h>
#include <linux/gsmmux.h>

// This is for close().
#include <unistd.h>
#include <wchar.h>

#include "types.h"
#include "rillog.h"
#include "util.h"
#include "channelbase.h"
#include "channel_data.h"
#include "nd_structs.h"
#include "repository.h"
#include "rril.h"
#include "te.h"
#include "util.h"
#include "data_util.h"

extern char* g_szDataPort1;
extern char* g_szDataPort2;
extern char* g_szDataPort3;
extern char* g_szDataPort4;
extern char* g_szDataPort5;
extern BOOL  g_bIsSocket;

// Init strings for this channel.
//  GPRS/UMTS data (1st primary context)

// Add any init cmd strings for this channel during PowerOn or Ready boot phase
INITSTRING_DATA DataPowerOnInitString = { "" };
INITSTRING_DATA DataReadyInitString = { "" };

// used for 6360 and 7160 modems.
extern int m_hsiChannelsReservedForClass1;
extern int m_hsiDataDirect;
extern int m_dataProfilePathAssignation[NUMBER_OF_APN_PROFILE];

// used by 6360 and 7160 modems.
UINT32 g_uiHSIChannel[RIL_MAX_NUM_IPC_CHANNEL] = {0};

// If MAX_CID_NUMERIC is increased, m_uiChildContexts shall be changed to a bigger
// data type also.
UINT32 CChannel_Data::MAX_CID_NUMERIC = 31;

CChannel_Data::CChannel_Data(UINT32 uiChannel)
:   CChannel(uiChannel),
    m_dataFailCause(PDP_FAIL_NONE),
    m_uiContextID(0),
    m_dataState(E_DATA_STATE_IDLE),
    m_ipcDataChannelMin(RIL_DEFAULT_IPC_CHANNEL_MIN),
    m_isRoutingEnabled(FALSE),
    m_refCount(0),
    m_uiChildContexts(0),
    m_pszIpAddresses(NULL),
    m_pszDnses(NULL),
    m_pszGateways(NULL),
    m_pszPcscfes(NULL)
{
    RIL_LOG_VERBOSE("CChannel_Data::CChannel_Data() - Enter\r\n");

    ResetDataCallInfo();

    CopyStringNullTerminate(m_szModemResourceName, RIL_DEFAULT_IPC_RESOURCE_NAME,
            sizeof(m_szModemResourceName));

    RIL_LOG_VERBOSE("CChannel_Data::CChannel_Data() - Exit\r\n");
}

CChannel_Data::~CChannel_Data()
{
    RIL_LOG_VERBOSE("CChannel_Data::~CChannel_Data() - Enter\r\n");

    delete[] m_paInitCmdStrings;
    m_paInitCmdStrings = NULL;

    delete[] m_pszIpAddresses;
    m_pszIpAddresses = NULL;

    delete[] m_pszDnses;
    m_pszDnses = NULL;

    delete[] m_pszGateways;
    m_pszGateways = NULL;

    delete[] m_pszPcscfes;
    m_pszPcscfes = NULL;

    RIL_LOG_VERBOSE("CChannel_Data::~CChannel_Data() - Exit\r\n");
}

//  Override from base class
BOOL CChannel_Data::OpenPort()
{
    BOOL bRetVal = FALSE;

    switch(m_uiRilChannel)
    {
        case RIL_CHANNEL_DATA1:
            RIL_LOG_INFO("CChannel_Data::OpenPort() - Opening COM Port: %s...\r\n", g_szDataPort1);
            RIL_LOG_INFO("CChannel_Data::OpenPort() - g_bIsSocket=[%d]...\r\n", g_bIsSocket);
            bRetVal = m_Port.Open(g_szDataPort1, g_bIsSocket);
            break;

        case RIL_CHANNEL_DATA2:
            RIL_LOG_INFO("CChannel_Data::OpenPort() - Opening COM Port: %s...\r\n", g_szDataPort2);
            RIL_LOG_INFO("CChannel_Data::OpenPort() - g_bIsSocket=[%d]...\r\n", g_bIsSocket);
            bRetVal = m_Port.Open(g_szDataPort2, g_bIsSocket);
            break;

        case RIL_CHANNEL_DATA3:
            RIL_LOG_INFO("CChannel_Data::OpenPort() - Opening COM Port: %s...\r\n", g_szDataPort3);
            RIL_LOG_INFO("CChannel_Data::OpenPort() - g_bIsSocket=[%d]...\r\n", g_bIsSocket);
            bRetVal = m_Port.Open(g_szDataPort3, g_bIsSocket);
            break;

        case RIL_CHANNEL_DATA4:
            RIL_LOG_INFO("CChannel_Data::OpenPort() - Opening COM Port: %s...\r\n", g_szDataPort4);
            RIL_LOG_INFO("CChannel_Data::OpenPort() - g_bIsSocket=[%d]...\r\n", g_bIsSocket);
            bRetVal = m_Port.Open(g_szDataPort4, g_bIsSocket);
            break;

        case RIL_CHANNEL_DATA5:
            RIL_LOG_INFO("CChannel_Data::OpenPort() - Opening COM Port: %s...\r\n", g_szDataPort5);
            RIL_LOG_INFO("CChannel_Data::OpenPort() - g_bIsSocket=[%d]...\r\n", g_bIsSocket);
            bRetVal = m_Port.Open(g_szDataPort5, g_bIsSocket);
            break;

        default:
            RIL_LOG_CRITICAL("CChannel_Data::OpenPort() -"
                    "channel does not exist m_uiRilChannel=%d\r\n", m_uiRilChannel);
            bRetVal = FALSE;
            break;
    }



    RIL_LOG_INFO("CChannel_Data::OpenPort() - Opening COM Port: %s\r\n",
            bRetVal ? "SUCCESS" : "FAILED!");

    return bRetVal;
}

BOOL CChannel_Data::FinishInit()
{
    RIL_LOG_VERBOSE("CChannel_Data::FinishInit() - Enter\r\n");

    BOOL bRet = FALSE;
    CRepository repository;

    //  Init our channel AT init commands.
    m_paInitCmdStrings = new INITSTRING_DATA[COM_MAX_INDEX];
    if (!m_paInitCmdStrings)
    {
        RIL_LOG_CRITICAL("CChannel_Data::FinishInit() : chnl=[%d] Could not create new "
                "INITSTRING_DATA\r\n", m_uiRilChannel);
        goto Error;
    }

    // Set the init command strings for this channel
    m_paInitCmdStrings[COM_BASIC_INIT_INDEX].szCmd = m_szChannelBasicInitCmd;
    m_paInitCmdStrings[COM_UNLOCK_INIT_INDEX].szCmd = m_szChannelUnlockInitCmd;

    m_paInitCmdStrings[COM_POWER_ON_INIT_INDEX] = DataPowerOnInitString;
    m_paInitCmdStrings[COM_READY_INIT_INDEX] = DataReadyInitString;

    // read the minimum data channel index
    if (!repository.Read(g_szGroupModem, g_szIpcDataChannelMin, m_ipcDataChannelMin))
    {
        RIL_LOG_WARNING("CChannel_Data::FinishInit() : Could not read min"
                " IPC Data channel from repository\r\n");
        m_ipcDataChannelMin = RIL_DEFAULT_IPC_CHANNEL_MIN;
    }

    //  Grab the Modem data channel resource name
    if (!repository.Read(g_szGroupModem, g_szModemResourceName, m_szModemResourceName,
                MAX_MDM_RESOURCE_NAME_SIZE))
    {
        RIL_LOG_WARNING("CChannel_Data::FinishInit() - Could not read modem resource"
                " name from repository, using default\r\n");
        // Set default value.
        CopyStringNullTerminate(m_szModemResourceName, RIL_DEFAULT_IPC_RESOURCE_NAME,
                sizeof(m_szModemResourceName));
    }
    RIL_LOG_INFO("CChannel_Data::FinishInit() - m_szModemResourceName=[%s]\r\n",
                m_szModemResourceName);

    bRet = TRUE;
Error:
    RIL_LOG_VERBOSE("CChannel_Data::FinishInit() - Exit\r\n");
    return bRet;
}

bool CChannel_Data::IsDataConnectionActive()
{
    RIL_LOG_VERBOSE("CChannel_Data::IsDataConnectionActive() - Enter\r\n");

    for (UINT32 i = RIL_CHANNEL_DATA1; i < g_uiRilChannelCurMax; i++)
    {
        if (NULL == g_pRilChannel[i]) // could be NULL if reserved channel
            continue;

        CChannel_Data* pChannelData = static_cast<CChannel_Data*>(g_pRilChannel[i]);
        if (pChannelData && pChannelData->GetContextID() > 0
                && E_DATA_STATE_ACTIVE == pChannelData->GetDataState())
        {
            return true;
        }
    }

    RIL_LOG_VERBOSE("CChannel_Data::IsDataConnectionActive() - Exit\r\n");
    return false;
}

UINT32 CChannel_Data::GetFirstActiveDataConnectionCid()
{
    RIL_LOG_VERBOSE("CChannel_Data::GetFirstActiveDataConnectionCid() - Enter\r\n");

    CMutex::Lock(CSystemManager::GetInstance().GetDataChannelAccessorMutex());
    UINT32 uiContextID = 0;

    for (UINT32 i = RIL_CHANNEL_DATA1; i < g_uiRilChannelCurMax; i++)
    {
        if (NULL == g_pRilChannel[i]) // could be NULL if reserved channel
            continue;

        CChannel_Data* pChannelData = static_cast<CChannel_Data*>(g_pRilChannel[i]);
        if (pChannelData != NULL
                && E_DATA_STATE_ACTIVE == pChannelData->GetDataState())
        {
            uiContextID = pChannelData->GetContextID();
            break;
        }
    }

    CMutex::Unlock(CSystemManager::GetInstance().GetDataChannelAccessorMutex());

    RIL_LOG_VERBOSE("CChannel_Data::GetFirstActiveDataConnectionCid() - Exit\r\n");
    return uiContextID;
}

//
//  Returns a pointer to the channel linked to the given interface name
//
CChannel_Data* CChannel_Data::GetChnlFromIfName(const char * ifName)
{
    RIL_LOG_VERBOSE("CChannel_Data::GetChnlFromIfName() - Enter\r\n");

    CMutex::Lock(CSystemManager::GetInstance().GetDataChannelAccessorMutex());

    extern CChannel* g_pRilChannel[RIL_CHANNEL_MAX];
    CChannel_Data* pChannelData = NULL;

    for (UINT32 i = RIL_CHANNEL_DATA1; i < g_uiRilChannelCurMax && i < RIL_CHANNEL_MAX; i++)
    {
        if (NULL == g_pRilChannel[i]) // could be NULL if reserved channel
            continue;

        CChannel_Data* pTemp = static_cast<CChannel_Data*>(g_pRilChannel[i]);
        if (pTemp)
        {
            char szTempName[MAX_INTERFACE_NAME_SIZE];
            pTemp->GetInterfaceName(szTempName, MAX_INTERFACE_NAME_SIZE);
            if (!strncmp(szTempName, ifName, MAX_INTERFACE_NAME_SIZE)) {
                pChannelData = pTemp;
                break;
            }
        }
    }

    CMutex::Unlock(CSystemManager::GetInstance().GetDataChannelAccessorMutex());

    RIL_LOG_VERBOSE("CChannel_Data::GetChnlFromIfName() - Exit\r\n");
    return pChannelData;
}

//
//  Returns a pointer to the channel linked to the given context ID
//
CChannel_Data* CChannel_Data::GetChnlFromContextID(UINT32 uiContextID)
{
    RIL_LOG_VERBOSE("CChannel_Data::GetChnlFromContextID() - Enter\r\n");

    CMutex::Lock(CSystemManager::GetInstance().GetDataChannelAccessorMutex());

    extern CChannel* g_pRilChannel[RIL_CHANNEL_MAX];
    CChannel_Data* pChannelData = NULL;

    for (UINT32 i = RIL_CHANNEL_DATA1; i < g_uiRilChannelCurMax && i < RIL_CHANNEL_MAX; i++)
    {
        if (NULL == g_pRilChannel[i]) // could be NULL if reserved channel
            continue;

        CChannel_Data* pTemp = static_cast<CChannel_Data*>(g_pRilChannel[i]);
        if (pTemp && pTemp->GetContextID() == uiContextID)
        {
            pChannelData = pTemp;
            break;
        }
    }

Error:
    CMutex::Unlock(CSystemManager::GetInstance().GetDataChannelAccessorMutex());

    RIL_LOG_VERBOSE("CChannel_Data::GetChnlFromContextID() - Exit\r\n");
    return pChannelData;
}

//
//  Returns a pointer to the channel linked to the given childcontext ID
//
CChannel_Data* CChannel_Data::GetChnlFromChildContextID(UINT32 uiChildContextID)
{
    RIL_LOG_VERBOSE("CChannel_Data::GetChnlFromChildContextID() - Enter\r\n");

    CMutex::Lock(CSystemManager::GetInstance().GetDataChannelAccessorMutex());

    extern CChannel* g_pRilChannel[RIL_CHANNEL_MAX];
    CChannel_Data* pChannelData = NULL;

    for (UINT32 i = RIL_CHANNEL_DATA1; i < g_uiRilChannelCurMax && i < RIL_CHANNEL_MAX; i++)
    {
        if (NULL == g_pRilChannel[i]) // could be NULL if reserved channel
            continue;

        CChannel_Data* pTemp = static_cast<CChannel_Data*>(g_pRilChannel[i]);
        if (pTemp && pTemp->HasChildContextID(uiChildContextID))
        {
            pChannelData = pTemp;
            break;
        }
    }

Error:
    CMutex::Unlock(CSystemManager::GetInstance().GetDataChannelAccessorMutex());

    RIL_LOG_VERBOSE("CChannel_Data::GetChnlFromChildContextID() - Exit\r\n");
    return pChannelData;
}


// Add a context ID to the list of childs contexts.
void CChannel_Data::AddChildContextID(UINT32 uiContextID)
{
    if (uiContextID == 0 || uiContextID > MAX_CID_NUMERIC) {
        RIL_LOG_CRITICAL("CChannel_Data::AddContextID() - Invalid contextID (%u)\r\n", uiContextID);
        return;
    }
    RIL_LOG_VERBOSE("CChannel_Data::AddContextID() - Enter\r\n");

    CMutex::Lock(CSystemManager::GetInstance().GetDataChannelAccessorMutex());
    m_uiChildContexts |= (1 << uiContextID);
    CMutex::Unlock(CSystemManager::GetInstance().GetDataChannelAccessorMutex());

    RIL_LOG_VERBOSE("CChannel_Data::AddContextID() - Exit\r\n");
}

// Remove a context ID to the list of childs contexts.
void CChannel_Data::RemoveChildContextID(UINT32 uiContextID)
{
    if (uiContextID == 0 || uiContextID > MAX_CID_NUMERIC) {
        RIL_LOG_CRITICAL("CChannel_Data::RemoveChildContextID() - Invalid contextID (%u)\r\n",
                uiContextID);
        return;
    }
    RIL_LOG_VERBOSE("CChannel_Data::RemoveChildContextID() - Enter\r\n");

    CMutex::Lock(CSystemManager::GetInstance().GetDataChannelAccessorMutex());
    m_uiChildContexts &= ~(1 << uiContextID);
    CMutex::Unlock(CSystemManager::GetInstance().GetDataChannelAccessorMutex());

    RIL_LOG_VERBOSE("CChannel_Data::RemoveChildContextID() - Exit\r\n");
}

void CChannel_Data::ClearChildsContextID()
{
    RIL_LOG_VERBOSE("CChannel_Data::ClearChildsContextID() - Enter\r\n");
    CMutex::Lock(CSystemManager::GetInstance().GetDataChannelAccessorMutex());
    m_uiChildContexts = 0;
    CMutex::Unlock(CSystemManager::GetInstance().GetDataChannelAccessorMutex());
    RIL_LOG_VERBOSE("CChannel_Data::ClearChildsContextID() - Exit\r\n");
}

// returns true if given contextID is a child from this context.
BOOL CChannel_Data::HasChildContextID(UINT32 uiContextID)
{
    if (uiContextID == 0 || uiContextID > MAX_CID_NUMERIC) {
        RIL_LOG_CRITICAL("CChannel_Data::HasChildContextID() - Invalid contextID (%u)\r\n",
                uiContextID);
        return FALSE;
    }
    return (m_uiChildContexts & (1 << uiContextID)) != 0;
}

// Find the first bit set starting from bit 0.
// returns the index of the first bit set, -1 if no bit set.
static int rff1(int entry) {
    if (entry == 0) return -1;

    int i = 0;
    while ((entry & (1 << i)) == 0) i++; // no infinite loop as not 0
    return i;
}

//  Used for 6360 and 7160 modems.
//  Returns a free RIL channel and a free hsi channel to use for data.
//  [out] UINT32 outCID - If successful, the new context ID, else 0.
//  return value - CChannel_Data* - If successful, the free data channel, else NULL.
//  Note: If successful the returned CChannel_Data* will have the new context ID set.
//        m_hsiDirect will be set to TRUE if the data channel use directly a hsi channel.
//        m_hsiChannel will contains the free hsi channel to use.
//  The scheme is that the context ID will be set to the data channel number.
//  i.e. RIL_CHANNEL_DATA1 = CID of 1
//       RIL_CHANNEL_DATA2 = CID of 2
//       ...
//       RIL_CHANNEL_DATA5 = CID of 5
//  No other context ID
CChannel_Data* CChannel_Data::GetFreeChnlsRilHsi(UINT32& outCID, int dataProfile)
{
    RIL_LOG_VERBOSE("CChannel_Data::GetFreeChnlsRilHsi() - Enter\r\n");

    CMutex::Lock(CSystemManager::GetInstance().GetDataChannelAccessorMutex());

    extern CChannel* g_pRilChannel[RIL_CHANNEL_MAX];

    CChannel_Data* pChannelData = NULL;

    // First try to get a free RIL channel, then for class 1
    // or class 2 apn, try to get a hsi channel.
    pChannelData = CChannel_Data::GetFreeChnl(outCID);
    if (NULL == pChannelData)
    {
        // Error, all channels full!
        RIL_LOG_CRITICAL("CChannel_Data::GetFreeChnlsRilHsi() - All channels full!!\r\n");
        outCID = 0;
    }
    else
    {
        GetChnlsRilHsi(pChannelData, dataProfile);
    }

    CMutex::Unlock(CSystemManager::GetInstance().GetDataChannelAccessorMutex());

    RIL_LOG_VERBOSE("CChannel_Data::GetFreeChnlsRilHsi() - Exit\r\n");
    return pChannelData;
}

//  Used for modems supporting LTE.
//  Reserves a free RIL channel and a free hsi channel to use for data.
//  [in] UINT32 uiContextID - Context ID to be associated with CChannel_Data.
//  return value - CChannel_Data* - If successful, the free data channel, else NULL.
//  Note: If successful the returned CChannel_Data* will have the new context ID set.
//        m_hsiDirect will be set to TRUE if the data channel use directly a hsi channel.
//        m_hsiChannel will contains the free hsi channel to use.
CChannel_Data* CChannel_Data::ReserveFreeChnlsRilHsi(const UINT32 uiContextID,
        const int dataProfile)
{
    RIL_LOG_VERBOSE("CChannel_Data::ReserveFreeChnlsRilHsi() - Enter\r\n");

    CMutex::Lock(CSystemManager::GetInstance().GetDataChannelAccessorMutex());

    extern CChannel* g_pRilChannel[RIL_CHANNEL_MAX];
    CChannel_Data* pChannelData = NULL;
    CChannel_Data* pFirstFreeChannel = NULL;

    for (UINT32 i = RIL_CHANNEL_DATA1; i < g_uiRilChannelCurMax && i < RIL_CHANNEL_MAX; i++)
    {
        if (NULL == g_pRilChannel[i]) // could be NULL if reserved channel
            continue;

        CChannel_Data* pTemp = static_cast<CChannel_Data*>(g_pRilChannel[i]);
        if (pTemp && 0 == pTemp->GetContextID())
        {
            // Free data channel found
            pChannelData = pTemp;
            RIL_LOG_INFO("CChannel_Data::ReserveFreeChnlsRilHsi() - "
                    "****** Setting chnl=[%d] to CID=[%u] ******\r\n", i, uiContextID);
            pChannelData->SetContextID(uiContextID);
            GetChnlsRilHsi(pChannelData, dataProfile);
            break;
        }
    }

    if (NULL == pChannelData)
    {
        // Error, all channels full!
        RIL_LOG_CRITICAL("CChannel_Data::ReserveFreeChnlsRilHsi() - All channels full\r\n");
    }

    CMutex::Unlock(CSystemManager::GetInstance().GetDataChannelAccessorMutex());

    RIL_LOG_VERBOSE("CChannel_Data::ReserveFreeChnlsRilHsi() - Exit\r\n");
    return pChannelData;
}

void CChannel_Data::GetChnlsRilHsi(CChannel_Data* pChannelData, const int dataProfile)
{
    RIL_LOG_VERBOSE("CChannel_Data::GetChnlsRilHsi() - Enter\r\n");

    CMutex::Lock(CSystemManager::GetInstance().GetDataChannelAccessorMutex());

    int hsiChannel = -1;
    int hsiDirect = FALSE;

    if (pChannelData != NULL)
    {
        int ipcDataChannelMin = pChannelData->GetIpcDataChannelMin();
        const UINT32 uiCID = pChannelData->GetContextID();
        int profile = dataProfile & DATA_PROFILE_AOSP_MASK;
        if (profile >= 0 && profile < NUMBER_OF_APN_PROFILE)
        {
            // According to the data profile class try to associate
            // a hsi channel to the RIL channel.
            switch (m_dataProfilePathAssignation[profile])
            {
                case 1:
                    // For APN of the class 1 a hsi channel is available. Find the first available.
                    RIL_LOG_INFO("CChannel_Data::GetChnlsRilHsi() -"
                            " data profile class: %d.\r\n",
                            m_dataProfilePathAssignation[profile]);
                    hsiChannel = GetFreeHSIChannel(uiCID,
                            ipcDataChannelMin, ipcDataChannelMin + m_hsiChannelsReservedForClass1);
                    if (hsiChannel == -1)
                    {
                        RIL_LOG_INFO("CChannel_Data::GetChnlsRilHsi() - No hsi channel for"
                                " a data profile class 1.\r\n");
                        pChannelData->SetContextID(0);
                        pChannelData = NULL;
                        goto Error;
                    }
                    hsiDirect = TRUE;
                    break;

                case 2:
                    // For APN of the class 2, check if there is a free hsi channel
                    // that can be used.
                    RIL_LOG_INFO("CChannel_Data::GetChnlsRilHsi() - data profile "
                            "class: %d.\r\n", m_dataProfilePathAssignation[profile]);
                    hsiChannel = GetFreeHSIChannel(uiCID,
                            ipcDataChannelMin + m_hsiChannelsReservedForClass1,
                            ipcDataChannelMin + m_hsiDataDirect);
                    if (hsiChannel != -1)
                    {
                        hsiDirect = TRUE;
                    }
                    break;

                default:
                    break;
            }
        }
        pChannelData->m_hsiChannel = hsiChannel;
        pChannelData->m_hsiDirect = hsiDirect;
        pChannelData->m_dataProfile = dataProfile;
    }

Error:
    CMutex::Unlock(CSystemManager::GetInstance().GetDataChannelAccessorMutex());
    RIL_LOG_VERBOSE("CChannel_Data::GetChnlsRilHsi() - Exit\r\n");
}

int CChannel_Data::GetFreeHSIChannel(UINT32 uiCID, int sIndex, int eIndex)
{
    RIL_LOG_VERBOSE("CChannel_Data::GetFreeHSIChannel() - Enter\r\n");

    CMutex::Lock(CSystemManager::GetInstance().GetDataChannelAccessorMutex());

    if (sIndex < 0 || eIndex > RIL_MAX_NUM_IPC_CHANNEL)
    {
        RIL_LOG_VERBOSE("CChannel_Data::GetFreeHSIChannel() - Index error\r\n");
        return -1;
    }

    for (int i = sIndex; i < eIndex; i++)
    {
        if (g_uiHSIChannel[i] == 0)
        {
            g_uiHSIChannel[i] = uiCID ;
            RIL_LOG_VERBOSE("CChannel_Data::GetFreeHSIChannel() - Success - Exit\r\n");
            CMutex::Unlock(CSystemManager::GetInstance().GetDataChannelAccessorMutex());
            return i;
        }
    }

    CMutex::Unlock(CSystemManager::GetInstance().GetDataChannelAccessorMutex());

    RIL_LOG_VERBOSE("CChannel_Data::GetFreeHSIChannel() - Not Success - Exit\r\n");
    return -1;
}

bool CChannel_Data::FreeHSIChannel(UINT32 uiCID)
{
    RIL_LOG_VERBOSE("CChannel_Data::FreeHSIChannel() - Enter\r\n");

    CMutex::Lock(CSystemManager::GetInstance().GetDataChannelAccessorMutex());

    for (int i = 0; i < RIL_MAX_NUM_IPC_CHANNEL; i++)
    {
        if (g_uiHSIChannel[i] == uiCID)
        {
            g_uiHSIChannel[i] = 0 ;
            RIL_LOG_VERBOSE("CChannel_Data::FreeHSIChannel() - Exit\r\n");
            CMutex::Unlock(CSystemManager::GetInstance().GetDataChannelAccessorMutex());
            return true;
        }
    }

    CMutex::Unlock(CSystemManager::GetInstance().GetDataChannelAccessorMutex());

    RIL_LOG_VERBOSE("CChannel_Data::FreeHSIChannel() - Exit\r\n");
    return false;
}

//
//  Returns a free channel to use for data
//  [out] UINT32 outCID - If successful, the new context ID, else 0.
//  return value - CChannel_Data* - If successful, the free data channel, else NULL.
//  Note: If successful the returned CChannel_Data* will have the new context ID set.
//
//  The scheme is that the context ID will be set to the data channel number.
//  i.e. RIL_CHANNEL_DATA1 = CID of 1
//       RIL_CHANNEL_DATA2 = CID of 2
//       ...
//       RIL_CHANNEL_DATA5 = CID of 5
//  No other context ID
CChannel_Data* CChannel_Data::GetFreeChnl(UINT32& outCID)
{
    RIL_LOG_VERBOSE("CChannel_Data::GetFreeChnl() - Enter\r\n");

    CMutex::Lock(CSystemManager::GetInstance().GetDataChannelAccessorMutex());

    extern CChannel* g_pRilChannel[RIL_CHANNEL_MAX];
    CChannel_Data* pChannelData = NULL;

    for (UINT32 i = RIL_CHANNEL_DATA1; i < g_uiRilChannelCurMax && i < RIL_CHANNEL_MAX; i++)
    {
        if (NULL == g_pRilChannel[i]) // could be NULL if reserved channel
            continue;

        CChannel_Data* pTemp = static_cast<CChannel_Data*>(g_pRilChannel[i]);
        if (pTemp && pTemp->GetContextID() == 0)
        {
            //  We found a free data channel.
            pChannelData = pTemp;
            outCID = (i - RIL_CHANNEL_DATA1) + 1;

            RIL_LOG_INFO("CChannel_Data::GetFreeChnl() - ****** Setting chnl=[%d] to CID=[%d]"
                    " ******\r\n", i, outCID);
            pChannelData->SetContextID(outCID);
            break;
        }
    }

Error:
    if (NULL == pChannelData)
    {
        // Error, all channels full!
        RIL_LOG_CRITICAL("CChannel_Data::GetFreeChnl() - All channels full!!\r\n");
        outCID = 0;
    }

    CMutex::Unlock(CSystemManager::GetInstance().GetDataChannelAccessorMutex());

    RIL_LOG_VERBOSE("CChannel_Data::GetFreeChnl() - Exit\r\n");
    return pChannelData;
}

//
//  Returns the Data Channel for the specified RIL channel number
//
CChannel_Data* CChannel_Data::GetChnlFromRilChannelNumber(UINT32 index)
{
    RIL_LOG_VERBOSE("CChannel_Data::GetChnlFromRilChannelNumber() - Enter\r\n");

    CMutex::Lock(CSystemManager::GetInstance().GetDataChannelAccessorMutex());

    extern CChannel* g_pRilChannel[RIL_CHANNEL_MAX];
    CChannel_Data* pChannelData = NULL;

    for (UINT32 i = RIL_CHANNEL_DATA1; i < g_uiRilChannelCurMax && i < RIL_CHANNEL_MAX; i++)
    {
        if (NULL == g_pRilChannel[i]) // could be NULL if reserved channel
            continue;

        CChannel_Data* pTemp = static_cast<CChannel_Data*>(g_pRilChannel[i]);
        if (pTemp && pTemp->GetRilChannel() == index)
        {
            pChannelData = pTemp;
            break;
        }
    }

Error:
    CMutex::Unlock(CSystemManager::GetInstance().GetDataChannelAccessorMutex());

    RIL_LOG_VERBOSE("CChannel_Data::GetChnlFromRilChannelNumber() - Exit\r\n");
    return pChannelData;
}



UINT32 CChannel_Data::GetContextID() const
{
    RIL_LOG_VERBOSE("CChannel_Data::GetContextID() - Enter\r\n");

    CMutex::Lock(CSystemManager::GetInstance().GetDataChannelAccessorMutex());

    UINT32 nCID = m_uiContextID;

    CMutex::Unlock(CSystemManager::GetInstance().GetDataChannelAccessorMutex());

    RIL_LOG_VERBOSE("CChannel_Data::GetContextID() - Exit\r\n");
    return nCID;
}

//
//  Sets this channel's context ID value
//
BOOL CChannel_Data::SetContextID(UINT32 dwContextID)
{
    RIL_LOG_VERBOSE("CChannel_Data::SetContextID() - Enter\r\n");

    CMutex::Lock(CSystemManager::GetInstance().GetDataChannelAccessorMutex());

    m_uiContextID = dwContextID;

    CMutex::Unlock(CSystemManager::GetInstance().GetDataChannelAccessorMutex());

    RIL_LOG_VERBOSE("CChannel_Data::SetContextID() - Exit\r\n");
    return TRUE;
}

void CChannel_Data::ResetDataCallInfo()
{
    RIL_LOG_VERBOSE("CChannel_Data::ResetDataCallInfo() - Enter\r\n");

    SetDataFailCause(PDP_FAIL_NONE);
    SetDataState(E_DATA_STATE_IDLE);

    UINT32 uiModemType = CTE::GetTE().GetModemType();
    if (MODEM_TYPE_XMM6360 == uiModemType
            || MODEM_TYPE_XMM7160 == uiModemType
            || MODEM_TYPE_XMM7260 == uiModemType)
    {
        FreeHSIChannel(m_uiContextID);
    }

    SetContextID(0);

    m_refCount = 0;
    m_isRoutingEnabled = FALSE;
    m_szApn[0] = '\0';
    m_szPdpType[0] = '\0';
    m_szInterfaceName[0] = '\0';

    delete[] m_pszIpAddresses;
    m_pszIpAddresses = NULL;
    delete[] m_pszDnses;
    m_pszDnses = NULL;
    delete[] m_pszGateways;
    m_pszGateways = NULL;
    delete[] m_pszPcscfes;
    m_pszPcscfes = NULL;

    RIL_LOG_VERBOSE("CChannel_Data::ResetDataCallInfo() - Exit\r\n");
}

void CChannel_Data::SetDataFailCause(int cause)
{
    RIL_LOG_VERBOSE("CChannel_Data::SetDataFailCause() - Enter\r\n");

    m_dataFailCause = cause;

    RIL_LOG_VERBOSE("CChannel_Data::SetDataFailCause() - Exit\r\n");
}

int CChannel_Data::GetDataFailCause()
{
    RIL_LOG_VERBOSE("CChannel_Data::GetDataFailCause() - Enter/Exit\r\n");
    return m_dataFailCause;
}

void CChannel_Data::SetApn(const char* pApn)
{
    RIL_LOG_VERBOSE("CChannel_Data::SetApn() - Enter/Exit\r\n");

    CopyStringNullTerminate(m_szApn, pApn, MAX_BUFFER_SIZE);
}

void CChannel_Data::GetApn(char* pApn, const int maxSize)
{
    RIL_LOG_VERBOSE("CChannel_Data::GetApn() - Enter\r\n");

    if (NULL != pApn && 0 < maxSize)
    {
        CopyStringNullTerminate(pApn, m_szApn, maxSize);
    }

    RIL_LOG_VERBOSE("CChannel_Data::GetApn() - Exit\r\n");
}

BOOL CChannel_Data::IsApnEqual(const char* pApn)
{
    RIL_LOG_VERBOSE("CChannel_Data::IsApnEqual() - Enter\r\n");

    BOOL bRet = FALSE;

    if (m_szApn[0] == '\0' || NULL == pApn)
    {
        bRet = TRUE;
    }
    else
    {
        if (NULL != strcasestr(m_szApn, pApn))
        {
            bRet = TRUE;
        }
    }

    return bRet;
}

void CChannel_Data::SetPdpType(const char* pPdpType)
{
    RIL_LOG_VERBOSE("CChannel_Data::SetPdpType() - Enter\r\n");

    strncpy(m_szPdpType, pPdpType, MAX_PDP_TYPE_SIZE-1);
    m_szPdpType[MAX_PDP_TYPE_SIZE-1] = '\0';

    RIL_LOG_VERBOSE("CChannel_Data::SetPdpType() - Exit\r\n");
}

void CChannel_Data::GetPdpType(char* pPdpType, const int maxSize)
{
    RIL_LOG_VERBOSE("CChannel_Data::GetPdpType() - Enter\r\n");

    if (NULL != pPdpType && 0 < maxSize)
    {
        strncpy(pPdpType, m_szPdpType, maxSize-1);
        pPdpType[maxSize-1] = '\0';
    }

    RIL_LOG_VERBOSE("CChannel_Data::GetPdpType() - Exit\r\n");
}

void CChannel_Data::SetInterfaceName(const char* pInterfaceName)
{
    RIL_LOG_VERBOSE("CChannel_Data::SetInterfaceName() - Enter\r\n");

    strncpy(m_szInterfaceName, pInterfaceName, MAX_INTERFACE_NAME_SIZE-1);
    m_szInterfaceName[MAX_INTERFACE_NAME_SIZE-1] = '\0';

    RIL_LOG_VERBOSE("CChannel_Data::SetInterfaceName() - Exit\r\n");
}

void CChannel_Data::GetInterfaceName(char* pInterfaceName, const int maxSize)
{
    RIL_LOG_VERBOSE("CChannel_Data::GetInterfaceName() - Enter\r\n");

    if (NULL != pInterfaceName && 0 < maxSize)
    {
        strncpy(pInterfaceName, m_szInterfaceName, maxSize-1);
        pInterfaceName[maxSize-1] = '\0';
    }

    RIL_LOG_VERBOSE("CChannel_Data::GetInterfaceName() - Exit\r\n");
}

void CChannel_Data::SetDataState(int state)
{
    RIL_LOG_VERBOSE("CChannel_Data::SetDataState() - Enter\r\n");

    CMutex::Lock(CSystemManager::GetInstance().GetDataChannelAccessorMutex());

    m_dataState = state;

    CMutex::Unlock(CSystemManager::GetInstance().GetDataChannelAccessorMutex());

    RIL_LOG_VERBOSE("CChannel_Data::SetDataState() - Exit\r\n");
}

int CChannel_Data::GetDataState()
{
    RIL_LOG_VERBOSE("CChannel_Data::GetDataState() - Enter\r\n");

    CMutex::Lock(CSystemManager::GetInstance().GetDataChannelAccessorMutex());

    int state = m_dataState;

    CMutex::Unlock(CSystemManager::GetInstance().GetDataChannelAccessorMutex());

    RIL_LOG_VERBOSE("CChannel_Data::GetDataState() - Exit\r\n");
    return state;
}

int CChannel_Data::GetMuxControlChannel()
{
    RIL_LOG_VERBOSE("CChannel_Data::GetMuxControlChannel() - Enter\r\n");

    int muxControlChannel = -1;
    UINT32 uiRilChannel = GetRilChannel();

    // Get the mux channel id corresponding to the control of the data channel
    switch (uiRilChannel)
    {
        case RIL_CHANNEL_DATA1:
            sscanf(g_szDataPort1, "/dev/gsmtty%d", &muxControlChannel);
            break;
        case RIL_CHANNEL_DATA2:
            sscanf(g_szDataPort2, "/dev/gsmtty%d", &muxControlChannel);
            break;
        case RIL_CHANNEL_DATA3:
            sscanf(g_szDataPort3, "/dev/gsmtty%d", &muxControlChannel);
            break;
        case RIL_CHANNEL_DATA4:
            sscanf(g_szDataPort4, "/dev/gsmtty%d", &muxControlChannel);
            break;
        case RIL_CHANNEL_DATA5:
            sscanf(g_szDataPort5, "/dev/gsmtty%d", &muxControlChannel);
            break;
        default:
            RIL_LOG_CRITICAL("CChannel_Data::GetMuxControlChannel() - Unknown mux channel"
                    "for RIL Channel [%u] \r\n", uiRilChannel);
    }

    RIL_LOG_VERBOSE("CChannel_Data::GetMuxControlChannel() - Exit\r\n");

    return muxControlChannel;
}

void CChannel_Data::RemoveInterface(BOOL bKeepInterfaceUp)
{
    RIL_LOG_VERBOSE("CChannel_Data::RemoveInterface() - Enter\r\n");

    char szNetworkInterfaceName[MAX_INTERFACE_NAME_SIZE] = {'\0'};
    CChannel_Data* pChannelData = NULL;
    struct gsm_netconfig netconfig;
    int fd = GetFD();
    int ret = -1;
    int s = -1;
    UINT32 uiRilChannel = GetRilChannel();
    BOOL bIsHSIDirect = IsHSIDirect();
    int state = GetDataState();

    GetInterfaceName(szNetworkInterfaceName, sizeof(szNetworkInterfaceName));

    RIL_LOG_INFO("CChannel_Data::RemoveInterface() - szNetworkInterfaceName=[%s]\r\n",
            szNetworkInterfaceName);

    if (!bIsHSIDirect && bKeepInterfaceUp)
    {
        RIL_LOG_CRITICAL("CChannel_Data::RemoveInterface() : cannot keep TTY interface up.\r\n");
        bKeepInterfaceUp = FALSE;
    }

    if (!bIsHSIDirect)
    {
        if (E_DATA_STATE_IDLE != state
                && E_DATA_STATE_INITING != state
                && E_DATA_STATE_ACTIVATING != state)
        {
            // Blocking TTY flow.
            // Security in order to avoid IP data in response buffer.
            // Not mandatory.
            BlockAndFlushChannel(BLOCK_CHANNEL_BLOCK_TTY,
                    FLUSH_CHANNEL_NO_FLUSH);
        }

        // Put the channel back into AT command mode
        netconfig.adaption = 3;
        netconfig.protocol = htons(ETH_P_IP);

        if (fd >= 0)
        {
            RIL_LOG_INFO("CChannel_Data::RemoveInterface() -"
                    " ***** PUTTING channel=[%u] in AT COMMAND MODE *****\r\n", uiRilChannel);
            ret = ioctl(fd, GSMIOC_DISABLE_NET, &netconfig);
        }
    }
    else
    {
        /*
         * HSI inteface ENABLE/DISABLE can be done only by the HSI driver.
         * Rapid RIL can only bring up or down the interface.
         */
        RIL_LOG_INFO("CChannel_Data::RemoveInterface() : Bring down hsi network interface\r\n");

        // Open socket for ifconfig and setFlags commands
        s = socket(AF_INET, SOCK_DGRAM, 0);
        if (s < 0)
        {
            RIL_LOG_CRITICAL("CChannel_Data::RemoveInterface() : cannot open control socket "
                    "(error %d / '%s')\n", errno, strerror(errno));
            goto Error;
        }

        struct ifreq ifr;
        memset(&ifr, 0, sizeof(struct ifreq));
        strncpy(ifr.ifr_name, szNetworkInterfaceName, IFNAMSIZ-1);
        ifr.ifr_name[IFNAMSIZ-1] = '\0';

        // set ipv4 address to 0.0.0.0 to unset ipv4 address,
        // ipv6 addresses are automatically cleared
        if (!setaddr(s, &ifr, "0.0.0.0"))
        {
            RIL_LOG_CRITICAL("CChannel_Data::RemoveInterface() : Error removing addr\r\n");
        }

        if (!setflags(s, &ifr, 0, IFF_UP))
        {
            RIL_LOG_CRITICAL("CChannel_Data::RemoveInterface() : Error setting flags\r\n");
        }

        if (bKeepInterfaceUp)
        {
            /* Need to put the interface back up to flush out packets that might still be received
             * at modem side.
             *
             * Previous 'down' is needed to remove all 'manual' routes that might still use this
             * interface.
             */
            if (!setflags(s, &ifr, IFF_UP, 0))
            {
                RIL_LOG_CRITICAL("CChannel_Data::RemoveInterface() : Error setting flags\r\n");
            }
        }

        if (close(s) < 0)
        {
            RIL_LOG_CRITICAL("CChannel_Data::RemoveInterface() : could not close control socket "
                    "(error %d / '%s')\n", errno, strerror(errno));
        }
    }

    RIL_LOG_INFO("[RIL STATE] PDP CONTEXT DEACTIVATION chnl=%u\r\n", uiRilChannel);

Error:
    if (!bIsHSIDirect)
    {
        if (E_DATA_STATE_IDLE != state
                && E_DATA_STATE_INITING != state
                && E_DATA_STATE_ACTIVATING != state)
        {
            // Flush buffers and Unblock read thread.
            // Security in order to avoid IP data in response buffer.
            // Will unblock Channel read thread and TTY.
            // Unblock read thread whatever the result is to avoid forever block
            FlushAndUnblockChannel(UNBLOCK_CHANNEL_UNBLOCK_ALL,
                    FLUSH_CHANNEL_FLUSH_ALL);
        }
    }
}

void CChannel_Data::GetDataConnectionType(PDP_TYPE& eDataConnectionType)
{
    RIL_LOG_VERBOSE("CChannel_Data::GetDataConnectionType() - Enter\r\n");

    if (strcmp(m_szPdpType, "IP") == 0)
    {
        eDataConnectionType = PDP_TYPE_IPV4;
    }
    else if (strcmp(m_szPdpType, "IPV6") == 0)
    {
        eDataConnectionType = PDP_TYPE_IPV6;
    }
    else if (strcmp(m_szPdpType, "IPV4V6") == 0)
    {
        eDataConnectionType = PDP_TYPE_IPV4V6;
    }
    else
    {
        eDataConnectionType = PDP_TYPE_UNKNOWN;
        RIL_LOG_CRITICAL("CChannel_Data::GetDataConnectionType() - PdpType: %s "
                "not supported\r\n", m_szPdpType);
    }

    RIL_LOG_VERBOSE("CChannel_Data::GetDataConnectionType() - Exit\r\n");
}

//////////////////////////////////
// helper functions
//
// Adds an address string to address array, indexed by cid
//
bool CChannel_Data::AddAddressString(const ADDR_TYPE type, const char* pszAddress)
{
    RIL_LOG_VERBOSE("CChannel_Data::AddAddressString() - Enter\r\n");
    BOOL bRet = FALSE;
    char* pszAddr = NULL;
    char* pszAddrTemp = NULL;
    static const char* const apszType[] = {"IP", "DNS", "GATEWAY", "PCSCF"};
    UINT32 uiLength = 0;

    // we do not add invalid address (0.0.0.0 for Ipv4 or :: for IPv6) and empty address
    if (NULL == pszAddress
            || strcmp(pszAddress, "0.0.0.0") == 0 || strcmp(pszAddress, "::") == 0)
    {
        RIL_LOG_WARNING("CChannel_Data::AddAddressString() - Invalid address!\r\n");
        goto Error;
    }

    uiLength = strlen(pszAddress);

    if (0 == uiLength)
    {
        RIL_LOG_VERBOSE("CChannel_Data::AddAddressString() - Invalid length!\r\n");
        goto Error;
    }

    switch (type)
    {
        case ADDR_IP:
            pszAddrTemp = m_pszIpAddresses;
            break;

        case ADDR_DNS:
            pszAddrTemp = m_pszDnses;
            break;

        case ADDR_GATEWAY:
            pszAddrTemp = m_pszGateways;
            break;

        case ADDR_PCSCF:
            pszAddrTemp = m_pszPcscfes;
            break;

        default: // undefined type
            RIL_LOG_WARNING("CChannel_Data::AddAddressString() - Undefined type\r\n");
            goto Error;
    }

    // new address string
    if (NULL == pszAddrTemp || '\0' == pszAddrTemp[0])
    {
        pszAddr = new char[uiLength + 1];
        if (NULL == pszAddr)
        {
            RIL_LOG_CRITICAL("CChannel_Data::AddAddressString() : Failed to allocate pszAddr!\r\n");
            goto Error;
        }

        snprintf(pszAddr, uiLength + 1, "%s", pszAddress);

        // memory leak if pointer is non-NULL
        delete[] pszAddrTemp;
        pszAddrTemp = pszAddr;

        RIL_LOG_INFO("CChannel_Data::AddAddressString() - Added %s address [%s],"
                "cid=%u\r\n", apszType[type], pszAddress, m_uiContextID);
    }
    // new address received, add it in the existing address string,
    // if the address is not already part of the string
    else if (strstr(pszAddrTemp, pszAddress) == NULL)
    {
        size_t oldLen = strlen(pszAddrTemp);
        size_t newAddLen = strlen(pszAddress);
        // one space as separator and one for 0 terminal
        size_t finalLen = oldLen + 1 + newAddLen + 1;

        pszAddr = new char[finalLen];
        if (NULL == pszAddr)
        {
            RIL_LOG_CRITICAL("CChannel_Data::AddAddressString() : Failed to allocate pszAddr!\r\n");
            goto Error;
        }

        snprintf(pszAddr, finalLen, "%s %s", pszAddrTemp, pszAddress);

        delete[] pszAddrTemp;
        pszAddrTemp = pszAddr;

        RIL_LOG_INFO("CChannel_Data::AddAddressString() - Added new %s address [%s],"
                "cid=%u\r\n", apszType[type], pszAddress, m_uiContextID);
    }

    // restore address pointer
    switch (type)
    {
        case ADDR_IP:
            m_pszIpAddresses = pszAddrTemp;
            break;

        case ADDR_DNS:
            m_pszDnses = pszAddrTemp;
            break;

        case ADDR_GATEWAY:
            m_pszGateways = pszAddrTemp;
            break;

        case ADDR_PCSCF:
            m_pszPcscfes = pszAddrTemp;
            break;

        default: // undefined type
            RIL_LOG_WARNING("CChannel_Data::AddAddressString() - Undefined type\r\n");
            goto Error;
    }

    bRet = TRUE;

Error:
    if (!bRet)
    {
        delete[] pszAddr;
        pszAddr = NULL;
    }
    RIL_LOG_VERBOSE("CChannel_Data::AddAddressString() - Exit\r\n");
    return bRet;
}

//
// Delete an addresses string from address array, indexed by cid
//
void CChannel_Data::DeleteAddressesString(const ADDR_TYPE type)
{
    switch (type)
    {
        case ADDR_IP:
            delete[] m_pszIpAddresses;
            m_pszIpAddresses = NULL;
            break;

        case ADDR_DNS:
            delete[] m_pszDnses;
            m_pszDnses = NULL;
            break;

        case ADDR_GATEWAY:
            delete[] m_pszGateways;
            m_pszGateways = NULL;
            break;

        case ADDR_PCSCF:
            delete[] m_pszPcscfes;
            m_pszPcscfes = NULL;
            break;

        default: // undefined type
            RIL_LOG_CRITICAL("CChannel_Data::DeleteAddressesString() - Undefined type\r\n");
            break;
    }
}

//
// Copy the addresses string of type type into pszAddresses,
// a pointer to a buffer of size addressBufferSize
// the buffer is not modified if the addresses string does not exist or is too long
// @param pszAddresses : the buffer to copy the adresses string in
// @param type: the type of addresses (see ADDR_TYPE)
// @param addressBufferSize: the size of pszAddresses buffer
//
void CChannel_Data::GetAddressString(char* pszAddresses, const ADDR_TYPE type,
        const int addressBufferSize)
{
    RIL_LOG_VERBOSE("CChannel_Data::GetAddressString() - Enter\r\n");

    BOOL bRes = FALSE;

    if (pszAddresses != NULL)
    {
        switch (type)
        {
            case ADDR_IP:
                if (NULL != m_pszIpAddresses && addressBufferSize > strlen(m_pszIpAddresses))
                {
                    strcpy(pszAddresses, m_pszIpAddresses);
                    bRes = TRUE;
                }
                break;

            case ADDR_DNS:
                if (NULL != m_pszDnses && addressBufferSize > strlen(m_pszDnses))
                {
                    strcpy(pszAddresses, m_pszDnses);
                    bRes = TRUE;
                }
                break;

            case ADDR_GATEWAY:
                if (NULL != m_pszGateways && addressBufferSize > strlen(m_pszGateways))
                {
                    strcpy(pszAddresses, m_pszGateways);
                    bRes = TRUE;
                }
                break;

            case ADDR_PCSCF:
                if (NULL != m_pszPcscfes && addressBufferSize > strlen(m_pszPcscfes))
                {
                    strcpy(pszAddresses, m_pszPcscfes);
                    bRes = TRUE;
                }
                break;

            default: // undefined type
                RIL_LOG_CRITICAL("CChannel_Data::GetAddressString() - Undefined address type\r\n");
                bRes = TRUE;
                break;
        }
    }

    if (!bRes)
    {
        RIL_LOG_CRITICAL("CChannel_Data::GetAddressString() - Overflow or NULL address\r\n");
    }
    RIL_LOG_VERBOSE("CChannel_Data::GetAddressString() - Exit\r\n");
}
