////////////////////////////////////////////////////////////////////////////
// cbs_info.cpp
//
// Copyright (C) Intel 2014.
//
//
// Description:
//    Collects Cell Broadcast Sms info and utilities.
//
/////////////////////////////////////////////////////////////////////////////
#include "cbs_info.h"


CCbsInfo::CCbsInfo()
{
    Clean();
}

CCbsInfo::~CCbsInfo()
{
    m_vActivatedBroadcastSms.clear();
    m_vBroadcastEtwSmsConfigInfo.clear();
}

void CCbsInfo::Clean()
{
    m_vActivatedBroadcastSms.clear();
    m_vBroadcastEtwSmsConfigInfo.clear();
    m_activatedIds.cmasIds = 0;
    m_activatedIds.etwsIds = 0;
    m_bEtwSmsTestActivated = false;
}

//
// Calculate the number of Ids in a given range.
// Returns zero if max value is less than min value.
//
int CCbsInfo::GetNumberOfIdsFromRange(int from, int to)
{
    if (to >= from)
    {
        return ((to-from) + 1);
    }

    return 0;
}
