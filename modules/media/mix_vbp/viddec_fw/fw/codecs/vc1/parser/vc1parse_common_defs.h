/* ///////////////////////////////////////////////////////////////////////
//
//               INTEL CORPORATION PROPRIETARY INFORMATION
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with Intel Corporation and may not be copied
//  or disclosed except in accordance with the terms of that agreement.
//        Copyright (c) 2008 Intel Corporation. All Rights Reserved.
//
//  Description: Common definitions for parsing VC-1 bitstreams.
//
*/

#ifndef _VC1PARSE_COMMON_DEFS_H_
#define _VC1PARSE_COMMON_DEFS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

    /** @weakgroup vc1parse_common_defs VC-1 Common Definitions */
    /** @ingroup vc1parse_common_defs */
    /*@{*/

    /** This defines the maximum number of horizontal macroblocks in a picture. */
#define VC1_WIDTH_MB_MAX         ((2048+15)/16)

    /** This defines the maximum number of vertical macroblocks in a picture. */
#define VC1_HEIGHT_MB_MAX        ((1088+15)/16)

    /** This defines the maximum number of bitplane storage per picture. */
#define VC1_MAX_BITPLANE_CHUNKS   3

    /** This defines the value for an invalid BFRACTION syntax element. */
#define VC1_BFRACTION_INVALID    0

    /** This defines the value for BFRACTION syntax element that defines a BI
    picture. */
#define VC1_BFRACTION_BI         9

    /** This enumeration defines the various supported profiles as defined in
    PROFILE syntax element. */
    enum
    {
        VC1_PROFILE_SIMPLE,
        VC1_PROFILE_MAIN,
        VC1_PROFILE_RESERVED,
        VC1_PROFILE_ADVANCED
    };

    /** This enumeration defines the frame coding mode as defined in FCM syntax
    element. */
    enum
    {
        VC1_FCM_PROGRESSIVE,
        VC1_FCM_FRAME_INTERLACE = 2,
        VC1_FCM_FIELD_INTERLACE = 3
    };

    /** This enumeration defines the various bitplane types as defined in IMODE
    syntax element. */
    enum
    {
        VC1_BITPLANE_RAW_MODE,
        VC1_BITPLANE_NORM2_MODE,
        VC1_BITPLANE_DIFF2_MODE,
        VC1_BITPLANE_NORM6_MODE,
        VC1_BITPLANE_DIFF6_MODE,
        VC1_BITPLANE_ROWSKIP_MODE,
        VC1_BITPLANE_COLSKIP_MODE
    };

    /** This enumeration defines the various motion vector modes as defined in
    MVMODE or MVMODE2 syntax element. */
    enum
    {
        VC1_MVMODE_1MV,
#ifdef VBP
        VC1_MVMODE_HPELBI_1MV,
        VC1_MVMODE_HPEL_1MV,
#else
        VC1_MVMODE_HPEL_1MV,
        VC1_MVMODE_HPELBI_1MV,
#endif
        VC1_MVMODE_MIXED_MV,
        VC1_MVMODE_INTENSCOMP
    };

    /** This enumeration defines the extended differential motion vector range flag
    as defined in DMVRANGE syntax element. */
    enum
    {
        VC1_DMVRANGE_NONE,
        VC1_DMVRANGE_HORIZONTAL_RANGE,
        VC1_DMVRANGE_VERTICAL_RANGE,
        VC1_DMVRANGE_HORIZONTAL_VERTICAL_RANGE
    };

    /** This enumeration defines the intensity compensation field as defined in
    INTCOMPFIELD syntax element. */
    enum
    {
        VC1_INTCOMP_TOP_FIELD    = 1,
        VC1_INTCOMP_BOTTOM_FIELD = 2,
        VC1_INTCOMP_BOTH_FIELD   = 3
    };

    /** This enumeration defines the differential quantizer profiles as defined in
    DQPROFILE syntax element. */
    enum
    {
        VC1_DQPROFILE_ALL4EDGES,
        VC1_DQPROFILE_DBLEDGES,
        VC1_DQPROFILE_SNGLEDGES,
        VC1_DQPROFILE_ALLMBLKS
    };

    /** This enumeration defines the conditional overlap flag as defined in CONDOVER
    syntax element. */
    enum
    {
        VC1_CONDOVER_FLAG_NONE = 0,
        VC1_CONDOVER_FLAG_ALL  = 2,
        VC1_CONDOVER_FLAG_SOME = 3
    };

    /** This enumeration defines the type of quantizer to be used and is derived
    from bitstream syntax. */
    enum
    {
        VC1_QUANTIZER_NONUNIFORM,
        VC1_QUANTIZER_UNIFORM
    };

    /** This structure represents the various bitplanes within VC-1 bitstream. */
    typedef struct
    {
        uint8_t invert;
        int32_t imode;
        uint32_t *databits;
    } vc1_Bitplane;

#ifdef VBP
#define VC1_MAX_HRD_NUM_LEAKY_BUCKETS   32

    typedef struct
    {
        uint32_t	 HRD_RATE;				 /** Maximum bit rate in bits per second */
        uint32_t	 HRD_BUFFER;			 /** Buffer size in bits */
        uint32_t	 HRD_FULLNESS;			 /** Buffer fullness in complete bits */
        uint32_t	 HRD_FULLFRACTION;		 /** Numerator of fractional bit buffer fullness count */
        uint32_t	 HRD_FULLDENOMINATOR;	 /** Denominator of fractional bit buffer fullness count */
    } vc1_leaky_bucket;

    typedef struct _vc1_hrd_state
    {
        uint8_t 		 BIT_RATE_EXPONENT; 							  /** Buckets
																			(0 if none specified) */
        uint8_t 		 BUFFER_SIZE_EXPONENT;
        vc1_leaky_bucket sLeakyBucket[VC1_MAX_HRD_NUM_LEAKY_BUCKETS];	/** Per-bucket information */
    } vc1_hrd_state, *vc1_hrd_state_ptr;
#endif

    /** This structure represents all bitstream metadata needed for register programming. */
    typedef struct
    {
        // From Sequence Layer for Advanced Profile
        uint8_t  PROFILE;                   /**  2 bit(s). */
        uint8_t  LEVEL;                     /**  3 bit(s). */
        uint8_t  CHROMAFORMAT;              /**  2 bit(s). */
        uint8_t  FRMRTQ;                    /**  3 bit(s). */

        uint8_t  BITRTQ;                    /**  5 bit(s). */
        uint8_t  POSTPROCFLAG;              /**  1 bit(s). */
        uint8_t  PULLDOWN;                  /**  1 bit(s). */
        uint8_t  INTERLACE;                 /**  1 bit(s). */

        uint8_t  TFCNTRFLAG;                /**  1 bit(s). */
        uint8_t  FINTERPFLAG;               /**  1 bit(s). */
        uint8_t  PSF;                       /**  1 bit(s). */
        uint8_t  HRD_NUM_LEAKY_BUCKETS;     /**  5 bit(s). */

        // From STRUCT_C
        uint8_t  MAXBFRAMES;                /**  3 bit(s). */
        uint8_t  MULTIRES;                  /**  1 bit(s). */

        // From EntryPoint Layer for Advanced Profile
        uint8_t BROKEN_LINK;
        uint8_t CLOSED_ENTRY;

        uint8_t PANSCAN_FLAG;
        uint8_t REFDIST_FLAG;
        uint8_t LOOPFILTER;
        uint8_t FASTUVMC;

        uint8_t EXTENDED_MV;
        uint8_t DQUANT;
        uint8_t VSTRANSFORM;
        uint8_t OVERLAP;

        uint8_t QUANTIZER;
        uint8_t EXTENDED_DMV;
        uint8_t RANGE_MAPY_FLAG;
        uint8_t RANGE_MAPY;

        uint8_t RANGE_MAPUV_FLAG;
        uint8_t RANGE_MAPUV;

        // From Picture Header
        uint8_t  RANGERED;                  /**  1 bit(s). */
        uint8_t  RNDCTRL;                   /**  1 bit(s), rcv specific. */

        // REFDIST is present only in field-interlaced mode on I/I, I/P, P/I, P/P frames
        // From Canmore, looks like this needs to be propagated to following B frames
        uint8_t  REFDIST;
        uint8_t  INTCOMPFIELD;              /**  ? bit(s)? */
        uint8_t  LUMSCALE2;                 /**  6 bit(s). */
        uint8_t  LUMSHIFT2;                 /**  6 bit(s). */

        uint8_t bp_raw[VC1_MAX_BITPLANE_CHUNKS];
        uint8_t res_1;

        // From SequenceLayerHeader, EntryPointHeader or Struct_A
        uint16_t width;
        uint16_t height;
        uint16_t widthMB;
        uint16_t heightMB;

#ifdef VBP
        uint8_t COLOR_FORMAT_FLAG;
        uint8_t MATRIX_COEF;
        uint8_t SYNCMARKER;
        uint8_t ASPECT_RATIO_FLAG;
        uint8_t ASPECT_RATIO;
        uint8_t ASPECT_HORIZ_SIZE;
        uint8_t ASPECT_VERT_SIZE;
        vc1_hrd_state hrd_initial_state;
#endif

    } vc1_metadata_t;

    /** This structure represents the sequence header for advanced profile. */
    typedef struct
    {
        union
        {
#ifndef MFDBIGENDIAN
            struct
            {
                unsigned BITRTQ_POSTPROC:5;
                unsigned FRMRTQ_POSTPROC:3;
                unsigned COLORDIFF_FORMAT:2;
                unsigned LEVEL:3;
                unsigned PROFILE:2;
                unsigned pad:17;
            } seq_flags;
#else
            struct
            {
                unsigned pad:17;
                unsigned PROFILE:2;
                unsigned LEVEL:3;
                unsigned COLORDIFF_FORMAT:2;
                unsigned FRMRTQ_POSTPROC:3;
                unsigned BITRTQ_POSTPROC:5;
            } seq_flags;
#endif
            uint32_t flags;
        };

        union
        {
#ifndef MFDBIGENDIAN
            struct
            {
                unsigned DISPLAY_EXT:1;
                unsigned PSF:1;
                unsigned RESERVED:1;
                unsigned FINTERPFLAG:1;
                unsigned TFCNTRFLAG:1;
                unsigned INTERLACE:1;
                unsigned PULLDOWN:1;
                unsigned MAX_CODED_HEIGHT:12;
                unsigned MAX_CODED_WIDTH:12;
                unsigned POSTPROCFLAG:1;
            } seq_max_size;
#else
            struct
            {
                unsigned POSTPROCFLAG:1;
                unsigned MAX_CODED_WIDTH:12;
                unsigned MAX_CODED_HEIGHT:12;
                unsigned PULLDOWN:1;
                unsigned INTERLACE:1;
                unsigned TFCNTRFLAG:1;
                unsigned FINTERPFLAG:1;
                unsigned RESERVED:1;
                unsigned PSF:1;
                unsigned DISPLAY_EXT:1;
            } seq_max_size;
#endif
            uint32_t max_size;
        };

        union
        {
#ifndef MFDBIGENDIAN
            struct
            {
                unsigned ASPECT_RATIO_FLAG:1;
                unsigned DISP_VERT_SIZE:14;
                unsigned DISP_HORIZ_SIZE:14;
                unsigned pad:3;
            } seq_disp_size;
#else
            struct
            {
                unsigned pad:3;
                unsigned DISP_HORIZ_SIZE:14;
                unsigned DISP_VERT_SIZE:14;
                unsigned ASPECT_RATIO_FLAG:1;
            } seq_disp_size;
#endif
            uint32_t disp_size;
        };

        uint8_t ASPECT_RATIO;   // 4 bits

        union
        {
#ifndef MFDBIGENDIAN
            struct
            {
                unsigned ASPECT_VERT_SIZE:8;
                unsigned ASPECT_HORIZ_SIZE:8;
                unsigned pad:16;
            } seq_aspect_size;
#else
            struct
            {
                unsigned pad:16;
                unsigned ASPECT_HORIZ_SIZE:8;
                unsigned ASPECT_VERT_SIZE:8;
            } seq_aspect_size;
#endif
            uint32_t aspect_size;
        };

        uint8_t FRAMERATE_FLAG; // 1b
        uint8_t FRAMERATEIND;   // 1b

        union
        {
#ifndef MFDBIGENDIAN
            struct
            {
                unsigned FRAMERATEDR:4;
                unsigned FRAMERATENR:8;
                unsigned pad:20;
            } seq_framerate_fraction;
#else
            struct
            {
                unsigned pad:20;
                unsigned FRAMERATENR:8;
                unsigned FRAMERATEDR:4;
            } seq_framerate_fraction;
#endif
            uint32_t framerate_fraction;
        };

        uint16_t FRAMERATEEXP;      // 16b
        uint8_t COLOR_FORMAT_FLAG; // 1b

        union
        {
#ifndef MFDBIGENDIAN
            struct
            {
                unsigned MATRIX_COEF:8;
                unsigned TRANSFER_CHAR:8;
                unsigned COLOR_PRIM:8;
                unsigned pad:8;
            } seq_color_format;
#else
            struct
            {
                unsigned pad:8;
                unsigned COLOR_PRIM:8;
                unsigned TRANSFER_CHAR:8;
                unsigned MATRIX_COEF:8;
            } seq_color_format;
#endif
            uint32_t color_format;
        };

        uint8_t HRD_PARAM_FLAG;         // 1b
        uint8_t HRD_NUM_LEAKY_BUCKETS;  // 5b
        // No need to parse remaining items - not needed so far
    } vc1_SequenceLayerHeader;

    /** This structure represents metadata for struct c. */
    typedef struct
    {
        union
        {
#ifndef MFDBIGENDIAN
            struct
            {
                unsigned res6:1;
                unsigned FINTERPFLAG:1;
                unsigned QUANTIZER:2;
                unsigned MAXBFRAMES:3;
                unsigned RANGERED:1;
                unsigned SYNCMARKER:1;
                unsigned OVERLAP:1;
                unsigned res5:1;
                unsigned VSTRANSFORM:1;
                unsigned DQUANT:2;
                unsigned EXTENDED_MV:1;
                unsigned FASTUVMC:1;
                unsigned res4:1;
                unsigned MULTIRES:1;
                unsigned res3:1;
                unsigned LOOPFILTER:1;
                unsigned BITRTQ_POSTPROC:5;
                unsigned FRMRTQ_POSTPROC:3;
                unsigned PROFILE:4;
            } struct_c;
#else
            struct
            {
                unsigned PROFILE:4;
                unsigned FRMRTQ_POSTPROC:3;
                unsigned BITRTQ_POSTPROC:5;
                unsigned LOOPFILTER:1;
                unsigned res3:1;
                unsigned MULTIRES:1;
                unsigned res4:1;
                unsigned FASTUVMC:1;
                unsigned EXTENDED_MV:1;
                unsigned DQUANT:2;
                unsigned VSTRANSFORM:1;
                unsigned res5:1;
                unsigned OVERLAP:1;
                unsigned SYNCMARKER:1;
                unsigned RANGERED:1;
                unsigned MAXBFRAMES:3;
                unsigned QUANTIZER:2;
                unsigned FINTERPFLAG:1;
                unsigned res6:1;
            } struct_c;
#endif
            uint32_t struct_c_rcv;
        };

        union
        {
#ifndef MFDBIGENDIAN
            struct
            {
                unsigned VERT_SIZE:16;
                unsigned HORIZ_SIZE:16;
            } struct_a;
#else
            struct
            {
                unsigned HORIZ_SIZE:16;
                unsigned VERT_SIZE:16;
            } struct_a;
#endif
            uint32_t struct_a_rcv;
        };

    } vc1_RcvSequenceHeader;

    /** This structure represents metadata for entry point layers. */
    typedef struct
    {
        union
        {
#ifndef MFDBIGENDIAN
            struct
            {
                unsigned QUANTIZER:2;
                unsigned OVERLAP:1;
                unsigned VSTRANSFORM:1;
                unsigned DQUANT:2;
                unsigned EXTENDED_MV:1;
                unsigned FASTUVMC:1;
                unsigned LOOPFILTER:1;
                unsigned REFDIST_FLAG:1;
                unsigned PANSCAN_FLAG:1;
                unsigned CLOSED_ENTRY:1;
                unsigned BROKEN_LINK:1;
                unsigned pad1:19;
            } ep_flags;
#else
            struct
            {
                unsigned pad1:19;
                unsigned BROKEN_LINK:1;
                unsigned CLOSED_ENTRY:1;
                unsigned PANSCAN_FLAG:1;
                unsigned REFDIST_FLAG:1;
                unsigned LOOPFILTER:1;
                unsigned FASTUVMC:1;
                unsigned EXTENDED_MV:1;
                unsigned DQUANT:2;
                unsigned VSTRANSFORM:1;
                unsigned OVERLAP:1;
                unsigned QUANTIZER:2;
            } ep_flags;
#endif
            uint32_t flags;
        };

        // Skipping HRD data because it is not needed for our processing

        union
        {
#ifndef MFDBIGENDIAN
            struct
            {
                unsigned CODED_HEIGHT:12;
                unsigned CODED_WIDTH:12;
                unsigned pad2:8;
            } ep_size;
#else
            struct
            {
                unsigned pad2:8;
                unsigned CODED_WIDTH:12;
                unsigned CODED_HEIGHT:12;
            } ep_size;
#endif
            uint32_t size;
        };

        uint8_t  CODED_SIZE_FLAG;           /**  1 bit(s). */
        uint8_t  EXTENDED_DMV;              /**  1 bit(s). */
        uint8_t  RANGE_MAPY_FLAG;           /**  1 bit(s). */
        uint8_t  RANGE_MAPY;                /**  3 bit(s). */
        uint8_t  RANGE_MAPUV_FLAG;          /**  1 bit(s). */
        uint8_t  RANGE_MAPUV;               /**  3 bit(s). */
    } vc1_EntryPointHeader;

    /** This structure represents metadata for slice and picture layers. */
    typedef struct
    {
        /* Slice layer. */
        uint16_t SLICE_ADDR;                /**  9 bit(s). */

        /* Picture layer for simple or main profile. */
        uint8_t  RANGEREDFRM;               /**  1 bit(s). */
        uint8_t  PTYPE;                     /**  4 bit(s)? */
        int8_t   BFRACTION_NUM;             /**  ? bit(s). */
        int16_t  BFRACTION_DEN;             /**  ? bit(s). */
        uint8_t  PQINDEX;                   /**  5 bit(s). */
        uint8_t  HALFQP;                    /**  1 bit(s). */
        uint8_t  PQUANTIZER;                /**  1 bit(s). */
        uint8_t  MVRANGE;                   /**  3 bit(s)? */
        uint8_t  MVMODE;                    /**  4 bit(s)? */
        uint8_t  MVMODE2;                   /**  3 bit(s)? */
        uint8_t  LUMSCALE;                  /**  6 bit(s). */
        uint8_t  LUMSHIFT;                  /**  6 bit(s). */
        uint8_t  MVTAB;                     /**  2 bit(s). */
        uint8_t  CBPTAB;                    /**  2 bit(s). */
        uint8_t  TTMBF;                     /**  1 bit(s). */
        uint8_t  TTFRM;                     /**  2 bit(s). */
        uint8_t  TRANSACFRM;                /**  2 bit(s)? */
        uint8_t  TRANSACFRM2;               /**  2 bit(s)? */
        uint8_t  TRANSDCTAB;                /**  1 bit(s). */

        /* Picture layer for advanced profile. */
        uint8_t  FCM;                       /**  2 bit(s)? */
        uint8_t  FPTYPE;                    /**  3 bit(s). */
        uint8_t  TFCNTR;                    /**  8 bit(s) */
        uint8_t  RPTFRM;                    /**  2 bit(s) */
        uint8_t  TFF;                       /**  1 bit(s). */
        uint8_t  RFF;                    	/**  1 bit(s) */
        uint8_t  RNDCTRL;                   /**  1 bit(s). */
        uint8_t  UVSAMP;                    /**  1 bit(s). */
        uint8_t  POSTPROC;                  /**  2 bit(s). */
        uint8_t  CONDOVER;                  /**  2 bit(s)? */
        uint8_t  DMVRANGE;                  /**  ? bit(s)? */
        uint8_t  MV4SWITCH;                 /**  1 bit(s). */
        uint8_t  INTCOMP;                   /**  1 bit(s). */
        uint8_t  MBMODETAB;                 /**  2 bit(s). */
        uint8_t  MV2BPTAB;                  /**  2 bit(s). */
        uint8_t  MV4BPTAB;                  /**  2 bit(s). */
        uint8_t  NUMREF;                    /**  1 bit(s). */
        uint8_t  REFFIELD;                  /**  1 bit(s). */

        /* PAN SCAN */
        uint8_t  PS_PRESENT;                /**  1 bit(s). */
        uint8_t number_of_pan_scan_window;	/** 4 max. */
        viddec_vc1_pan_scan_window_t PAN_SCAN_WINDOW[VIDDEC_PANSCAN_MAX_OFFSETS];

        /* VOPDQUANT. */
        uint8_t  PQDIFF;                    /**  3 bit(s). */
        uint8_t  ABSPQ;                     /**  5 bit(s). */
        uint8_t  DQUANTFRM;                 /**  1 bit(s). */
        uint8_t  DQPROFILE;                 /**  2 bit(s). */
        uint8_t  DQSBEDGE;                  /**  2 bit(s). */
        uint8_t  DQBILEVEL;                 /**  1 bit(s). */

        /* Others. */
        uint8_t  PTypeField1;
        uint8_t  PTypeField2;
        uint32_t PQUANT;
        uint8_t  CurrField;
        uint8_t  BottomField;
        uint32_t UniformQuant;

#ifdef VBP
        uint8_t  raw_MVTYPEMB;
        uint8_t  raw_DIRECTMB;
        uint8_t  raw_SKIPMB;
        uint8_t  raw_ACPRED;
        uint8_t  raw_FIELDTX;
        uint8_t  raw_OVERFLAGS;
        uint8_t  raw_FORWARDMB;

        vc1_Bitplane MVTYPEMB;
        vc1_Bitplane DIRECTMB;
        vc1_Bitplane SKIPMB;
        vc1_Bitplane ACPRED;
        vc1_Bitplane FIELDTX;
        vc1_Bitplane OVERFLAGS;
        vc1_Bitplane FORWARDMB;
        uint32_t  ALTPQUANT;
        uint8_t		DQDBEDGE;
#endif

    } vc1_PictureLayerHeader;

    /*@}*/

#ifdef __cplusplus
}
#endif /* __cplusplus. */

#endif /* _VC1PARSE_COMMON_DEFS_H_. */
