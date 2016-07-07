////////////////////////////////////////////////////////////////////////////
// cbs_info.h
//
// Copyright (C) Intel 2014.
//
//
// Description:
//    Collects Cell Broadcast Sms info and utilities.
//
/////////////////////////////////////////////////////////////////////////////
#ifndef CBS_INFO_H
#define CBS_INFO_H

#include <utils/Vector.h>
#include <telephony/ril.h>

class CCbsInfo
{
public:
    struct IdRange
    {
        int  minVal;
        int  maxVal;
    };

    struct CbmIds
    {
        int etwsIds;
        int cmasIds;
    };

    enum {
        ETWS_FIRST = 4352
    };

    enum EtwsMsgIds {
        ETWS_EARTHQUAKE = ETWS_FIRST,
        ETWS_TSUNAMI,
        ETWS_EARTHQUAKE_AND_TSUNAMI,
        ETWS_TEST,
        ETWS_OTHER_EMERGENCY,
        ETWS_EXTENSION_1,
        ETWS_EXTENSION_2,
        ETWS_EXTENSION_3
    };

    static const int ETWS_LAST = ETWS_EXTENSION_3;

public:
    CCbsInfo();
    ~CCbsInfo();

    void Clean();

    // Calculate the length of the range
    int GetNumberOfIdsFromRange(int from, int to);

    // Caches the ranges of activated SMS CB/Cmas msg ids
    android::Vector<IdRange> m_vActivatedBroadcastSms;

    // Number of activated SMS CB/Cmas msg ids
    // Number of activated ETWS SMS CB msg ids
    CbmIds m_activatedIds;

    // Indicates if Etws test msg id is activated
    bool m_bEtwSmsTestActivated;

    // Caches the ranges of ETWS SMS CB/Cmas msg ids to be activated
    android::Vector<RIL_GSM_BroadcastSmsConfigInfo> m_vBroadcastEtwSmsConfigInfo;
};
#endif

