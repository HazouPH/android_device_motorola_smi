////////////////////////////////////////////////////////////////////////////
// hardwareconfig.cpp
//
// Copyright (C) Intel 2014.
//
//
// Description:
//     CHardwareConfig handles the hardware configuration supported by the RIL
//
/////////////////////////////////////////////////////////////////////////////

#include "hardwareconfig.h"
#include "rillog.h"
#include "extract.h"

#include <stdio.h>

extern char* g_szClientId;

CHardwareConfig* CHardwareConfig::m_pInstance = NULL;

CHardwareConfig& CHardwareConfig::GetInstance()
{
    //RIL_LOG_VERBOSE("CHardwareConfig::GetInstance() - Enter\r\n");
    if (!m_pInstance)
    {
        m_pInstance = new CHardwareConfig;
        if (!m_pInstance)
        {
            RIL_LOG_CRITICAL("CHardwareConfig::GetInstance() - Cannot create instance\r\n");

            //  Just call exit and let rild clean everything up.
            exit(0);
        }
    }
    //RIL_LOG_VERBOSE("CSystemManager::GetInstance() - Exit\r\n");
    return *m_pInstance;
}

bool CHardwareConfig::Destroy()
{
    RIL_LOG_INFO("CHardwareConfig::Destroy() - Enter\r\n");
    if (m_pInstance)
    {
        delete m_pInstance;
        m_pInstance = NULL;
    }
    else
    {
        RIL_LOG_VERBOSE("CHardwareConfig::Destroy() - WARNING - Called with no instance\r\n");
    }
    RIL_LOG_INFO("CHardwareConfig::Destroy() - Exit\r\n");
    return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
CHardwareConfig::CHardwareConfig()
  : m_isMultiSIM(FALSE),
    m_isMultiModem(FALSE),
    m_subscriptionId(0),
    m_modemId(0),
    m_SIMId(0)
{
    RIL_LOG_INFO("CHardwareConfig::CHardwareConfig() - Enter\r\n");

    m_apHardwareConfigs = new RIL_HardwareConfig* [MAX_HARDWARE_CONFIG];

    if (m_apHardwareConfigs != NULL)
    {
        memset(m_apHardwareConfigs, 0x00, MAX_HARDWARE_CONFIG * sizeof(RIL_HardwareConfig*));
    }
    else
    {
        RIL_LOG_CRITICAL("CHardwareConfig::CHardwareConfig() - \r\n"
                "Couldn't allocate m_apHardwareConfigs");
    }

    RIL_LOG_INFO("CHardwareConfig::CHardwareConfig() - Exit\r\n");
}

///////////////////////////////////////////////////////////////////////////////
CHardwareConfig::~CHardwareConfig()
{
    RIL_LOG_INFO("CHardwareConfig::~CHardwareConfig() - Enter\r\n");

    if (m_apHardwareConfigs)
    {
        for (int i = 0; i < MAX_HARDWARE_CONFIG; i++)
        {
            delete m_apHardwareConfigs[i];
            m_apHardwareConfigs[i] = NULL;
        }

        delete [] m_apHardwareConfigs;

        m_apHardwareConfigs = NULL;
    }

    RIL_LOG_INFO("CHardwareConfig::~CHardwareConfig() - Exit\r\n");
}

//  Create the hardware config
bool CHardwareConfig::CreateHardwareConfig(tcs_cfg_t* pConfig)
{
    RIL_LOG_INFO("CHardwareConfig::CreateHardwareConfig() - ENTER\r\n");
    int count = 0;
    bool ret = FALSE;

    // Need to set the subscription ID first based on the client id
    if (!SetSubscriptionId())
    {
        RIL_LOG_CRITICAL("%s - Failed to set the subscription id", __FUNCTION__);
    }

    // Check if we are in multi modem config
    if (pConfig->nb > 1)
    {
        SetMultiModem(TRUE);
    }

    // else Check if we are in multi SIM config
    else if (pConfig->mdm[0].chs.nb > 1)
    {
        SetMultiSIM(TRUE);
    }

    for (size_t i = 0; i < pConfig->nb; i++)
    {
        for (size_t j = 0; j < pConfig->mdm[i].chs.nb; j++)
        {
            if (count++ == m_subscriptionId)
            {
                RIL_HardwareConfig* modemConfig = new RIL_HardwareConfig();
                modemConfig->type = RIL_HARDWARE_CONFIG_MODEM;
                snprintf(modemConfig->uuid, RIL_HARDWARE_CONFIG_UUID_LENGTH, "%s%d", "modem", i);
                modemConfig->state = RIL_HARDWARE_CONFIG_STATE_ENABLED;
                modemConfig->cfg.modem.rilModel = 0;
                modemConfig->cfg.modem.rat = 0;
                modemConfig->cfg.modem.maxVoice = 1;
                modemConfig->cfg.modem.maxData = 1;
                modemConfig->cfg.modem.maxStandby = 1;
                m_apHardwareConfigs[0] = modemConfig;
                m_modemId = i;

                RIL_HardwareConfig* simConfig = new RIL_HardwareConfig();
                simConfig->type = RIL_HARDWARE_CONFIG_SIM;
                snprintf(simConfig->uuid, RIL_HARDWARE_CONFIG_UUID_LENGTH, "%s%d", "sim", j);
                simConfig->state = RIL_HARDWARE_CONFIG_STATE_ENABLED;
                snprintf(simConfig->cfg.sim.modemUuid,
                        RIL_HARDWARE_CONFIG_UUID_LENGTH,
                        "%s%d",
                        "modem",
                        i);
                m_apHardwareConfigs[1] = simConfig;
                m_SIMId = j;

                RIL_LOG_INFO("CHardwareConfig::CreateHardwareConfig() - EXIT\r\n");
                ret = TRUE;
                return ret;
            }
        }
    }

Error:
    RIL_LOG_INFO("CHardwareConfig::CreateHardwareConfig() - EXIT\r\n");
    return ret;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
bool CHardwareConfig::SetSubscriptionId(void)
{
    int clientId  = E_RIL_CLIENT_ID_0; /* Default clientID is E_RIL_CLIENT_ID_0 */
    int conv = 0;
    bool ret = false;

    if (g_szClientId != NULL)
    {
        const char* pszDummyPtr = NULL;

        if (ExtractInt((char*)g_szClientId, conv, pszDummyPtr))
        {
            clientId = conv;
            ret = true;
        }
        else
        {
            RIL_LOG_CRITICAL("%s - Failed to extract clientId: %s",
                    __FUNCTION__,
                    g_szClientId);
            return ret;
        }
    }

    switch (clientId)
    {
    case E_RIL_CLIENT_ID_0:
        m_subscriptionId = E_RIL_SUBSCRIPION_ID_0;
        break;

    case E_RIL_CLIENT_ID_2:
        m_subscriptionId = E_RIL_SUBSCRIPION_ID_1;
        break;

    case E_RIL_CLIENT_ID_3:
        m_subscriptionId = E_RIL_SUBSCRIPION_ID_2;
        break;
    }

    return ret;
}


