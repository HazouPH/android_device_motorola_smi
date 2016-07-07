////////////////////////////////////////////////////////////////////////////
// cellInfo_cache.cpp
//
// Copyright 2009 Intrinsyc Software International, Inc.  All rights reserved.
// Patents pending in the United States of America and other jurisdictions.
//
//
// Description:
//    Implements the cellInfo cache class which caches the cell information of
//    neighbouring and serving cells.
//
/////////////////////////////////////////////////////////////////////////////

#include "cellInfo_cache.h"
#include "util.h"
#include "rillog.h"
#include "rril.h"

BOOL operator==(const RIL_CellIdentityGsm& lhs, const RIL_CellIdentityGsm& rhs)
{
    return (lhs.mnc == rhs.mnc && lhs.mcc == rhs.mcc && lhs.lac == rhs.lac && lhs.cid == rhs.cid);
}

BOOL operator==(const RIL_CellIdentityWcdma& lhs, const RIL_CellIdentityWcdma& rhs)
{
    return (lhs.mnc == rhs.mnc && lhs.mcc == rhs.mcc && lhs.lac == rhs.lac
            && lhs.cid == rhs.cid && lhs.psc == rhs.psc);
}

BOOL operator==(const RIL_CellIdentityLte& lhs, const RIL_CellIdentityLte& rhs)
{
    return (lhs.mnc == rhs.mnc && lhs.mcc == rhs.mcc && lhs.ci == rhs.ci
            && lhs.pci == rhs.pci && lhs.tac == rhs.tac);
}

BOOL operator==(const RIL_GW_SignalStrength& lhs, const RIL_GW_SignalStrength& rhs)
{
    return (lhs.signalStrength == rhs.signalStrength && lhs.bitErrorRate == rhs.bitErrorRate);
}

BOOL operator==(const RIL_SignalStrengthWcdma& lhs, const RIL_SignalStrengthWcdma& rhs)
{
    return (lhs.signalStrength == rhs.signalStrength && lhs.bitErrorRate == rhs.bitErrorRate);
}

BOOL operator==(const RIL_LTE_SignalStrength_v8& lhs, const RIL_LTE_SignalStrength_v8& rhs)
{
    return (lhs.signalStrength == rhs.signalStrength && lhs.rsrp == rhs.rsrp
            && lhs.rsrq == rhs.rsrq && lhs.rssnr == rhs.rssnr
            && lhs.cqi == rhs.cqi && lhs.timingAdvance == rhs.timingAdvance);
}

BOOL operator==(const RIL_CellInfo& lhs, const RIL_CellInfo& rhs)
{
     // Check whether cell info type is the same.
     // if yes, based on the type, check the values
     if (lhs.cellInfoType == rhs.cellInfoType)
     {
         switch(lhs.cellInfoType)
         {
         case RIL_CELL_INFO_TYPE_GSM:
             return (lhs.CellInfo.gsm.cellIdentityGsm == rhs.CellInfo.gsm.cellIdentityGsm
                     && lhs.CellInfo.gsm.signalStrengthGsm == rhs.CellInfo.gsm.signalStrengthGsm);
         case RIL_CELL_INFO_TYPE_WCDMA:
             return (lhs.CellInfo.wcdma.cellIdentityWcdma == rhs.CellInfo.wcdma.cellIdentityWcdma
                     && lhs.CellInfo.wcdma.signalStrengthWcdma ==
                     rhs.CellInfo.wcdma.signalStrengthWcdma);
         case RIL_CELL_INFO_TYPE_LTE:
             return (lhs.CellInfo.lte.cellIdentityLte == rhs.CellInfo.lte.cellIdentityLte
                     && lhs.CellInfo.lte.signalStrengthLte == rhs.CellInfo.lte.signalStrengthLte);
         default:
             return FALSE;
         }
     }
     return FALSE;
}

BOOL operator==(const RIL_CellIdentityGsm_v2& lhs, const RIL_CellIdentityGsm_v2& rhs)
{
    return (lhs.mnc == rhs.mnc && lhs.mcc == rhs.mcc && lhs.lac == rhs.lac && lhs.cid == rhs.cid
            && lhs.basestationId == rhs.basestationId && lhs.arfcn == rhs.arfcn);
}

BOOL operator==(const RIL_CellIdentityWcdma_v2& lhs, const RIL_CellIdentityWcdma_v2& rhs)
{
    return (lhs.mnc == rhs.mnc && lhs.mcc == rhs.mcc && lhs.lac == rhs.lac
            && lhs.cid == rhs.cid && lhs.psc == rhs.psc && lhs.dluarfcn == rhs.dluarfcn
            && lhs.uluarfcn == rhs.uluarfcn && lhs.pathloss == rhs.pathloss);
}

BOOL operator==(const RIL_CellIdentityLte_v2& lhs, const RIL_CellIdentityLte_v2& rhs)
{
    return (lhs.mnc == rhs.mnc && lhs.mcc == rhs.mcc && lhs.ci == rhs.ci
            && lhs.pci == rhs.pci && lhs.tac == rhs.tac && lhs.dlearfcn == rhs.dlearfcn
            && lhs.ulearfcn == rhs.ulearfcn && lhs.pathloss == rhs.pathloss);
}

BOOL operator==(const RIL_GW_SignalStrength_v2& lhs, const RIL_GW_SignalStrength_v2& rhs)
{
    return (lhs.signalStrength == rhs.signalStrength && lhs.bitErrorRate == rhs.bitErrorRate
            && lhs.rxLev == rhs.rxLev && lhs.timingAdvance == rhs.timingAdvance);
}

BOOL operator==(const RIL_SignalStrengthWcdma_v2& lhs, const RIL_SignalStrengthWcdma_v2& rhs)
{
    return (lhs.signalStrength == rhs.signalStrength && lhs.bitErrorRate == rhs.bitErrorRate
            && lhs.rscp == rhs.rscp && lhs.ecNo == rhs.ecNo);
}

BOOL operator==(const RIL_CellInfo_v2& lhs, const RIL_CellInfo_v2& rhs)
{
     // Check whether cell info type is the same.
     // if yes, based on the type, check the values
     if (lhs.cellInfoType == rhs.cellInfoType)
     {
         int cellInfoType = static_cast<int>(lhs.cellInfoType);
         switch (cellInfoType)
         {
         case RIL_CELL_INFO_TYPE_GSM_V2:
             return (lhs.CellInfo.gsm.cellIdentityGsm == rhs.CellInfo.gsm.cellIdentityGsm
                     && lhs.CellInfo.gsm.signalStrengthGsm == rhs.CellInfo.gsm.signalStrengthGsm);
         case RIL_CELL_INFO_TYPE_WCDMA_V2:
             return (lhs.CellInfo.wcdma.cellIdentityWcdma == rhs.CellInfo.wcdma.cellIdentityWcdma
                     && lhs.CellInfo.wcdma.signalStrengthWcdma ==
                     rhs.CellInfo.wcdma.signalStrengthWcdma);
         case RIL_CELL_INFO_TYPE_LTE_V2:
             return (lhs.CellInfo.lte.cellIdentityLte == rhs.CellInfo.lte.cellIdentityLte
                     && lhs.CellInfo.lte.signalStrengthLte == rhs.CellInfo.lte.signalStrengthLte);
         default:
             return FALSE;
         }
     }
     return FALSE;
}

CellInfoCache::CellInfoCache()
{
    memset(&m_sCellInfo, 0, sizeof(S_ND_N_CELL_INFO_DATA));
    memset(&m_sCellInfov2, 0, sizeof(S_ND_N_CELL_INFO_DATA_V2));
    m_cacheSize = 0;
    m_pCacheLock = new CMutex();
}

CellInfoCache::~CellInfoCache()
{
    delete m_pCacheLock;
}

BOOL CellInfoCache::getCellInfo(P_ND_N_CELL_INFO_DATA pRetData, int& itemCount)
{
    if (pRetData == NULL)
    {
        return FALSE;
    }
    // loop through the cache and copy the info to the pRetData
    CMutex::Lock(m_pCacheLock);
    itemCount = m_cacheSize;
    for (int i = 0; i < m_cacheSize; i++)
    {
        pRetData->aRilCellInfo[i] = m_sCellInfo.aRilCellInfo[i];
    }
    CMutex::Unlock(m_pCacheLock);
    return TRUE;
}

int CellInfoCache::checkCache(const RIL_CellInfo& pData)
{
    RIL_LOG_VERBOSE("CellInfoCache::checkCache() %d\r\n", m_cacheSize);
    for (int i = 0; i < m_cacheSize; i++)
    {
        if (pData == m_sCellInfo.aRilCellInfo[i])
        {
            RIL_LOG_VERBOSE("CellInfoCache::checkCache() - Found match at %d\r\n",i);
            return i;
        }
    }
    return -1;
}

BOOL CellInfoCache::updateCache(const P_ND_N_CELL_INFO_DATA pData, const int itemCount)
{
    BOOL ret = FALSE;

    RIL_LOG_VERBOSE("CellInfoCache::updateCache() - item count %d \r\n", itemCount);
    if (NULL == pData || itemCount > RRIL_MAX_CELL_ID_COUNT)
    {
        RIL_LOG_INFO("CellInfoCache::updateCache() - Invalid data\r\n");
        goto Error;
    }
    else
    {
        // if there were more items in the cache before
        if (itemCount != m_cacheSize)
        {
            ret = TRUE;
        }
        else
        {
            for (int i = 0; i < itemCount; i++)
            {
                if (checkCache(pData->aRilCellInfo[i]) < 0 ) // new item
                {
                    ret = TRUE;
                    break;
                }
            }
        }
    }

    if (ret)
    {
        RIL_LOG_INFO("CellInfoCache::updateCache() -"
                "Updating cache with %d items\r\n", itemCount);
        // Access mutex
        CMutex::Lock(m_pCacheLock);
        m_cacheSize = itemCount;
        memset(&m_sCellInfo, 0, sizeof(S_ND_N_CELL_INFO_DATA));
        for (int i = 0; i < m_cacheSize; i++)
        {
            m_sCellInfo.aRilCellInfo[i] = pData->aRilCellInfo[i];
        }
        // release mutex
        CMutex::Unlock(m_pCacheLock);
    }
Error:
    return ret;
}

BOOL CellInfoCache::getCellInfo(P_ND_N_CELL_INFO_DATA_V2 pRetData, int& itemCount)
{
    if (pRetData == NULL)
    {
        return FALSE;
    }
    // loop through the cache and copy the info to the pRetData
    CMutex::Lock(m_pCacheLock);
    itemCount = m_cacheSize;
    for (int i = 0; i < m_cacheSize; i++)
    {
        pRetData->aRilCellInfo[i] = m_sCellInfov2.aRilCellInfo[i];
    }
    CMutex::Unlock(m_pCacheLock);
    return TRUE;
}

int CellInfoCache::checkCache(const RIL_CellInfo_v2& pData)
{
    RIL_LOG_VERBOSE("CellInfoCache::checkCache() %d\r\n", m_cacheSize);
    for (int i = 0; i < m_cacheSize; i++)
    {
        if (pData == m_sCellInfov2.aRilCellInfo[i])
        {
            RIL_LOG_VERBOSE("CellInfoCache::checkCache() - Found match at %d\r\n",i);
            return i;
        }
    }
    return -1;
}

BOOL CellInfoCache::updateCache(const P_ND_N_CELL_INFO_DATA_V2 pData, const int itemCount)
{
    BOOL ret = FALSE;

    RIL_LOG_VERBOSE("CellInfoCache::updateCache() - item count %d \r\n", itemCount);
    if (NULL == pData || itemCount > RRIL_MAX_CELL_ID_COUNT)
    {
        RIL_LOG_INFO("CellInfoCache::updateCache() - Invalid data\r\n");
        goto Error;
    }
    else
    {
        // if there were more items in the cache before
        if (itemCount != m_cacheSize)
        {
            ret = TRUE;
        }
        else
        {
            for (int i = 0; i < itemCount; i++)
            {
                if (checkCache(pData->aRilCellInfo[i]) < 0 ) // new item
                {
                    ret = TRUE;
                    break;
                }
            }
        }
    }

    if (ret)
    {
        RIL_LOG_INFO("CellInfoCache::updateCache() -"
                "Updating V2 cache with %d items\r\n", itemCount);
        // Access mutex
        CMutex::Lock(m_pCacheLock);
        m_cacheSize = itemCount;
        memset(&m_sCellInfov2, 0, sizeof(S_ND_N_CELL_INFO_DATA_V2));
        for (int i = 0; i < m_cacheSize; i++)
        {
            m_sCellInfov2.aRilCellInfo[i] = pData->aRilCellInfo[i];
        }
        // release mutex
        CMutex::Unlock(m_pCacheLock);
    }
Error:
    return ret;
}
