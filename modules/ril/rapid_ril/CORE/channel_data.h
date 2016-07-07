////////////////////////////////////////////////////////////////////////////
// channel_Data.h
//
// Copyright 2005-2007 Intrinsyc Software International, Inc.  All rights reserved.
// Patents pending in the United States of America and other jurisdictions.
//
//
// Description:
//    Defines the CChannel_Data class, which is used to
//    facilitate the use of multiple data channels.
//    GPRS/UMTS data (1st primary context)
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(RIL_CHANNEL_DATA_H)
#define RIL_CHANNEL_DATA_H

#include "channel_nd.h"
#include "nd_structs.h"

class CChannel_Data : public CChannel
{
public:
    enum ADDR_TYPE {
        ADDR_IP,
        ADDR_DNS,
        ADDR_GATEWAY,
        ADDR_PCSCF
    };

    CChannel_Data(UINT32 uiChannel);
    virtual ~CChannel_Data();

private:
    //  Prevent assignment: Declared but not implemented.
    CChannel_Data(const CChannel_Data& rhs);  // Copy Constructor
    CChannel_Data& operator=(const CChannel_Data& rhs);  //  Assignment operator

public:
    //  public port interface
    BOOL OpenPort();

    /*
     * Resets the data call information such as context ID, data state,
     * fail cause, apn, ip address, DNS, network interface, gateway and
     * the pdp type. Based on modem type, also frees the HSI channel.
     */
    void ResetDataCallInfo();

    /*
     * Based on the channel settings, brings down the hsi network interface
     * or puts the channel back into AT command mode.
     *
     * If bKeepInterfaceUp is TRUE, keep kernel data interface up after
     * deactivation (to flush possible data coming from modem).
     */
    void RemoveInterface(BOOL bKeepInterfaceUp = FALSE);

    // get / set functions
    void SetDataFailCause(int cause);
    int GetDataFailCause();

    UINT32 GetContextID() const;
    BOOL SetContextID( UINT32 dwContextID );

    void SetApn(const char* pApn);
    void GetApn(char* pApn, const int maxSize);
    BOOL IsApnEqual(const char* pApn);

    void SetPdpType(const char* pPdpType);
    void GetPdpType(char* pPdpType, const int maxSize);

    void SetInterfaceName(const char* pInterfaceName);
    void GetInterfaceName(char* pInterfaceName, const int maxSize);

    void SetDataState(int state);
    int GetDataState();

    //
    // helper functions to convert ContextID, Dlci and Channel
    //
    static CChannel_Data* GetChnlFromContextID(UINT32 dwContextID);
    static CChannel_Data* GetChnlFromChildContextID(UINT32 dwContextID);
    static CChannel_Data* GetChnlFromIfName(const char * ifName);
    static CChannel_Data* GetChnlFromRilChannelNumber(UINT32 index);
    static bool IsDataConnectionActive();
    static CChannel_Data* ReserveFreeChnlsRilHsi(const UINT32 uiContextID, const int dataProfile);
    static void GetChnlsRilHsi(CChannel_Data* pChannelData, const int dataProfile);
    static UINT32 GetFirstActiveDataConnectionCid();

    // used by 6360 and 7160 modems.
    static int GetFreeHSIChannel(UINT32 uiCID, int sindex, int eIndex);
    static bool FreeHSIChannel(UINT32 uiCID);

    //  This function returns the next free data channel.  Also, populates the
    //  context ID of returned channel.
    //  If error, then NULL is returned.
    static CChannel_Data* GetFreeChnl(UINT32& outCID);
    // used by 6360 and 7160 modems.
    static CChannel_Data* GetFreeChnlsRilHsi(UINT32& outCID, int dataProfile);

    // used by 6360 and 7160 modems.
    int GetDataProfile() { return m_dataProfile; };
    int GetHSIChannel() { return m_hsiChannel; };
    BOOL IsHSIDirect() { return m_hsiDirect; };

    char* GetModemResourceName() { return m_szModemResourceName; }
    int GetIpcDataChannelMin() { return m_ipcDataChannelMin; }

    int GetMuxControlChannel();

    void SetRoutingEnabled(BOOL isEnabled) { m_isRoutingEnabled = isEnabled; }
    BOOL IsRoutingEnabled() { return m_isRoutingEnabled; }

    void IncrementRefCount() { m_refCount++; }
    void DecrementRefCount()
    {
        m_refCount = (m_refCount > 0) ? m_refCount - 1 : 0;
    }

    int GetRefCount() { return m_refCount; }

    BOOL HasChildContextID(UINT32 uiCID);
    void AddChildContextID(UINT32 uiCID);
    void RemoveChildContextID(UINT32 uiCID);
    void ClearChildsContextID();

    void GetDataConnectionType(PDP_TYPE& pszDataConnectionType);

    //
    // helper functions to handle IP/DNS/PCSCF/GATEWAY addresses
    //
    bool AddAddressString(const ADDR_TYPE type, const char* pszAddress);
    void DeleteAddressesString(const ADDR_TYPE type);
    void GetAddressString(char* pszAddresses, const ADDR_TYPE type, const int addressBufferSize);

    static UINT32 MAX_CID_NUMERIC;

private:
    int m_dataFailCause;
    UINT32 m_uiContextID;
    int m_dataState;

    char m_szApn[MAX_BUFFER_SIZE];
    char m_szPdpType[MAX_PDP_TYPE_SIZE];

    char m_szInterfaceName[MAX_INTERFACE_NAME_SIZE];

    // used by 6360 and 7160 modems.
    int m_dataProfile;
    BOOL m_hsiDirect;
    int m_hsiChannel;

    char m_szModemResourceName[MAX_MDM_RESOURCE_NAME_SIZE];
    int m_ipcDataChannelMin;
    BOOL m_isRoutingEnabled;
    int m_refCount;
    UINT32 m_uiChildContexts;

    char* m_pszIpAddresses;
    char* m_pszDnses;
    char* m_pszGateways;
    char* m_pszPcscfes;

protected:
    BOOL FinishInit();
};

#endif  // RIL_CHANNEL_DATA_H
