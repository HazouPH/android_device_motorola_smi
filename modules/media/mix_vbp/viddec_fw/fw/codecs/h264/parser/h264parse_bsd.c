/* ///////////////////////////////////////////////////////////////////////
//
//               INTEL CORPORATION PROPRIETARY INFORMATION
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with Intel Corporation and may not be copied
//  or disclosed except in accordance with the terms of that agreement.
//        Copyright (c) 2001-2006 Intel Corporation. All Rights Reserved.
//
//  Description:    h264 bistream decoding
//
///////////////////////////////////////////////////////////////////////*/


#include "h264.h"
#include "h264parse.h"
#include "viddec_parser_ops.h"





/**
   get_codeNum     :Get codenum based on sec 9.1 of H264 spec.
   @param      cxt : Buffer adress & size are part inputs, the cxt is updated
                     with codeNum & sign on sucess.
                     Assumption: codeNum is a max of 32 bits

   @retval       1 : Sucessfuly found a code num, cxt is updated with codeNum, sign, and size of code.
   @retval       0 : Couldn't find a code in the current buffer.
   be freed.
*/

uint32_t h264_get_codeNum(void *parent, h264_Info* pInfo)
{
    int32_t    leadingZeroBits= 0;
    uint32_t    temp = 0, match = 0, noOfBits = 0, count = 0;
    uint32_t   codeNum =0;
    uint32_t   bits_offset =0, byte_offset =0;
    uint8_t    is_emul =0;
    uint8_t    is_first_byte = 1;
    uint32_t   length =0;
    uint32_t   bits_need_add_in_first_byte =0;
    int32_t    bits_operation_result=0;

    //remove warning
    pInfo = pInfo;

    ////// Step 1: parse through zero bits until we find a bit with value 1.
    viddec_pm_get_au_pos(parent, &bits_offset, &byte_offset, &is_emul);


    while (!match)
    {
        if ((bits_offset != 0) && ( is_first_byte == 1))
        {
            //we handle byte at a time, if we have offset then for first
            //   byte handle only 8 - offset bits
            noOfBits = (uint8_t)(8 - bits_offset);
            bits_operation_result = viddec_pm_peek_bits(parent, &temp, noOfBits);


            temp = (temp << bits_offset);
            if (temp!=0)
            {
                bits_need_add_in_first_byte = bits_offset;
            }
            is_first_byte =0;
        }
        else
        {
            noOfBits = 8;/* always 8 bits as we read a byte at a time */
            bits_operation_result = viddec_pm_peek_bits(parent, &temp, 8);

        }

        if (-1==bits_operation_result)
        {
            return MAX_INT32_VALUE;
        }

        if (temp != 0)
        {
            // if byte!=0 we have at least one bit with value 1.
            count=1;
            while (((temp & 0x80) != 0x80) && (count <= noOfBits))
            {
                count++;
                temp = temp <<1;
            }
            //At this point we get the bit position of 1 in current byte(count).

            match = 1;
            leadingZeroBits += count;
        }
        else
        {
            // we don't have a 1 in current byte
            leadingZeroBits += noOfBits;
        }

        if (!match)
        {
            //actually move the bitoff by viddec_pm_get_bits
            viddec_pm_get_bits(parent, &temp, noOfBits);
        }
        else
        {
            //actually move the bitoff by viddec_pm_get_bits
            viddec_pm_get_bits(parent, &temp, count);
        }

    }
    ////// step 2: Now read the next (leadingZeroBits-1) bits to get the encoded value.


    if (match)
    {

        viddec_pm_get_au_pos(parent, &bits_offset, &byte_offset, &is_emul);
        /* bit position in current byte */
        //count = (uint8_t)((leadingZeroBits + bits_offset)& 0x7);
        count = ((count + bits_need_add_in_first_byte)& 0x7);

        leadingZeroBits --;
        length =  leadingZeroBits;
        codeNum = 0;
        noOfBits = 8 - count;


        while (leadingZeroBits > 0)
        {
            if (noOfBits < (uint32_t)leadingZeroBits)
            {
                viddec_pm_get_bits(parent, &temp, noOfBits);


                codeNum = (codeNum << noOfBits) | temp;
                leadingZeroBits -= noOfBits;
            }
            else
            {
                viddec_pm_get_bits(parent, &temp, leadingZeroBits);

                codeNum = (codeNum << leadingZeroBits) | temp;
                leadingZeroBits = 0;
            }


            noOfBits = 8;
        }
        // update codeNum = 2 ** (leadingZeroBits) -1 + read_bits(leadingZeroBits).
        codeNum = codeNum + (1 << length) -1;

    }

    viddec_pm_get_au_pos(parent, &bits_offset, &byte_offset, &is_emul);
    if (bits_offset!=0)
    {
        viddec_pm_peek_bits(parent, &temp, 8-bits_offset);
    }

    return codeNum;
}


/*---------------------------------------*/
/*---------------------------------------*/
int32_t h264_GetVLCElement(void *parent, h264_Info* pInfo, uint8_t bIsSigned)
{
    int32_t sval = 0;
    signed char sign;

    sval = h264_get_codeNum(parent , pInfo);

    if (bIsSigned) //get signed integer golomb code else the value is unsigned
    {
        sign = (sval & 0x1)?1:-1;
        sval = (sval +1) >> 1;
        sval = sval * sign;
    }

    return sval;
} // Ipp32s H264Bitstream::GetVLCElement(bool bIsSigned)

///
/// Check whether more RBSP data left in current NAL
///
uint8_t h264_More_RBSP_Data(void *parent, h264_Info * pInfo)
{
    uint8_t cnt = 0;

    uint8_t  is_emul =0;
    uint8_t 	cur_byte = 0;
    int32_t  shift_bits =0;
    uint32_t ctr_bit = 0;
    uint32_t bits_offset =0, byte_offset =0;

    //remove warning
    pInfo = pInfo;

    if (!viddec_pm_is_nomoredata(parent))
        return 1;

    viddec_pm_get_au_pos(parent, &bits_offset, &byte_offset, &is_emul);

    shift_bits = 7-bits_offset;

    // read one byte
    viddec_pm_get_cur_byte(parent, &cur_byte);

    ctr_bit = ((cur_byte)>> (shift_bits--)) & 0x01;

    // a stop bit has to be one
    if (ctr_bit==0)
        return 1;

    while (shift_bits>=0 && !cnt)
    {
        cnt |= (((cur_byte)>> (shift_bits--)) & 0x01);   // set up control bit
    }

    return (cnt);
}



///////////// EOF/////////////////////

