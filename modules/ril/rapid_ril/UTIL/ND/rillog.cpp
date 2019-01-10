////////////////////////////////////////////////////////////////////////////
// rillog.cpp
//
// Copyright 2009 Intrinsyc Software International, Inc.  All rights reserved.
// Patents pending in the United States of America and other jurisdictions.
//
//
// Description:
//    Implements RIL log class.
//
/////////////////////////////////////////////////////////////////////////////

#include "types.h"
#include "repository.h"
#include "rillog.h"
#include <utils/Log.h>
#include <cutils/properties.h>

#undef LOG_NDEBUG
#define LOG_NDEBUG 0

/*
 * Simplified macro to send a radio log message using a given tag and level.
 */
#ifndef RLOG
#if LOG_NDEBUG
#define RLOG(level, tag, ...)   ((void)0)
#else
#define RLOG(level, tag, ...) ((void)__android_log_buf_print(LOG_ID_RADIO, level, tag, __VA_ARGS__))
#endif
#endif

UINT8 CRilLog::m_uiFlags = 0x00;
BOOL  CRilLog::m_bInitialized = FALSE;
BOOL  CRilLog::m_bFullLogBuild = FALSE;
char  CRilLog::m_szSubscriptionID[SUBSCRIPTIONID_MAX_LENGTH];

void CRilLog::Init(int subscriptionID)
{
    CRepository repository;
    int         iLogLevel;
    char        szBuildType[PROPERTY_VALUE_MAX] = {0};

    RLOGI("Subscription ID value : %d", subscriptionID);
    if (subscriptionID)
    {
        snprintf(m_szSubscriptionID, sizeof(m_szSubscriptionID)-1, "%d",
            subscriptionID);
        m_szSubscriptionID[sizeof(m_szSubscriptionID)-1] = '\0';  // KW fix
    }
    else
    {
        strncpy(m_szSubscriptionID, SUBSCRIPTIONID_DEFAULT_VALUE,
                sizeof(m_szSubscriptionID)-1);
        m_szSubscriptionID[sizeof(m_szSubscriptionID)-1] = '\0';  // KW fix
    }

    /*
     * Check if the build is an engineering build.
     * If not the case, only CRITICAL level of log will be activated.
     */
    property_get("ro.build.type", szBuildType, "");
    m_bFullLogBuild = (strcmp(szBuildType, "eng") == 0) ||
            (strcmp(szBuildType, "userdebug") == 0);

    if (repository.Read(g_szGroupLogging, g_szLogLevel, iLogLevel))
    {
        RLOGI("Log level [%d]\r\n", iLogLevel);
        switch(iLogLevel)
        {
            case 1: // Verbose
                if (m_bFullLogBuild)
                    m_uiFlags |= E_RIL_VERBOSE_LOG;
                // fall through
            case 2: // Info
                if (m_bFullLogBuild)
                    m_uiFlags |= E_RIL_INFO_LOG;
                // fall through
            case 3:
                if (m_bFullLogBuild)
                    m_uiFlags |= E_RIL_WARNING_LOG;
                // fall through
            case 4:
            default:
                m_uiFlags |= E_RIL_CRITICAL_LOG;
                break;
        }
    }
    else
    {
        m_uiFlags = E_RIL_CRITICAL_LOG;
    }
    m_uiFlags |= E_RIL_VERBOSE_LOG;
    m_uiFlags |= E_RIL_INFO_LOG;
    m_uiFlags |= E_RIL_WARNING_LOG;
    m_uiFlags |= E_RIL_CRITICAL_LOG;

    m_bInitialized = TRUE;
}

void CRilLog::Verbose(const char* const szFormatString, ...)
{
    if (m_bInitialized && (m_uiFlags & E_RIL_VERBOSE_LOG))
    {
        va_list argList;
        char szLogText[m_uiMaxLogBufferSize];
        char szNewTag[LOG_TAG_MAX_LENGTH];

        va_start(argList, szFormatString);
        vsnprintf(szLogText, m_uiMaxLogBufferSize, szFormatString, argList);
        va_end(argList);

        if (strcmp(m_szSubscriptionID, SUBSCRIPTIONID_DEFAULT_VALUE)!=0)
        {
            snprintf(szNewTag, LOG_TAG_MAX_LENGTH, "%s%s", LOG_TAG, m_szSubscriptionID);
            RLOG(ANDROID_LOG_DEBUG, szNewTag, "%s", szLogText);
        }
        else
        {
            RLOGD("%s", szLogText);
        }
    }
}

void CRilLog::Info(const char* const szFormatString, ...)
{
    if (m_bInitialized && (m_uiFlags & E_RIL_INFO_LOG))
    {
        va_list argList;
        char szLogText[m_uiMaxLogBufferSize];
        char szNewTag[LOG_TAG_MAX_LENGTH];

        va_start(argList, szFormatString);
        vsnprintf(szLogText, m_uiMaxLogBufferSize, szFormatString, argList);
        va_end(argList);

        if (strcmp(m_szSubscriptionID, SUBSCRIPTIONID_DEFAULT_VALUE)!=0)
        {
            snprintf(szNewTag, LOG_TAG_MAX_LENGTH, "%s%s", LOG_TAG, m_szSubscriptionID);
            RLOG(ANDROID_LOG_INFO, szNewTag, "%s", szLogText);
        }
        else
        {
            RLOGI("%s", szLogText);
        }
    }
}

void CRilLog::Warning(const char* const szFormatString, ...)
{
    if (m_bInitialized && (m_uiFlags & E_RIL_WARNING_LOG))
    {
        va_list argList;
        char szLogText[m_uiMaxLogBufferSize];
        char szNewTag[LOG_TAG_MAX_LENGTH];

        va_start(argList, szFormatString);
        vsnprintf(szLogText, m_uiMaxLogBufferSize, szFormatString, argList);
        va_end(argList);

        if (strcmp(m_szSubscriptionID, SUBSCRIPTIONID_DEFAULT_VALUE)!=0)
        {
            snprintf(szNewTag, LOG_TAG_MAX_LENGTH, "%s%s", LOG_TAG, m_szSubscriptionID);
            RLOG(ANDROID_LOG_WARN, szNewTag, "%s", szLogText);
        }
        else
        {
            RLOGW("%s", szLogText);
        }
    }
}

void CRilLog::Critical(const char* const szFormatString, ...)
{
    if (m_bInitialized && (m_uiFlags & E_RIL_CRITICAL_LOG))
    {
        va_list argList;
        char szLogText[m_uiMaxLogBufferSize];
        char szNewTag[LOG_TAG_MAX_LENGTH];

        va_start(argList, szFormatString);
        vsnprintf(szLogText, m_uiMaxLogBufferSize, szFormatString, argList);
        va_end(argList);

        if (strcmp(m_szSubscriptionID, SUBSCRIPTIONID_DEFAULT_VALUE)!=0)
        {
            snprintf(szNewTag, LOG_TAG_MAX_LENGTH, "%s%s", LOG_TAG, m_szSubscriptionID);
            RLOG(ANDROID_LOG_ERROR, szNewTag, "%s", szLogText);
        }
        else
        {
            RLOGE("%s", szLogText);
        }
    }
}

