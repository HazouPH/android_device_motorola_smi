/* ///////////////////////////////////////////////////////////////////////
//
//               INTEL CORPORATION PROPRIETARY INFORMATION
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with Intel Corporation and may not be copied
//  or disclosed except in accordance with the terms of that agreement.
//        Copyright (c) 2008 Intel Corporation. All Rights Reserved.
//
//  Description: Parses VC-1 bitstreams.
//
*/

#include "vc1parse.h"

#ifdef VBP
#include "viddec_pm.h"
#endif

/*----------------------------------------------------------------------------*/


/* put one bit into a buffer
 * used for bitplane decoding, each bit correspond to a MB
 * HW requires row to start at DW (32 bits) boundary
 * input: value - bit value
 *        mbx - image width in MB
 *        mby - image height in MB
 *        x   - x location (column) of MB in MB unit
 *        y   - y location (row) of MB in MB unit
 * output: outp - buffer to fill
 */
//#define put_bit(value,x,y,mbx,mby,invert,outp)
static inline void put_bit( uint32_t value, int x, int y, int mbx, int mby, uint8_t invert, uint32_t* outp)
{
    int bit;
    uint32_t *out;

    bit = mby;

    value ^= invert;
    if (!value) return; /* assume buffer is initialized with zeros */

    out = outp;
    /* go to corresponding row location in DW unit */
    out += (( mbx + 31 ) >> 5) * y;
    out +=  x >> 5; /* go to corresponding column location in DW unit */
    bit = x & 0x1f; /* compute remaining bits */
    *out |= 1 << bit; /* put bit */
}

/* if b is the bit at location (x,y)
 * b = b^invert
 * used for bitplane decoding, each bit correspond to a MB
 * HW requires row to start at DW (32 bits) boundary
 * input: value - bit value
 *        x   - x location (column) of MB in MB unit
 *        y   - y location (row) of MB in MB unit
 *        mbx - image width in MB
 * output: outp - buffer to fill
 * returns bit value
 */
static inline int xor_bit(  int x, int y, int mbx, uint32_t invert, uint32_t* outp)
{
    int bit;
    uint32_t *out;
    uint8_t value;
    //if (invert == 0) return; /* do nothing if XOR with 0 */

    out = outp;
    out += (( mbx + 31 ) >> 5) * y; /* go to corresponding row location in DW unit */
    out +=  x >> 5; /* go to corresponding row location in DW unit */
    bit = x & 0x1f; /* compute remaining bits */

    if (invert == 1)
        *out ^= (1 << bit); /* put XOR bit */
    value = (*out & (1 << bit)) >> bit; /* return bit value */

    return(value);

}

/* get bit at location (x,y)
 * used for bitplane decoding, each bit correspond to a MB
 * HW requires row to start at DW (32 bits) boundary
 * input: value - bit value
 *        x   - x location (column) of MB in MB unit
 *        y   - y location (row) of MB in MB unit
 *        mbx - image width in MB
 *        outp - bit buffer in dwords
 * returns bit value
 */
static inline int get_bit(  int x, int y, int mbx, uint32_t* outp)
{
    int bit;
    uint32_t *out;
    uint8_t value;

    out = outp;
    out += (( mbx + 31 ) >> 5) * y; /* go to corresponding row location in DW unit */
    out +=  x >> 5; /* go to corresponding row location in DW unit */
    bit = x & 0x1f; /* compute remaining bits */
    value = (*out & (1 << bit)) >> bit; /* return bit value */

    return(value);

}

static void vc1_InverseDiff(vc1_Bitplane *pBitplane, int32_t widthMB, int32_t heightMB)
{
    int32_t i, j, previousBit=0, temp;

    for (i = 0; i < heightMB; i++)
    {
        for (j = 0; j < widthMB; j++)
        {
            if ((i == 0 && j == 0))
            {
                previousBit=xor_bit(j, i, widthMB, pBitplane->invert,
                                    pBitplane->databits);
            }
            else if (j == 0) /* XOR with TOP */
            {
                previousBit = get_bit(0, i-1, widthMB, pBitplane->databits);
                temp=xor_bit(j, i, widthMB, previousBit,
                             pBitplane->databits);
                previousBit = temp;
            }
            //TODO isSameAsTop can be optimized
            else if (((i > 0) && (previousBit !=
                                  get_bit(j, i-1, widthMB, pBitplane->databits))))
            {
                temp=xor_bit(j, i, widthMB, pBitplane->invert,
                             pBitplane->databits);
                previousBit = temp;
            }
            else
            {
                temp=xor_bit(j, i, widthMB, previousBit,
                             pBitplane->databits);
                previousBit = temp;
            }
        }
    }
}


/*----------------------------------------------------------------------------*/
/* implement normal 2 mode bitplane decoding, SMPTE 412M 8.7.3.2
 * width, height are in MB unit.
 */
static void vc1_Norm2ModeDecode(void* ctxt, vc1_Bitplane *pBitplane,
                                int32_t width, int32_t height)
{
    int32_t i;
    int32_t tmp_databits = 0;

    int32_t row[2], col[2];
    int8_t tmp=0;

    /* disable pBitplane->invert in the Norm2 decode stage of
       VC1_BITPLANE_DIFF2_MODE */
    if (pBitplane->imode == VC1_BITPLANE_DIFF2_MODE)
    {
        tmp = pBitplane->invert;
        pBitplane->invert=0;
    }

    // By default, initialize the values for the even case
    col[0] = 0;   /* i%width; */
    row[0] = 0;   /* i/width; */
    col[1] = 1;   /* (i+1)%width; */
    row[1] = 0;   /* (i+1)/width; */

    // If width*height is odd, the first bit is the value of the bitplane
    // for the first macroblock
    if ((width*height) & 1) /* first bit if size is odd */
    {
        VC1_GET_BITS(1, tmp_databits);
        put_bit(tmp_databits, 0, 0, width, height, pBitplane->invert,
                pBitplane->databits);

        // Modify initialization for odd sizes
        col[0] = 1;   /* i%width; */
        col[1] = 2;   /* (i+1)%width; */

        // Consider special case where width is 1
        if (width == 1)
        {
            col[0] = 0;   /* i%width; */
            row[0] = 1;   /* i/width; */
            col[1] = 0;   /* (i+1)%width; */
            row[1] = 2;   /* (i+1)/width; */
        }
    }

    /* decode every pair of bits in natural scan order */
    for (i = (width*height) & 1; i < (width*height/2)*2; i += 2)
    {
        int32_t tmp = 0;

        //col[0]=i%width;
        //row[0]=i/width;
        //col[1]=(i+1)%width;
        //row[1]=(i+1)/width;

        VC1_GET_BITS(1, tmp);
        if (tmp == 0)
        {
            put_bit(0, col[0],row[0], width, height, pBitplane->invert,
                    pBitplane->databits);
            put_bit(0, col[1],row[1], width, height, pBitplane->invert,
                    pBitplane->databits);
        }
        else
        {
            VC1_GET_BITS(1, tmp);
            if (tmp == 1)
            {
                put_bit(1, col[0],row[0], width, height, pBitplane->invert,
                        pBitplane->databits);
                put_bit(1, col[1],row[1], width, height, pBitplane->invert,
                        pBitplane->databits);
            }
            else
            {
                VC1_GET_BITS(1, tmp);
                if (tmp == 0)
                {
                    put_bit(1, col[0],row[0], width, height, pBitplane->invert,
                            pBitplane->databits);
                    put_bit(0, col[1],row[1], width, height, pBitplane->invert,
                            pBitplane->databits);
                }
                else
                {
                    put_bit(0, col[0],row[0], width, height, pBitplane->invert,
                            pBitplane->databits);
                    put_bit(1, col[1],row[1], width, height, pBitplane->invert,
                            pBitplane->databits);
                }
            }
        }

        // Consider special case where width is 1
        if (width == 1)
        {
            row[0] += 2;
            row[1] += 2;
        }
        else
        {
            col[0] += 2;   /* i%width; */
            if ( col[0] >= width )
            {
                // For odd sizes, col[0] can alternatively start at 0 and 1
                col[0] -= width;
                row[0]++;
            }

            col[1] += 2;   /* (i+1)%width; */
            if ( col[1] >= width )
            {
                // For odd sizes, col[1] can alternatively start at 0 and 1
                col[1] -= width;
                row[1]++;
            }
        }
    }

    /* restore value */
    pBitplane->invert=tmp;
}

/*----------------------------------------------------------------------------*/
/* compute Normal-6 mode bitplane decoding
 * algorithm is described in SMPTE 421M 8.7.3.4
 * width, height are in MB unit.
 */
static void vc1_Norm6ModeDecode(void* ctxt, vc1_Bitplane *pBitplane,
                                int32_t width, int32_t height)
{
    vc1_Status status;
    int32_t i, j, k;
    int32_t ResidualX = 0;
    int32_t ResidualY = 0;
    uint8_t _2x3tiled = (((width%3)!=0)&&((height%3)==0));

    int32_t row, col;
    int8_t tmp=0;

    /* disable pBitplane->invert in the Norm2 decode stage of
       VC1_BITPLANE_DIFF2_MODE */
    if (pBitplane->imode == VC1_BITPLANE_DIFF6_MODE)
    {
        tmp = pBitplane->invert;
        pBitplane->invert=0;
    }

    if (_2x3tiled)
    {
        int32_t sizeW = width/2;
        int32_t sizeH = height/3;

        for (i = 0; i < sizeH; i++)
        {
            row = 3*i; /* compute row location for tile */

            for (j = 0; j < sizeW; j++)
            {
                col = 2*j + (width & 1); /* compute column location for tile */

                /* get k=sum(bi2^i) were i is the ith bit of the tile */
                status = vc1_DecodeHuffmanOne(ctxt, &k, VC1_BITPLANE_K_TBL);
                VC1_ASSERT(status == VC1_STATUS_OK);

                /* put bits in tile */
                put_bit(k&1, col, row, width, height, pBitplane->invert,
                        pBitplane->databits);
                put_bit(((k&2)>>1), col+1, row, width, height,
                        pBitplane->invert,pBitplane->databits);

                put_bit(((k&4)>>2), col, row+1, width, height,
                        pBitplane->invert,pBitplane->databits);
                put_bit(((k&8)>>3), col+1, row+1, width, height,
                        pBitplane->invert,pBitplane->databits);

                put_bit(((k&16)>>4), col, row+2, width, height,
                        pBitplane->invert,pBitplane->databits);
                put_bit(((k&32)>>5), col+1, row+2, width,
                        height,pBitplane->invert, pBitplane->databits);
            }
        }
        ResidualX = width & 1;
        ResidualY = 0;
    }
    else /* 3x2 tile */
    {
        int32_t sizeW = width/3;
        int32_t sizeH = height/2;

        for (i = 0; i < sizeH; i++)
        {
            row = 2*i + (height&1) ; /* compute row location for tile */

            for (j = 0; j < sizeW; j++)
            {
                col = 3*j + (width%3); /* compute column location for tile */

                /* get k=sum(bi2^i) were i is the ith bit of the tile */
                status = vc1_DecodeHuffmanOne(ctxt, &k, VC1_BITPLANE_K_TBL);
                VC1_ASSERT(status == VC1_STATUS_OK);

                put_bit(k&1, col, row, width, height,pBitplane->invert,
                        pBitplane->databits);
                put_bit((k&2)>>1, col+1, row, width, height, pBitplane->invert,
                        pBitplane->databits);
                put_bit((k&4)>>2, col+2, row, width, height, pBitplane->invert,
                        pBitplane->databits);

                put_bit((k&8)>>3, col, row+1, width, height,pBitplane->invert,
                        pBitplane->databits);
                put_bit((k&16)>>4, col+1, row+1, width,
                        height,pBitplane->invert, pBitplane->databits);
                put_bit((k&32)>>5, col+2, row+1, width,
                        height,pBitplane->invert, pBitplane->databits);
            }
        }
        ResidualX = width % 3;
        ResidualY = height & 1;
    }

    for (i = 0; i < ResidualX; i++)
    {
        int32_t ColSkip;
        VC1_GET_BITS(1, ColSkip);

        //if (1 == ColSkip)
        {
            for (j = 0; j < height; j++)
            {
                int32_t Value = 0;
                if (1 == ColSkip) VC1_GET_BITS(1, Value);

                put_bit(Value, i, j, width, height,pBitplane->invert,pBitplane->databits);
            }
        }
    }

    for (j = 0; j < ResidualY; j++)
    {
        int32_t RowSkip;
        VC1_GET_BITS(1, RowSkip);
        //if (1 == RowSkip)
        {
            for (i = ResidualX; i < width; i++)
            {
                int32_t Value = 0;
                if (1 == RowSkip) VC1_GET_BITS(1, Value);

                put_bit(Value, i, j, width, height,pBitplane->invert,pBitplane->databits);
            }
        }
    }

    /* restore value */
    pBitplane->invert=tmp;

}

/*----------------------------------------------------------------------------*/
/* initialize bitplane to array of zeros
 * each row begins with a dword
 * input:
 *    width: widh in MB unit
 *    height: height in MB unit
 * returns even bitplane size in dwords
 */
int initBitplane(vc1_Bitplane *pBitplane,uint32_t width, uint32_t height)
{
    int i;
    int numDword = 0;

    numDword = ((width + 31)>>5) *  height;
    numDword += numDword & 1; /* add 1 in case numDword is odd */

    for (i=0; i<numDword; i++) pBitplane->databits[i] = 0;
    return(numDword);
}

/*----------------------------------------------------------------------------*/
/* modified IPP code for bitplane decoding
 *    width: width in MB unit
 *    height: height in MB unit
 */
vc1_Status vc1_DecodeBitplane(void* ctxt, vc1_Info *pInfo,
                              uint32_t width, uint32_t height, vc1_bpp_type_t bpnum)
{
    uint32_t i, j;
    uint32_t tempValue;
    vc1_Status status = VC1_STATUS_OK;
    uint32_t biplaneSz; /* bitplane sz in dwords */
    vc1_Bitplane bp;
    vc1_Bitplane *bpp = &bp;

    // By default, set imode to raw
    pInfo->metadata.bp_raw[bpnum - VIDDEC_WORKLOAD_VC1_BITPLANE0] = true;

    // bitplane data would be temporarily stored in the vc1 context
    bpp->databits = pInfo->bitplane;

    /* init bitplane to zero, function retunr bitplane buffer size in dword */
    biplaneSz = initBitplane(bpp, width, height);

    VC1_GET_BITS(1, tempValue);
    bpp->invert = (uint8_t) tempValue;

    bpp->imode = -1;

    if ((status = vc1_DecodeHuffmanOne(ctxt, &bpp->imode,VC1_BITPLANE_IMODE_TBL)) != VC1_STATUS_OK)
    {
        return status;
    }

    // If the imode is VC1_BITPLANE_RAW_MODE: bitplane information is in the MB layer
    // there is no need to parse for bitplane information in the picture layer
    // Only bits need to be appropriately set in the block control register
    // In all other modes, bitplane information follows and needs to be parsed and sent to the decoder

    if (bpp->imode == VC1_BITPLANE_NORM2_MODE)
    {
        vc1_Norm2ModeDecode(ctxt, bpp, width, height);
    }
    else if (bpp->imode == VC1_BITPLANE_DIFF2_MODE)
    {
        vc1_Norm2ModeDecode(ctxt, bpp, width, height);
        vc1_InverseDiff(bpp, width, height);
    }
    else if (bpp->imode == VC1_BITPLANE_NORM6_MODE)
    {
        vc1_Norm6ModeDecode(ctxt, bpp, width, height);

    }
    else if (bpp->imode == VC1_BITPLANE_DIFF6_MODE)
    {
        vc1_Norm6ModeDecode(ctxt, bpp, width, height);
        vc1_InverseDiff(bpp, width, height);
    }
    else if (bpp->imode == VC1_BITPLANE_ROWSKIP_MODE)
    {

        for (i = 0; i < height; i++)
        {
            VC1_GET_BITS(1, tempValue);
            /* if tempValue==0,  put row of zeros Dwords*/
            if (tempValue == 1)
            {
                for (j = 0; j < width; j++)
                {
                    VC1_GET_BITS(1, tempValue);
                    put_bit( tempValue, j, i, width, height, bpp->invert,bpp->databits);
                }
            }
            else if (bpp->invert) { //TO TEST
                for (j = 0; j < width; j++) {
                    put_bit( 0, j, i, width, height, bpp->invert, bpp->databits);
                }
            }
        }

    }
    else if (bpp->imode == VC1_BITPLANE_COLSKIP_MODE)
    {
        for (i = 0; i < width; i++)
        {
            VC1_GET_BITS(1, tempValue);
            /* if tempValue==0, and invert == 0, fill column with zeros */
            if (tempValue == 1)
            {
                for (j = 0; j < height; j++)
                {
                    VC1_GET_BITS(1, tempValue);
                    put_bit( tempValue, i, j, width, height, bpp->invert, bpp->databits);
                }
            }
            else if (bpp->invert) { // fill column with ones
                for (j = 0; j < height; j++) {
                    put_bit( 0, i, j, width, height, bpp->invert, bpp->databits);
                }
            }//end for else
        }
    }

    if (bpp->imode != VC1_BITPLANE_RAW_MODE)
    {
        uint32_t* pl;
        int sizeinbytes,nitems,i;
        viddec_workload_item_t    wi;
        uint32_t *bit_dw;

        pInfo->metadata.bp_raw[bpnum - VIDDEC_WORKLOAD_VC1_BITPLANE0] = false;

        sizeinbytes = ((( width + 31 ) / 32)) * (height) * 4;

        pl = bpp->databits;
        bit_dw = bpp->databits;

        // How many payloads must be generated
        nitems = (sizeinbytes + (sizeof(wi.data.data_payload) - 1)) /
                 sizeof(wi.data.data_payload);

        // Dump DMEM to an array of workitems
        for ( i = 0; i < nitems; i++ )
        {
            wi.vwi_type           =  bpnum;
            wi.data.data_offset   = (char *)pl - (char *)bit_dw; // offset within struct

            wi.data.data_payload[0] = pl[0];
            wi.data.data_payload[1] = pl[1];
            pl += 2;

            viddec_pm_append_workitem( ctxt, &wi, false);
        }
    }

#ifdef VBP
    {
        viddec_pm_cxt_t     *cxt    = (viddec_pm_cxt_t *)ctxt;
        vc1_viddec_parser_t *parser = (vc1_viddec_parser_t *)(cxt->codec_data);

        if (biplaneSz > 4096)
        {
            /* bigger than we got, so let's bail with a non meaningful error. */
            return VC1_STATUS_ERROR;
        }

        /* At this point bp contains the information we need for the bit-plane */
        /* bpnum is the enumeration that tells us which bitplane this is for.  */
        /* pInfo->picLayerHeader.ACPRED is one of the bitplanes I need to fill.*/
        switch (bpnum)
        {
        case VIDDEC_WORKLOAD_VC1_BITPLANE0:
            if (pInfo->picLayerHeader.PTYPE == VC1_B_FRAME)
            {
                if (bp.imode != VC1_BITPLANE_RAW_MODE)
                {
                    pInfo->picLayerHeader.FORWARDMB.invert = bp.invert;
                    pInfo->picLayerHeader.FORWARDMB.imode = bp.imode;
                    for (i = 0; i < biplaneSz; i++)
                    {
                        parser->bp_forwardmb[i] = bp.databits[i];
                    }
                    pInfo->picLayerHeader.FORWARDMB.databits = parser->bp_forwardmb;
                }
                else
                {
                    pInfo->picLayerHeader.raw_FORWARDMB = 1;
                }
            }
            if ( (pInfo->picLayerHeader.PTYPE == VC1_I_FRAME)
                    || (pInfo->picLayerHeader.PTYPE == VC1_BI_FRAME) )
            {
                if (bp.imode != VC1_BITPLANE_RAW_MODE)
                {
                    pInfo->picLayerHeader.ACPRED.invert = bp.invert;
                    pInfo->picLayerHeader.ACPRED.imode = bp.imode;
                    for (i = 0; i < biplaneSz; i++)
                    {
                        parser->bp_acpred[i] = bp.databits[i];
                    }
                    pInfo->picLayerHeader.ACPRED.databits = parser->bp_acpred;
                }
                else
                {
                    pInfo->picLayerHeader.raw_ACPRED = 1;
                }
            }
            if (pInfo->picLayerHeader.PTYPE == VC1_P_FRAME)
            {
                if (bp.imode != VC1_BITPLANE_RAW_MODE)
                {
                    pInfo->picLayerHeader.MVTYPEMB.invert = bp.invert;
                    pInfo->picLayerHeader.MVTYPEMB.imode = bp.imode;
                    for (i = 0; i < biplaneSz; i++)
                    {
                        parser->bp_mvtypemb[i] = bp.databits[i];
                    }
                    pInfo->picLayerHeader.MVTYPEMB.databits = parser->bp_mvtypemb;
                }
                else
                {
                    pInfo->picLayerHeader.raw_MVTYPEMB = 1;
                }
            }
            break;
        case VIDDEC_WORKLOAD_VC1_BITPLANE1:
            if ( (pInfo->picLayerHeader.PTYPE == VC1_I_FRAME)
                    || (pInfo->picLayerHeader.PTYPE == VC1_BI_FRAME) )
            {
                if (bp.imode != VC1_BITPLANE_RAW_MODE)
                {
                    pInfo->picLayerHeader.OVERFLAGS.invert = bp.invert;
                    pInfo->picLayerHeader.OVERFLAGS.imode = bp.imode;
                    for (i = 0; i < biplaneSz; i++)
                    {
                        parser->bp_overflags[i] = bp.databits[i];
                    }
                    pInfo->picLayerHeader.OVERFLAGS.databits = parser->bp_overflags;
                }
                else
                {
                    pInfo->picLayerHeader.raw_OVERFLAGS = 1;
                }
            }
            if ( (pInfo->picLayerHeader.PTYPE == VC1_P_FRAME)
                    || (pInfo->picLayerHeader.PTYPE == VC1_B_FRAME) )
            {
                if (bp.imode != VC1_BITPLANE_RAW_MODE)
                {
                    pInfo->picLayerHeader.SKIPMB.invert = bp.invert;
                    pInfo->picLayerHeader.SKIPMB.imode = bp.imode;
                    for (i = 0; i < biplaneSz; i++)
                    {
                        parser->bp_skipmb[i] = bp.databits[i];
                    }
                    pInfo->picLayerHeader.SKIPMB.databits = parser->bp_skipmb;
                }
                else
                {
                    pInfo->picLayerHeader.raw_SKIPMB = 1;
                }
            }
            break;
        case VIDDEC_WORKLOAD_VC1_BITPLANE2:
            if ( (pInfo->picLayerHeader.PTYPE == VC1_P_FRAME)
                    || (pInfo->picLayerHeader.PTYPE == VC1_B_FRAME) )
            {
                if (bp.imode != VC1_BITPLANE_RAW_MODE)
                {
                    pInfo->picLayerHeader.DIRECTMB.invert = bp.invert;
                    pInfo->picLayerHeader.DIRECTMB.imode = bp.imode;
                    for (i = 0; i < biplaneSz; i++)
                    {
                        parser->bp_directmb[i] = bp.databits[i];
                    }
                    pInfo->picLayerHeader.DIRECTMB.databits = parser->bp_directmb;
                }
                else
                {
                    pInfo->picLayerHeader.raw_DIRECTMB = 1;
                }
            }
            if ( (pInfo->picLayerHeader.PTYPE == VC1_I_FRAME)
                    || (pInfo->picLayerHeader.PTYPE == VC1_BI_FRAME) )
            {
                if (bp.imode != VC1_BITPLANE_RAW_MODE)
                {
                    pInfo->picLayerHeader.FIELDTX.invert = bp.invert;
                    pInfo->picLayerHeader.FIELDTX.imode = bp.imode;
                    for (i = 0; i < biplaneSz; i++)
                    {
                        parser->bp_fieldtx[i] = bp.databits[i];
                    }
                    pInfo->picLayerHeader.FIELDTX.databits = parser->bp_fieldtx;
                }
                else
                {
                    pInfo->picLayerHeader.raw_FIELDTX = 1;
                }
            }
            break;
        }
    }
#endif

    return status;
}
