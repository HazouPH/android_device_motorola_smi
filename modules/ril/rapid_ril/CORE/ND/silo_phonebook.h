////////////////////////////////////////////////////////////////////////////
// silo_Phonebook.h
//
// Copyright 2005-2007 Intrinsyc Software International, Inc.  All rights reserved.
// Patents pending in the United States of America and other jurisdictions.
//
//
// Description:
//    Defines the CSilo_Phonebook class, which provides response handlers and
//    parsing functions for the phonebook-related RIL components.
//
/////////////////////////////////////////////////////////////////////////////
//
//  Phonebook silo class.  This class handles phonebook functionality including:
//  -Read/Write/Delete phonebook entries
//  -Get/Set phonebook options
//

#ifndef RRIL_SILO_PHONEBOOK_H
#define RRIL_SILO_PHONEBOOK_H

#include "silo.h"

class CSilo_Phonebook : public CSilo
{
public:
    CSilo_Phonebook(CChannel* pChannel);
    virtual ~CSilo_Phonebook();

protected:
    //  Parse notification functions here.
    virtual BOOL ParsePBREADY(CResponse* const pResponse, const char*& rszPointer);
};

#endif // RRIL_SILO_PHONEBOOK_H

