////////////////////////////////////////////////////////////////////////////
// silo_misc.cpp
//
// Copyright 2012 Intrinsyc Software International, Inc.  All rights reserved.
// Patents pending in the United States of America and other jurisdictions.
//
//
// Description:
//    Provides response handlers and parsing functions for the misc-related
//    RIL components.
//
/////////////////////////////////////////////////////////////////////////////

#include "types.h"
#include "rillog.h"
#include "response.h"
#include "rildmain.h"
#include "callbacks.h"
#include "silo_misc.h"
#include "extract.h"
#include "oemhookids.h"
#include "te.h"

#include <arpa/inet.h>

//
//
CSilo_MISC::CSilo_MISC(CChannel* pChannel)
: CSilo(pChannel)
{
    RIL_LOG_VERBOSE("CSilo_MISC::CSilo_MISC() - Enter\r\n");

    // AT Response Table
    static ATRSPTABLE pATRspTable[] =
    {
        { "+XDRVI: "   , (PFN_ATRSP_PARSE)&CSilo_MISC::ParseXDRVI  },
        { "+XTS: ", (PFN_ATRSP_PARSE)&CSilo_MISC::ParseXTS },
        { "+XFREQINFO: ", (PFN_ATRSP_PARSE)&CSilo_MISC::ParseXADPCLKFREQINFO },
        { ""           , (PFN_ATRSP_PARSE)&CSilo_MISC::ParseNULL }
    };

    m_pATRspTable = pATRspTable;
    RIL_LOG_VERBOSE("CSilo_MISC::CSilo_MISC() - Exit\r\n");
}


//
//
CSilo_MISC::~CSilo_MISC()
{
    RIL_LOG_VERBOSE("CSilo_MISC::~CSilo_MISC() - Enter\r\n");
    RIL_LOG_VERBOSE("CSilo_MISC::~CSilo_MISC() - Exit\r\n");
}

char* CSilo_MISC::GetBasicInitString()
{
    // misc silo-related channel basic init string
    const char szMiscBasicInitString[] = "+XGENDATA";

    if (!ConcatenateStringNullTerminate(m_szBasicInitString,
            sizeof(m_szBasicInitString), szMiscBasicInitString))
    {
        RIL_LOG_CRITICAL("CSilo_MISC::GetBasicInitString() : Failed to copy basic init "
                "string!\r\n");
        return NULL;
    }

    if (MODEM_TYPE_XMM2230 != CTE::GetTE().GetModemType())
    {
        if (!ConcatenateStringNullTerminate(m_szBasicInitString,
                sizeof(m_szBasicInitString), "|+XPOW=0,0,0"))
        {
            RIL_LOG_CRITICAL("CSilo_Network::GetURCInitString() : Failed to copy +XPOW "
                    "string!\r\n");
            return NULL;
        }
    }

    return m_szBasicInitString;
}

//
// Thermal sensor notification
//
BOOL CSilo_MISC::ParseXDRVI(CResponse* const pResponse, const char*& rszPointer)
{
    RIL_LOG_VERBOSE("CSilo_MISC::ParseXDRVI() - Enter\r\n");
    BOOL   fRet = FALSE;
    sOEM_HOOK_RAW_UNSOL_THERMAL_ALARM_IND* pData = NULL;
    UINT32 nIpcChrGrp;
    UINT32 nIpcChrTempThresholdInd;
    UINT32 nXdrvResult;
    UINT32 nSensorId;
    UINT32 nTemp;

    if (NULL == pResponse)
    {
        RIL_LOG_CRITICAL("CSilo_MISC::ParseXDRVI() - pResponse is NULL.\r\n");
        goto Error;
    }

    pResponse->SetUnsolicitedFlag(TRUE);

    // Extract "<IPC_CHR_GRP>"
    if (!ExtractUInt32(rszPointer, nIpcChrGrp, rszPointer))
    {
        RIL_LOG_CRITICAL("CSilo_MISC::ParseXDRVI() - Could not parse nIpcChrGrp.\r\n");
        goto Error;
    }

    // Parse <IPC_CHR_TEMP_THRESHOLD_IND>
    if (!SkipString(rszPointer, ",", rszPointer) ||
        !ExtractUInt32(rszPointer, nIpcChrTempThresholdInd, rszPointer))
    {
         RIL_LOG_CRITICAL("CSilo_MISC::ParseXDRVI() -"
                 " Unable to parse <IPC_CHR_TEMP_THRESHOLD_IND>!\r\n");
         goto Error;
    }

    // Parse <xdrv_result>
    if (!SkipString(rszPointer, ",", rszPointer) ||
        !ExtractUInt32(rszPointer, nXdrvResult, rszPointer))
    {
         RIL_LOG_CRITICAL("CSilo_MISC::ParseXDRVI() - Unable to parse <xdrv_result>!\r\n");
         goto Error;
    }

    // Parse <temp_sensor_id>
    if (!SkipString(rszPointer, ",", rszPointer) ||
        !ExtractUInt32(rszPointer, nSensorId, rszPointer))
    {
         RIL_LOG_CRITICAL("CSilo_MISC::ParseXDRVI() - Unable to parse <temp_sensor_id>!\r\n");
         goto Error;
    }

    // Parse <temp>
    if (!SkipString(rszPointer, ",", rszPointer) ||
        !ExtractUInt32(rszPointer, nTemp, rszPointer))
    {
         RIL_LOG_CRITICAL("CSilo_MISC::ParseXDRVI() - Unable to parse <temp>!\r\n");
         goto Error;
    }

    RIL_LOG_INFO("CSilo_MISC::ParseXDRVI - IPC_CHR_GRP: %u, IPC_CHR_TEMP_THRESHOLD_IND: %u,"
            " xdrv_result: %u\r\n", nIpcChrGrp, nIpcChrTempThresholdInd, nXdrvResult);
    RIL_LOG_INFO("CSilo_MISC::ParseXDRVI - temp_sensor_id: %u, temp: %u\r\n", nSensorId, nTemp);

    pData = (sOEM_HOOK_RAW_UNSOL_THERMAL_ALARM_IND*)malloc(
            sizeof(sOEM_HOOK_RAW_UNSOL_THERMAL_ALARM_IND));
    if (NULL == pData)
    {
        RIL_LOG_CRITICAL("CSilo_MISC::ParseXDRVI() - Could not allocate memory for pData.\r\n");
        goto Error;
    }
    memset(pData, 0, sizeof(sOEM_HOOK_RAW_UNSOL_THERMAL_ALARM_IND));

    pData->nCommand = RIL_OEM_HOOK_RAW_UNSOL_THERMAL_ALARM_IND;
    pData->nSensorId = nSensorId;
    pData->nTemp = nTemp;

    pResponse->SetResultCode(RIL_UNSOL_OEM_HOOK_RAW);

    if (!pResponse->SetData((void*)pData, sizeof(sOEM_HOOK_RAW_UNSOL_THERMAL_ALARM_IND), FALSE))
    {
        goto Error;
    }

    fRet = TRUE;
Error:
    if (!fRet)
    {
        free(pData);
        pData = NULL;
    }

    RIL_LOG_VERBOSE("CSilo_MISC::ParseXDRVI() - Exit\r\n");
    return fRet;
}

//
// Thermal sensor notification
//
BOOL CSilo_MISC::ParseXTS(CResponse* const pResponse, const char*& pszPointer)
{
    RIL_LOG_VERBOSE("CSilo_MISC::ParseXTS() - Enter\r\n");
    BOOL bRet = FALSE;
    sOEM_HOOK_RAW_UNSOL_THERMAL_ALARM_IND_V2* pData = NULL;
    UINT32 uiAlarmId;
    UINT32 uiOnOff;
    char szSensorId[MAX_SENSOR_ID_SIZE] = {0};
    int temp;

    if (NULL == pResponse)
    {
        RIL_LOG_CRITICAL("CSilo_MISC::ParseXTS() - pResponse is NULL.\r\n");
        goto Error;
    }

    pResponse->SetUnsolicitedFlag(TRUE);

    // Parse <temp_sensor_id>
    if (!ExtractUnquotedString(pszPointer, ",", szSensorId, sizeof(szSensorId), pszPointer))
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::ParseXTS() - Unable to parse <temp_sensor_id>!\r\n");
        goto Error;
    }

    // Parse <AlarmId>
    if (!SkipString(pszPointer, ",", pszPointer)
            || !ExtractUInt32(pszPointer, uiAlarmId, pszPointer))
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::ParseXTS() - Unable to parse <AlarmId>!\r\n");
        goto Error;
    }

    // Parse <OnOff>
    if (!SkipString(pszPointer, ",", pszPointer)
            || !ExtractUInt32(pszPointer, uiOnOff, pszPointer))
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::ParseXTS() - Unable to parse <onOff>!\r\n");
        goto Error;
    }

    // Parse <Temp>
    if (!SkipString(pszPointer, ",", pszPointer)
            || !ExtractInt(pszPointer, temp, pszPointer))
    {
        RIL_LOG_CRITICAL("CTE_XMM6260::ParseXTS() - Unable to parse <Temp>!\r\n");
        goto Error;
    }

    RIL_LOG_INFO("CSilo_MISC::ParseXTS - Sensor: %s, OnOff: %u, temp: %d\r\n",
            szSensorId, uiOnOff, temp);

    pData = (sOEM_HOOK_RAW_UNSOL_THERMAL_ALARM_IND_V2*)malloc(
            sizeof(sOEM_HOOK_RAW_UNSOL_THERMAL_ALARM_IND_V2));
    if (NULL == pData)
    {
        RIL_LOG_CRITICAL("CSilo_MISC::ParseXTS() - Could not allocate memory for pData.\r\n");
        goto Error;
    }
    memset(pData, 0, sizeof(sOEM_HOOK_RAW_UNSOL_THERMAL_ALARM_IND_V2));

    pData->command = RIL_OEM_HOOK_RAW_UNSOL_THERMAL_ALARM_IND_V2;
    CopyStringNullTerminate(pData->szSensorName, szSensorId, sizeof(pData->szSensorName));
    pData->sensorNameLength = strlen(pData->szSensorName);
    pData->temperature = temp;
    pResponse->SetResultCode(RIL_UNSOL_OEM_HOOK_RAW);

    if (!pResponse->SetData((void*)pData, sizeof(sOEM_HOOK_RAW_UNSOL_THERMAL_ALARM_IND_V2), FALSE))
    {
        goto Error;
    }

    bRet = TRUE;
Error:
    if (!bRet)
    {
        free(pData);
        pData = NULL;
    }

    RIL_LOG_VERBOSE("CSilo_MISC::ParseXTS() - Exit\r\n");
    return bRet;
}

BOOL CSilo_MISC::ParseXADPCLKFREQINFO(CResponse* const pResponse,
        const char*& pszPointer)
{
    RIL_LOG_VERBOSE("CSilo_MISC::ParseXADPCLKFREQINFO() - Enter\r\n");
    BOOL bRet = FALSE;
    sOEM_HOOK_RAW_UNSOL_ADPCLK_FREQ_INFO_NOTIF* pData = NULL;
    long long centFreq;
    int freqSpread;
    int noisePower;
    int nParams = 1;
    RIL_RESULT_CODE res = RRIL_RESULT_ERROR;

    if (NULL == pResponse)
    {
        RIL_LOG_CRITICAL("CSilo_MISC::ParseXADPCLKFREQINFO() - pResponse is NULL.\r\n");
        goto Error;
    }

    // If response is received here, than we are sure that is the unsolicited one.
    pResponse->SetUnsolicitedFlag(TRUE);

    // Expected URC: +XFREQINFO: <centFreq>, <freqSpread>, <noisePower>
    // Parse <centFreq>
    if (!ExtractLongLong(pszPointer, centFreq, pszPointer))
    {
        RIL_LOG_CRITICAL("CSilo_MISC::ParseXADPCLKFREQINFO() -"
                " Unable to parse <centFreq>\r\n");
        goto Error;
    }

    // Parse <freqSpread>
    if (!SkipString(pszPointer, ",", pszPointer)
             || !ExtractInt(pszPointer, freqSpread, pszPointer))
    {
        RIL_LOG_CRITICAL("CSilo_MISC::ParseXADPCLKFREQINFO() -"
                " Unable to parse <freqSpread>\r\n");
        goto Error;
    }

    // Parse <noisePower>
    if (!SkipString(pszPointer, ",", pszPointer)
             || !ExtractInt(pszPointer, noisePower, pszPointer))
    {
        RIL_LOG_CRITICAL("CSilo_MISC::ParseXADPCLKFREQINFO() -"
                " Unable to parse <noisePower>\r\n");
        goto Error;
    }

    RIL_LOG_INFO("CSilo_MISC::ParseXADPCLKFREQINFO - centFreq: %lld, freqSpread: %d,"
            " noisePower: %d\r\n", centFreq, freqSpread, noisePower);

    pData = (sOEM_HOOK_RAW_UNSOL_ADPCLK_FREQ_INFO_NOTIF*) malloc(
            sizeof(sOEM_HOOK_RAW_UNSOL_ADPCLK_FREQ_INFO_NOTIF));
    if (NULL == pData)
    {
        RIL_LOG_CRITICAL("CSilo_MISC::ParseXADPCLKFREQINFO() -"
                "Could not allocate memory for response");
        goto Error;
    }
    memset(pData, 0, sizeof(sOEM_HOOK_RAW_UNSOL_ADPCLK_FREQ_INFO_NOTIF));

    pData->commandId = RIL_OEM_HOOK_RAW_UNSOL_ADPCLK_FREQ_INFO_NOTIF;
    pData->centFreq = centFreq;
    pData->freqSpread = freqSpread;
    pData->noisePower = noisePower;
    pResponse->SetResultCode(RIL_UNSOL_OEM_HOOK_RAW);

    if (!pResponse->SetData((void*)pData,
            sizeof(sOEM_HOOK_RAW_UNSOL_ADPCLK_FREQ_INFO_NOTIF), FALSE))
    {
        goto Error;
    }

    bRet = TRUE;
Error:
    if (!bRet)
    {
        free(pData);
        pData = NULL;
    }

    RIL_LOG_VERBOSE("CSilo_MISC::ParseXADPCLKFREQINFO() - Exit\r\n");
    return bRet;
}