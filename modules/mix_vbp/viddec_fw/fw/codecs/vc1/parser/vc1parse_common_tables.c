/* ///////////////////////////////////////////////////////////////////////
//
//               INTEL CORPORATION PROPRIETARY INFORMATION
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with Intel Corporation and may not be copied
//  or disclosed except in accordance with the terms of that agreement.
//        Copyright (c) 2008 Intel Corporation. All Rights Reserved.
//
//  Description: Contains tables for VLC decoding of syntax elements in simple
//  or main profile of VC-1 bitstream.
//
*/

#include "vc1parse.h"

const uint8_t VC1_MVMODE_LOW_TBL[] =
{
    VC1_MVMODE_HPELBI_1MV,
    VC1_MVMODE_1MV,
    VC1_MVMODE_HPEL_1MV,
    VC1_MVMODE_MIXED_MV,
    VC1_MVMODE_INTENSCOMP
};

const uint8_t VC1_MVMODE_HIGH_TBL[] =
{
    VC1_MVMODE_1MV,
    VC1_MVMODE_MIXED_MV,
    VC1_MVMODE_HPEL_1MV,
    VC1_MVMODE_HPELBI_1MV,
    VC1_MVMODE_INTENSCOMP
};

const int32_t VC1_BITPLANE_IMODE_TBL[] =
{
    4, /* max bits */
    1, /* total subtables */
    4, /* subtable sizes */

    0, /* 1-bit codes */
    2, /* 2-bit codes */
    2, VC1_BITPLANE_NORM2_MODE,
    3, VC1_BITPLANE_NORM6_MODE,
    3, /* 3-bit codes */
    1, VC1_BITPLANE_DIFF2_MODE,
    2, VC1_BITPLANE_ROWSKIP_MODE,
    3, VC1_BITPLANE_COLSKIP_MODE,
    2, /* 4-bit codes */
    0, VC1_BITPLANE_RAW_MODE,
    1, VC1_BITPLANE_DIFF6_MODE,
    -1
};

/* This VLC table is used for decoding of k in bitplane. */
const int32_t VC1_BITPLANE_K_TBL[] =
{
    13, /* max bits */
    2,  /* total subtables */
    6,7,/* subtable sizes */

    1, /* 1-bit codes */
    1,       0 ,
    0, /* 2-bit codes */
    0, /* 3-bit codes */
    6, /* 4-bit codes */
    2, 1,    3, 2,     4, 4,    5, 8,
    6, 16,   7, 32,
    0, /* 5-bit codes */
    1, /* 6-bit codes */
    (3 << 1)| 1,     63,
    0, /* 7-bit codes */
    15, /* 8-bit codes */
    0, 3,    1, 5,    2, 6,    3, 9,
    4, 10,   5, 12,   6, 17,   7, 18,
    8, 20,   9, 24,   10, 33,  11, 34,
    12, 36,  13, 40,  14, 48,
    6, /* 9-bit codes */
    (3 << 4)| 7,    31,
    (3 << 4)| 6,    47,
    (3 << 4)| 5,    55,
    (3 << 4)| 4,    59,

    (3 << 4)| 3,    61,
    (3 << 4)| 2,    62,
    20, /* 10-bit codes */
    (1 << 6)| 11,  11,
    (1 << 6)|  7,  7 ,
    (1 << 6)| 13,  13,
    (1 << 6)| 14,  14,

    (1 << 6)| 19,  19,
    (1 << 6)| 21,  21,
    (1 << 6)| 22,  22,
    (1 << 6)| 25,  25,

    (1 << 6)| 26,  26,
    (1 << 6)| 28,  28,
    (1 << 6)|  3,  35,
    (1 << 6)|  5,  37,

    (1 << 6)|  6,  38,
    (1 << 6)|  9,  41,
    (1 << 6)| 10,  42,
    (1 << 6)| 12,  44,

    (1 << 6)| 17,  49,
    (1 << 6)| 18,  50,
    (1 << 6)| 20,  52,
    (1 << 6)| 24,  56,
    0,  /* 11-bit codes */
    0,  /* 12-bit codes */
    15, /* 13-bit codes */
    (3 << 8)| 14,  15,
    (3 << 8)| 13,  23,
    (3 << 8)| 12,  27,
    (3 << 8)| 11,  29,

    (3 << 8)| 10,  30,
    (3 << 8)|  9,  39,
    (3 << 8)|  8,  43,
    (3 << 8)|  7,  45,

    (3 << 8)|  6,  46,
    (3 << 8)|  5,  51,
    (3 << 8)|  4,  53,
    (3 << 8)|  3,  54,

    (3 << 8)|  2,  57,
    (3 << 8)|  1,  58,
    (3 << 8)|  0,  60,
    -1
};

/* This VLC table is used for decoding of BFRACTION. */
const int32_t VC1_BFRACTION_TBL[] =
{
    7,        /* max bits */
    2,        /* total subtables */
    3,4,    /* subtable sizes */
    0,        /* 1-bit codes */
    0,        /* 2-bit codes */
    7,        /* 3-bit codes */
    0x00,1,2,    0x01,1,3,    0x02,2,3,    0x03,1,4,
    0x04,3,4,    0x05,1,5,    0x06,2,5,
    0,        /* 4-bit codes */
    0,        /* 5-bit codes */
    0,        /* 6-bit codes */
    16,    /* 7-bit codes */
    0x70, 3,5,    0x71, 4,5,    0x72, 1,6,    0x73, 5,6,
    0x74, 1,7,    0x75, 2,7,    0x76, 3,7,    0x77, 4,7,
    0x78, 5,7,    0x79, 6,7,    0x7A, 1,8,    0x7B, 3,8,
    0x7C, 5,8,    0x7D, 7,8,
    0x7E, VC1_BFRACTION_INVALID,VC1_BFRACTION_INVALID,
    0x7F, VC1_BFRACTION_BI, VC1_BFRACTION_BI,

    -1
};

/* This table is used for VLC decoding of REFDIST. */
const int32_t VC1_REFDIST_TBL[] =
{
    16, /* Max bits. */
    3, /* Total sub-tables. */
    5, 6, 5, /* Sub-table sizes. */

    0, /* 1-bit codes. */
    3, /* 2-bit codes. */
    0,  0,       1,  1,       2,  2,
    1, /* 3-bit codes. */
    6,  3,
    1, /* 4-bit codes. */
    14,  4,
    1, /* 5-bit codes. */
    30,  5,
    1, /* 6-bit codes. */
    62,  6,
    1, /* 7-bit codes. */
    126,  7,
    1, /* 8-bit codes. */
    254,  8,
    1, /* 9-bit codes. */
    510,  9,
    1, /* 10-bit codes. */
    1022, 10,
    1, /* 11-bit codes. */
    2046, 11,
    1, /* 12-bit codes. */
    4094, 12,
    1, /* 13-bit codes. */
    8190, 13,
    1, /* 14-bit codes. */
    16382, 14,
    1, /* 15-bit codes. */
    32766, 15,
    1, /* 16-bit codes. */
    65534, 16,
    -1  /* end of table. */
};
