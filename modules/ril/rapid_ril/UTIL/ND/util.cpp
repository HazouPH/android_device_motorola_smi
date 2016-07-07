////////////////////////////////////////////////////////////////////////////
// util.cpp
//
// Copyright 2005-2007 Intrinsyc Software International, Inc.  All rights reserved.
// Patents pending in the United States of America and other jurisdictions.
//
//
// Description:
//    Provides implementations for the RIL Utility functions.
//    Also includes related class implementations.
//
/////////////////////////////////////////////////////////////////////////////

#include "types.h"
#include "../../CORE/util.h"
#include "extract.h"

#include <wchar.h>
#include <sys/select.h>
#include <arpa/inet.h>

#include <assert.h>


#define minimum_of(a,b) (((a) < (b)) ? (a) : (b))


/** GSM ALPHABET
 **/

const int  GSM_7BITS_ESCAPE = 0x1b;
const int  GSM_7BITS_UNKNOWN = 0;

static const unsigned short   gsm7bits_to_unicode[128] = {
  '@', 0xa3,  '$', 0xa5, 0xe8, 0xe9, 0xf9, 0xec, 0xf2, 0xc7, '\n', 0xd8, 0xf8, '\r', 0xc5, 0xe5,
0x394,  '_',0x3a6,0x393,0x39b,0x3a9,0x3a0,0x3a8,0x3a3,0x398,0x39e,    0, 0xc6, 0xe6, 0xdf, 0xc9,
  ' ',  '!',  '"',  '#', 0xa4,  '%',  '&', '\'',  '(',  ')',  '*',  '+',  ',',  '-',  '.',  '/',
  '0',  '1',  '2',  '3',  '4',  '5',  '6',  '7',  '8',  '9',  ':',  ';',  '<',  '=',  '>',  '?',
 0xa1,  'A',  'B',  'C',  'D',  'E',  'F',  'G',  'H',  'I',  'J',  'K',  'L',  'M',  'N',  'O',
  'P',  'Q',  'R',  'S',  'T',  'U',  'V',  'W',  'X',  'Y',  'Z', 0xc4, 0xd6,0x147, 0xdc, 0xa7,
 0xbf,  'a',  'b',  'c',  'd',  'e',  'f',  'g',  'h',  'i',  'j',  'k',  'l',  'm',  'n',  'o',
  'p',  'q',  'r',  's',  't',  'u',  'v',  'w',  'x',  'y',  'z', 0xe4, 0xf6, 0xf1, 0xfc, 0xe0,
};

static const unsigned short  gsm7bits_extend_to_unicode[128] = {
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,'\f',   0,   0,   0,   0,   0,
    0,   0,   0,   0, '^',   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0, '{', '}',   0,   0,   0,   0,   0,'\\',
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, '[', '~', ']',   0,
  '|',   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,0x20ac, 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
};

//
// Table used to map semi-byte values to hex characters
//
static const char g_rgchSemiByteToCharMap[16] =
        { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

//
// Convert a semi-byte into its character representation
//
char SemiByteToChar(const BYTE bByte, const BOOL fHigh)
{
    UINT8 bSemiByte = (fHigh ? (bByte & 0xf0) >> 4 : bByte & 0x0f);

    if (0x10 <= bSemiByte)
    {
        RIL_LOG_CRITICAL("SemiByteToChar() : Invalid bByte argument: bByte: 0x%X fHigh: %s\r\n",
                bByte, fHigh ? "TRUE" : "FALSE");
        return g_rgchSemiByteToCharMap[0];
    }
    else
    {
        return g_rgchSemiByteToCharMap[bSemiByte];
    }
}


//
// Combine 2 characters representing semi-bytes into a byte
//
BYTE SemiByteCharsToByte(const char chHigh, const char chLow)
{
    BYTE bRet;

    if ('0' <= chHigh && '9' >= chHigh)
    {
        bRet = (chHigh - '0') << 4;
    }
    else
    {
        bRet = (0x0a + chHigh - 'A') << 4;
    }

    if ('0' <= chLow && '9' >= chLow)
    {
        bRet |= (chLow - '0');
    }
    else
    {
        bRet |= (0x0a + chLow - 'A');
    }
    return bRet;
}


//
//
//
BOOL GSMHexToGSM(const char* sIn, const UINT32 cbIn, BYTE* sOut,
                            const UINT32 cbOut, UINT32& rcbUsed)
{
    const char* pchIn = sIn;
    const char* pchInEnd = sIn + cbIn;
    BYTE* pchOut = sOut;
    BYTE* pchOutEnd = sOut + cbOut;
    BOOL fRet = FALSE;

    while (pchIn < pchInEnd - 1 && pchOut < pchOutEnd)
    {
        *pchOut++ = SemiByteCharsToByte(*pchIn, *(pchIn + 1));
        pchIn += 2;
    }

    rcbUsed = pchOut - sOut;
    fRet = TRUE;
    return fRet;
}


//
//
//
BOOL GSMToGSMHex(const BYTE* sIn, const UINT32 cbIn, char* sOut, const UINT32 cbOut,
                                                                    UINT32& rcbUsed)
{
    const BYTE* pchIn = sIn;
    const BYTE* pchInEnd = sIn + cbIn;
    char* pchOut = sOut;
    char* pchOutEnd = sOut + cbOut;
    BOOL fRet = FALSE;

    while (pchIn < pchInEnd && pchOut < pchOutEnd - 1)
    {
        *pchOut = g_rgchSemiByteToCharMap[((*pchIn) & 0xf0) >> 4];
        pchOut++;
        *pchOut = g_rgchSemiByteToCharMap[(*pchIn) & 0x0f];
        pchOut++;

        pchIn++;
    }

    rcbUsed = pchOut - sOut;
    fRet = TRUE;
    return fRet;
}

//  UCS2 to UTF8
//
// Helper fuction for UCS2 to UTF8 conversion below.
int utf8_write( unsigned char* utf8, int offset, int v)
{
    int result;

    if (v < 128)
    {
        result = 1;
        if (utf8)
            utf8[offset] = (unsigned char) v;
    }
    else if (v < 0x800)
    {
        result = 2;
        if (utf8)
        {
            utf8[offset+0] = (unsigned char)(0xc0 | (v >> 6));
            utf8[offset+1] = (unsigned char)(0x80 | (v & 0x3f));
        }
    }
    else if (v < 0x10000)
    {
        result = 3;
        if (utf8)
        {
            utf8[offset+0] = (unsigned char)(0xe0 | (v >> 12));
            utf8[offset+1] = (unsigned char)(0x80 | ((v >> 6) & 0x3f));
            utf8[offset+2] = (unsigned char)(0x80 | (v & 0x3f));
        }
    }
    else {
        result = 4;
        if (utf8)
        {
            utf8[offset+0] = (unsigned char)(0xf0 | ((v >> 18) & 0x7));
            utf8[offset+1] = (unsigned char)(0x80 | ((v >> 12) & 0x3f));
            utf8[offset+2] = (unsigned char)(0x80 | ((v >> 6) & 0x3f));
            utf8[offset+3] = (unsigned char)(0x80 | (v & 0x3f));
        }
    }
    return  result;
}

int ucs2_to_utf8(const unsigned char* ucs2, int ucs2len, unsigned char* buf)
{
    int nn;
    int result = 0;

    // 0xFF is the value used for padding. Stop the conversion when 0xFF is encountered.
    for (nn = 0; nn < ucs2len && ucs2[0] != 0xFF; ucs2 += 2, nn++)
    {
        int c = (ucs2[0] << 8) | ucs2[1];
        result += utf8_write(buf, result, c);
    }
    return result;
}


CSelfExpandBuffer::CSelfExpandBuffer() : m_szBuffer(NULL), m_uiUsed(0), m_nCapacity(0)
{
}

CSelfExpandBuffer::~CSelfExpandBuffer()
{
    delete[] m_szBuffer;
    m_szBuffer = NULL;
}

BOOL CSelfExpandBuffer::Append(const char* szIn, UINT32 nLength)
{
    BOOL   bRet = FALSE;
    UINT32 nNewSize;

    if (NULL != szIn && nLength != 0)
    {
        if (NULL == m_szBuffer)
        {
            m_szBuffer = new char[m_nChunkSize];
            if (NULL == m_szBuffer)
                goto Error;

            memset(m_szBuffer, 0, m_nChunkSize);

            m_nCapacity = m_nChunkSize;
        }

        for (nNewSize = m_nCapacity; (nNewSize - m_uiUsed) <= nLength; nNewSize += m_nChunkSize);
        if (nNewSize != m_nCapacity)
        {
            // allocate more space for the data
            char* tmp = new char[nNewSize];
            if (NULL == tmp)
                goto Error;

            memcpy(tmp, m_szBuffer, m_uiUsed);
            delete[] m_szBuffer;
            m_szBuffer = tmp;
            m_nCapacity = nNewSize;
        }
        memcpy(m_szBuffer + m_uiUsed, szIn, nLength);
        m_uiUsed += nLength;
        m_szBuffer[m_uiUsed] = '\0';
    }

    bRet = TRUE;

Error:
    return bRet;
}

BOOL CopyStringNullTerminate(char* const pszOut, const char* pszIn, const UINT32 cbOut)
{
    BOOL fRet = TRUE;

    //RIL_LOG_VERBOSE("CopyStringNullTerminate() - Enter\r\n");

    if ((NULL != pszIn) && (NULL != pszOut))
    {
        UINT32 cbIn = strlen(pszIn);

        strncpy(pszOut, pszIn, cbOut);

        //  Klokworks fix here
        pszOut[cbOut - 1] = '\0';
        //  End Klokworks fix

        if (cbOut <= cbIn)
        {
            fRet = FALSE;
        }
    }

    //RIL_LOG_VERBOSE("CopyStringNullTerminate() - Exit\r\n");

    return fRet;
}

BOOL PrintStringNullTerminate(char* const pszOut, const UINT32 cbOut, const char* pszFormat, ... )
{
    BOOL fRet = TRUE;
    int iWritten;

    //RIL_LOG_VERBOSE("PrintStringNullTerminate() - Enter\r\n");

    va_list args;
    va_start(args, pszFormat);

    iWritten = vsnprintf(pszOut, cbOut, pszFormat, args);

    if (0 > iWritten)
    {
        fRet = FALSE;
        pszOut[0] = '\0';
    }
    else if ((UINT32)iWritten >= cbOut)
    {
        fRet = FALSE;
        pszOut[cbOut - 1] = '\0';
    }

    va_end(args);

    //RIL_LOG_VERBOSE("PrintStringNullTerminate() - Exit\r\n");
    return fRet;
}

BOOL ConcatenateStringNullTerminate(char* const pszOut, const size_t cbOut, const char* const pszIn)
{
    BOOL fRet = FALSE;
    size_t outLen = strlen(pszOut);

    //RIL_LOG_VERBOSE("ConcatenateStringNullTerminate() - Enter\r\n");

    // We verify that pszOut size is not longer than the buffer which contains it
    assert(outLen < cbOut);

    if ((outLen < cbOut) && (strlen(pszIn) < (cbOut - outLen)))
    {
        // Use remaining space but reserve room for '\0'
        strncat(pszOut, pszIn, (cbOut - outLen) - 1U);
        fRet = TRUE;
    }

    //RIL_LOG_VERBOSE("ConcatenateStringNullTerminate() - Exit\r\n");

    return fRet;
}

CRLFExpandedString::CRLFExpandedString(const char* const pszIn, const int nInLen)
{
    UINT32 nCRLFs = 0;
    UINT32 nNewLen = 0;
    UINT32 nOther = 0;
    char* pszNewString = NULL;
    m_pszString = NULL;

    if (NULL == pszIn)
    {
        return;
    }

    for (int nWalk = 0; nWalk < nInLen; nWalk++)
    {
        if ((0x0A == pszIn[nWalk]) || (0x0D == pszIn[nWalk]))
        {
            nCRLFs++;
        }
        else if ( (pszIn[nWalk] < 0x20) || (pszIn[nWalk] > 0x7E) )
        {
            nOther++;
        }
    }

    //RIL_LOG_INFO("CRLFExpandedString::CRLFExpandedString() :"
   //        " Found %d instances of CR and LF combined\r\n", nCRLFs);

    // Size increase for each instance is from 1 char to 4 chars
    nNewLen = nInLen + (nCRLFs * 3) + (nOther * 3) + 1;
    m_pszString = new char[nNewLen];
    if (NULL == m_pszString)
    {
        return;
    }
    memset(m_pszString, 0, nNewLen);

    for (int nWalk = 0; nWalk < nInLen; nWalk++)
    {
        if (0x0A == pszIn[nWalk])
        {
            strncat(m_pszString, "<lf>", nNewLen - strlen(m_pszString) - 1);
        }
        else if (0x0D == pszIn[nWalk])
        {
            strncat(m_pszString, "<cr>", nNewLen - strlen(m_pszString) - 1);
        }
        else if ((pszIn[nWalk] >= 0x20) && (pszIn[nWalk] <= 0x7E))
        {
            strncat(m_pszString, &pszIn[nWalk], 1);
        }
        else
        {
            char szTmp[5] = {0};
            snprintf(szTmp, 5, "[%02X]", pszIn[nWalk]);
            strncat(m_pszString, szTmp, nNewLen - strlen(m_pszString) - 1);
        }
    }
}

CRLFExpandedString::~CRLFExpandedString()
{
    if (NULL != m_pszString)
    {
        delete [] m_pszString;
        m_pszString = NULL;
    }
}

void Sleep(UINT32 dwTimeInMs)
{
    struct timeval tv;

    tv.tv_sec = dwTimeInMs / 1000;
    tv.tv_usec = (dwTimeInMs % 1000) * 1000;

    select(0, NULL, NULL, NULL, &tv);
}

UINT32 GetTickCount()
{
    struct timeval t;
    gettimeofday( &t, NULL );
    return (t.tv_sec * 1000) + (t.tv_usec / 1000);
}

char* ConvertUCS2ToUTF8(const char* pHexBuffer, const UINT32 hexBufferLength)
{
    BYTE* pByteBuffer = NULL;
    UINT32 byteBufferUsed = 0;
    char* pUtf8Buffer = NULL;
    int utf8Count = 0;

    if (NULL == pHexBuffer || 0 >= hexBufferLength)
    {
        RIL_LOG_INFO("ConvertUCS2ToUTF8 - Invalid argument\r\n");
        return NULL;
    }

    if (0 != hexBufferLength % 2)
    {
        RIL_LOG_CRITICAL("ConvertUCS2ToUTF8 - String was not a multiple of 2.\r\n");
        return NULL;
    }

    pByteBuffer = new BYTE[(hexBufferLength / 2) + 1];
    if (NULL == pByteBuffer)
    {
        RIL_LOG_CRITICAL("ConvertUCS2ToUTF8 - Cannot allocate %d bytes for pByteBuffer\r\n",
                (hexBufferLength / 2) + 1);
        goto Error;
    }
    memset(pByteBuffer, 0, ((hexBufferLength / 2) + 1));

    if (!GSMHexToGSM(pHexBuffer, hexBufferLength, pByteBuffer,
                                (hexBufferLength / 2 ) + 1, byteBufferUsed))
    {
        RIL_LOG_CRITICAL("ConvertUCS2ToUTF8 - GSMHexToGSM conversion failed\r\n");
        goto Error;
    }

    pByteBuffer[byteBufferUsed] = '\0';

    utf8Count = ucs2_to_utf8(pByteBuffer, byteBufferUsed / 2, NULL);

    pUtf8Buffer = new char[utf8Count + 1];
    if (NULL == pUtf8Buffer)
    {
        RIL_LOG_CRITICAL("ConvertUCS2ToUTF8 - Cannot allocate %d bytes for pUtf8Buffer",
                utf8Count + 1);
        goto Error;
    }

    RIL_LOG_INFO("ConvertUCS2ToUTF8 - utf8Count: %d\r\n", utf8Count);
    ucs2_to_utf8(pByteBuffer, byteBufferUsed / 2, (unsigned char*) pUtf8Buffer);

    pUtf8Buffer[utf8Count] = '\0';

Error:
    delete[] pByteBuffer;
    return pUtf8Buffer;
}

// convert an Integer into a byte array in Big Endian format
void convertIntToByteArray(unsigned char* byteArray, int value)
{
    convertIntToByteArrayAt(byteArray, value, 0);
}

// convert an Integer into a byte array in Big Endian format starting at 'pos'
void convertIntToByteArrayAt(unsigned char* byteArray, int value, int pos)
{
    for (UINT32 i = 0; i < sizeof(int); i++)
    {
        byteArray[i + pos] = (unsigned char) ((htonl(value) >> (i*8)) & 0xFF);
    }
}

int utf8_from_gsm8(const BYTE* pSrcBuffer, int length, char* pUtf8Buffer)
{
    int  result  = 0;
    int  escaped = 0;

    for (; length > 0; length--)
    {
        int c = *pSrcBuffer++;

        if (c == 0xFF)
            break;

        if (c == GSM_7BITS_ESCAPE)
        {
            if (escaped)
            { /* two escape characters => one space */
                c = 0x20;
                escaped = 0;
            }
            else
            {
                escaped = 1;
                continue;
            }
        }
        else
        {
            if (c >= 0x80)
            {
                c = 0x20;
                escaped = 0;
            }
            else if (escaped)
            {
                c = gsm7bits_extend_to_unicode[c];
            }
            else
            {
                c = gsm7bits_to_unicode[c];
            }
        }

        result += utf8_write((unsigned char*) pUtf8Buffer, result, c);
    }

    return  result;
}

int GetUtf8Count(BYTE* pAlphaBuffer, int base, int len, int offset)
{
    int utf8Count = 0;

    while (len > 0)
    {
        int c = pAlphaBuffer[offset];
        if (c >= 0x80)
        {
            utf8Count += utf8_write(NULL, utf8Count, base + (c & 0x7F));
            offset++;
            len--;
        }
        else
        {
            /* GSM character set */
            int count;
            for (count = 0; count < len && pAlphaBuffer[offset+count] < 0x80; count++);

            utf8Count += utf8_from_gsm8(pAlphaBuffer, count, NULL);
            offset += count;
            len -= count;
        }
    }

    return utf8Count;
}

BOOL convertGsmToUtf8HexString(BYTE* pAlphaBuffer, int offset, const int length,
        char* pszUtf8HexString, const int maxUtf8HexStringLength)
{
    BOOL bRet = FALSE;
    BOOL bIsUCS2 = FALSE;
    int utf8Count = 0;
    int len = 0;
    int base = 0;

    if (NULL == pAlphaBuffer || 0 > length
            || NULL == pszUtf8HexString || 0 > maxUtf8HexStringLength)
        goto Error;

    if (pAlphaBuffer[offset] == 0x80)
    {
        /* UCS2 source encoding */
        int ucs2Len = (length - 1) / 2;

        pAlphaBuffer += 1;
        utf8Count = ucs2_to_utf8(pAlphaBuffer, ucs2Len, NULL);

        if (utf8Count > maxUtf8HexStringLength)
        {
            goto Error;
        }

        utf8Count = ucs2_to_utf8(pAlphaBuffer, ucs2Len, (unsigned char*) pszUtf8HexString);
    }
    else
    {
        if (length >= 3 && pAlphaBuffer[offset] == 0x81)
        {
            len = pAlphaBuffer[offset + 1] & 0xFF;
            if (len > length - 3)
            {
                len = length - 3;
            }

            base = ((pAlphaBuffer[offset + 2] & 0xFF) << 7);
            offset += 3;
            bIsUCS2 = TRUE;
        }
        else if (length >= 4 && pAlphaBuffer[offset] == 0x82)
        {
            len = pAlphaBuffer[offset + 1] & 0xFF;
            if (len > length - 4)
            {
                len = length - 4;
            }

            base = ((pAlphaBuffer[offset + 2] & 0xFF) << 8) |
                    (pAlphaBuffer[offset + 3] & 0xFF);
            offset += 4;
            bIsUCS2 = TRUE;
        }

        if (bIsUCS2)
        {
            utf8Count = GetUtf8Count(pAlphaBuffer, base, len, offset);

            if (utf8Count < maxUtf8HexStringLength)
            {
                utf8Count = 0;
                while (len > 0)
                {
                    int  c = pAlphaBuffer[offset];
                    if (c >= 0x80)
                    {
                        utf8Count += utf8_write((unsigned char*) pszUtf8HexString, utf8Count,
                                base + (c & 0x7F));
                        offset++;
                        len--;
                    }
                    else
                    {
                        /* GSM character set */
                        int count;
                        for (count = 0; count < len && pAlphaBuffer[offset+count] < 0x80; count++);

                        utf8Count += utf8_from_gsm8(pAlphaBuffer, count,
                                pszUtf8HexString + utf8Count);
                        offset += count;
                        len -= count;
                    }
                }
            }
        }
        else
        {
            utf8Count = utf8_from_gsm8(pAlphaBuffer + offset, length, NULL);

            if (utf8Count < maxUtf8HexStringLength)
            {
                utf8Count = utf8_from_gsm8(pAlphaBuffer + offset, length, pszUtf8HexString);
            }
            else
            {
                goto Error;
            }
        }
    }

    bRet = TRUE;
Error:
    RIL_LOG_INFO("convertGsmToUtf8HexString - utf8Count: %d, "
            "maxUtf8HexStringLength: %d\r\n", utf8Count, maxUtf8HexStringLength);

    if (NULL != pszUtf8HexString)
    {
        pszUtf8HexString[MIN(utf8Count, maxUtf8HexStringLength)] = '\0';
    }

    return bRet;
}

/**
 * Utility function to translate Hexadecimal array (characters) onto byte array (numeric values).
 *
 * @param szHexArray An allocated string of hexadecimal characters.
 * @param uiLength Length of Hex array.
 * @param szByteArray An allocated bytes/UINT8 array.
 * @return true if extraction went well
 */
BOOL extractByteArrayFromString(const char* szHexArray, const UINT32 uiLength, UINT8* szByteArray)
{
    char szOneByte[3];
    UINT32 uiVal = 0;
    UINT32 uiCount = 0;

    for (UINT32 i = 0; i < uiLength-1; i += 2)
    {
        szOneByte[0] = szHexArray[i];
        szOneByte[1] = szHexArray[i+1];
        szOneByte[2] = '\0';
        // Several solution here, strtol could be used (maybe cleaner than scanf)
        int ret = sscanf(szOneByte, "%02x", &uiVal);
        if (ret == EOF) return FALSE;

        szByteArray[uiCount] = (UINT8)uiVal; // ex: convert 'D0' chars onto 1 byte 0xD0

        uiCount++;
    }

    return TRUE;
}

/**
 * Utility function to translate byte array (numeric values) onto Hexadecimal array (characters).
 *
 * @param szByteArray An allocated bytes/UINT8 array.
 * @param uiLength Length of bytes/UINT8 array.
 * @param szByteArray  An allocated string will be filled with hexadecimal characters.
 * @return true if extraction went well
 */
BOOL convertByteArrayIntoString(const UINT8* szByteArray, const UINT32 uiLength, char* pszHexArray)
{
    for (UINT32 i = 0; i < uiLength; i++)
    {
        int ret = sprintf(pszHexArray, "%02X", szByteArray[i]);
        if (ret != 2) return FALSE;
        pszHexArray += ret;
    }
    return TRUE;
}
