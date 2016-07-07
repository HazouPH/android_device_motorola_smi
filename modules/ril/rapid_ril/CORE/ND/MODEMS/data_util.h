////////////////////////////////////////////////////////////////////////////
// data_util.h
//
// Copyright 2005-2007 Intrinsyc Software International, Inc.  All rights reserved.
// Patents pending in the United States of America and other jurisdictions.
//
//
// Description:
//    Provides implementations for the RIL Utility functions specific to
//    data.
/////////////////////////////////////////////////////////////////////////////

#include "types.h"

//  This is for socket-related calls.
#include <sys/socket.h>
#include <arpa/inet.h>
#include <linux/gsmmux.h>

// Helper functions for configuring data connections
BOOL setaddr6(int sockfd6, struct ifreq* ifr, const char* addr);
BOOL setaddr(int s, struct ifreq* ifr, const char* addr);
BOOL setflags(int s, struct ifreq* ifr, int set, int clr);

int MapErrorCodeToRilDataFailCause(UINT32 uiCause);

// Helper function to extract local address and subnet mask from <LOCAL_ADDR and SUBNET_MASK>
// pAddressAndSubnetMask [IN] - The Local Address and subnet mask.
// pIPv4LocalAddr [OUT] - Extracted local address in IPv4 format.
// uiIPv4LocalAddrSize [IN] - The size of the pIPv4LocalAddr buffer
// pIPv6LocalAddr [OUT] - Extracted local address in IPv6 format.
// uiIPv6LocalAddrSize [IN] - The size of the pIPv6LocalAddr buffer
// pIPv4SubnetMask [OUT] - Extracted subnet mask in IPv4 format.
// uiIPv4LocalAddrSize [IN] - The size of the pIPv4SubnetMask buffer
// pIPv6SubnetMask [OUT] - Extracted subnet mask in IPv6 format..
// uiIP6SubnetMaskSize [IN] - The size of pIPv6SubnetMask buffer
//
// If IPv4 format a1.a2.a3.a4.m1.m2.m3.m4, then a1.a2.a3.a4 is extracted into pIPv4LocalAddr and
// m1.m2.m3.m4 is extracted into pIPv4SubnetMask.
// If Ipv6 format a1.a2.a3.a4.a5.a6.a7.a8.a9.a10.a12.a13.a14.a15.a16.m1.m2.m3.m4. ... .m15.m16,
// then a1.a2.a3.a4.a5. ... .a15.a16 is extracted to pIPv6LocalAddr and m1.m2.m3.m4. ... .m15.m16
// is extracted into pIPv6SubnetMask
BOOL ExtractLocalAddressAndSubnetMask(char* pAddressAndSubnetMask,
       char* pIPv4LocalAddr, UINT32 uiIPv4LocalAddrSize,
       char* pIPv6LocalAddr, UINT32 uiIPv6LocalAddrSize,
       char* pIPv4SubnetMask, UINT32 uiIPv4SubnetMaskSize,
       char* pIPv6SubnetMask, UINT32 uiIP6SubnetMaskSize);

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
                                      int ipOutSize2);
