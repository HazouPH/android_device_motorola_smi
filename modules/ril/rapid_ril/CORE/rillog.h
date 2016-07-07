////////////////////////////////////////////////////////////////////////////
// rillog.cpp
//
// Copyright 2009 Intrinsyc Software International, Inc.  All rights reserved.
// Patents pending in the United States of America and other jurisdictions.
//
//
// Description:
//    Declares RIL log class.
//
/////////////////////////////////////////////////////////////////////////////

#ifndef RRIL_LOG_H
#define RRIL_LOG_H

#define LOG_TAG "RILR"

const int LOG_TAG_MAX_LENGTH = 6;
const int SUBSCRIPTIONID_MAX_LENGTH = 6;
const char SUBSCRIPTIONID_DEFAULT_VALUE[] = "none";

#include "types.h"

#define RIL_LOG_VERBOSE(format, ...)    CRilLog::Verbose(format, ## __VA_ARGS__)
#define RIL_LOG_INFO(format, ...)       CRilLog::Info(format, ## __VA_ARGS__)
#define RIL_LOG_WARNING(format, ...)    CRilLog::Warning(format, ## __VA_ARGS__)
#define RIL_LOG_CRITICAL(format, ...)   CRilLog::Critical(format, ## __VA_ARGS__)

class CRilLog
{
public:
    static void Init(int subscriptionID);
    static void Verbose(const char* const szFormatString, ...);
    static void Info(const char* const szFormatString, ...);
    static void Warning(const char* const szFormatString, ...);
    static void Critical(const char* const szFormatString, ...);

    static inline BOOL IsFullLogBuild() { return m_bFullLogBuild; }

private:
    static const UINT32 m_uiMaxLogBufferSize = 1024;
    enum
    {
        E_RIL_VERBOSE_LOG  = 0x01,
        E_RIL_INFO_LOG     = 0x02,
        E_RIL_WARNING_LOG  = 0x04,
        E_RIL_CRITICAL_LOG = 0x08
    };

    static UINT8 m_uiFlags;
    static BOOL  m_bInitialized;
    static BOOL  m_bFullLogBuild;
    static char  m_szSubscriptionID[SUBSCRIPTIONID_MAX_LENGTH];
};

#endif // RRIL_LOG_H

