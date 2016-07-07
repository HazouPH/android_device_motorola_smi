////////////////////////////////////////////////////////////////////////////
// initializer.h
//
// Copyright 2013 Intrinsyc Software International, Inc.  All rights reserved.
// Patents pending in the United States of America and other jurisdictions.
//
//
// Description:
//      Defines the CInitializer base class and subclasses used to
//      create channels/silos and initialize the modem at start-up.
//
/////////////////////////////////////////////////////////////////////////////

#ifndef RRIL_INITIALIZER_H
#define RRIL_INITIALIZER_H

#include "types.h"
#include "com_init_index.h"

class CChannel;
class CSilo;

enum SILO_TYPE {
    SILO_TYPE_VOICE = 0,
    SILO_TYPE_SIM,
    SILO_TYPE_SMS,
    SILO_TYPE_DATA,
    SILO_TYPE_NETWORK,
    SILO_TYPE_PHONEBOOK,
    SILO_TYPE_MISC,
    SILO_TYPE_IMS,
    SILO_TYPE_RFCOEXISTENCE,
    //9 and 10 reserved for new silos
    SILO_TYPE_COMMON = 11,
    SILO_MAX = 12
};

class CInitializer
{
public:
    CInitializer();
    virtual ~CInitializer();

    virtual BOOL Initialize();
    virtual void Destroy();

    BOOL InitializeModem();

    //  Create and initialize the channels, but don't actually open the ports.
    virtual BOOL CreateChannels();
    void DeleteChannels();

    //  For resetting modem
    BOOL OpenChannelPortsOnly();
    void CloseChannelPorts();

    BOOL CreateSilos(CChannel* pChannel, int siloConfig);

    void TriggerSimUnlockedEvent() { CEvent::Signal(m_pSimUnlockedEvent); }
    void TriggerModemPoweredOffEvent() const { CEvent::Signal(m_pModemPoweredOffEvent); }
    void TriggerRadioPoweredOnEvent() const { CEvent::Signal(m_pRadioPoweredOnEvent); }
    void TriggerInitStringCompleteEvent(UINT32 uiChannel, eComInitIndex eInitIndex);

    void ResetChannelCompletedInit();
    void ResetStartupEvents();

    CEvent* GetModemPoweredOffEvent() { return m_pModemPoweredOffEvent; }
    CEvent* GetModemBasicInitCompleteEvent() { return m_pModemBasicInitCompleteEvent; }

    int GetCancelWaitPipeFd() { return m_cancelWaitPipeFds[0]; }

private:
    //  Prevent assignment: Declared but not implemented.
    CInitializer(const CInitializer& rhs);  // Copy Constructor
    CInitializer& operator=(const CInitializer& rhs);  //  Assignment operator

    struct SILO_CONFIG {
        const char* channel; // channel repository key name
        int silosDefault; // default silo config
    };

    // This mutex manages the access to ports as a whole
    // for operations such as opening and closing, thus
    // preventing access by multiple threads.
    CMutex* m_pPortsManagerMutex;

    CChannel* CreateChannel(UINT32 eIndex);
    BOOL IsChannelUndefined(int channel);

    CSilo* CreateSilo(CChannel* pChannel, int siloType);
    int GetSiloConfig(UINT32 channel);

    // Modem initialization helper functions (called by component init functions)
    BOOL SendModemInitCommands(eComInitIndex eInitIndex);
    static void* StartModemInitializationThreadWrapper(void* pArg);
    void StartModemInitializationThread();

    // Internal Init helper functions
    void SetChannelCompletedInit(UINT32 uiChannel, eComInitIndex eInitIndex);
    BOOL IsChannelCompletedInit(UINT32 uiChannel, eComInitIndex eInitIndex);
    BOOL VerifyAllChannelsCompletedInit(eComInitIndex eInitIndex);

    CEvent* m_pModemBasicInitCompleteEvent;
    CEvent* m_pSimUnlockedEvent;
    CEvent* m_pRadioPoweredOnEvent;
    CEvent* m_pModemPoweredOffEvent;
    CEvent* m_pInitStringCompleteEvent;

    BOOL m_rgfChannelCompletedInit[RIL_CHANNEL_MAX][COM_MAX_INDEX];

    /*
     * This holds the read and write pipe descriptors. Upon data in read pipe
     * descriptor, channel response thread will cancel the response polling and
     * exit the response thread.
     */
    int m_cancelWaitPipeFds[2];
};

///////////////////////////////////////////////////////////////////////////////
//
// IPC Initializer subclasses

class CInitIPCUSB : public CInitializer
{
public:
    CInitIPCUSB();
    virtual ~CInitIPCUSB();
};

class CInitIPCHSI : public CInitializer
{
public:
    CInitIPCHSI();
    virtual ~CInitIPCHSI();

protected:
    virtual BOOL InitializeHSI();
};

#endif // RRIL_INITIALIZER_H
