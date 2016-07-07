////////////////////////////////////////////////////////////////////////////
// data_util.cpp
//
// Copyright 2005-2007 Intrinsyc Software International, Inc.  All rights reserved.
// Patents pending in the United States of America and other jurisdictions.
//
//
// Description:
//    Provides implementations for the RIL Utility functions specific to
//    data.
/////////////////////////////////////////////////////////////////////////////

//  This is for close().
#include <unistd.h>
#include <wchar.h>

//  This is for socket-related calls.
#include <sys/ioctl.h>

#include <errno.h>
#include "data_util.h"
#include "rillog.h"
#include "ril_result.h"
#include "te.h"
#include "util.h"

int MapErrorCodeToRilDataFailCause(UINT32 uiCause)
{
    RIL_LOG_VERBOSE("MapErrorCodeToRilDataFailCause() - Enter\r\n");

    switch (uiCause)
    {
        case CEER_CAUSE_OPERATOR_DETERMINED_BARRING:
            return PDP_FAIL_OPERATOR_BARRED;
        case CEER_CAUSE_INSUFFICIENT_RESOURCES:
            return PDP_FAIL_INSUFFICIENT_RESOURCES;
        case CEER_CAUSE_MISSING_UNKNOWN_APN:
            return PDP_FAIL_MISSING_UKNOWN_APN;
        case CEER_CAUSE_UNKNOWN_PDP_ADDRESS_TYPE:
            return PDP_FAIL_UNKNOWN_PDP_ADDRESS_TYPE;
        case CEER_CAUSE_USER_AUTHENTICATION_FAILED:
            return PDP_FAIL_USER_AUTHENTICATION;
        case CEER_CAUSE_ACTIVATION_REJECTED_BY_GGSN:
            return PDP_FAIL_ACTIVATION_REJECT_GGSN;
        case CEER_CAUSE_ACTIVATION_REJECT_UNSPECIFIED:
            return PDP_FAIL_ACTIVATION_REJECT_UNSPECIFIED;
        case CEER_CAUSE_OPTION_NOT_SUPPORTED:
            return PDP_FAIL_SERVICE_OPTION_NOT_SUPPORTED;
        case CEER_CAUSE_OPTION_NOT_SUBSCRIBED:
        case CEER_CAUSE_OPTION_NOT_SUBSCRIBED_ESM:
            return PDP_FAIL_SERVICE_OPTION_NOT_SUBSCRIBED;
        case CEER_CAUSE_OPTION_TEMP_OUT_OF_ORDER:
            return PDP_FAIL_SERVICE_OPTION_OUT_OF_ORDER;
        case CEER_CAUSE_NSPAI_ALREADY_USED:
            return PDP_FAIL_NSAPI_IN_USE;
        case CEER_CAUSE_PDP_AUTHENTICATION_FAILURE:
            return PDP_FAIL_USER_AUTHENTICATION;
        default:
            return PDP_FAIL_ERROR_UNSPECIFIED;
    }
}

void init_sockaddr_in(struct sockaddr_in* sin, const char* addr)
{
    sin->sin_family = AF_INET;
    sin->sin_port = 0;
    sin->sin_addr.s_addr = inet_addr(addr);
}

BOOL setaddr6(int sockfd6, struct ifreq* ifr, const char* addr)
{
    int ret;

    struct in6_ifreq ifr6;
    inet_pton(AF_INET6, addr, &ifr6.ifr6_addr);

    ioctl(sockfd6, SIOGIFINDEX, ifr);
    ifr6.ifr6_ifindex = ifr->ifr_ifindex;
    ifr6.ifr6_prefixlen = 64; //prefix_len;

    RIL_LOG_INFO("setaddr6() - calling SIOCSIFADDR ADDR:%s INET:%s\r\n",addr,ifr->ifr_name);
    errno = 0;
    ret = ioctl(sockfd6, SIOCSIFADDR, &ifr6);
    if (ret < 0)
    {
        RIL_LOG_CRITICAL("setaddr6() : SIOCSIFADDR : %s\r\n",
                strerror(errno));
        return FALSE;
    }

    return TRUE;
}

BOOL setaddr(int s, struct ifreq* ifr, const char* addr)
{
    int ret;
    init_sockaddr_in((struct sockaddr_in*) &ifr->ifr_addr, addr);
    RIL_LOG_INFO("setaddr - calling SIOCSIFADDR\r\n");
    errno = 0; // NOERROR
    ret = ioctl(s, SIOCSIFADDR, ifr);
    if (ret < 0)
    {
        RIL_LOG_CRITICAL("setaddr() : SIOCSIFADDR : %s\r\n",
            strerror(errno));
        return FALSE;
    }
    return TRUE;
}

BOOL setflags(int s, struct ifreq* ifr, int set, int clr)
{
    int ret;
    RIL_LOG_INFO("setflags - calling SIOCGIFFLAGS\r\n");
    ret = ioctl(s, SIOCGIFFLAGS, ifr);
    if (ret < 0)
    {
        RIL_LOG_CRITICAL("setflags : SIOCGIFFLAGS : %s\r\n", strerror(errno));
        return FALSE;
    }

    ifr->ifr_flags = (ifr->ifr_flags & (~clr)) | set;
    RIL_LOG_INFO("setflags - calling SIOCGIFFLAGS 2\r\n");
    ret = ioctl(s, SIOCSIFFLAGS, ifr);
    if (ret < 0)
    {
        RIL_LOG_CRITICAL("setflags: SIOCSIFFLAGS 2 : %s\r\n", strerror(errno));
        return FALSE;
    }
    return TRUE;
}

BOOL ExtractLocalAddressAndSubnetMask(char* pAddressAndSubnetMask,
        char* pIPv4LocalAddr, UINT32 uiIPv4LocalAddrSize,
        char* pIPv6LocalAddr, UINT32 uiIPv6LocalAddrSize,
        char* pIPv4SubnetMask, UINT32 uiIPv4SubnetMaskSize,
        char* pIPv6SubnetMask, UINT32 uiIPv6SubnetMaskSize)
{
    RIL_LOG_VERBOSE("ExtractLocalAddressAndSubnetMask() - Enter\r\n");

    BOOL bRet = FALSE;

    //  Sanity checks
    if ((NULL == pAddressAndSubnetMask) || (NULL == pIPv4LocalAddr) || (0 == uiIPv4LocalAddrSize)
            || (NULL == pIPv6LocalAddr) || (0 == uiIPv6LocalAddrSize))
    {
        RIL_LOG_CRITICAL("ExtractLocalAddressAndSubnetMask() : Invalid inputs!\r\n");
        return FALSE;
    }

    int nDot = 0;
    char szSubnetMask[MAX_IPADDR_SIZE] = {'\0'};
    char szAddress[MAX_IPADDR_SIZE] = {'\0'};

    for (int i = 0; pAddressAndSubnetMask[i] != '\0'; i++)
    {
        if ('.' == pAddressAndSubnetMask[i])
        {
            nDot++;
            switch (nDot)
            {
                case 4:
                    CopyStringNullTerminate(szAddress, pAddressAndSubnetMask,
                            MIN(i + 1, MAX_IPADDR_SIZE));
                    CopyStringNullTerminate(szSubnetMask, pAddressAndSubnetMask + i + 1,
                            MAX_IPADDR_SIZE);
                    break;
                case 16:
                    // Note: Ipv6 address subnet mask overwrites the Ipv4 address and subnet mask
                    CopyStringNullTerminate(szAddress, pAddressAndSubnetMask,
                            MIN(i + 1, MAX_IPADDR_SIZE));
                    CopyStringNullTerminate(szSubnetMask, pAddressAndSubnetMask + i + 1,
                            MAX_IPADDR_SIZE);
                    break;
            }
        }
    }

    if (!ConvertIPAddressToAndroidReadable(szAddress, pIPv4LocalAddr, uiIPv4LocalAddrSize,
            pIPv6LocalAddr, uiIPv6LocalAddrSize))
    {
        RIL_LOG_CRITICAL("ExtractLocalAddressAndSubnetMask() - "
                "Local address conversion failed\r\n");

        goto Error;
    }

    if ((NULL == pIPv4SubnetMask) || (0 == uiIPv4SubnetMaskSize)
            || (NULL == pIPv6SubnetMask) || (0 == uiIPv6SubnetMaskSize))
    {
        RIL_LOG_CRITICAL("ExtractLocalAddressAndSubnetMask() - "
                "subnet mask not provided\r\n");

        goto Error;
    }

    if (!ConvertIPAddressToAndroidReadable(szSubnetMask,
            pIPv4SubnetMask, uiIPv4SubnetMaskSize, pIPv6SubnetMask, uiIPv6SubnetMaskSize))
    {
        RIL_LOG_CRITICAL("ExtractLocalAddressAndSubnetMask() - "
                "subnet mask conversion failed\r\n");
        goto Error;
    }

    bRet = TRUE;

Error:
    RIL_LOG_VERBOSE("ExtractLocalAddressAndSubnetMask() - Exit\r\n");
    return bRet;
}

// Helper function to convert IP addresses to Android-readable format.
// szIpIn [IN] - The IP string to be converted
// szIpOut [OUT] - The converted IPv4 address in Android-readable format if there is an IPv4 address.
// ipOutSize [IN] - The size of the szIpOut buffer
// szIpOut2 [OUT] - The converted IPv6 address in Android-readable format if there is an IPv6 address.
// ipOutSize [IN] - The size of szIpOut2 buffer
//
// If IPv4 format a1.a2.a3.a4, then szIpIn is copied to szIpOut.
// If Ipv6 format:
// Convert a1.a2.a3.a4.a5. ... .a15.a16 to XXXX:XXXX:XXXX:XXXX:XXXX:XXXX:XXXX:XXXX IPv6 format
// output is copied to szIpOut2
// If Ipv4v6 format:
// Convert a1.a2.a3.a4.a5. ... .a19.a20 to
// a1.a2.a3.a4 to szIpOut
// XXXX:XXXX:XXXX:XXXX:XXXX:XXXX:XXXX:XXXX (a5-a20) to szIpOut2
// If szIpOut2 is NULL, then this parameter is ignored
BOOL ConvertIPAddressToAndroidReadable(char* szIpIn,
                                       char* szIpOut,
                                       int ipOutSize,
                                       char* szIpOut2,
                                       int ipOutSize2)
{
    RIL_LOG_VERBOSE("ConvertIPAddressToAndroidReadable() - Enter\r\n");
    BOOL bRet = FALSE;
    const int MAX_AIPV4_INDEX = 4;      // Number of 'a' values read from modem for IPv4 case
    const int MAX_AIPV6_INDEX = 16;     // Number of 'a' values read from modem for IPv6 case
    const int MAX_AIPV4V6_INDEX = 20;   // Number of 'a' values read from modem for IPv4v6 case

    //  Sanity checks
    if ( (NULL == szIpIn) || (NULL == szIpOut) || (0 == ipOutSize))
    {
        RIL_LOG_CRITICAL("ConvertIPAddressToAndroidReadable() : Invalid inputs!\r\n");
        return FALSE;
    }

    //  Count number of '.'
    int nDotCount = 0;
    for (UINT32 i = 0; szIpIn[i] != '\0'; i++)
    {
        if ('.' == szIpIn[i])
        {
            //  Found a '.'
            nDotCount++;
        }
    }

    //  If 3 dots, IPv4. If 15 dots, IPv6. If 19 dots, then IPv4v6.
    switch(nDotCount)
    {
        case 3:
        {
            //  Extract a1...a4 into uiIP.
            //  Then convert aAddress to szIpOut.
            UINT32 uiIP[MAX_AIPV4_INDEX] = {'\0'};
            if (EOF == sscanf(szIpIn, "%u.%u.%u.%u",
                    &uiIP[0], &uiIP[1], &uiIP[2], &uiIP[3]))
            {
                RIL_LOG_CRITICAL("ConvertIPAddressToAndroidReadable() -"
                        " cannot sscanf into ipv4 format\r\n");
                goto Error;
            }

            if (255 < uiIP[0] || 255 < uiIP[1] || 255 < uiIP[2] || 255 < uiIP[3])
            {
                RIL_LOG_INFO("ConvertIPAddressToAndroidReadable() -"
                        "Wrong IPv4 Address : %u.%u.%u.%u\r\n",
                        uiIP[0], uiIP[1], uiIP[2], uiIP[3]);
                goto Error;
            }
            else
            {
                if (snprintf(szIpOut, ipOutSize,
                        "%u.%u.%u.%u", uiIP[0], uiIP[1], uiIP[2], uiIP[3]) <= 0)
                {
                    RIL_LOG_CRITICAL("ConvertIPAddressToAndroidReadable() -"
                            " error with snprintf() ipv4 part\r\n");
                    goto Error;
                }
            }
        }
        break;

        case 15:
        {
            //  IPv6 format.  Need to re-format this to Android IPv6 format.

            //  Extract a1...a16 into aIP.
            //  Then convert aAddress to szIpOut.
            UINT32 aIP[MAX_AIPV6_INDEX] = {0};
            unsigned char acIP[MAX_AIPV6_INDEX] = {0};
            if (EOF == sscanf(szIpIn, "%u.%u.%u.%u.%u.%u.%u.%u.%u.%u.%u.%u.%u.%u.%u.%u",
                    &aIP[0], &aIP[1], &aIP[2], &aIP[3], &aIP[4], &aIP[5], &aIP[6], &aIP[7],
                    &aIP[8], &aIP[9], &aIP[10], &aIP[11], &aIP[12], &aIP[13], &aIP[14], &aIP[15]))
            {
                RIL_LOG_CRITICAL("ConvertIPAddressToAndroidReadable() -"
                        " cannot sscanf into aIP[]! ipv6\r\n");
                goto Error;
            }

            //  Loop through array, check values from modem is from 0-255.
            for (int i=0; i<MAX_AIPV6_INDEX; i++)
            {
                if (aIP[i] > 255)
                {
                    //  Value is not between 0-255.
                    RIL_LOG_CRITICAL("ConvertIPAddressToAndroidReadable() -"
                            " ipv6 aIP[%d] not in range 0-255. val=%u\r\n", i, aIP[i]);
                    goto Error;
                }
            }

            //  Convert unsigned int to unsigned char (for inet_ntop)
            //  The value read in should be in range 0-255.
            for (int i=0; i<MAX_AIPV6_INDEX; i++)
            {
                acIP[i] = (unsigned char)(aIP[i]);
            }

            if (NULL != szIpOut2 && 0 != ipOutSize2)
            {

                if (inet_ntop(AF_INET6, (void*)acIP, szIpOut2, ipOutSize2) == NULL)
                {
                    RIL_LOG_CRITICAL("ConvertIPAddressToAndroidReadable() -"
                            " cannot inet_ntop ipv6\r\n");
                    goto Error;
                }
            }
        }
        break;

        case 19:
        {
            //  IPv4v6 format.  Grab a1.a2.a3.a4 and that's the IPv4 part.  The rest is IPv6.
            //  Extract a1...a20 into aIP.
            //  Then IPv4 part is extracted into szIpOut.
            //  IPV6 part is extracted into szIpOut2.
            UINT32 aIP[MAX_AIPV4V6_INDEX] = {0};
            if (EOF == sscanf(szIpIn, "%u.%u.%u.%u.%u.%u.%u.%u.%u.%u.%u.%u.%u.%u."
                    "%u.%u.%u.%u.%u.%u", &aIP[0], &aIP[1], &aIP[2], &aIP[3],
                    &aIP[4], &aIP[5], &aIP[6], &aIP[7], &aIP[8], &aIP[9], &aIP[10], &aIP[11],
                    &aIP[12], &aIP[13], &aIP[14], &aIP[15], &aIP[16], &aIP[17], &aIP[18], &aIP[19]
                    ))
            {
                RIL_LOG_CRITICAL("ConvertIPAddressToAndroidReadable() -"
                        " cannot sscanf into aIP[]! ipv4v6\r\n");
                goto Error;
            }

            //  Loop through array, check values from modem is from 0-255.
            for (int i=0; i<MAX_AIPV4V6_INDEX; i++)
            {
                if (aIP[i] > 255)
                {
                    //  Value is not between 0-255.
                    RIL_LOG_CRITICAL("ConvertIPAddressToAndroidReadable() -"
                            " ipv4v6 aIP[%d] not in range 0-255. val=%u\r\n", i, aIP[i]);
                    goto Error;
                }
            }

            if (snprintf(szIpOut, ipOutSize,
                    "%u.%u.%u.%u",
                    aIP[0], aIP[1], aIP[2], aIP[3]) <= 0)
            {
                RIL_LOG_CRITICAL("ConvertIPAddressToAndroidReadable() -"
                        " error with snprintf()! ipv4v6 v4 part\r\n");
                goto Error;
            }

            if (NULL != szIpOut2 && 0 != ipOutSize2)
            {
                unsigned char acIP[MAX_AIPV6_INDEX] = {0};

                //  Convert unsigned int to unsigned char (for inet_ntop)
                //  The value read in should be in range 0-255, from check done above.
                for (int i=0; i<MAX_AIPV6_INDEX; i++)
                {
                    acIP[i] = (unsigned char)(aIP[i+4]);
                }

                if (inet_ntop(AF_INET6, (void*)acIP, szIpOut2, ipOutSize2) == NULL)
                {
                    RIL_LOG_CRITICAL("ConvertIPAddressToAndroidReadable() -"
                            " cannot inet_ntop ipv4v6\r\n");
                    goto Error;
                }
            }
        }
        break;

        default:
            RIL_LOG_CRITICAL("ConvertIPAddressToAndroidReadable() -"
                    " Unknown address format nDotCount=[%d]\r\n", nDotCount);
            goto Error;
    }

    bRet = TRUE;

Error:
    RIL_LOG_VERBOSE("ConvertIPAddressToAndroidReadable() - Exit\r\n");
    return bRet;
}
