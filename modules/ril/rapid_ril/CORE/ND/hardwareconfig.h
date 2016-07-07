////////////////////////////////////////////////////////////////////////////
// hardwareconfig.h
//
// Copyright (C) Intel 2014.
//
//
// Description:
//     CHardwareConfig handles the hardware configuration supported by the RIL
//
/////////////////////////////////////////////////////////////////////////////

#ifndef CHARDWARECONFIG_H
#define CHARDWARECONFIG_H

#include "types.h"
#include "tcs.h"

#include <telephony/ril.h>

// Max Hardware Config constants
const int MAX_HARDWARE_CONFIG = 2;

///////////////////////////////////////////////////////////////////////////////
class CHardwareConfig
{
public:
    static CHardwareConfig& GetInstance();
    static bool Destroy();

private:
    CHardwareConfig();
    ~CHardwareConfig();

    //  Prevent assignment: Declared but not implemented.
    CHardwareConfig(const CHardwareConfig& rhs);  // Copy Constructor
    CHardwareConfig& operator=(const CHardwareConfig& rhs);  //  Assignment operator

    // This function sets the multiple SIM flag.
    void SetMultiSIM(bool isMultiSIM) { m_isMultiSIM = isMultiSIM; }

    // This function sets the multiple modem flag.
    void SetMultiModem(bool isMultiModem) { m_isMultiModem = isMultiModem; }

    // This function sets the subscription id.
    bool SetSubscriptionId(void);

public:
    // This function returns true if we have multiple SIMs.
    bool IsMultiSIM() { return m_isMultiSIM; }

    // This function returns true if we have multiple Modems.
    bool IsMultiModem() { return m_isMultiModem; }

    // This function creates the hardware config array
    bool CreateHardwareConfig(tcs_cfg_t* pConfig);

    // This function returns the subscription id
    bool GetSubscriptionId() { return m_subscriptionId; }

    // This function returns the modem id
    bool GetModemId() { return m_modemId; }

    // This function returns the SIM id
    bool GetSIMId() { return m_SIMId; }

private:
    // Client IDs passed with "-c" option
    enum RIL_CLIENT_ID {
        E_RIL_CLIENT_ID_0 = 0,
        E_RIL_CLIENT_ID_2 = 2,
        E_RIL_CLIENT_ID_3 = 3
    };

    // Subscription IDs that will be used in the vendor ril
    enum RIL_SUBSCRIPTION_ID {
        E_RIL_SUBSCRIPION_ID_0,
        E_RIL_SUBSCRIPION_ID_1,
        E_RIL_SUBSCRIPION_ID_2
    };

    static CHardwareConfig* m_pInstance;

    bool m_isMultiSIM;

    bool m_isMultiModem;

    int m_subscriptionId;

    int m_modemId;

    int m_SIMId;

    // Hardware config array
    RIL_HardwareConfig** m_apHardwareConfigs;
};

#endif // CHARDWARECONFIG_H

