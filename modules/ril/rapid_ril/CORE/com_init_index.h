////////////////////////////////////////////////////////////////////////////
// com_init_index.h
//
// Copyright 2011 Intrinsyc Software International, Inc.  All rights reserved.
// Patents pending in the United States of America and other jurisdictions.
//
//
// Description:
//    Defines the init indexes.
//
/////////////////////////////////////////////////////////////////////////////

#ifndef RRIL_COMINITINDEX_H
#define RRIL_COMINITINDEX_H


enum eComInitIndex
{
    COM_BASIC_INIT_INDEX = 0,   // Commands to be sent at beginning of init sequence
    COM_UNLOCK_INIT_INDEX,      // Commands to be sent following the successful unlocking
                                // of the SIM (or immediately if SIM not locked)
    COM_POWER_ON_INIT_INDEX,    // Commands to be sent following the modem being powered
                                // on via AT+CFUN=1
    COM_READY_INIT_INDEX,       // Commands to be sent once the modem is powered on and the
                                // SIM is unlocked
    COM_MAX_INDEX
};


#endif // RRIL_COMINITINDEX_H
