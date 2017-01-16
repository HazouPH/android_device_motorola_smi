/*//////////////////////////////////////////////////////////////////////////////
//
//                  INTEL CORPORATION PROPRIETARY INFORMATION
//     This software is supplied under the terms of a license agreement or
//     nondisclosure agreement with Intel Corporation and may not be copied
//     or disclosed except in accordance with the terms of that agreement.
//          Copyright(c) 2011 Intel Corporation. All Rights Reserved.
//
*/

#ifndef __MC_VERSION_H__
#define __MC_VERSION_H__

// Gets major.minor version and revision
void MediaCodecs_GetVersion(int &major, int &minor, int &revision);

// Returns version string in the form "major.minor.revision"
const char* MediaCodecs_GetVersion();

#endif