////////////////////////////////////////////////////////////////////////////
// cellInfo_cache.h
//
// Copyright 2009 Intrinsyc Software International, Inc.  All rights reserved.
// Patents pending in the United States of America and other jurisdictions.
//
//
// Description:
//    Defines the cellInfo cache class which caches the cell information of
//    neighbouring and serving cells.
//
/////////////////////////////////////////////////////////////////////////////

#ifndef RRIL_CELLINFO_CACHE_H
#define RRIL_CELLINFO_CACHE_H

#include "types.h"
#include "nd_structs.h"
#include "sync_ops.h"


class CellInfoCache
{
public:
    CellInfoCache();
    ~CellInfoCache();
    BOOL updateCache(const P_ND_N_CELL_INFO_DATA_V12 pData, const int itemCount);
    BOOL updateCache(const P_ND_N_CELL_INFO_DATA_V2 pData, const int itemCount);
    BOOL getCellInfo(P_ND_N_CELL_INFO_DATA_V12 pRetData, int& itemCount);
    BOOL getCellInfo(P_ND_N_CELL_INFO_DATA_V2 pRetData, int& itemCount);
    bool IsCellInfoCacheEmpty() { return m_cacheSize <= 0; }

private:
    int checkCache(const RIL_CellInfo_v12& pData);
    int checkCache(const RIL_CellInfo_v2& pData);
    S_ND_N_CELL_INFO_DATA_V12 m_sCellInfo;
    S_ND_N_CELL_INFO_DATA_V2 m_sCellInfov2;
    int m_cacheSize;
    CMutex* m_pCacheLock;
};

#endif
