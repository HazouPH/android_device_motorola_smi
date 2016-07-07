////////////////////////////////////////////////////////////////////////////
// extract.h
//
// Copyright 2005-2007 Intrinsyc Software International, Inc.  All rights reserved.
// Patents pending in the United States of America and other jurisdictions.
//
//
// Description:
//  Provides helper functions from extracting information elements from response
//  strings.
//
/////////////////////////////////////////////////////////////////////////////

#pragma once



// Takes in a string to search for a substring. If found, returns true and rszEnd
// points to the first character after szSkip.
BOOL FindAndSkipString(const char* szStart, const char* szSkip, const char*& rszEnd);

// Moves the pointer past any spaces in the response. Returns TRUE if spaces were skipped.
BOOL SkipSpaces(const char* szStart, const char*& rszEnd);

// Takes in a string to compare with a substring. If the substring matches with
// szStart, the function returns true and rszEnd points to the first character after szSkip.
BOOL SkipString(const char* szStart, const char* szSkip, const char*& rszEnd);

// Looks for a carriage return/line feed at the start of szStart. If found, returns true
// and sets rszEnd to the first character after the pattern.
BOOL SkipRspStart(const char* szStart, const char* szSkip, const char*& rszEnd);

// Looks for a carriage return/line feed at the start of szStart. If found, returns true
// and sets rszEnd to the first character after the pattern. This is identical to SkipRspStart
// but is provided in case the modem responds with a different pattern for responds ends than
// beginnings.
BOOL SkipRspEnd(const char* szStart, const char* szSkip, const char*& rszEnd);

// Looks for a carriage return/line feed anywhere in szStart. If found, returns true
// and sets rszEnd to the first character after the pattern.
BOOL FindAndSkipRspEnd(const char* szStart, const char* szSkip, const char*& rszEnd);

// Parses the given cmdStr and extracts arguments separated by a colon ','.
// Inserts arguments into given char pointers' array.
UINT32 FindRspArgs(const char* pszCmdStr, const char* pszEndLine, char** aPtrArgs,
                                                                 UINT32 uinMaxArgs);

// Takes the digits in szStart and stores them into a UINT32. If a space follows the last
// digit it will also be consumed. Returns TRUE if at least one digit is found.
BOOL ExtractUInt32(const char* szStart, UINT32& rnValue, const char*& rszEnd);

// Extracts a string enclosed by quotes into a given buffer. Returns TRUE if two quotes are
// found and the buffer given is large enough to contain the string and a NULL termination
// character.
BOOL ExtractQuotedString(const char* szStart, char* szOutput, const UINT32 cbOutput,
                                                               const char*& rszEnd);

// Extracts a string ended by szDelimiter into a given buffer. Returns TRUE if szDelimiter
// is found and the buffer given is large enough to contain the string and a NULL termination
// character.
BOOL ExtractUnquotedString(const char* szStart, const char cDelimiter, char* szOutput,
                                          const UINT32 cbOutput, const char*& rszEnd);
BOOL ExtractUnquotedString(const char* szStart, const char* szDelimiter, char* szOutput,
                                            const UINT32 cbOutput, const char*& rszEnd);

// Extracts a UINT32 from hex ascii
BOOL ExtractHexUInt32(const char* szStart, UINT32& rdwValue, const char*& rszEnd);

// Allocates memory for the quoted string extracted from the given buffer and returns it. Caller
// must delete the memory when finished with it.
BOOL ExtractQuotedStringWithAllocatedMemory(const char* szStart, char*& rszString,
                                          UINT32& rcbString, const char*& rszEnd);

// Allocates memory for the unquoted string extracted from the given buffer and returns it.
// Caller must delete the memory when finished with it.
BOOL ExtractUnquotedStringWithAllocatedMemory(const char* szStart, const char chDelimiter,
                                char*& rszString, UINT32& rcbString, const char*& rszEnd);

// Extracts a decimal number and stores it as a 16.16 fixed point value
BOOL ExtractFixedPointValue(const char* szStart, UINT32& rdwFPValue, const char*& rszEnd);

// Extracts a decimal value and returns it as a double
BOOL ExtractDouble(const char* szStart, double& rdbValue, const char*& rszEnd);


BOOL ExtractIntAndConvertToUInt32(const char* szData, UINT32& rnVal, const char*& rszRemainder);

BOOL ExtractUpperBoundedUInt32(const char* szData, const UINT32 nUpperLimit, UINT32& rnVal,
                                                                const char*& rszRemainder);

BOOL ExtractLowerBoundedUInt32(const char* szData, const UINT32 nLowerLimit, UINT32& rnVal,
                                                                const char*& rszRemainder);

BOOL ExtractLongLong(const char* pszData, long long& nVal, const char*& pszRemainder,
        int base = 10);
BOOL ExtractInt(const char* pszData, int& nVal, const char*& pszRemainder, int base = 10);
BOOL ExtractQuotedHexLongLong(const char* pszData, long long& nVal, const char*& pszRemainder);
BOOL ExtractQuotedHexInt(const char* pszData, int& nVal, const char*& pszRemainder);
BOOL ExtractQuotedHexUnsignedInt(const char* pszData, unsigned int& nVal, const char*& pszRemainder);
