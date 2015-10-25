/*!
 ***********************************************************************
 *  \file: h264_dpb_ctl.c
 *
 ***********************************************************************
 */

#include "viddec_fw_debug.h"
#include "viddec_parser_ops.h"

#include "viddec_fw_workload.h"
#include "viddec_pm.h"
#include "viddec_h264_parse.h"


//#include <limits.h>
#include "h264parse.h"
#include "h264parse_dpb.h"
//#include "h264_debug.h"

#ifndef NULL
#define NULL 0
#endif
//#ifndef USER_MODE
//#define NULL 0
//#endif

///////////////////////// DPB init //////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
// Init DPB
// Description: init dpb, which should be called while open
//
//////////////////////////////////////////////////////////////////////////////

void h264_init_dpb(h264_DecodedPictureBuffer * p_dpb)
{
    int32_t i;

    //// Init DPB to zero
    //h264_memset(p_dpb, 0x0, sizeof(h264_DecodedPictureBuffer) );


    for (i=0; i<NUM_DPB_FRAME_STORES; i++)
    {
        p_dpb->fs[i].fs_idc = MPD_DPB_FS_NULL_IDC;
        p_dpb->fs_dpb_idc[i] = MPD_DPB_FS_NULL_IDC;
    }
    p_dpb->used_size = 0;
    p_dpb->fs_dec_idc = MPD_DPB_FS_NULL_IDC;
    p_dpb->fs_non_exist_idc = MPD_DPB_FS_NULL_IDC;

    return;
}


///////////////////////// Reference list management //////////////////////////

/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
//////////////////////////////////////////////////////////////////////////////
// h264_dpb_add_ref_list ()
//
// Adds an idc to the long term reference list
//////////////////////////////////////////////////////////////////////////////
void h264_dpb_add_ref_list(h264_DecodedPictureBuffer * p_dpb, int32_t ref_idc)
{
    p_dpb->fs_ref_idc[p_dpb->ref_frames_in_buffer] = ref_idc;
    p_dpb->ref_frames_in_buffer++;
}
/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
//////////////////////////////////////////////////////////////////////////////
// h264_dpb_add_ltref_list ()
//
// Adds an idc to the long term reference list
//////////////////////////////////////////////////////////////////////////////
void h264_dpb_add_ltref_list(h264_DecodedPictureBuffer * p_dpb, int32_t ref_idc)
{
    p_dpb->fs_ltref_idc[p_dpb->ltref_frames_in_buffer] = ref_idc;
    p_dpb->ltref_frames_in_buffer++;
}
/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
//////////////////////////////////////////////////////////////////////////////
// h264_dpb_update_all_ref_lists (h264_DecodedPictureBuffer * p_dpb,int32_t NonExisting)
//
// Decide whether the current picture needs to be added to the reference lists
// active_fs should be set-up prior to calling this function
//
// Check if we need to search the lists here
// or can we go straight to adding to ref lists..
//////////////////////////////////////////////////////////////////////////////

void h264_dpb_insert_ref_lists(h264_DecodedPictureBuffer * p_dpb, int32_t NonExisting)
{
    if (NonExisting)
        h264_dpb_set_active_fs(p_dpb,p_dpb->fs_non_exist_idc);
    else
        h264_dpb_set_active_fs(p_dpb, p_dpb->fs_dec_idc);

    //if(active_fs->is_reference)
    if (p_dpb->active_fs->frame.used_for_reference)
    {
        if (viddec_h264_get_is_long_term(p_dpb->active_fs))
        {
            if (viddec_h264_get_dec_structure(p_dpb->active_fs) == FRAME)
                h264_dpb_add_ltref_list(p_dpb, p_dpb->active_fs->fs_idc);
            else
            {
                uint32_t found_in_list = 0, i = 0;
                for (i = 0; (i < p_dpb->ltref_frames_in_buffer) && (found_in_list == 0); i++) {
                    if (p_dpb->fs_ltref_idc[i] == p_dpb->active_fs->fs_idc) found_in_list = 1;
                }

                if (found_in_list == 0) h264_dpb_add_ltref_list(p_dpb, p_dpb->active_fs->fs_idc);
            }
        }
        else
        {
            if (viddec_h264_get_dec_structure(p_dpb->active_fs) == FRAME) {
                h264_dpb_add_ref_list(p_dpb, p_dpb->active_fs->fs_idc);
            } else
            {
                uint32_t found_in_list = 0, i = 0;

                for (i = 0; (i < p_dpb->ref_frames_in_buffer) && (found_in_list == 0); i++)
                {
                    if (p_dpb->fs_ref_idc[i] == p_dpb->active_fs->fs_idc) found_in_list = 1;
                }

                if (found_in_list == 0) h264_dpb_add_ref_list(p_dpb, p_dpb->active_fs->fs_idc);
            }
        }
    }

    return;

}

/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
//////////////////////////////////////////////////////////////////////////////
// Set active fs
//////////////////////////////////////////////////////////////////////////////

void h264_dpb_set_active_fs(h264_DecodedPictureBuffer * p_dpb, int32_t index)
{
    p_dpb->active_fs = &p_dpb->fs[index];
}
/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
//////////////////////////////////////////////////////////////////////////////
// Sort reference list
//////////////////////////////////////////////////////////////////////////////

void h264_list_sort(uint8_t *list, int32_t *sort_indices, int32_t size, int32_t desc)
{
    int32_t j, k, temp, idc;

    // Dodgy looking for embedded code here...
    if (size > 1)
    {
        for (j = 0; j < size-1; j = j + 1) {
            for (k = j + 1; k < size; k = k + 1) {
                if ((desc & (sort_indices[j] < sort_indices[k]))|
                        (~desc & (sort_indices[j] > sort_indices[k])) )
                {
                    temp = sort_indices[k];
                    sort_indices[k] = sort_indices[j];
                    sort_indices[j] = temp;
                    idc = list[k];
                    list[k] = list[j];
                    list[j] = idc;
                }
            }
        }
    }
}

/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
//////////////////////////////////////////////////////////////////////////////
// h264_dpb_pic_is_bottom_field_ref ()
//
// Used to sort a list based on a corresponding sort indices
//////////////////////////////////////////////////////////////////////////////

int32_t h264_dpb_pic_is_bottom_field_ref(h264_DecodedPictureBuffer * p_dpb, int32_t long_term)
{
    int32_t temp;
    if (long_term) temp = ((p_dpb->active_fs->bottom_field.used_for_reference) && (p_dpb->active_fs->bottom_field.is_long_term))  ? 1 : 0;
    else          temp = ((p_dpb->active_fs->bottom_field.used_for_reference) && !(p_dpb->active_fs->bottom_field.is_long_term)) ? 1 : 0;

    return temp;
}
/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
//////////////////////////////////////////////////////////////////////////////
// h264_dpb_pic_is_top_field_ref ()
//
// Used to sort a list based on a corresponding sort indices
//////////////////////////////////////////////////////////////////////////////

int32_t h264_dpb_pic_is_top_field_ref(h264_DecodedPictureBuffer * p_dpb, int32_t long_term)
{
    int32_t temp;
    if (long_term)
        temp = ((p_dpb->active_fs->top_field.used_for_reference) && (p_dpb->active_fs->top_field.is_long_term))  ? 1 : 0;
    else
        temp = ((p_dpb->active_fs->top_field.used_for_reference) && !(p_dpb->active_fs->top_field.is_long_term)) ? 1 : 0;

    return temp;
}


/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
//////////////////////////////////////////////////////////////////////////////
// h264_dpb_gen_pic_list_from_frame_list ()
//
// Used to sort a list based on a corresponding sort indices
//////////////////////////////////////////////////////////////////////////////

int32_t h264_dpb_gen_pic_list_from_frame_list(h264_DecodedPictureBuffer *p_dpb, uint8_t *pic_list, uint8_t *frame_list, int32_t currPicStructure, int32_t list_size, int32_t long_term)
{
    int32_t top_idx, bot_idx, got_pic, list_idx;
    int32_t lterm;

    list_idx = 0;
    lterm = (long_term)? 1:0;

    if (list_size) {


        top_idx = 0;
        bot_idx = 0;

        if (currPicStructure == TOP_FIELD) {
            while ((top_idx < list_size)||(bot_idx < list_size))
            {
                /////////////////////////////////////////// ref Top Field
                got_pic = 0;
                while ((top_idx < list_size) & ~got_pic)
                {
                    h264_dpb_set_active_fs(p_dpb, frame_list[top_idx]);
                    if ((viddec_h264_get_is_used(p_dpb->active_fs))&0x1)
                    {
                        if (h264_dpb_pic_is_top_field_ref(p_dpb, long_term))
                        {
                            pic_list[list_idx] = PUT_LIST_LONG_TERM_BITS(lterm) + frame_list[top_idx] + PUT_LIST_INDEX_FIELD_BIT(0);  // top_field
                            list_idx++;
                            got_pic = 1;
                        }
                    }
                    top_idx++;
                }

                /////////////////////////////////////////// ref Bottom Field
                got_pic = 0;
                while ((bot_idx < list_size) & ~got_pic)
                {
                    h264_dpb_set_active_fs(p_dpb, frame_list[bot_idx]);
                    if ((viddec_h264_get_is_used(p_dpb->active_fs))&0x2)
                    {
                        if (h264_dpb_pic_is_bottom_field_ref(p_dpb, long_term))
                        {
                            pic_list[list_idx] = PUT_LIST_LONG_TERM_BITS(lterm) + frame_list[bot_idx] + PUT_LIST_INDEX_FIELD_BIT(1);  // bottom_field
                            list_idx++;
                            got_pic = 1;
                        }
                    }
                    bot_idx++;
                }
            }
        }

        /////////////////////////////////////////////// current Bottom Field
        if (currPicStructure == BOTTOM_FIELD)	{
            while ((top_idx < list_size)||(bot_idx < list_size))
            {
                /////////////////////////////////////////// ref Top Field
                got_pic = 0;
                while ((bot_idx < list_size) && (!(got_pic)))
                {
                    h264_dpb_set_active_fs(p_dpb, frame_list[bot_idx]);
                    if ((viddec_h264_get_is_used(p_dpb->active_fs))&0x2) {
                        if (h264_dpb_pic_is_bottom_field_ref(p_dpb, long_term)) {
                            // short term ref pic
                            pic_list[list_idx] = PUT_LIST_LONG_TERM_BITS(lterm) + frame_list[bot_idx] + PUT_LIST_INDEX_FIELD_BIT(1);  // bottom_field
                            list_idx++;
                            got_pic = 1;
                        }
                    }
                    bot_idx++;
                }

                /////////////////////////////////////////// ref Bottom Field
                got_pic = 0;
                while ((top_idx < list_size) && (!(got_pic)))
                {
                    h264_dpb_set_active_fs(p_dpb, frame_list[top_idx]);
                    if ((viddec_h264_get_is_used(p_dpb->active_fs))&0x1) {
                        if (h264_dpb_pic_is_top_field_ref(p_dpb, long_term)) {
                            // short term ref pic
                            pic_list[list_idx] = PUT_LIST_LONG_TERM_BITS(lterm) + frame_list[top_idx] + PUT_LIST_INDEX_FIELD_BIT(0);  // top_field
                            list_idx++;
                            got_pic = 1;
                        }
                    }
                    top_idx++;
                }
            }
        }
    }

    return list_idx;
}

/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
//////////////////////////////////////////////////////////////////////////////
// h264_dpb_remove_ref_list ()
//
// Removes an idc from the refernce list and updates list after
//

void h264_dpb_remove_ref_list(h264_DecodedPictureBuffer * p_dpb, int32_t ref_idc)
{
    uint8_t idx   = 0;
    int32_t Found = 0;

    while ((idx < p_dpb->ref_frames_in_buffer) && (!(Found)))
    {
        if (p_dpb->fs_ref_idc[idx] == ref_idc)
            Found = 1;
        else
            idx++;
    }

    if (Found)
    {
        // Move the remainder of the list up one
        while (idx < p_dpb->ref_frames_in_buffer - 1) {
            p_dpb->fs_ref_idc[idx] = p_dpb->fs_ref_idc[idx + 1];
            idx ++;
        }

        p_dpb->fs_ref_idc[idx] = MPD_DPB_FS_NULL_IDC; // Clear the last one
        p_dpb->ref_frames_in_buffer--;
    }

    return;
}
/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
//////////////////////////////////////////////////////////////////////////////
// h264_dpb_remove_ltref_list ()
//
// Removes an idc from the long term reference list and updates list after
//////////////////////////////////////////////////////////////////////////////

void h264_dpb_remove_ltref_list(h264_DecodedPictureBuffer * p_dpb,int32_t ref_idc)
{
    uint8_t idx   = 0;
    int32_t Found = 0;

    while ((idx < p_dpb->ltref_frames_in_buffer) && (!(Found)))
    {
        if (p_dpb->fs_ltref_idc[idx] == ref_idc) Found = 1;
        else idx++;
    }

    if (Found)
    {
        // Move the remainder of the list up one
        while (idx <(uint8_t)(p_dpb->ltref_frames_in_buffer - 1))
        {
            p_dpb->fs_ltref_idc[idx] = p_dpb->fs_ltref_idc[idx + 1];
            idx ++;
        }
        p_dpb->fs_ltref_idc[idx] = MPD_DPB_FS_NULL_IDC;		// Clear the last one

        p_dpb->ltref_frames_in_buffer--;
    }

    return;
}


/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
//////////////////////////////////////////////////////////////////////////////
// h264_dpb_init_lists ()
//
// Used to initialise the reference lists
// Also assigns picture numbers and long term picture numbers if P OR B slice
//////////////////////////////////////////////////////////////////////////////
void h264_dpb_update_ref_lists(h264_Info * pInfo)
{
    h264_DecodedPictureBuffer * p_dpb = &pInfo->dpb;

    int32_t MaxFrameNum = 1 << (pInfo->active_SPS.log2_max_frame_num_minus4 + 4);

    uint8_t list0idx, list0idx_1, listltidx;
    uint8_t idx;

    uint8_t add_top, add_bottom, diff;
    uint8_t list_idc;
    uint8_t check_non_existing, skip_picture;


    uint8_t gen_pic_fs_list0[16];
    uint8_t gen_pic_fs_list1[16];
    uint8_t gen_pic_fs_listlt[16];
    uint8_t gen_pic_pic_list[32];  // check out these sizes...

    uint8_t sort_fs_idc[16];
    int32_t list_sort_number[16];

#ifdef DUMP_HEADER_INFO
    static int cc1 = 0;
    //OS_INFO("-------------cc1= %d\n",cc1);    /////// DEBUG info
    if (cc1 == 255)
        idx = 0;
#endif

    list0idx = list0idx_1 = listltidx = 0;

    if (pInfo->SliceHeader.structure == FRAME)
    {
        ////////////////////////////////////////////////// short term handling
        for (idx = 0; idx < p_dpb->ref_frames_in_buffer; idx++)
        {
            h264_dpb_set_active_fs(p_dpb, p_dpb->fs_ref_idc[idx]);

            if ((viddec_h264_get_is_used(p_dpb->active_fs) == 3)&&(p_dpb->active_fs->frame.used_for_reference == 3))
            {
                if (p_dpb->active_fs->frame_num > pInfo->img.frame_num)
                    p_dpb->active_fs->frame_num_wrap = p_dpb->active_fs->frame_num - MaxFrameNum;
                else
                    p_dpb->active_fs->frame_num_wrap = p_dpb->active_fs->frame_num;

                p_dpb->active_fs->frame.pic_num     = p_dpb->active_fs->frame_num_wrap;

                // Use this opportunity to sort list for a p-frame
                if (pInfo->SliceHeader.slice_type == h264_PtypeP)
                {
                    sort_fs_idc[list0idx]      = p_dpb->fs_ref_idc[idx];
                    list_sort_number[list0idx] = p_dpb->active_fs->frame.pic_num;
                    list0idx++;
                }
            }
        }

        if (pInfo->SliceHeader.slice_type == h264_PtypeP)
        {
            h264_list_sort(sort_fs_idc, list_sort_number, list0idx, 1);
            for (idx = 0; idx < list0idx; idx++)
                p_dpb->listX_0[idx] = (sort_fs_idc[idx]);  // frame

            p_dpb->listXsize[0] = list0idx;
        }

        ////////////////////////////////////////////////// long term handling
        for (idx = 0; idx < p_dpb->ltref_frames_in_buffer; idx++)
        {
            h264_dpb_set_active_fs(p_dpb, p_dpb->fs_ltref_idc[idx]);
            if ((viddec_h264_get_is_used(p_dpb->active_fs) == 3) && (viddec_h264_get_is_long_term(p_dpb->active_fs) == 3) && (p_dpb->active_fs->frame.used_for_reference == 3))
            {
                p_dpb->active_fs->frame.long_term_pic_num = p_dpb->active_fs->frame.long_term_frame_idx;

                if (pInfo->SliceHeader.slice_type == h264_PtypeP)
                {
                    sort_fs_idc[list0idx-p_dpb->listXsize[0]]       = p_dpb->fs_ltref_idc[idx];
                    list_sort_number[list0idx-p_dpb->listXsize[0]]  = p_dpb->active_fs->frame.long_term_pic_num;
                    list0idx++;
                }
            }
        }

        if (pInfo->SliceHeader.slice_type == h264_PtypeP)
        {
            h264_list_sort(sort_fs_idc, list_sort_number, list0idx-p_dpb->listXsize[0], 0);
            for (idx = p_dpb->listXsize[0]; idx < list0idx; idx++) {
                p_dpb->listX_0[idx] = (1<<6) + sort_fs_idc[idx-p_dpb->listXsize[0]];
            }
            p_dpb->listXsize[0] = list0idx;
        }
    }
    else   /// Field base
    {
        if (pInfo->SliceHeader.structure == TOP_FIELD)
        {
            add_top    = 1;
            add_bottom = 0;
        }
        else
        {
            add_top    = 0;
            add_bottom = 1;
        }

        ////////////////////////////////////////////P0: Short term handling
        for (idx = 0; idx < p_dpb->ref_frames_in_buffer; idx++)
        {
            h264_dpb_set_active_fs(p_dpb, p_dpb->fs_ref_idc[idx]);
            if (p_dpb->active_fs->frame.used_for_reference)
            {
                if (p_dpb->active_fs->frame_num > pInfo->SliceHeader.frame_num) {
                    p_dpb->active_fs->frame_num_wrap = p_dpb->active_fs->frame_num - MaxFrameNum;
                } else {
                    p_dpb->active_fs->frame_num_wrap = p_dpb->active_fs->frame_num;
                }

                if ((p_dpb->active_fs->frame.used_for_reference)&0x1) {
                    p_dpb->active_fs->top_field.pic_num    = (p_dpb->active_fs->frame_num_wrap << 1) + add_top;
                }

                if ((p_dpb->active_fs->frame.used_for_reference)&0x2) {
                    p_dpb->active_fs->bottom_field.pic_num = (p_dpb->active_fs->frame_num_wrap << 1) + add_bottom;
                }

                if (pInfo->SliceHeader.slice_type == h264_PtypeP) {
                    sort_fs_idc[list0idx]      = p_dpb->fs_ref_idc[idx];
                    list_sort_number[list0idx] = p_dpb->active_fs->frame_num_wrap;
                    list0idx++;
                }
            }
        }

        if (pInfo->SliceHeader.slice_type == h264_PtypeP)
        {
            h264_list_sort(sort_fs_idc, list_sort_number, list0idx, 1);
            for (idx = 0; idx < list0idx; idx++) {
                gen_pic_fs_list0[idx] = sort_fs_idc[idx];
            }

            p_dpb->listXsize[0] = 0;
            p_dpb->listXsize[0] = h264_dpb_gen_pic_list_from_frame_list(p_dpb, gen_pic_pic_list, gen_pic_fs_list0, pInfo->img.structure, list0idx, 0);

            for (idx = 0; idx < p_dpb->listXsize[0]; idx++)
            {
                p_dpb->listX_0[idx] = gen_pic_pic_list[idx];
            }
        }

        ////////////////////////////////////////////P0: long term handling
        for (idx = 0; idx < p_dpb->ltref_frames_in_buffer; idx++)
        {
            h264_dpb_set_active_fs(p_dpb, p_dpb->fs_ltref_idc[idx]);

            if (viddec_h264_get_is_long_term(p_dpb->active_fs)&0x1) {
                p_dpb->active_fs->top_field.long_term_pic_num    = (p_dpb->active_fs->top_field.long_term_frame_idx << 1) + add_top;
            }

            if (viddec_h264_get_is_long_term(p_dpb->active_fs)&0x2) {
                p_dpb->active_fs->bottom_field.long_term_pic_num = (p_dpb->active_fs->bottom_field.long_term_frame_idx << 1) + add_bottom;
            }

            if (pInfo->SliceHeader.slice_type == h264_PtypeP)
            {
                sort_fs_idc[listltidx]      = p_dpb->fs_ltref_idc[idx];
                list_sort_number[listltidx] = p_dpb->active_fs->long_term_frame_idx;
                listltidx++;
            }
        }

        if (pInfo->SliceHeader.slice_type == h264_PtypeP)
        {
            h264_list_sort(sort_fs_idc, list_sort_number, listltidx, 0);
            for (idx = 0; idx < listltidx; idx++) {
                gen_pic_fs_listlt[idx] = sort_fs_idc[idx];
            }
            list0idx_1 = h264_dpb_gen_pic_list_from_frame_list(p_dpb, gen_pic_pic_list, gen_pic_fs_listlt, pInfo->img.structure, listltidx, 1);

            for (idx = 0; idx < list0idx_1; idx++) {
                p_dpb->listX_0[p_dpb->listXsize[0]+idx] = gen_pic_pic_list[idx];
            }
            p_dpb->listXsize[0] += list0idx_1;
        }
    }


    if (pInfo->SliceHeader.slice_type == h264_PtypeI)
    {
        p_dpb->listXsize[0] = 0;
        p_dpb->listXsize[1] = 0;
        return;
    }

    if (pInfo->SliceHeader.slice_type == h264_PtypeP)
    {
        //// Forward done above
        p_dpb->listXsize[1] = 0;
    }


    // B-Slice
    // Do not include non-existing frames for B-pictures when cnt_type is zero

    if (pInfo->SliceHeader.slice_type == h264_PtypeB)
    {
        list0idx = list0idx_1 = listltidx = 0;
        skip_picture = 0;

        if (pInfo->active_SPS.pic_order_cnt_type == 0)
            check_non_existing = 1;
        else
            check_non_existing = 0;

        if (pInfo->SliceHeader.structure == FRAME)
        {
            for (idx = 0; idx < p_dpb->ref_frames_in_buffer; idx++)
            {
                h264_dpb_set_active_fs(p_dpb, p_dpb->fs_ref_idc[idx]);
                if (viddec_h264_get_is_used(p_dpb->active_fs) == 3)
                {
                    if (check_non_existing)
                    {
                        if (viddec_h264_get_is_non_existent(p_dpb->active_fs)) skip_picture = 1;
                        else                           skip_picture = 0;
                    }

                    if (skip_picture == 0)
                    {
                        if ((p_dpb->active_fs->frame.used_for_reference==3) && (!(p_dpb->active_fs->frame.is_long_term)))
                        {
                            if (pInfo->img.framepoc >= p_dpb->active_fs->frame.poc)
                            {
                                sort_fs_idc[list0idx]      = p_dpb->fs_ref_idc[idx];
                                list_sort_number[list0idx] = p_dpb->active_fs->frame.poc;
                                list0idx++;
                            }
                        }
                    }
                }
            }

            h264_list_sort(sort_fs_idc, list_sort_number, list0idx, 1);
            for (idx = 0; idx < list0idx; idx++) {
                p_dpb->listX_0[idx] = sort_fs_idc[idx];
            }

            list0idx_1 = list0idx;

            /////////////////////////////////////////B0:  Short term handling
            for (idx = 0; idx < p_dpb->ref_frames_in_buffer; idx++)
            {
                h264_dpb_set_active_fs(p_dpb, p_dpb->fs_ref_idc[idx]);

                if (viddec_h264_get_is_used(p_dpb->active_fs) == 3)
                {
                    if (check_non_existing)
                    {
                        if (viddec_h264_get_is_non_existent(p_dpb->active_fs))	skip_picture = 1;
                        else							skip_picture = 0;
                    }

                    if (skip_picture == 0)
                    {
                        if ((p_dpb->active_fs->frame.used_for_reference) && (!(p_dpb->active_fs->frame.is_long_term)))
                        {
                            if (pInfo->img.framepoc < p_dpb->active_fs->frame.poc)
                            {
                                sort_fs_idc[list0idx-list0idx_1]      = p_dpb->fs_ref_idc[idx];
                                list_sort_number[list0idx-list0idx_1] = p_dpb->active_fs->frame.poc;
                                list0idx++;
                            }
                        }
                    }
                }
            }

            h264_list_sort(sort_fs_idc, list_sort_number, list0idx-list0idx_1, 0);
            for (idx = list0idx_1; idx < list0idx; idx++) {
                p_dpb->listX_0[idx] = sort_fs_idc[idx-list0idx_1];
            }

            for (idx = 0; idx < list0idx_1; idx++) {
                p_dpb->listX_1[list0idx-list0idx_1+idx] = p_dpb->listX_0[idx];
            }

            for (idx = list0idx_1; idx < list0idx; idx++) {
                p_dpb->listX_1[idx-list0idx_1] = p_dpb->listX_0[idx];
            }

            p_dpb->listXsize[0] = list0idx;
            p_dpb->listXsize[1] = list0idx;

            /////////////////////////////////////////B0:  long term handling
            list0idx = 0;

            // Can non-existent pics be set as long term??
            for (idx = 0; idx < p_dpb->ltref_frames_in_buffer; idx++)
            {
                h264_dpb_set_active_fs(p_dpb, p_dpb->fs_ltref_idc[idx]);

                if ((viddec_h264_get_is_used(p_dpb->active_fs) == 3) && (viddec_h264_get_is_long_term(p_dpb->active_fs) == 3))
                {
                    // if we have two fields, both must be long-term
                    sort_fs_idc[list0idx]      = p_dpb->fs_ltref_idc[idx];
                    list_sort_number[list0idx] = p_dpb->active_fs->frame.long_term_pic_num;
                    list0idx++;
                }
            }

            h264_list_sort(sort_fs_idc, list_sort_number, list0idx, 0);
            for (idx = p_dpb->listXsize[0]; idx < (p_dpb->listXsize[0]+list0idx); idx = idx + 1)
            {
                p_dpb->listX_0[idx] = (1<<6) + sort_fs_idc[idx-p_dpb->listXsize[0]];
                p_dpb->listX_1[idx] = (1<<6) + sort_fs_idc[idx-p_dpb->listXsize[0]];
            }

            p_dpb->listXsize[0] += list0idx;
            p_dpb->listXsize[1] += list0idx;
        }
        else  // Field
        {
            for (idx = 0; idx < p_dpb->ref_frames_in_buffer; idx++)
            {
                h264_dpb_set_active_fs(p_dpb, p_dpb->fs_ref_idc[idx]);

                if (viddec_h264_get_is_used(p_dpb->active_fs))	{
                    if (check_non_existing) {
                        if (viddec_h264_get_is_non_existent(p_dpb->active_fs))
                            skip_picture = 1;
                        else
                            skip_picture = 0;
                    }

                    if (skip_picture == 0)  {
                        if (pInfo->img.ThisPOC >= p_dpb->active_fs->frame.poc) {
                            sort_fs_idc[list0idx]      = p_dpb->fs_ref_idc[idx];
                            list_sort_number[list0idx] = p_dpb->active_fs->frame.poc;
                            list0idx++;
                        }
                    }
                }
            }

            h264_list_sort(sort_fs_idc, list_sort_number, list0idx, 1);
            for (idx = 0; idx < list0idx; idx = idx + 1) {
                gen_pic_fs_list0[idx] = sort_fs_idc[idx];
            }

            list0idx_1 = list0idx;

            ///////////////////////////////////////////// B1: Short term handling
            for (idx = 0; idx < p_dpb->ref_frames_in_buffer; idx++)
            {
                h264_dpb_set_active_fs(p_dpb, p_dpb->fs_ref_idc[idx]);
                if (viddec_h264_get_is_used(p_dpb->active_fs))
                {
                    if (check_non_existing) {
                        if (viddec_h264_get_is_non_existent(p_dpb->active_fs))
                            skip_picture = 1;
                        else
                            skip_picture = 0;
                    }

                    if (skip_picture == 0) {
                        if (pInfo->img.ThisPOC < p_dpb->active_fs->frame.poc) {
                            sort_fs_idc[list0idx-list0idx_1]      = p_dpb->fs_ref_idc[idx];
                            list_sort_number[list0idx-list0idx_1] = p_dpb->active_fs->frame.poc;
                            list0idx++;
                        }
                    }
                }
            }

            ///// Generate frame list from sorted fs
            /////
            h264_list_sort(sort_fs_idc, list_sort_number, list0idx-list0idx_1, 0);
            for (idx = list0idx_1; idx < list0idx; idx++)
                gen_pic_fs_list0[idx] = sort_fs_idc[idx-list0idx_1];

            for (idx = 0; idx < list0idx_1; idx++)
                gen_pic_fs_list1[list0idx-list0idx_1+idx] = gen_pic_fs_list0[idx];

            for (idx = list0idx_1; idx < list0idx; idx++)
                gen_pic_fs_list1[idx-list0idx_1] = gen_pic_fs_list0[idx];

            ///// Generate List_X0
            /////
            p_dpb->listXsize[0] = h264_dpb_gen_pic_list_from_frame_list(p_dpb, gen_pic_pic_list, gen_pic_fs_list0, pInfo->img.structure, list0idx, 0);

            for (idx = 0; idx < p_dpb->listXsize[0]; idx++)
                p_dpb->listX_0[idx] = gen_pic_pic_list[idx];

            //// Generate List X1
            ////
            p_dpb->listXsize[1] = h264_dpb_gen_pic_list_from_frame_list(p_dpb, gen_pic_pic_list, gen_pic_fs_list1, pInfo->img.structure, list0idx, 0);

            for (idx = 0; idx < p_dpb->listXsize[1]; idx++)
                p_dpb->listX_1[idx] = gen_pic_pic_list[idx];

            ///////////////////////////////////////////// B1: long term handling
            for (idx = 0; idx < p_dpb->ltref_frames_in_buffer; idx++)
            {
                h264_dpb_set_active_fs(p_dpb, p_dpb->fs_ltref_idc[idx]);
                sort_fs_idc[listltidx]      = p_dpb->fs_ltref_idc[idx];
                list_sort_number[listltidx] = p_dpb->active_fs->long_term_frame_idx;
                listltidx++;
            }

            h264_list_sort(sort_fs_idc, list_sort_number, listltidx, 0);
            for (idx = 0; idx < listltidx; idx++)
                gen_pic_fs_listlt[idx] = sort_fs_idc[idx];

            list0idx_1 = h264_dpb_gen_pic_list_from_frame_list(p_dpb, gen_pic_pic_list, gen_pic_fs_listlt, pInfo->img.structure, listltidx, 1);

            for (idx = 0; idx < list0idx_1; idx++)
            {
                p_dpb->listX_0[p_dpb->listXsize[0]+idx] = gen_pic_pic_list[idx];
                p_dpb->listX_1[p_dpb->listXsize[1]+idx] = gen_pic_pic_list[idx];
            }

            p_dpb->listXsize[0] += list0idx_1;
            p_dpb->listXsize[1] += list0idx_1;
        }
    }

    // Setup initial list sizes at this point
    p_dpb->nInitListSize[0] = p_dpb->listXsize[0];
    p_dpb->nInitListSize[1] = p_dpb->listXsize[1];
    if (pInfo->SliceHeader.slice_type != h264_PtypeI)
    {
        if ((p_dpb->listXsize[0]==p_dpb->listXsize[1]) && (p_dpb->listXsize[0] > 1))
        {
            // check if lists are identical, if yes swap first two elements of listX[1]
            diff = 0;
            for (idx = 0; idx < p_dpb->listXsize[0]; idx = idx + 1)
            {
                if (p_dpb->listX_0[idx] != p_dpb->listX_1[idx]) diff = 1;
            }


            if (!(diff))
            {
                list_idc       = p_dpb->listX_1[0];
                p_dpb->listX_1[0] = p_dpb->listX_1[1];
                p_dpb->listX_1[1] = list_idc;
            }
        }

        // set max size
        if (p_dpb->listXsize[0] > pInfo->SliceHeader.num_ref_idx_l0_active)
        {
            p_dpb->listXsize[0] = pInfo->SliceHeader.num_ref_idx_l0_active;
        }


        if (p_dpb->listXsize[1] > pInfo->SliceHeader.num_ref_idx_l1_active)
        {
            p_dpb->listXsize[1] = pInfo->SliceHeader.num_ref_idx_l1_active;
        }



    }



    /// DPB reorder list
    h264_dpb_reorder_lists(pInfo);

    return;
}   //// End of init_dpb_list


/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
//////////////////////////////////////////////////////////////////////////////
// h264_dpb_get_short_term_pic ()
//
// Sets active_fs to point to frame store containing picture with given picNum
// Sets field_flag, bottom_field and err_flag based on the picture and whether
// it is available or not...
//
static frame_param_ptr h264_dpb_get_short_term_pic(h264_Info * pInfo,int32_t pic_num, int32_t *bottom_field_bit)
{
    register uint32_t idx;
    register frame_param_ptr temp_fs;

    h264_DecodedPictureBuffer *p_dpb = &pInfo->dpb;

    *bottom_field_bit = 0;
    for (idx = 0; idx < p_dpb->ref_frames_in_buffer; idx++)
    {
        temp_fs = &p_dpb->fs[p_dpb->fs_ref_idc[idx]];
        if (pInfo->SliceHeader.structure == FRAME)
        {
            if (temp_fs->frame.used_for_reference == 3)
                if (!(temp_fs->frame.is_long_term))
                    if (temp_fs->frame.pic_num == pic_num) return temp_fs;
        }
        else // current picture is a field
        {
            if (temp_fs->frame.used_for_reference&0x1)
                if (!(temp_fs->top_field.is_long_term))
                    if (temp_fs->top_field.pic_num == pic_num)
                    {
                        return temp_fs;
                    }

            if (temp_fs->frame.used_for_reference&0x2)
                if (!(temp_fs->bottom_field.is_long_term))
                    if (temp_fs->bottom_field.pic_num == pic_num)
                    {
                        *bottom_field_bit = PUT_LIST_INDEX_FIELD_BIT(1);
                        return temp_fs;
                    }
        }
    }
    return NULL;
}

/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
//////////////////////////////////////////////////////////////////////////////
// h264_dpb_get_long_term_pic ()
//
// Sets active_fs to point to frame store containing picture with given picNum
//

static frame_param_ptr h264_dpb_get_long_term_pic(h264_Info * pInfo,int32_t long_term_pic_num, int32_t *bottom_field_bit)
{
    register uint32_t idx;
    register frame_param_ptr temp_fs;
    h264_DecodedPictureBuffer *p_dpb = &pInfo->dpb;

    *bottom_field_bit = 0;
    for (idx = 0; idx < p_dpb->ltref_frames_in_buffer; idx++)
    {
        temp_fs = &p_dpb->fs[p_dpb->fs_ltref_idc[idx]];
        if (pInfo->SliceHeader.structure == FRAME)
        {
            if (temp_fs->frame.used_for_reference == 3)
                if (temp_fs->frame.is_long_term)
                    if (temp_fs->frame.long_term_pic_num == long_term_pic_num)
                        return temp_fs;
        }
        else
        {
            if (temp_fs->frame.used_for_reference&0x1)
                if (temp_fs->top_field.is_long_term)
                    if (temp_fs->top_field.long_term_pic_num == long_term_pic_num)
                        return temp_fs;

            if (temp_fs->frame.used_for_reference&0x2)
                if (temp_fs->bottom_field.is_long_term)
                    if (temp_fs->bottom_field.long_term_pic_num == long_term_pic_num)
                    {
                        *bottom_field_bit = PUT_LIST_INDEX_FIELD_BIT(1);
                        return temp_fs;
                    }
        }
    }
    return NULL;
}

/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
//////////////////////////////////////////////////////////////////////////////
// h264_dpb_reorder_ref_pic_list ()
//
// Used to sort a list based on a corresponding sort indices
//

struct list_value_t
{
    int32_t value;
    struct list_value_t *next;
};

struct linked_list_t
{
    struct list_value_t *begin;
    struct list_value_t *end;
    struct list_value_t *entry;
    struct list_value_t *prev_entry;
    struct list_value_t list[32];
};

static void linked_list_initialize (struct linked_list_t *lp, uint8_t *vp, int32_t size)
{
    struct list_value_t *lvp;

    lvp            = lp->list;
    lp->begin      = lvp;
    lp->entry      = lvp;
    lp->end        = lvp + (size-1);
    lp->prev_entry = NULL;

    while (lvp <= lp->end)
    {
        lvp->value = *(vp++);
        lvp->next  = lvp + 1;
        lvp++;
    }
    lp->end->next = NULL;
    return;
}
/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
static void linked_list_reorder (struct linked_list_t *lp, int32_t list_value)
{
    register struct list_value_t *lvp = lp->entry;
    register struct list_value_t *lvp_prev;

    if (lvp == NULL) {
        lp->end->value = list_value;  // replace the end entry
    } else if ((lp->begin==lp->end)||(lvp==lp->end))  // replece the begin/end entry and set the entry to NULL
    {
        lp->entry->value = list_value;
        lp->prev_entry   = lp->entry;
        lp->entry        = NULL;
    }
    else if (lvp->value==list_value)  // the entry point matches
    {
        lp->prev_entry = lvp;
        lp->entry      = lvp->next;
    }
    else if (lvp->next == lp->end) // the entry is just before the end
    {
        // replace the end and swap the end and entry points
        //                  lvp
        //  prev_entry  => entry                    => old_end
        //                 old_end & new_prev_entry => new_end & entry
        lp->end->value = list_value;

        if (lp->prev_entry)
            lp->prev_entry->next = lp->end;
        else
            lp->begin            = lp->end;

        lp->prev_entry = lp->end;
        lp->end->next  = lvp;
        lp->end        = lvp;
        lvp->next      = NULL;
    }
    else
    {
        lvp_prev = NULL;
        while (lvp->next) // do not check the end but we'll be in the loop at least once
        {
            if (lvp->value == list_value) break;
            lvp_prev = lvp;
            lvp = lvp->next;
        }
        lvp->value = list_value;   // force end matches
        if (lvp_prev != NULL)
        {
            // remove lvp from the list
            lvp_prev->next = lvp->next;
        }
        if (lvp==lp->end) lp->end = lvp_prev;

        // insert lvp in front of lp->entry
        if (lp->entry==lp->begin)
        {
            lvp->next = lp->begin;
            lp->begin = lvp;
        }
        else
        {
            lvp->next = lp->entry;
            lp->prev_entry->next = lvp;
        }
        lp->prev_entry = lvp;
    }
    return;
}
/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
static void linked_list_output (struct linked_list_t *lp, int32_t *vp)
{
    register int32_t *ip1;
    register struct list_value_t *lvp;

    lvp  = lp->begin;
    ip1  = vp;
    while (lvp)
    {
        *(ip1++) = lvp->value;
        lvp = lvp->next;
    }
    return;
}
/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
int32_t h264_dpb_reorder_ref_pic_list(h264_Info * pInfo,int32_t list_num, int32_t num_ref_idx_active)
{
    h264_DecodedPictureBuffer *p_dpb = &pInfo->dpb;
    uint8_t                   *remapping_of_pic_nums_idc;
    list_reordering_num_t		*list_reordering_num;
    int32_t                    bottom_field_bit;

    int32_t  maxPicNum, currPicNum, picNumLXNoWrap, picNumLXPred, pic_num;
    int32_t  refIdxLX;
    int32_t  i;

    int32_t    PicList[32] = {0};
    struct linked_list_t ll;
    struct linked_list_t *lp = &ll;     // should consider use the scratch space

    // declare these below as registers gave me 23 cy/MB for the worst frames in Allegro_Combined_CABAC_07_HD, YHu
    register frame_param_ptr temp_fs;
    register int32_t temp;
    register uint8_t  *ip1;

    maxPicNum = 1 << (pInfo->active_SPS.log2_max_frame_num_minus4 + 4);


    if (list_num == 0) // i.e list 0
    {
        ip1 = p_dpb->listX_0;
        remapping_of_pic_nums_idc = pInfo->SliceHeader.sh_refpic_l0.reordering_of_pic_nums_idc;
        list_reordering_num       = pInfo->SliceHeader.sh_refpic_l0.list_reordering_num;
    }
    else
    {
        ip1 = p_dpb->listX_1;
        remapping_of_pic_nums_idc = pInfo->SliceHeader.sh_refpic_l1.reordering_of_pic_nums_idc;
        list_reordering_num       = pInfo->SliceHeader.sh_refpic_l1.list_reordering_num;
    }


    linked_list_initialize (lp, ip1, num_ref_idx_active);

    currPicNum = pInfo->SliceHeader.frame_num;
    if (pInfo->SliceHeader.structure != FRAME)
    {

        /* The reason it is + 1 I think, is because the list is based on polarity
           expand later...
        */
        maxPicNum  <<= 1;
        currPicNum <<= 1;
        currPicNum++;
    }

    picNumLXPred = currPicNum;
    refIdxLX = 0;

    for (i = 0; remapping_of_pic_nums_idc[i] != 3; i++)
    {
        if (i > MAX_NUM_REF_FRAMES)
        {
            break;
        }

        if (remapping_of_pic_nums_idc[i] < 2) // - short-term re-ordering
        {
            temp = (list_reordering_num[i].abs_diff_pic_num_minus1 + 1);
            if (remapping_of_pic_nums_idc[i] == 0)
            {
                temp = picNumLXPred - temp;
                if (temp < 0 ) picNumLXNoWrap = temp + maxPicNum;
                else           picNumLXNoWrap = temp;
            }
            else // (remapping_of_pic_nums_idc[i] == 1)
            {
                temp += picNumLXPred;
                if (temp  >=  maxPicNum) picNumLXNoWrap = temp - maxPicNum;
                else                     picNumLXNoWrap = temp;
            }

            // Updates for next iteration of the loop
            picNumLXPred = picNumLXNoWrap;

            if (picNumLXNoWrap > currPicNum ) pic_num = picNumLXNoWrap - maxPicNum;
            else                              pic_num = picNumLXNoWrap;

            temp_fs = h264_dpb_get_short_term_pic(pInfo, pic_num, &bottom_field_bit);
            if (temp_fs)
            {
                temp = bottom_field_bit + PUT_FS_IDC_BITS(temp_fs->fs_idc);
                linked_list_reorder (lp, temp);
            }
        }
        else //(remapping_of_pic_nums_idc[i] == 2) long-term re-ordering
        {
            pic_num = list_reordering_num[i].long_term_pic_num;

            temp_fs = h264_dpb_get_long_term_pic(pInfo, pic_num, &bottom_field_bit);
            if (temp_fs)
            {
                temp = PUT_LIST_LONG_TERM_BITS(1) + bottom_field_bit + PUT_FS_IDC_BITS(temp_fs->fs_idc);
                linked_list_reorder (lp, temp);
            }
        }
    }

    linked_list_output (lp, PicList);

    if (0 == list_num )
    {
        for (i=0; i<num_ref_idx_active; i++)
        {
            pInfo->slice_ref_list0[i]=(uint8_t)PicList[i];
        }
    }
    else
    {
        for (i=0; i<num_ref_idx_active; i++)
        {
            pInfo->slice_ref_list1[i]=(uint8_t)PicList[i];
        }
    }


    // Instead of updating the now reordered list here, just write it down...
    // This way, we can continue to hold the initialised list in p_dpb->listX_0
    // and therefore not need to update it every slice

    //h264_dpb_write_list(list_num, PicList, num_ref_idx_active);

    return num_ref_idx_active;
}

/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */


void h264_dpb_RP_check_list (h264_Info * pInfo)
{
    h264_DecodedPictureBuffer *p_dpb = &pInfo->dpb;
    uint8_t    *p_list = pInfo->slice_ref_list0;

    //
    // If the decoding start from RP and without exact point, all B frames belong to previous GOP should be throw away!
    //

    if ((pInfo->SliceHeader.slice_type == h264_PtypeB)&&(pInfo->sei_b_state_ready ==0) && pInfo->sei_rp_received) {
        pInfo->wl_err_curr |= VIDDEC_FW_WORKLOAD_ERR_NOTDECODABLE;
        pInfo->wl_err_curr |= (FRAME << FIELD_ERR_OFFSET);
    }


    //
    // Repare Ref list if it damaged with RP recovery only
    //
    if ((pInfo->SliceHeader.slice_type == h264_PtypeP) && pInfo->sei_rp_received)
    {

        int32_t idx, rp_found = 0;

        if ( ((pInfo->SliceHeader.num_ref_idx_l0_active == 1)&&(pInfo->SliceHeader.structure == FRAME)) ||
                ((pInfo->SliceHeader.num_ref_idx_l0_active == 2)&&(pInfo->SliceHeader.structure != FRAME)) )
        {
            if (pInfo->SliceHeader.sh_refpic_l0.ref_pic_list_reordering_flag)
            {
                p_list = pInfo->slice_ref_list0;
            }
            else
            {
                p_list = pInfo->dpb.listX_0;
                //pInfo->sei_rp_received = 0;
                //return;
            }


            for (idx = 0; idx < p_dpb->used_size; idx++) {
                if (p_dpb->fs_dpb_idc[idx] == pInfo->last_I_frame_idc) {
                    rp_found = 1;
                    break;
                }
            }
            if (rp_found) {
#if 0
                int32_t poc;

                ///// Clear long-term ref list
                for (idx = 0; idx < p_dpb->ltref_frames_in_buffer; idx++)
                {
                    h264_dpb_unmark_for_reference(p_dpb, p_dpb->fs_ltref_idc[0]);
                    h264_dpb_remove_ltref_list(p_dpb, p_dpb->fs_ltref_idc[0]);
                }

                ///// Clear short-term ref list
                //while(p_dpb->used_size>1)
                for (idx = 0; idx < p_dpb->used_size; idx++)
                {
                    int32_t idx_pos;
                    //// find smallest non-output POC
                    h264_dpb_get_smallest_poc(p_dpb, &poc, &idx_pos);

                    //// Remove all frames in previous GOP
                    if ((idx_pos != MPD_DPB_FS_NULL_IDC) && (p_dpb->fs_dpb_idc[idx_pos] != pInfo->last_I_frame_idc))
                    {
                        // Remove from ref-list
                        h264_dpb_unmark_for_reference(p_dpb, p_dpb->fs_dpb_idc[idx_pos]);
                        h264_dpb_remove_ref_list(p_dpb, p_dpb->fs_dpb_idc[idx_pos]);

                        // Output from DPB
                        //h264_dpb_set_active_fs(p_dpb, p_dpb->fs_dpb_idc[idx]);
                        //if((active_fs->is_output == 0) && (active_fs->is_non_existent == 0))
                        {
                            //int32_t existing;
                            //h264_dpb_frame_output(pInfo, p_dpb->fs_dpb_idc[idx], 0, &existing);
                            //p_dpb->last_output_poc = poc;
                        }
                        //h264_dpb_remove_frame_from_dpb(p_dpb, idx);		// Remove dpb.fs_dpb_idc[pos]

                    }
                }
#endif

                ///// Set the reference to last I frame
                if ( (pInfo->last_I_frame_idc!=255)&&(pInfo->last_I_frame_idc!=p_list[0]))
                {
                    /// Repaire the reference list now
                    h264_dpb_unmark_for_reference(p_dpb, p_list[0]);
                    h264_dpb_remove_ref_list(p_dpb, p_list[0]);
                    p_list[0] = pInfo->last_I_frame_idc;
                    if (pInfo->SliceHeader.structure != FRAME)
                        p_list[1] = (pInfo->last_I_frame_idc ^ 0x20);
                }
            }
        }

        pInfo->sei_rp_received = 0;
        pInfo->sei_b_state_ready = 1;

    }


    return;
}


/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
//////////////////////////////////////////////////////////////////////////////
// h264_dpb_reorder_lists ()
//
// Used to sort a list based on a corresponding sort indices
//

void h264_dpb_reorder_lists(h264_Info * pInfo)
{
    int32_t currSliceType = pInfo->SliceHeader.slice_type;

    if (currSliceType == h264_PtypeP )
    {
        /////////////////////////////////////////////// Reordering reference list for P slice
        /// Forward reordering
        if (pInfo->SliceHeader.sh_refpic_l0.ref_pic_list_reordering_flag)
            h264_dpb_reorder_ref_pic_list(pInfo, 0, pInfo->SliceHeader.num_ref_idx_l0_active);
        else
        {

        }
        pInfo->dpb.listXsize[0]=pInfo->SliceHeader.num_ref_idx_l0_active;
    } else if (currSliceType == h264_PtypeB)
    {
        /////////////////////////////////////////////// Reordering reference list for B slice
        /// Forward reordering
        if (pInfo->SliceHeader.sh_refpic_l0.ref_pic_list_reordering_flag)
            h264_dpb_reorder_ref_pic_list(pInfo, 0, pInfo->SliceHeader.num_ref_idx_l0_active);
        else
        {

        }
        pInfo->dpb.listXsize[0]=pInfo->SliceHeader.num_ref_idx_l0_active;

        /// Backward reordering
        if (pInfo->SliceHeader.sh_refpic_l1.ref_pic_list_reordering_flag)
            h264_dpb_reorder_ref_pic_list(pInfo, 1, pInfo->SliceHeader.num_ref_idx_l1_active);
        else
        {

        }
        pInfo->dpb.listXsize[1]=pInfo->SliceHeader.num_ref_idx_l1_active;
    }

    //// Check if need recover reference list with previous recovery point
    if (!pInfo->img.second_field)
    {
        h264_dpb_RP_check_list(pInfo);
    }


    return;
}

////////////////////////////////////////// DPB management //////////////////////

//////////////////////////////////////////////////////////////////////////////
// avc_dpb_get_non_output_frame_number ()
//
// get total non output frame number in the DPB.
//
static int32_t avc_dpb_get_non_output_frame_number(h264_Info * pInfo)
{
    int32_t idx;
    int32_t number=0;
    h264_DecodedPictureBuffer *p_dpb = &pInfo->dpb;

    for (idx = 0; idx < p_dpb->used_size; idx++)
    {
        h264_dpb_set_active_fs(p_dpb, p_dpb->fs_dpb_idc[idx]);

        if (viddec_h264_get_is_output(p_dpb->active_fs) == 0)
        {
            (number)++;
        }
    }

    return number;
}


/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
//// Store previous picture in DPB, and then update DPB queue, remove unused frames from DPB

void h264_dpb_store_previous_picture_in_dpb(h264_Info * pInfo,int32_t NonExisting, int32_t use_old)
{
    h264_DecodedPictureBuffer *p_dpb = &pInfo->dpb;

    int32_t used_for_reference;
    int32_t is_direct_output;
    int32_t second_field_stored = 0;
    int32_t poc;
    int32_t pos;
    int32_t flag;
    int32_t first_field_non_ref = 0;
    int32_t idr_flag;

    if (NonExisting) {
        if (p_dpb->fs_non_exist_idc == MPD_DPB_FS_NULL_IDC)
            return;
        h264_dpb_set_active_fs(p_dpb, p_dpb->fs_non_exist_idc);
    } else {
        if (p_dpb->fs_dec_idc == MPD_DPB_FS_NULL_IDC)
            return;
        h264_dpb_set_active_fs(p_dpb, p_dpb->fs_dec_idc);
    }

    if (NonExisting == 0)
    {
        //active_fs->sps_disp_index = (next_sps_disp_entry == 0)? 7 : next_sps_disp_entry - 1;
        pInfo->img.last_has_mmco_5       = 0;
        pInfo->img.last_pic_bottom_field = pInfo->img.bottom_field_flag;

        //used_for_reference = (use_old) ? !(old_pInfo->img.old_disposable_flag) : !(pInfo->img.disposable_flag);
        used_for_reference = (use_old) ? !(pInfo->old_slice.nal_ref_idc==0) : !(pInfo->SliceHeader.nal_ref_idc==0);

        switch (viddec_h264_get_dec_structure(p_dpb->active_fs))
        {
        case(TOP_FIELD)   : {
            p_dpb->active_fs->top_field.used_for_reference = used_for_reference;
            viddec_h264_set_is_top_used(p_dpb->active_fs, 1);
            //p_dpb->active_fs->crc_field_coded     = 1;
        }
        break;
        case(BOTTOM_FIELD): {
            p_dpb->active_fs->bottom_field.used_for_reference = used_for_reference << 1;
            viddec_h264_set_is_bottom_used(p_dpb->active_fs, 1);
            //p_dpb->active_fs->crc_field_coded     = 1;
        }
        break;
        default: {
            p_dpb->active_fs->frame.used_for_reference = used_for_reference?3:0;
            viddec_h264_set_is_frame_used(p_dpb->active_fs, 3);
            //if(pInfo->img.MbaffFrameFlag) p_dpb->active_fs->crc_field_coded  = 1;

        }
        break;
        }

        //freeze_assert = use_old ? old_pInfo->img.sei_freeze_this_image : pInfo->img.sei_freeze_this_image;
        //if (freeze_assert)  sei_information.disp_frozen = 1;

        idr_flag = use_old ? pInfo->old_slice.idr_flag : pInfo->SliceHeader.idr_flag;
        if (idr_flag) {
            h264_dpb_idr_memory_management (pInfo, &pInfo->active_SPS, pInfo->img.no_output_of_prior_pics_flag);
        } else {
            // adaptive memory management
            if (used_for_reference & pInfo->SliceHeader.sh_dec_refpic.adaptive_ref_pic_marking_mode_flag) {
                h264_dpb_adaptive_memory_management(pInfo);
            }
        }
        // Reset the active frame store - could have changed in mem management ftns
        h264_dpb_set_active_fs(p_dpb, p_dpb->fs_dec_idc);

        if ((viddec_h264_get_dec_structure(p_dpb->active_fs) == TOP_FIELD)||(viddec_h264_get_dec_structure(p_dpb->active_fs) == BOTTOM_FIELD))
        {
            // check for frame store with same pic_number -- always true in my case, YH
            // when we allocate frame store for the second field, we make sure the frame store for the second
            // field is the one that contains the first field of the frame- see h264_dpb_init_frame_store()
            // This is different from JM model.
            // In this way we don't need to move image data around and can reduce memory bandwidth.
            // simply check if the check if the other field has been decoded or not

            if (viddec_h264_get_is_used(p_dpb->active_fs) != 0)
            {
                if (pInfo->img.second_field)
                {
                    h264_dpb_insert_picture_in_dpb(pInfo, used_for_reference, 0, NonExisting, use_old);
                    second_field_stored = 1;
                }
            }
        }
    }
    else
    { // Set up locals for non-existing frames
        used_for_reference = 1;

        p_dpb->active_fs->frame.used_for_reference = used_for_reference?3:0;
        viddec_h264_set_is_frame_used(p_dpb->active_fs, 3);
        viddec_h264_set_dec_structure(p_dpb->active_fs, FRAME);
        pInfo->img.structure = FRAME;
    }

    is_direct_output = 0;
    if (NonExisting == 0)
    {
        if (p_dpb->used_size >= p_dpb->BumpLevel)
        {
            // non-reference frames may be output directly
            h264_dpb_set_active_fs(p_dpb, p_dpb->fs_dec_idc);

            if ((used_for_reference == 0) && (viddec_h264_get_is_used(p_dpb->active_fs) == 3))
            {
                h264_dpb_get_smallest_poc (p_dpb, &poc, &pos);
                h264_dpb_set_active_fs(p_dpb, p_dpb->fs_dec_idc);
                if ((pos == MPD_DPB_FS_NULL_IDC) || (pInfo->img.ThisPOC < poc))
                {
                    is_direct_output = 1;
                }
            }
        }
    }

    if (NonExisting) {
        h264_dpb_sliding_window_memory_management(p_dpb, NonExisting, pInfo->active_SPS.num_ref_frames);
    } else if (pInfo->SliceHeader.idr_flag == 0) {
        if (used_for_reference) {
            if (pInfo->img.second_field == 0) {
                if (pInfo->SliceHeader.sh_dec_refpic.adaptive_ref_pic_marking_mode_flag == 0) {
                    h264_dpb_sliding_window_memory_management(p_dpb, NonExisting, pInfo->active_SPS.num_ref_frames);
                }
            }
        }
    }

    h264_dpb_remove_unused_frame_from_dpb(p_dpb, &flag);

    //if (is_direct_output == 0)
    {
        if ((pInfo->img.second_field == 0) || (NonExisting))
        {
            h264_dpb_insert_picture_in_dpb(pInfo, used_for_reference, 1, NonExisting, use_old);
        }

        // In an errored stream we saw a condition where
        // p_dpb->ref_frames_in_buffer + p_dpb->ltref_frames_in_buffer > p_dpb->BumpLevel,
        // which in itself is an error, but this means first_field_non_ref will
        // not get set and causes problems for h264_dpb_queue_update()
        if ((pInfo->img.structure != FRAME) && (pInfo->img.second_field == 0)) {
            if (used_for_reference ==	0)
                if (p_dpb->ref_frames_in_buffer + p_dpb->ltref_frames_in_buffer == p_dpb->BumpLevel)
                    first_field_non_ref = 1;
        }

    }

    if (NonExisting)
        h264_dpb_set_active_fs(p_dpb, p_dpb->fs_non_exist_idc);
    else
        h264_dpb_set_active_fs(p_dpb, p_dpb->fs_dec_idc);

    if (NonExisting == 0)
    {
        if ((pInfo->img.second_field == 1) || (pInfo->img.structure == FRAME))
        {
            //h264_send_new_decoded_frame();
            if ((p_dpb->OutputCtrl) && (is_direct_output == 0))
                h264_dpb_output_one_frame_from_dpb(pInfo, 0, 0,pInfo->active_SPS.num_ref_frames);

            // Pictures inserted by this point - check if we have reached the specified output
            // level (if one has been specified) so we can begin on next call

            /*
            Fixed HSD 212625---------------should compare OutputLevel with non-output frame number in dpb, not the used number in dpb
            if((p_dpb->OutputLevelValid)&&(p_dpb->OutputCtrl == 0))
            {
            	if(p_dpb->used_size == p_dpb->OutputLevel)
            	p_dpb->OutputCtrl = 1;
            }
            */

            if (p_dpb->OutputLevelValid)
            {
                int32_t non_output_frame_number=0;
                non_output_frame_number = avc_dpb_get_non_output_frame_number(pInfo);

                if (non_output_frame_number == p_dpb->OutputLevel)
                    p_dpb->OutputCtrl = 1;
                else
                    p_dpb->OutputCtrl = 0;
            }
            else {
                p_dpb->OutputCtrl = 0;
            }
        }
    }

    while (p_dpb->used_size > (p_dpb->BumpLevel + first_field_non_ref))
        //while(p_dpb->used_size > p_dpb->BumpLevel)
    {
        h264_dpb_queue_update(pInfo, 1, 0, 0,pInfo->active_SPS.num_ref_frames); // flush a frame
        //h264_dpb_remove_unused_frame_from_dpb(p_dpb, &flag);
    }

    //
    // Do not output "direct output" pictures until the sempahore has been set that the pic is
    // decoded!!
    //
    if (is_direct_output) {
        h264_dpb_queue_update(pInfo, 1, 1, 0,pInfo->active_SPS.num_ref_frames);
        //h264_dpb_remove_unused_frame_from_dpb(p_dpb, &flag);
    }

    //
    // Add reference pictures into Reference list
    //
    if (used_for_reference) {
        h264_dpb_insert_ref_lists(&pInfo->dpb, NonExisting);
    }

    h264_dpb_remove_unused_frame_from_dpb(p_dpb, &flag);


    return;
} ////////////// End of DPB store pic


/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
//////////////////////////////////////////////////////////////////////////////
// h264_dpb_insert_picture_in_dpb ()
//
// Insert the decoded picture into the DPB. A free DPB position is necessary
// for frames, .
// This ftn tends to fill out the framestore's top level parameters from the
// storable picture's parameters within it. It is called from  h264_dpb_store_picture_in_dpb()
//
// This function finishes by updating the reference lists - this means it must be called after
// h264_dpb_sliding_window_memory_management()
//
// In the case of a frame it will call h264_dpb_split_field()
// In the case of the second field of a complementary field pair it calls h264_dpb_combine_field()
//

void h264_dpb_insert_picture_in_dpb(h264_Info * pInfo,int32_t used_for_reference, int32_t add2dpb, int32_t NonExisting, int32_t use_old)
{
    h264_DecodedPictureBuffer *p_dpb = &pInfo->dpb;

    if (NonExisting == 0) {
        h264_dpb_set_active_fs(p_dpb, p_dpb->fs_dec_idc);
        p_dpb->active_fs->frame_num = (use_old) ? pInfo->old_slice.frame_num : pInfo->SliceHeader.frame_num;
    }
    else {
        h264_dpb_set_active_fs(p_dpb, p_dpb->fs_non_exist_idc);
        p_dpb->active_fs->frame_num = p_dpb->active_fs->frame.pic_num;
    }

    if (add2dpb) {
        p_dpb->fs_dpb_idc[p_dpb->used_size] = p_dpb->active_fs->fs_idc;
        p_dpb->used_size++;
    }


    switch (viddec_h264_get_dec_structure(p_dpb->active_fs))
    {
    case FRAME : {
        viddec_h264_set_is_frame_used(p_dpb->active_fs, 3);
        p_dpb->active_fs->frame.used_for_reference = used_for_reference?3:0;
        if (used_for_reference)
        {
            p_dpb->active_fs->frame.used_for_reference = 3;
            if (p_dpb->active_fs->frame.is_long_term)
                viddec_h264_set_is_frame_long_term(p_dpb->active_fs, 3);
        }
        // Split frame to 2 fields for prediction
        h264_dpb_split_field(p_dpb, pInfo);

    }
    break;
    case TOP_FIELD : {
        viddec_h264_set_is_top_used(p_dpb->active_fs, 1);

        p_dpb->active_fs->top_field.used_for_reference = used_for_reference;
        if (used_for_reference)
        {
            p_dpb->active_fs->frame.used_for_reference |= 0x1;
            if (p_dpb->active_fs->top_field.is_long_term)
            {
                viddec_h264_set_is_top_long_term(p_dpb->active_fs, 1);
                p_dpb->active_fs->long_term_frame_idx = p_dpb->active_fs->top_field.long_term_frame_idx;
            }
        }
        if (viddec_h264_get_is_used(p_dpb->active_fs) == 3) {
            h264_dpb_combine_field(p_dpb, use_old); // generate frame view
        }
        else
        {
            p_dpb->active_fs->frame.poc      = p_dpb->active_fs->top_field.poc;
        }

    }
    break;
    case BOTTOM_FIELD : {
        viddec_h264_set_is_bottom_used(p_dpb->active_fs, 1);

        p_dpb->active_fs->bottom_field.used_for_reference = (used_for_reference<<1);
        if (used_for_reference)
        {
            p_dpb->active_fs->frame.used_for_reference |= 0x2;
            if (p_dpb->active_fs->bottom_field.is_long_term)
            {
                viddec_h264_set_is_bottom_long_term(p_dpb->active_fs, 1);
                p_dpb->active_fs->long_term_frame_idx = p_dpb->active_fs->bottom_field.long_term_frame_idx;
            }
        }
        if (viddec_h264_get_is_used(p_dpb->active_fs) == 3) {
            h264_dpb_combine_field(p_dpb, use_old); // generate frame view
        }
        else
        {
            p_dpb->active_fs->frame.poc = p_dpb->active_fs->bottom_field.poc;
        }

    }
    break;
    }
    /*
    	if ( gRestartMode.LastRestartType  == RESTART_SEI )
    	{
    		if ( p_dpb->active_fs->open_gop_entry ) dpb.WaitSeiRecovery = 1;
    	}

    	gRestartMode.LastRestartType = 0xFFFF;
    */

    return;
} ////// End of insert picture in DPB

/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
//////////////////////////////////////////////////////////////////////////////
// h264_dpb_mm_unmark_short_term_for_reference ()
//
// Adaptive Memory Management: Mark short term picture unused
//

void h264_dpb_mm_unmark_short_term_for_reference(h264_Info * pInfo, int32_t difference_of_pic_nums_minus1)
{
    int32_t picNumX;
    int32_t currPicNum;
    uint32_t idx;
    int32_t unmark_done;
    h264_DecodedPictureBuffer *p_dpb = &pInfo->dpb;

    if (pInfo->img.structure == FRAME)
        currPicNum = pInfo->img.frame_num;
    else
        currPicNum = (pInfo->img.frame_num << 1) + 1;

    picNumX = currPicNum - (difference_of_pic_nums_minus1 + 1);

    unmark_done = 0;

    for (idx =0; (idx < p_dpb->ref_frames_in_buffer) && (!(unmark_done)); idx++)
    {
        h264_dpb_set_active_fs(p_dpb, p_dpb->fs_ref_idc[idx]);

        if (pInfo->img.structure == FRAME)
        {
            /* If all pic numbers in the list are different (and they should be)
               we should terminate the for loop the moment we match pic numbers,
               no need to continue to check - hence set unmark_done
            */

            if ((p_dpb->active_fs->frame.used_for_reference == 3) && (viddec_h264_get_is_long_term(p_dpb->active_fs) == 0) &&
                    (p_dpb->active_fs->frame.pic_num == picNumX))
            {
                h264_dpb_unmark_for_reference(p_dpb, p_dpb->active_fs->fs_idc);
                h264_dpb_remove_ref_list(p_dpb, p_dpb->active_fs->fs_idc);
                unmark_done = 1;
            }
        }
        else
        {
            /*
               If we wish to unmark a short-term picture by picture number when the current picture
               is a field, we have to unmark the corresponding field as unused for reference,
               and also if it was part of a frame or complementary reference field pair, the
               frame is to be marked as unused. However the opposite field may still be used as a
               reference for future fields

               How will this affect the reference list update ftn coming after??

            */
            if ((p_dpb->active_fs->frame.used_for_reference&0x1) && (!(viddec_h264_get_is_long_term(p_dpb->active_fs)&0x01))&&
                    (p_dpb->active_fs->top_field.pic_num == picNumX) )
            {
                p_dpb->active_fs->top_field.used_for_reference = 0;
                p_dpb->active_fs->frame.used_for_reference &= 2;

                unmark_done = 1;

                //Check if other field is used for short-term reference, if not remove from list...
                if (p_dpb->active_fs->bottom_field.used_for_reference == 0)
                    h264_dpb_remove_ref_list(p_dpb, p_dpb->fs_ref_idc[idx]);
            }
            if ((p_dpb->active_fs->frame.used_for_reference&0x2) && (!(viddec_h264_get_is_long_term(p_dpb->active_fs)&0x2)) &&
                    (p_dpb->active_fs->bottom_field.pic_num == picNumX) )
            {
                p_dpb->active_fs->bottom_field.used_for_reference = 0;
                p_dpb->active_fs->frame.used_for_reference &= 1;

                unmark_done = 1;

                //Check if other field is used for reference, if not remove from list...
                if (p_dpb->active_fs->top_field.used_for_reference == 0)
                    h264_dpb_remove_ref_list(p_dpb, p_dpb->fs_ref_idc[idx]);
            }
        }
    }

    return;
}

/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
////////////////////////////////////////////////////////////////////////////////////
// h264_dpb_mm_unmark_long_term_for_reference ()
//
// Adaptive Memory Management: Mark long term picture unused
//
// In a frame situation the long_term_pic_num will refer to another frame.
// Thus we can call h264_dpb_unmark_for_long_term_reference() and then remove the picture
// from the list
//
// If the current picture is a field, long_term_pic_num will refer to another field
// It is also the case that each individual field should have a unique picture number
// 8.2.5.4.2 suggests that when curr pic is a field, an mmco == 2 operation
// should be accompanied by a second op to unmark the other field as being unused
///////////////////////////////////////////////////////////////////////////////////

void h264_dpb_mm_unmark_long_term_for_reference (h264_Info * pInfo, int32_t long_term_pic_num)
{
    uint32_t idx;
    int32_t unmark_done;
    h264_DecodedPictureBuffer *p_dpb = &pInfo->dpb;

    unmark_done = 0;
    for (idx = 0; (idx < p_dpb->ltref_frames_in_buffer) && (!(unmark_done)); idx++)
    {
        h264_dpb_set_active_fs(p_dpb, p_dpb->fs_ltref_idc[idx]);

        if (pInfo->img.structure == FRAME)
        {
            if ((p_dpb->active_fs->frame.used_for_reference==3) && (viddec_h264_get_is_long_term(p_dpb->active_fs)==3) &&
                    (p_dpb->active_fs->frame.long_term_pic_num == long_term_pic_num))
            {
                h264_dpb_unmark_for_long_term_reference(p_dpb, p_dpb->fs_ltref_idc[idx]);
                h264_dpb_remove_ltref_list(p_dpb, p_dpb->fs_ltref_idc[idx]);
                unmark_done = 1;
            }
        }
        else
        {
            /// Check top field
            if ((p_dpb->active_fs->frame.used_for_reference&0x1) && (viddec_h264_get_is_long_term(p_dpb->active_fs)&0x1) &&
                    (p_dpb->active_fs->top_field.long_term_pic_num == long_term_pic_num) )
            {
                p_dpb->active_fs->top_field.used_for_reference = 0;
                p_dpb->active_fs->top_field.is_long_term = 0;
                p_dpb->active_fs->frame.used_for_reference &= 2;
                viddec_h264_set_is_frame_long_term(p_dpb->active_fs, 2);

                unmark_done = 1;

                //Check if other field is used for long term reference, if not remove from list...
                if ((p_dpb->active_fs->bottom_field.used_for_reference == 0) || (p_dpb->active_fs->bottom_field.is_long_term == 0))
                    h264_dpb_remove_ltref_list(p_dpb, p_dpb->fs_ltref_idc[idx]);
            }

            /// Check Bottom field
            if ((p_dpb->active_fs->frame.used_for_reference&0x2) && (viddec_h264_get_is_long_term(p_dpb->active_fs)&0x2) &&
                    (p_dpb->active_fs->bottom_field.long_term_pic_num == long_term_pic_num) )
            {
                p_dpb->active_fs->bottom_field.used_for_reference = 0;
                p_dpb->active_fs->bottom_field.is_long_term = 0;
                p_dpb->active_fs->frame.used_for_reference &= 1;
                viddec_h264_set_is_frame_long_term(p_dpb->active_fs, 1);

                unmark_done = 1;
                //Check if other field is used for long term reference, if not remove from list...
                if ((p_dpb->active_fs->top_field.used_for_reference == 0) || (p_dpb->active_fs->top_field.is_long_term == 0))
                {
                    h264_dpb_remove_ltref_list(p_dpb, p_dpb->fs_ltref_idc[idx]);
                }
            }
        } // field structure
    } //for(idx)

    return;
}
/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
//////////////////////////////////////////////////////////////////////////////
// h264_dpb_get_pic_struct_by_pic_num
//
// Searches the fields appearing in short term reference list
// Returns the polarity of the field with pic_num = picNumX
//////////////////////////////////////////////////////////////////////////////

int32_t h264_dpb_get_pic_struct_by_pic_num(h264_DecodedPictureBuffer *p_dpb, int32_t picNumX)
{
    uint32_t idx;
    int32_t pic_struct = INVALID;
    int32_t found = 0;

    for (idx =0; (idx < p_dpb->ref_frames_in_buffer) && (!(found)); idx++)
    {
        h264_dpb_set_active_fs(p_dpb, p_dpb->fs_ref_idc[idx]);

        if ((p_dpb->active_fs->frame.used_for_reference&0x1) && (!(viddec_h264_get_is_long_term(p_dpb->active_fs)&0x01))&&
                (p_dpb->active_fs->top_field.pic_num == picNumX) )
        {
            found = 1;
            pic_struct = TOP_FIELD;

        }
        if ((p_dpb->active_fs->frame.used_for_reference&0x2) && (!(viddec_h264_get_is_long_term(p_dpb->active_fs)&0x2)) &&
                (p_dpb->active_fs->bottom_field.pic_num == picNumX) )
        {
            found = 1;
            pic_struct = BOTTOM_FIELD;

        }
    }

    return pic_struct;
}
/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
//////////////////////////////////////////////////////////////////////////////
// h264_dpb_mm_assign_long_term_frame_idx ()
//
// Assign a long term frame index to a short term picture
// Both lists must be updated as part of this process...
//////////////////////////////////////////////////////////////////////////////

void h264_dpb_mm_assign_long_term_frame_idx(h264_Info * pInfo, int32_t difference_of_pic_nums_minus1, int32_t long_term_frame_idx)
{
    h264_DecodedPictureBuffer *p_dpb = &pInfo->dpb;
    int32_t picNumX;
    int32_t currPicNum;
    int32_t polarity = 0;

    if (pInfo->img.structure == FRAME) {
        currPicNum = pInfo->img.frame_num;
    } else {
        currPicNum = (pInfo->img.frame_num << 1) + 1;
    }

    picNumX = currPicNum - (difference_of_pic_nums_minus1 + 1);

    // remove frames / fields with same long_term_frame_idx
    if (pInfo->img.structure == FRAME) {
        h264_dpb_unmark_long_term_frame_for_reference_by_frame_idx(p_dpb, long_term_frame_idx);
    } else {
        polarity = h264_dpb_get_pic_struct_by_pic_num(p_dpb, picNumX);

        if (polarity != INVALID)
            h264_dpb_unmark_long_term_field_for_reference_by_frame_idx(p_dpb, long_term_frame_idx, p_dpb->active_fs->fs_idc, polarity);
    }

    h264_dpb_mark_pic_long_term(pInfo, long_term_frame_idx, picNumX);

    return;
}
/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
//////////////////////////////////////////////////////////////////////////////
// h264_dpb_mm_update_max_long_term_frame_idx ()
//
// Set new max long_term_frame_idx
//

void h264_dpb_mm_update_max_long_term_frame_idx(h264_DecodedPictureBuffer *p_dpb,int32_t max_long_term_frame_idx_plus1)
{
    //h264_DecodedPictureBuffer *p_dpb = &pInfo->dpb;
    int32_t idx;
    int32_t temp;
    int32_t removed_count;
    int32_t idx2 = 0;

    p_dpb->max_long_term_pic_idx = max_long_term_frame_idx_plus1 - 1;

    temp = p_dpb->ltref_frames_in_buffer;
    removed_count = 0;

    // check for invalid frames
    for (idx = 0; idx < temp; idx++)
    {
        idx2 = idx - removed_count;
        if (idx2 < 16 && idx2 > 0)
        {
            h264_dpb_set_active_fs(p_dpb, p_dpb->fs_ltref_idc[idx2]);

            if (p_dpb->active_fs->long_term_frame_idx > p_dpb->max_long_term_pic_idx)
            {
                removed_count++;
                h264_dpb_unmark_for_long_term_reference(p_dpb, p_dpb->fs_ltref_idc[idx2]);
                h264_dpb_remove_ltref_list(p_dpb, p_dpb->fs_ltref_idc[idx2]);
            }
        }
    }
    return;
}
/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
//////////////////////////////////////////////////////////////////////////////
// h264_dpb_mm_unmark_all_short_term_for_reference ()
//
// Unmark all short term refernce pictures
//

void h264_dpb_mm_unmark_all_short_term_for_reference (h264_DecodedPictureBuffer *p_dpb)
{
    int32_t idx;
    int32_t temp = p_dpb->ref_frames_in_buffer;

    for (idx = 0; idx < temp; idx++)
    {
        h264_dpb_unmark_for_reference(p_dpb, p_dpb->fs_ref_idc[0]);
        h264_dpb_remove_ref_list(p_dpb, p_dpb->fs_ref_idc[0]);
    }
    return;
}
/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
//////////////////////////////////////////////////////////////////////////////
// h264_dpb_mm_mark_current_picture_long_term ()
//
// Marks the current picture as long term after unmarking any long term picture
// already assigned with the same long term frame index
//

void h264_dpb_mm_mark_current_picture_long_term(h264_DecodedPictureBuffer *p_dpb, int32_t long_term_frame_idx)
{
    int32_t picNumX;
    h264_dpb_set_active_fs(p_dpb, p_dpb->fs_dec_idc);

    if (viddec_h264_get_dec_structure(p_dpb->active_fs) == FRAME)
    {
        h264_dpb_unmark_long_term_frame_for_reference_by_frame_idx(p_dpb, long_term_frame_idx);
        h264_dpb_set_active_fs(p_dpb, p_dpb->fs_dec_idc);
        p_dpb->active_fs->frame.is_long_term        = 1;
        p_dpb->active_fs->frame.long_term_frame_idx = long_term_frame_idx;
        p_dpb->active_fs->frame.long_term_pic_num   = long_term_frame_idx;
    }
    else
    {
        if (viddec_h264_get_dec_structure(p_dpb->active_fs) == TOP_FIELD)
        {
            picNumX = (p_dpb->active_fs->top_field.pic_num << 1) + 1;
            p_dpb->active_fs->top_field.is_long_term        = 1;
            p_dpb->active_fs->top_field.long_term_frame_idx = long_term_frame_idx;

            // Assign long-term pic num
            p_dpb->active_fs->top_field.long_term_pic_num   = (long_term_frame_idx << 1) + 1;
        }
        else
        {
            picNumX = (p_dpb->active_fs->bottom_field.pic_num << 1) + 1;
            p_dpb->active_fs->bottom_field.is_long_term        = 1;
            p_dpb->active_fs->bottom_field.long_term_frame_idx = long_term_frame_idx;

            // Assign long-term pic num
            p_dpb->active_fs->bottom_field.long_term_pic_num   = (long_term_frame_idx << 1) + 1;

        }
        h264_dpb_unmark_long_term_field_for_reference_by_frame_idx(p_dpb, long_term_frame_idx, p_dpb->fs_dec_idc, viddec_h264_get_dec_structure(p_dpb->active_fs));
    }
    // Add to long term list
    //h264_dpb_add_ltref_list(p_dpb->fs_dec_idc);

    return;
}
/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
//////////////////////////////////////////////////////////////////////////////
// h264_dpb_unmark_long_term_frame_for_reference_by_frame_idx ()
//
// Mark a long-term reference frame or complementary field pair unused for referemce
// NOTE: Obviously this ftn cannot be used to unmark individual fields...
//////////////////////////////////////////////////////////////////////////////

void h264_dpb_unmark_long_term_frame_for_reference_by_frame_idx(h264_DecodedPictureBuffer *p_dpb, int32_t long_term_frame_idx)
{
    uint32_t idx;
    for (idx =0; idx < p_dpb->ltref_frames_in_buffer; idx++)
    {
        h264_dpb_set_active_fs(p_dpb, p_dpb->fs_ltref_idc[idx]);

        if (p_dpb->active_fs->long_term_frame_idx == long_term_frame_idx)
        {
            h264_dpb_unmark_for_long_term_reference(p_dpb, p_dpb->fs_ltref_idc[idx]);
            h264_dpb_remove_ltref_list(p_dpb, p_dpb->fs_ltref_idc[idx]);
        }
    }
    return;
}

/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
//////////////////////////////////////////////////////////////////////////////
// h264_dpb_unmark_long_term_field_for_reference_by_frame_idx ()
//
// Mark a long-term reference field unused for reference. However if it is the
// complementary field (opposite polarity) of the picture stored in fs_idc,
// we do not unmark it
//////////////////////////////////////////////////////////////////////////////

void h264_dpb_unmark_long_term_field_for_reference_by_frame_idx(h264_DecodedPictureBuffer *p_dpb, int32_t long_term_frame_idx, int32_t fs_idc, int32_t polarity)
{
    uint32_t idx;
    int32_t found = 0;
    int32_t is_complement = 0;

    for (idx = 0; (idx < p_dpb->ltref_frames_in_buffer) && (found == 0); idx++)
    {
        h264_dpb_set_active_fs(p_dpb, p_dpb->fs_ltref_idc[idx]);
        if (p_dpb->active_fs->long_term_frame_idx == long_term_frame_idx)
        {
            if (p_dpb->active_fs->fs_idc == fs_idc)
            {
                // Again these seem like redundant checks but for safety while until JM is updated
                if (polarity == TOP_FIELD)
                    is_complement = (p_dpb->active_fs->bottom_field.is_long_term)? 1:0;
                else if (polarity == BOTTOM_FIELD)
                    is_complement = (p_dpb->active_fs->top_field.is_long_term)   ? 1:0;
            }
            found = 1;
        }
    }

    if (found) {
        if (is_complement == 0)
        {
            h264_dpb_unmark_for_long_term_reference(p_dpb, p_dpb->fs_ltref_idc[idx-1]);
            h264_dpb_remove_ltref_list(p_dpb, p_dpb->fs_ltref_idc[idx-1]);
        }
    }

    return;
}


/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
//////////////////////////////////////////////////////////////////////////////
// h264_dpb_mark_pic_long_term ()
//
// This is used on a picture already in the dpb - i.e. not for the current picture
// dpb_split / dpb_combine field will perform ftnality in that case
//
// Marks a picture as used for long-term reference. Adds it to the long-term
// reference list. Also removes it from the short term reference list if required
//
// Note: if the current picture is a frame, the picture to be marked will be a
// short-term reference frame or short-term complemenetary reference field pair
// We use the pic_num assigned to the frame part of the structure to locate it
// Both its fields will have their long_term_frame_idx and long_term_pic_num
// assigned to be equal to long_term_frame_idx
//
// If the current picture is a field, the picture to be marked will be a
// short-term reference field. We use the pic_nums assigned to the field parts of
// the structure to identify the appropriate field. We assign the long_term_frame_idx
// of the field equal to long_term_frame_idx.
//
// We also check to see if this marking has resulted in both fields of the frame
// becoming long_term. If it has, we update the frame part of the structure by
// setting its long_term_frame_idx
//////////////////////////////////////////////////////////////////////////////

void h264_dpb_mark_pic_long_term(h264_Info * pInfo, int32_t long_term_frame_idx, int32_t picNumX)
{
    h264_DecodedPictureBuffer *p_dpb = &pInfo->dpb;
    uint32_t idx;
    int32_t mark_done;
    int32_t polarity = 0;

    mark_done = 0;

    if (pInfo->img.structure == FRAME)
    {
        for (idx = 0; (idx < p_dpb->ref_frames_in_buffer) && (!(mark_done)); idx++)
        {
            h264_dpb_set_active_fs(p_dpb, p_dpb->fs_ref_idc[idx]);

            if (p_dpb->active_fs->frame.used_for_reference == 3)
            {
                if ((!(p_dpb->active_fs->frame.is_long_term))&&(p_dpb->active_fs->frame.pic_num == picNumX))
                {
                    p_dpb->active_fs->long_term_frame_idx = long_term_frame_idx;
                    p_dpb->active_fs->frame.long_term_frame_idx = long_term_frame_idx;
                    p_dpb->active_fs->top_field.long_term_frame_idx = long_term_frame_idx;
                    p_dpb->active_fs->bottom_field.long_term_frame_idx = long_term_frame_idx;

                    p_dpb->active_fs->frame.is_long_term = 1;
                    p_dpb->active_fs->top_field.is_long_term = 1;
                    p_dpb->active_fs->bottom_field.is_long_term = 1;

                    viddec_h264_set_is_frame_long_term(p_dpb->active_fs, 3);
                    mark_done = 1;

                    // Assign long-term pic num
                    p_dpb->active_fs->frame.long_term_pic_num   = long_term_frame_idx;
                    p_dpb->active_fs->top_field.long_term_pic_num    = long_term_frame_idx;
                    p_dpb->active_fs->bottom_field.long_term_pic_num = long_term_frame_idx;
                    // Add to long term list
                    h264_dpb_add_ltref_list(p_dpb, p_dpb->fs_ref_idc[idx]);
                    // Remove from short-term list
                    h264_dpb_remove_ref_list(p_dpb, p_dpb->fs_ref_idc[idx]);
                }
            }
        }
    }
    else
    {
        polarity = h264_dpb_get_pic_struct_by_pic_num(p_dpb, picNumX);
        p_dpb->active_fs->long_term_frame_idx = long_term_frame_idx;         /////BUG

        if (polarity == TOP_FIELD)
        {
            p_dpb->active_fs->top_field.long_term_frame_idx = long_term_frame_idx;
            p_dpb->active_fs->top_field.is_long_term        = 1;
            viddec_h264_set_is_top_long_term(p_dpb->active_fs, 1);

            // Assign long-term pic num
            p_dpb->active_fs->top_field.long_term_pic_num   = (long_term_frame_idx << 1) + ((pInfo->img.structure == TOP_FIELD) ? 1 : 0);

        }
        else if (polarity == BOTTOM_FIELD)
        {
            p_dpb->active_fs->bottom_field.long_term_frame_idx = long_term_frame_idx;
            p_dpb->active_fs->bottom_field.is_long_term        = 1;
            viddec_h264_set_is_bottom_long_term(p_dpb->active_fs, 1);

            // Assign long-term pic num
            p_dpb->active_fs->bottom_field.long_term_pic_num   = (long_term_frame_idx << 1) + ((pInfo->img.structure == BOTTOM_FIELD) ? 1 : 0);
        }

        if (viddec_h264_get_is_long_term(p_dpb->active_fs) == 3)
        {
            p_dpb->active_fs->frame.is_long_term = 1;
            p_dpb->active_fs->frame.long_term_frame_idx = long_term_frame_idx;
            h264_dpb_remove_ref_list(p_dpb, p_dpb->active_fs->fs_idc);
        }
        else
        {
            // We need to add this idc to the long term ref list...
            h264_dpb_add_ltref_list(p_dpb, p_dpb->active_fs->fs_idc);

            // If the opposite field is not a short term reference, remove it from the
            // short term list. Since we know top field is a reference but both are not long term
            // we can simply check that both fields are not references...
            if (p_dpb->active_fs->frame.used_for_reference != 3)
                h264_dpb_remove_ref_list(p_dpb, p_dpb->active_fs->fs_idc);
        }
    }
    return;
} ///// End of mark pic long term


/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
//////////////////////////////////////////////////////////////////////////////
// h264_dpb_adaptive_memory_management ()
//
// Perform Adaptive memory control decoded reference picture marking process
//////////////////////////////////////////////////////////////////////////////

void h264_dpb_adaptive_memory_management (h264_Info * pInfo)
{
    h264_DecodedPictureBuffer *p_dpb = &pInfo->dpb;
    int32_t idx;

    idx = 0;

    while (idx < pInfo->SliceHeader.sh_dec_refpic.dec_ref_pic_marking_count)
    {
        switch (pInfo->SliceHeader.sh_dec_refpic.memory_management_control_operation[idx])
        {
        case   1: {	//Mark a short-term reference picture as	�unused for reference?
            h264_dpb_mm_unmark_short_term_for_reference(pInfo,
                    pInfo->SliceHeader.sh_dec_refpic.difference_of_pic_num_minus1[idx]);
        }
        break;
        case   2: {	//Mark a long-term reference picture as 	�unused for reference?
            h264_dpb_mm_unmark_long_term_for_reference(pInfo,
                    pInfo->SliceHeader.sh_dec_refpic.long_term_pic_num[idx]);
        }
        break;
        case  3: {		//Mark a short-term reference picture as	"used for long-term reference" and assign a long-term frame index to it
            h264_dpb_mm_assign_long_term_frame_idx(pInfo,
                                                   pInfo->SliceHeader.sh_dec_refpic.difference_of_pic_num_minus1[idx],
                                                   pInfo->SliceHeader.sh_dec_refpic.long_term_frame_idx[idx]);
        }
        break;
        case  4: {	//Specify the maximum long-term frame index and
            //mark all long-term reference pictureshaving long-term frame indices greater than
            //the maximum value as "unused for reference"
            h264_dpb_mm_update_max_long_term_frame_idx (&pInfo->dpb,
                    pInfo->SliceHeader.sh_dec_refpic.max_long_term_frame_idx_plus1[idx]);
        }
        break;
        case  5: {		//Mark all reference pictures as	"unused for reference" and set the MaxLongTermFrameIdx variable to
            // "no long-term frame indices"
            h264_dpb_mm_unmark_all_short_term_for_reference(&pInfo->dpb);
            h264_dpb_mm_update_max_long_term_frame_idx(&pInfo->dpb, 0);
            pInfo->img.last_has_mmco_5 = 1;
        }
        break;
        case   6: {	//Mark the current picture as	"used for long-term reference" and assign a long-term frame index to it
            h264_dpb_mm_mark_current_picture_long_term(&pInfo->dpb,
                    pInfo->SliceHeader.sh_dec_refpic.long_term_frame_idx[idx]);
        }
        break;
        }
        idx++;
    }


    if (pInfo->img.last_has_mmco_5)
    {
        pInfo->img.frame_num = 0;
        pInfo->SliceHeader.frame_num=0;
        h264_dpb_set_active_fs(p_dpb, p_dpb->fs_dec_idc);

        if (viddec_h264_get_dec_structure(p_dpb->active_fs) == FRAME)
        {
            pInfo->img.bottompoc -= p_dpb->active_fs->frame.poc;
            pInfo->img.toppoc    -= p_dpb->active_fs->frame.poc;


            p_dpb->active_fs->frame.poc = 0;
            p_dpb->active_fs->frame.pic_num = 0;
            p_dpb->active_fs->frame_num = 0;
        }

        else if (viddec_h264_get_dec_structure(p_dpb->active_fs) == TOP_FIELD)
        {
            p_dpb->active_fs->top_field.poc = p_dpb->active_fs->top_field.pic_num = 0;
            pInfo->img.toppoc = p_dpb->active_fs->top_field.poc;
        }
        else if (viddec_h264_get_dec_structure(p_dpb->active_fs) == BOTTOM_FIELD)
        {
            p_dpb->active_fs->bottom_field.poc = p_dpb->active_fs->bottom_field.pic_num = 0;
            pInfo->img.bottompoc = 0;
        }

        h264_dpb_flush_dpb(pInfo, 1, pInfo->img.second_field,pInfo->active_SPS.num_ref_frames);
    }
    // Reset the marking count operations for the current picture...
    pInfo->SliceHeader.sh_dec_refpic.dec_ref_pic_marking_count = 0;

    return;
} ////// End of adaptive memory management

/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
//////////////////////////////////////////////////////////////////////////////
// h264_dpb_gaps_in_frame_num_mem_management ()
//
// Produces a set of frame_nums pertaining to "non-existing" pictures
// Calls h264_dpb_store_picture_in_dpb
//////////////////////////////////////////////////////////////////////////////

void h264_dpb_gaps_in_frame_num_mem_management(h264_Info * pInfo)
{
    int32_t		temp_frame_num = 0;
    int32_t		idx, prev_idc;
    int32_t 	prev_frame_num_plus1_wrap;
    uint32_t	temp = 0;
    int32_t MaxFrameNum = 1 << (pInfo->active_SPS.log2_max_frame_num_minus4 + 4);
    seq_param_set_used_ptr  active_sps = &pInfo->active_SPS;
    h264_DecodedPictureBuffer *p_dpb = &pInfo->dpb;

    pInfo->img.gaps_in_frame_num = 0;

    // pInfo->img.last_has_mmco_5 set thru store_picture_in_dpb
    if (pInfo->img.last_has_mmco_5)
    {
        // If the previous picture was an unpaired field, mark it as a dangler
        if (p_dpb->used_size)
        {
            idx = p_dpb->used_size-1;
            prev_idc = p_dpb->fs_dpb_idc[idx];
            if (prev_idc != MPD_DPB_FS_NULL_IDC)
            {
                h264_dpb_set_active_fs(p_dpb, p_dpb->fs_dpb_idc[idx]);
                p_dpb->active_fs->frame_num =0;
            }
        }
        pInfo->img.PreviousFrameNumOffset = 0;
        //CONFORMANCE_ISSUE
        pInfo->img.PreviousFrameNum = 0;

    }

    // Check for gaps in frame_num
    if (pInfo->SliceHeader.idr_flag) {
        pInfo->img.PreviousFrameNum = pInfo->img.frame_num;
    }
    // Have we re-started following a recovery point message?
    /*
    	else if(got_sei_recovery || aud_got_restart){
    		pInfo->img.PreviousFrameNum = pInfo->img.frame_num;
    		//got_sei_recovery = 0;
    		//aud_got_restart  = 0;
    	}
    */
    else if (pInfo->img.frame_num != pInfo->img.PreviousFrameNum)
    {
        if (MaxFrameNum) {
            ldiv_mod_u((uint32_t)(pInfo->img.PreviousFrameNum + 1), (uint32_t)MaxFrameNum, &temp);
        } else {
            temp = (uint32_t)pInfo->img.PreviousFrameNum + 1;
        }
        prev_frame_num_plus1_wrap = temp;
        if (pInfo->img.frame_num != prev_frame_num_plus1_wrap)
        {
            pInfo->img.gaps_in_frame_num = (pInfo->img.frame_num < pInfo->img.PreviousFrameNum)? ((MaxFrameNum + pInfo->img.frame_num -1) - pInfo->img.PreviousFrameNum): (pInfo->img.frame_num - pInfo->img.PreviousFrameNum - 1);
            // We should test for an error here - should infer an unintentional loss of pictures
        }
    }


    //if(active_sps->gaps_in_frame_num_value_allowed_flag == 0) {
    if (pInfo->img.gaps_in_frame_num && (active_sps->gaps_in_frame_num_value_allowed_flag == 0)) {
        // infer an unintentional loss of pictures
        // only invoke following process for a conforming bitstream
        // when gaps_in_frame_num_value_allowed_flag is equal to 1
        pInfo->img.gaps_in_frame_num = 0;
#ifdef VBP
#ifdef SW_ERROR_CONCEALEMNT
        pInfo->sw_bail = 1;
#endif
#endif
        //mfd_printf("ERROR STREAM??\n");
        ////// Error handling here----
    }

    /////// Removed following OLO source (Sodaville H.D)
    //else if (pInfo->img.gaps_in_frame_num  > active_sps->num_ref_frames) {
    //	// No need to produce any more non-existent frames than the amount required to flush the dpb
    //	pInfo->img.gaps_in_frame_num = active_sps->num_ref_frames;
    //mfd_printf("gaps in frame: %d\n", gaps_in_frame_num);
    //}

    // If the previous picture was an unpaired field, mark it as a dangler
    if (p_dpb->used_size)
    {
        idx = p_dpb->used_size-1;
        prev_idc = p_dpb->fs_dpb_idc[idx];
        if (prev_idc != MPD_DPB_FS_NULL_IDC)
        {
            h264_dpb_set_active_fs(p_dpb, p_dpb->fs_dpb_idc[idx]);
            if (viddec_h264_get_is_used(p_dpb->active_fs) != 3) {
                h264_dpb_mark_dangling_field(p_dpb, p_dpb->active_fs->fs_idc);  //, DANGLING_TYPE_GAP_IN_FRAME
            }
        }
    }

    while (temp_frame_num < pInfo->img.gaps_in_frame_num)
    {
        h264_dpb_assign_frame_store(pInfo, 1);

        // Set up initial markings - not sure if all are needed
        viddec_h264_set_dec_structure(p_dpb->active_fs, FRAME);

        if (MaxFrameNum)
            ldiv_mod_u((uint32_t)(pInfo->img.PreviousFrameNum + 1), (uint32_t)MaxFrameNum, &temp);

        p_dpb->active_fs->frame.pic_num = temp;
        p_dpb->active_fs->long_term_frame_idx        = 0;
        p_dpb->active_fs->frame.long_term_pic_num    = 0;
        viddec_h264_set_is_frame_long_term(p_dpb->active_fs, 0);

        // Note the call below will overwrite some aspects of the img structure with info relating to the
        // non-existent picture
        // However, since this is called before h264_hdr_decoding_poc() for the current existing picture
        // it should be o.k.
        if (pInfo->img.pic_order_cnt_type)
            h264_hdr_decoding_poc(pInfo, 1, temp);

        pInfo->img.structure = FRAME;
        p_dpb->active_fs->frame.poc = pInfo->img.framepoc;

        // call store_picture_in_dpb

        h264_dpb_store_previous_picture_in_dpb(pInfo, 1, 0);

        h264_hdr_post_poc(pInfo, 1, temp, 0);

        temp_frame_num++;
    }
}

/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */

//////////////////////////////////////////////////////////////////////////////
// h264_dpb_unmark_for_reference ()
//
// Mark FrameStore unused for reference. Removes it from the short term reference list
//////////////////////////////////////////////////////////////////////////////

void h264_dpb_unmark_for_reference(h264_DecodedPictureBuffer *p_dpb, int32_t fs_idc)
{
    h264_dpb_set_active_fs(p_dpb, fs_idc);

    if (viddec_h264_get_is_used(p_dpb->active_fs)&0x1)  p_dpb->active_fs->top_field.used_for_reference = 0;
    if (viddec_h264_get_is_used(p_dpb->active_fs)&0x2)  p_dpb->active_fs->bottom_field.used_for_reference = 0;
    if (viddec_h264_get_is_used(p_dpb->active_fs) == 3) p_dpb->active_fs->frame.used_for_reference = 0;

    p_dpb->active_fs->frame.used_for_reference = 0;
    return;
}


/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
//////////////////////////////////////////////////////////////////////////////
// h264_dpb_unmark_for_long_term_reference ()
//
// mark FrameStore unused for reference and reset long term flags
// This function does not remove it form the long term list
//////////////////////////////////////////////////////////////////////////////

void h264_dpb_unmark_for_long_term_reference(h264_DecodedPictureBuffer *p_dpb, int32_t fs_idc)
{
    h264_dpb_set_active_fs(p_dpb, fs_idc);

    if (viddec_h264_get_is_used(p_dpb->active_fs)&0x1)
    {
        p_dpb->active_fs->top_field.used_for_reference = 0;
        p_dpb->active_fs->top_field.is_long_term = 0;
    }

    if (viddec_h264_get_is_used(p_dpb->active_fs)&0x2)
    {
        p_dpb->active_fs->bottom_field.used_for_reference = 0;
        p_dpb->active_fs->bottom_field.is_long_term = 0;
    }
    if (viddec_h264_get_is_used(p_dpb->active_fs) == 3)
    {
        p_dpb->active_fs->frame.used_for_reference = 0;
        p_dpb->active_fs->frame.is_long_term = 0;
    }

    p_dpb->active_fs->frame.used_for_reference = 0;
    viddec_h264_set_is_frame_long_term(p_dpb->active_fs, 0);

    return;
}


/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
//////////////////////////////////////////////////////////////////////////////
// h264_dpb_mark_dangling_field
//
// Tells HW previous field was dangling
// Marks it in SW as so
// Takes appropriate actions. - sys_data needs thought through...
//////////////////////////////////////////////////////////////////////////////

void h264_dpb_mark_dangling_field(h264_DecodedPictureBuffer *p_dpb, int32_t fs_idc)
{

    h264_dpb_set_active_fs(p_dpb, fs_idc);

    //PRINTF(MFD_NONE, " fs_idc = %d DANGLING_TYPE = %d \n", fs_idc,  reason);
    /*
    Make the check that it has not already been marked
    This covers the situation of a dangling field followed by a
    frame which is direct output (i.e. never entered into the dpb).
    In this case we could attempt  to mark the prev unpaired field
    as a dangler twice which would upset the HW dpb_disp_q count
    */

    if (viddec_h264_get_is_dangling(p_dpb->active_fs) == 0)
    {
        switch (viddec_h264_get_dec_structure(p_dpb->active_fs))
        {
        case TOP_FIELD:
            viddec_h264_set_is_dangling(p_dpb->active_fs, 1);
            //PRINTF(MFD_NONE,  "FN:%d  fs_idc=%d  FRAME_FLAG_DANGLING_TOP_FIELD\n ", (h264_frame_number+1), p_dpb->active_fs->fs_idc);
            break;
        case BOTTOM_FIELD:
            //PRINTF(MFD_NONE,  " FN:%d  fs_idc=%d  FRAME_FLAG_DANGLING_BOTTOM_FIELD \n ", (h264_frame_number+1), p_dpb->active_fs->fs_idc);
            viddec_h264_set_is_dangling(p_dpb->active_fs, 1);
            break;
        default:
            //PRINTF(MFD_NONE,  "FN:%d  fs_idc=%d  DANGLING: FATAL_ERROR\n ", (h264_frame_number+1), p_dpb->active_fs->fs_idc);
            break;
        }

        //h264_send_new_decoded_frame();
    }
    return;
}

/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */


//////////////////////////////////////////////////////////////////////////////
// h264_dpb_is_used_for_reference ()
//
// Check if one of the frames/fields in active_fs is used for reference
//
void h264_dpb_is_used_for_reference(h264_DecodedPictureBuffer *p_dpb, int32_t * flag)
{

    /* Check out below for embedded */
    *flag = 0;
    if (p_dpb->active_fs->frame.used_for_reference)
        *flag = 1;
    else if (viddec_h264_get_is_used(p_dpb->active_fs) ==3) // frame
        *flag = p_dpb->active_fs->frame.used_for_reference;
    else
    {
        if (viddec_h264_get_is_used(p_dpb->active_fs)&0x1) // top field
            *flag = p_dpb->active_fs->top_field.used_for_reference;
        if (viddec_h264_get_is_used(p_dpb->active_fs)&0x2) // bottom field
            *flag = *flag ||  p_dpb->active_fs->bottom_field.used_for_reference;
    }
}

/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
//////////////////////////////////////////////////////////////////////////////
// h264_dpb_idr_memory_management ()
//
// Perform Memory management for idr pictures
//////////////////////////////////////////////////////////////////////////////

void h264_dpb_idr_memory_management (h264_Info * pInfo,seq_param_set_used_ptr active_sps, int32_t no_output_of_prior_pics_flag)
{
    h264_DecodedPictureBuffer *p_dpb = &pInfo->dpb;
    uint32_t	idx;
    uint32_t	i;
    int32_t		DPB_size;
    int32_t		FrameSizeInBytes, FrameSizeInMbs;
    uint32_t	data;
    int32_t		num_ref_frames = active_sps->num_ref_frames;
    int32_t		level_idc = active_sps->level_idc;
    uint32_t    temp_bump_level=0;


    /// H.D-----
    /// There are 2 kinds of dpb flush defined, one is with display, the other is without display
    /// The function name dpb_flush actually is just the first, and the 2nd one is for error case or no_prior_output
    /// We will rewrite the code below to make it clean and clear
    ///
    if (no_output_of_prior_pics_flag)
    {

        // free all stored pictures
        for (idx = 0; idx < p_dpb->used_size; idx = idx + 1)
        {
            h264_dpb_set_active_fs(p_dpb, p_dpb->fs_dpb_idc[idx]);

            //mfd_printf(" directly freeing fs_idc = %d DSN = 0x%x \n",p_dpb->active_fs->fs_idc, p_dpb->active_fs->first_dsn);
            viddec_h264_set_is_frame_used(p_dpb->active_fs, 0);
            //if( (p_dpb->active_fs->frame_sent == 0x01) && (p_dpb->active_fs->is_output == 0x0))
            {
                //DECODED_FRAME sent but not DISPLAY_FRAME
                h264_dpb_unmark_for_reference(p_dpb, p_dpb->active_fs->fs_idc);
                h264_dpb_remove_ref_list(p_dpb, p_dpb->active_fs->fs_idc);
                //h264_send_new_display_frame(0x01); //send ignore_frame signal to Host
#ifndef USE_AVC_SHORT_FORMAT
                ///  Add into drop-out list for all frms in dpb without display
                if (!(viddec_h264_get_is_non_existent(p_dpb->active_fs)))   {
                    if ( viddec_h264_get_is_output(&(p_dpb->fs[p_dpb->fs_dpb_idc[idx]])) ) {			//// This frame has been displayed but not released
                        p_dpb->frame_id_need_to_be_removed[p_dpb->frame_numbers_need_to_be_removed] = p_dpb->fs_dpb_idc[idx];
                        p_dpb->frame_numbers_need_to_be_removed ++;
                    } else {																		//// This frame will be removed without display
                        p_dpb->frame_id_need_to_be_dropped[p_dpb->frame_numbers_need_to_be_dropped] = p_dpb->fs_dpb_idc[idx];
                        p_dpb->frame_numbers_need_to_be_dropped ++;
                    }
                }
#endif
            }

        }

        ////////////////////////////////////////// Reset Reference list
        for (i = 0; i < p_dpb->ref_frames_in_buffer; i++)
            p_dpb->fs_ref_idc[i] = MPD_DPB_FS_NULL_IDC;

        for (i = 0; i < p_dpb->ltref_frames_in_buffer; i++)
            p_dpb->fs_ltref_idc[i] = MPD_DPB_FS_NULL_IDC;

        ////////////////////////////////////////// Reset DPB and dpb list
        for (i = 0; i < p_dpb->used_size; i++) {
            p_dpb->fs[p_dpb->fs_dpb_idc[i]].fs_idc = MPD_DPB_FS_NULL_IDC;
            p_dpb->fs_dpb_idc[i] = MPD_DPB_FS_NULL_IDC;
        }

        p_dpb->used_size = 0;
        p_dpb->ref_frames_in_buffer   = 0;
        p_dpb->ltref_frames_in_buffer = 0;

        p_dpb->last_output_poc = 0x80000000;
    }
    else {
        h264_dpb_flush_dpb(pInfo, 1, pInfo->img.second_field, num_ref_frames);
    }

    if (p_dpb->fs_dec_idc != MPD_DPB_FS_NULL_IDC) // added condition for use of DPB initialization
    {
        h264_dpb_set_active_fs(p_dpb, p_dpb->fs_dec_idc);
        if (pInfo->img.long_term_reference_flag)
        {
            p_dpb->max_long_term_pic_idx      = 0;
            switch (viddec_h264_get_dec_structure(p_dpb->active_fs))
            {
            case FRAME        :
                p_dpb->active_fs->frame.is_long_term = 1;
            case TOP_FIELD    :
                p_dpb->active_fs->top_field.is_long_term = 1;
            case BOTTOM_FIELD :
                p_dpb->active_fs->bottom_field.is_long_term = 1;
            }
            p_dpb->active_fs->long_term_frame_idx = 0;
        }
        else
        {
            p_dpb->max_long_term_pic_idx = MPD_DPB_FS_NULL_IDC;
            viddec_h264_set_is_frame_long_term(p_dpb->active_fs, 0);
        }
    }

    p_dpb->OutputLevel      = 0;
    p_dpb->OutputLevelValid = 0;
    p_dpb->OutputCtrl = 0;


    // Set up bumping level - do this every time a parameters set is activated...
    if (active_sps->sps_disp.vui_parameters_present_flag)
    {
        if (active_sps->sps_disp.vui_seq_parameters.bitstream_restriction_flag)
        {
            //p_dpb->OutputLevel      = active_sps->sps_disp.vui_seq_parameters.num_reorder_frames;
            //p_dpb->OutputLevelValid = 1;
        }
    }

    // Set up bumping level - do this every time a parameters set is activated...
    switch (level_idc)
    {
    case h264_Level1b:
    case h264_Level1:
    {
        if ((active_sps->profile_idc < 100) && ((active_sps->constraint_set_flags & 0x1) == 0)) {
            DPB_size =	 338;
        }
        else {
            DPB_size =	 149;
        }

        break;
    }
    case h264_Level11:
    {
        DPB_size = 338;
        break;
    }
    case h264_Level12:
    case h264_Level13:
    case h264_Level2:
    {
        DPB_size = 891;
        break;
    }
    case h264_Level21:
    {
        DPB_size = 1782;
        break;
    }
    case h264_Level22:
    case h264_Level3:
    {
        DPB_size = 3038;
        break;
    }
    case h264_Level31:
    {
        DPB_size = 6750;
        break;
    }
    case h264_Level32:
    {
        DPB_size = 7680;
        break;
    }
    case h264_Level4:
    case h264_Level41:
    {
        DPB_size = 12288;
        break;
    }
    case h264_Level42:
    {
        DPB_size = 13056;
        break;
    }
    case h264_Level5:
    {
        DPB_size = 41400;
        break;
    }
    case h264_Level51:
    {
        DPB_size = 69120;
        break;
    }
    default  :
        DPB_size =   69120;
        break;
    }

    FrameSizeInMbs = pInfo->img.PicWidthInMbs * pInfo->img.FrameHeightInMbs;
    FrameSizeInBytes = (FrameSizeInMbs << 8) + (FrameSizeInMbs << 7);

    if (FrameSizeInBytes)
    {

        temp_bump_level = ldiv_mod_u((DPB_size << 10), FrameSizeInBytes, &data);

        if (temp_bump_level > 255)
        {
            p_dpb->BumpLevel = 255;
        }
        else
        {
            p_dpb->BumpLevel = (uint8_t)temp_bump_level;
        }
    }

    if (p_dpb->BumpLevel == 0)
        p_dpb->BumpLevel = active_sps->num_ref_frames + 1;

    if (p_dpb->BumpLevel > 16)
        p_dpb->BumpLevel = 16;


    if (active_sps->sps_disp.vui_parameters_present_flag && active_sps->sps_disp.vui_seq_parameters.bitstream_restriction_flag) {

        if (active_sps->sps_disp.vui_seq_parameters.max_dec_frame_buffering > p_dpb->BumpLevel) {
            //MFD_PARSER_DEBUG(ERROR_H264_DPB);
            //// err handling here

            //// For some ilegal clips, the max dpb length described in vui might exceed the sps's value
            //// To guarantee normal playback, just select the vui value to override
            p_dpb->BumpLevel = active_sps->sps_disp.vui_seq_parameters.max_dec_frame_buffering;
        }
        else {
            p_dpb->BumpLevel = (active_sps->sps_disp.vui_seq_parameters.max_dec_frame_buffering > 1) ?
                               (active_sps->sps_disp.vui_seq_parameters.max_dec_frame_buffering) : 1;
        }
    }


    // A new sequence means automatic frame release
    //sei_information.disp_frozen = 0;

    return;
} //// End --- dpb_idr_memory_management

/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
//////////////////////////////////////////////////////////////////////////////
// h264_dpb_remove_frame_from_dpb ()
//
// remove one frame from DPB
// The parameter index, is the location of the frame to be removed in the
// fs_dpb_idc list. The used size is decremented by one
//////////////////////////////////////////////////////////////////////////////

void h264_dpb_remove_frame_from_dpb(h264_DecodedPictureBuffer *p_dpb, int32_t idx)
{
    int32_t fs_idc;
    uint32_t i;

    fs_idc = p_dpb->fs_dpb_idc[idx];

    h264_dpb_set_active_fs(p_dpb, fs_idc);
    viddec_h264_set_is_frame_used(p_dpb->active_fs, 0);

#ifndef USE_AVC_SHORT_FORMAT
    //add to support frame relocation interface to host
    if (!(viddec_h264_get_is_non_existent(p_dpb->active_fs)))
    {
        p_dpb->frame_id_need_to_be_removed[p_dpb->frame_numbers_need_to_be_removed] = p_dpb->fs[fs_idc].fs_idc;
        p_dpb->frame_numbers_need_to_be_removed ++;
    }
#endif
    ///////////////////////////////////////// Reset FS
    p_dpb->fs[fs_idc].fs_idc = MPD_DPB_FS_NULL_IDC;

    /////Remove unused frame from dpb-list
    i = idx;
    while ( (i + 1)< p_dpb->used_size)
    {
        p_dpb->fs_dpb_idc[i] = p_dpb->fs_dpb_idc[i + 1];
        i ++;
    }
    p_dpb->fs_dpb_idc[i] = MPD_DPB_FS_NULL_IDC;

    ////////////////////////////
    p_dpb->used_size--;

    return;
}

/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */

//////////////////////////////////////////////////////////////////////////////
// h264_dpb_remove_unused_frame_from_dpb ()
//
// Remove a picture from DPB which is no longer needed.
// Search for a frame which is not used for reference and has previously been placed
// in the output queue - if find one call h264_dpb_remove_frame_from_dpb() and
// set flag 1
//////////////////////////////////////////////////////////////////////////////

void h264_dpb_remove_unused_frame_from_dpb(h264_DecodedPictureBuffer *p_dpb, int32_t * flag)
{
    uint32_t idx;
    int32_t first_non_exist_valid, non_exist_idx;
    int32_t used_for_reference = 0;

    *flag = 0;
    first_non_exist_valid = 0x0;
    non_exist_idx = 0x0;

    for (idx = 0; (idx < p_dpb->used_size) && (*flag == 0); idx++)
    {
        h264_dpb_set_active_fs(p_dpb, p_dpb->fs_dpb_idc[idx]);
        h264_dpb_is_used_for_reference(p_dpb, &used_for_reference);

        //if( (used_for_reference == 0x0 ) && active_fs->is_output &&  active_fs->is_non_existent == 0x0)
        //{
        //PRINTF(MFD_NONE, " requesting to send FREE: fs_idc = %d fb_id = %d \n", active_fs->fs_idc, active_fs->fb_id);
        //dpb_release_fb(&h264_dpb, active_fs->fb_id, 1);
        //}

        if (viddec_h264_get_is_output(p_dpb->active_fs) && (used_for_reference == 0))
        {
            h264_dpb_remove_frame_from_dpb(p_dpb, idx);
            *flag = 1;
        }
        /*
        /////// Removed following OLO source (Sodaville H.D)
        		else if ( (first_non_exist_valid == 0x0) && p_dpb->active_fs->is_non_existent )
        		{
        			first_non_exist_valid = 0x01;
        			non_exist_idx = idx;
        		}
        */
    }
    /*
    /////// Removed following OLO source (Sodaville H.D)
    	if ( *flag == 0x0  && first_non_exist_valid) {
    	   h264_dpb_remove_frame_from_dpb(p_dpb,non_exist_idx);
    	  *flag = 1;
    	}
    */
    return;
}	//// End of h264_dpb_remove_unused_frame_from_dpb


/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
//////////////////////////////////////////////////////////////////////////////
// h264_dpb_get_smallest_poc ()
//
// find smallest POC in the DPB which has not as yet been output
// This function only checks for frames and dangling fields...
// unless the dpb used size is one, in which case it will accept an unpaired field
//////////////////////////////////////////////////////////////////////////////
void h264_dpb_get_smallest_poc(h264_DecodedPictureBuffer *p_dpb, int32_t *poc, int32_t *pos)
{
    int32_t poc_int;
    uint32_t idx;
    int32_t first_non_output = 1;

    *pos = MPD_DPB_FS_NULL_IDC;

    h264_dpb_set_active_fs(p_dpb, p_dpb->fs_dpb_idc[0]);
    poc_int = p_dpb->active_fs->frame.poc;

    for (idx = 0; idx < p_dpb->used_size; idx++)
    {
        if (idx >= (NUM_DPB_FRAME_STORES + 2)) {
            break;
        }
        h264_dpb_set_active_fs(p_dpb, p_dpb->fs_dpb_idc[idx]);

        if (viddec_h264_get_is_output(p_dpb->active_fs) == 0)
        {
            //PRINTF(MFD_NONE, " active_fs->fs_idc = %d active_fs->is_used = %d, active_fs->is_dangling = %d , active_fs->poc = %d \n", active_fs->fs_idc, active_fs->is_used, active_fs->is_dangling, active_fs->poc);
            if ((viddec_h264_get_is_used(p_dpb->active_fs) == 3) || (viddec_h264_get_is_dangling(p_dpb->active_fs)))
            {
                if (first_non_output)
                {
                    *pos = idx;
                    first_non_output = 0;
                    poc_int = p_dpb->active_fs->frame.poc;
                }
                else if (poc_int > p_dpb->active_fs->frame.poc)
                {
                    poc_int = p_dpb->active_fs->frame.poc;
                    *pos = idx;
                }
            }
            else if (p_dpb->used_size == 1)
            {
                poc_int = p_dpb->active_fs->frame.poc;
                *pos = idx;
            }
        }
    }

    *poc = poc_int;

    return;
}

/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
//////////////////////////////////////////////////////////////////////////////
// h264_dpb_split_field ()
//
// Extract field information from a frame
//////////////////////////////////////////////////////////////////////////////

void h264_dpb_split_field (h264_DecodedPictureBuffer *p_dpb, h264_Info * pInfo)
{

    //p_dpb->active_fs->frame.poc          = p_dpb->active_fs->frame.poc;
    //  p_dpb->active_fs->top_field.poc     = p_dpb->active_fs->frame.poc;
    // This line changed on 11/05/05 KMc
    p_dpb->active_fs->top_field.poc     = pInfo->img.toppoc;
    p_dpb->active_fs->bottom_field.poc  = pInfo->img.bottompoc;

    p_dpb->active_fs->top_field.used_for_reference    = p_dpb->active_fs->frame.used_for_reference & 1;
    p_dpb->active_fs->bottom_field.used_for_reference = p_dpb->active_fs->frame.used_for_reference >> 1;

    p_dpb->active_fs->top_field.is_long_term = p_dpb->active_fs->frame.is_long_term;
    p_dpb->active_fs->bottom_field.is_long_term = p_dpb->active_fs->frame.is_long_term;

    p_dpb->active_fs->long_term_frame_idx = p_dpb->active_fs->frame.long_term_frame_idx;
    p_dpb->active_fs->top_field.long_term_frame_idx = p_dpb->active_fs->frame.long_term_frame_idx;
    p_dpb->active_fs->bottom_field.long_term_frame_idx = p_dpb->active_fs->frame.long_term_frame_idx;


    // Assign field mvs attached to MB-Frame buffer to the proper buffer
    //! Generate field MVs from Frame MVs
    // ...
    // these will be done in RTL through using proper memory mapping
    return;
}


/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
//////////////////////////////////////////////////////////////////////////////
// h264_dpb_combine_field (int32_t use_old)
//
// Generate a frame from top and bottom fields
//////////////////////////////////////////////////////////////////////////////

void h264_dpb_combine_field(h264_DecodedPictureBuffer *p_dpb, int32_t use_old)
{

    //remove warning
    use_old = use_old;

    p_dpb->active_fs->frame.poc = (p_dpb->active_fs->top_field.poc < p_dpb->active_fs->bottom_field.poc)?
                           p_dpb->active_fs->top_field.poc: p_dpb->active_fs->bottom_field.poc;

    //p_dpb->active_fs->frame.poc = p_dpb->active_fs->poc;


    p_dpb->active_fs->frame.used_for_reference = p_dpb->active_fs->top_field.used_for_reference |(p_dpb->active_fs->bottom_field.used_for_reference);

    p_dpb->active_fs->frame.is_long_term = p_dpb->active_fs->top_field.is_long_term |(p_dpb->active_fs->bottom_field.is_long_term <<1);

    if (p_dpb->active_fs->frame.is_long_term)
        p_dpb->active_fs->frame.long_term_frame_idx = p_dpb->active_fs->long_term_frame_idx;

    return;

}

/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */

//////////////////////////////////////////////////////////////////////////////
// h264_dpb_sliding_window_memory_management ()
//
// Perform Sliding window decoded reference picture marking process
// It must be the reference frame, complementary reference field pair
// or non-paired reference field that has the smallest value of
// FrameNumWrap which is marked as unused for reference. Note : We CANNOT
// simply use frame_num!!!!
//
// Although we hold frame_num_wrap in SW, currently, this is not
// being updated for every picture (the b-picture parameter non-update
// phenomenon of the reference software)
//////////////////////////////////////////////////////////////////////////////

void h264_dpb_sliding_window_memory_management(h264_DecodedPictureBuffer *p_dpb, int32_t NonExisting, int32_t num_ref_frames)
{
    // if this is a reference pic with sliding window, unmark first ref frame
    // should this be (p_dpb->ref_frames_in_buffer + p_dpb->ltref_frames_in_buffer)
    // Rem: adaptive marking can be on a slice by slice basis so we
    // could have pictures merked as long term reference in adaptive marking and then
    //  the marking mode changed back to sliding_window_memory_management
    if (p_dpb->ref_frames_in_buffer >= (num_ref_frames - p_dpb->ltref_frames_in_buffer))
    {
        h264_dpb_unmark_for_reference(p_dpb, p_dpb->fs_ref_idc[0]);
        h264_dpb_remove_ref_list(p_dpb, p_dpb->fs_ref_idc[0]);

        if (NonExisting == 0)
        {
            h264_dpb_set_active_fs(p_dpb, p_dpb->fs_dec_idc);
            viddec_h264_set_is_frame_long_term(p_dpb->active_fs, 0);
        }
    }
}

/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
//////////////////////////////////////////////////////////////////////////////
// h264_dpb_store_picture_in_dpb ()
//
// First we run the marking procedure.
// Then, before we add the current frame_store to the list of refernce stores we run some checks
// These include checking the number of existing reference frames
// in DPB and if necessary, flushing frames.
//
// \param NonExisting
//    If non-zero this is called to store a non-existing frame resulting from gaps_in_frame_num
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// h264_dpb_frame_output ()
//
// If direct == 1, Directly output a frame without storing it in the p_dpb->
// Therefore we must set is_used to 0, which I guess means it will not appear
// in the fs_dpb_idc list and is_output to 1 which means it should be in the
// fs_output_idc list.
//
// If it is a non-existing pcture we do not actually place it in the output queue
//////////////////////////////////////////////////////////////////////////////

void h264_dpb_frame_output(h264_Info * pInfo,int32_t fs_idc, int32_t direct, int32_t * existing)
{
    h264_DecodedPictureBuffer *p_dpb = &pInfo->dpb;

    h264_dpb_set_active_fs(p_dpb, fs_idc);

    //h264_dpb_push_output_queue();
    if (pInfo->sei_information.disp_frozen)
    {
        // check pocs
        if (p_dpb->active_fs->top_field.poc >= pInfo->sei_information.freeze_POC)
        {
            if (p_dpb->active_fs->top_field.poc <  pInfo->sei_information.release_POC)
            {
                viddec_h264_set_is_top_skipped(p_dpb->active_fs, 1);
            }
            else
            {
                pInfo->sei_information.disp_frozen = 0;
            }
        }

        if (p_dpb->active_fs->bottom_field.poc >=  pInfo->sei_information.freeze_POC)
        {
            if (p_dpb->active_fs->bottom_field.poc <  pInfo->sei_information.release_POC)
            {
                viddec_h264_set_is_bottom_skipped(p_dpb->active_fs, 1);
            }
            else
            {
                pInfo->sei_information.disp_frozen = 0;
            }
        }
    }

    if ( viddec_h264_get_broken_link_picture(p_dpb->active_fs) )
        pInfo->sei_information.broken_link = 1;

    if ( pInfo->sei_information.broken_link)
    {
        // Check if this was the recovery point picture - going to have recovery point on
        // a frame basis
        if (viddec_h264_get_recovery_pt_picture(p_dpb->active_fs))
        {
            pInfo->sei_information.broken_link = 0;
            // Also reset wait on sei recovery point picture
            p_dpb->WaitSeiRecovery         = 0;
        }
        else
        {
            viddec_h264_set_is_frame_skipped(p_dpb->active_fs, 3);
        }
    }
    else
    {
        // even if this is not a broken - link, we need to follow SEI recovery point rules
        // Did we use SEI recovery point for th elast restart?
        if ( p_dpb->WaitSeiRecovery )
        {
            if ( viddec_h264_get_recovery_pt_picture(p_dpb->active_fs) ) {
                p_dpb->WaitSeiRecovery         = 0;
            } else {
                viddec_h264_set_is_frame_skipped(p_dpb->active_fs, 3);
            }
        }
    }

    if ( p_dpb->SuspendOutput )
    {
        if ( viddec_h264_get_open_gop_entry(p_dpb->active_fs) ) {
            p_dpb->SuspendOutput      = 0;
        } else {
            viddec_h264_set_is_frame_skipped(p_dpb->active_fs, 3);
        }
    }

    //h264_send_new_display_frame(0x0);
    viddec_h264_set_is_output(p_dpb->active_fs, 1);

    if (viddec_h264_get_is_non_existent(p_dpb->active_fs) == 0)
    {
        *existing = 1;
#ifndef USE_AVC_SHORT_FORMAT
        p_dpb->frame_id_need_to_be_displayed[p_dpb->frame_numbers_need_to_be_displayed]=p_dpb->active_fs->fs_idc;
        p_dpb->frame_numbers_need_to_be_displayed++;
#endif
        //if(direct)
        //h264_dpb_remove_frame_from_dpb(p_dpb, p_dpb->active_fs->fs_idc);		// Remove dpb.fs_dpb_idc[pos]
    }
    else
    {
        *existing = 0;
    }

    if (direct) {
        viddec_h264_set_is_frame_used(p_dpb->active_fs, 0);
        p_dpb->active_fs->frame.used_for_reference = 0;
        p_dpb->active_fs->top_field.used_for_reference = 0;
        p_dpb->active_fs->bottom_field.used_for_reference = 0;
        p_dpb->active_fs->fs_idc = MPD_DPB_FS_NULL_IDC;
    }
    return;
} ///////// End of dpb frame output


/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
//////////////////////////////////////////////////////////////////////////////
// h264_dpb_output_one_frame_from_dpb ()
//
// Output one frame stored in the DPB. Basiclly this results in its placment
// in the fs_output_idc list.
// Placement in the output queue should cause an automatic removal from the dpb
// if the frame store is not being used as a reference
// This may need another param for a frame request so that it definitely outputs one non-exiosting frame
//////////////////////////////////////////////////////////////////////////////
int32_t h264_dpb_output_one_frame_from_dpb(h264_Info* pInfo,int32_t direct, int32_t request, int32_t num_ref_frames)
{
    h264_DecodedPictureBuffer *p_dpb = &pInfo->dpb;
    int32_t poc;
    int32_t pos;
    int32_t used_for_reference;

    int32_t existing = 0;
    int32_t is_refused = 0;
    int32_t is_pushed = 0;

    //remove warning
    request = request;

    if (direct)
    {
        h264_dpb_frame_output(pInfo, p_dpb->fs_dec_idc, 1, &existing);
    }
    else
    {
        if (p_dpb->used_size != 0)
        {
            // Should this be dpb.not_as_yet_output_num > 0 ??
            // There should maybe be a is_refused == 0 condition instead...
            while ((p_dpb->used_size > 0) && (existing == 0) && (is_refused == 0))
            {
                // find smallest non-output POC
                h264_dpb_get_smallest_poc(p_dpb, &poc, &pos);
                if (pos != MPD_DPB_FS_NULL_IDC)
                {
                    // put it into the output queue
                    h264_dpb_frame_output(pInfo, p_dpb->fs_dpb_idc[pos], 0, &existing);

                    p_dpb->last_output_poc = poc;
                    if (existing) is_pushed = 1;
                    // If non-reference, free frame store and move empty store to end of buffer

                    h264_dpb_is_used_for_reference(p_dpb, &used_for_reference);
                    if (!(used_for_reference))
                        h264_dpb_remove_frame_from_dpb(p_dpb, pos);		// Remove dpb.fs_dpb_idc[pos]
                }
                else
                {
                    int32_t flag;
                    uint32_t idx;

                    // This is basically an error condition caused by too many reference frames in the DPB.
                    // It should only happen in errored streams, and can happen if this picture had an MMCO,
                    // thus disabling h264_dpb_sliding_window_memory_management(), which would normally have
                    // unmarked the oldest reference frame.
                    h264_dpb_sliding_window_memory_management(p_dpb, 0,num_ref_frames);
                    h264_dpb_remove_unused_frame_from_dpb(p_dpb, &flag);

                    if (flag == 0) {
                        for (idx = 0; idx < p_dpb->used_size; idx++)
                        {
                            h264_dpb_set_active_fs(p_dpb, p_dpb->fs_dpb_idc[idx]);
                            h264_dpb_is_used_for_reference(p_dpb, &used_for_reference);

                            if (used_for_reference) {
                                break;
                            }
                        }

                        if (idx < p_dpb->used_size) {
                            // Short term
                            h264_dpb_unmark_for_reference(p_dpb, p_dpb->fs_dpb_idc[idx]);
                            h264_dpb_remove_ref_list(p_dpb, p_dpb->fs_dpb_idc[idx]);

                            // Long term
                            h264_dpb_unmark_for_long_term_reference(p_dpb, p_dpb->fs_dpb_idc[idx]);
                            h264_dpb_remove_ltref_list(p_dpb, p_dpb->fs_dpb_idc[idx]);

                            // Remove from DPB
                            h264_dpb_remove_unused_frame_from_dpb(p_dpb, &flag);
                        }
                    }
                    return 1;
                }
            }
        }
    }

    return is_pushed;
}


/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */

//////////////////////////////////////////////////////////////////////////////
// h264_dpb_queue_update
//
// This should be called anytime the output queue might be changed
//////////////////////////////////////////////////////////////////////////////

int32_t h264_dpb_queue_update(h264_Info* pInfo,int32_t push, int32_t direct, int32_t frame_request, int32_t num_ref_frames)
{

    int32_t frame_output = 0;

    if (push)
    {
        frame_output = h264_dpb_output_one_frame_from_dpb(pInfo, direct, 0, num_ref_frames);
    }
    else if (frame_request)
    {
        frame_output = h264_dpb_output_one_frame_from_dpb(pInfo, 0, 1,num_ref_frames);
    }


    return frame_output;

}

/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */

//////////////////////////////////////////////////////////////////////////////
// h264_dpb_flush_dpb ()
//
// Unmarks all reference pictures in the short-term and long term lists and
// in doing so resets the lists.
//
// Flushing the dpb, adds all the current frames in the dpb, not already on the output list
// to the output list and removes them from the dpb (they will all be marked as unused for
// reference first)
//////////////////////////////////////////////////////////////////////////////

void h264_dpb_flush_dpb (h264_Info* pInfo,int32_t output_all, int32_t keep_complement, int32_t num_ref_frames)
{
    h264_DecodedPictureBuffer *p_dpb = &pInfo->dpb;

    int32_t idx, flag;
    int32_t ref_frames_in_buffer;

    ref_frames_in_buffer = p_dpb->ref_frames_in_buffer;

    for (idx = 0; idx < ref_frames_in_buffer; idx++) {
        h264_dpb_unmark_for_reference(p_dpb, p_dpb->fs_ref_idc[0]);
        h264_dpb_remove_ref_list(p_dpb, p_dpb->fs_ref_idc[0]);
    }

    ref_frames_in_buffer = p_dpb->ltref_frames_in_buffer;

    for (idx = 0; idx < ref_frames_in_buffer; idx++)
    {
        h264_dpb_unmark_for_long_term_reference(p_dpb, p_dpb->fs_ltref_idc[0]);
        h264_dpb_remove_ltref_list(p_dpb, p_dpb->fs_ltref_idc[0]);
    }

    // output frames in POC order
    if (output_all) {
        while ((p_dpb->used_size > 0) && (p_dpb->used_size - keep_complement)) {
            h264_dpb_queue_update(pInfo, 1, 0, 0,num_ref_frames);
        }
    }

    flag = 1;
    while (flag) {
        h264_dpb_remove_unused_frame_from_dpb(p_dpb, &flag);
    }

    return;
}

/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
//////////////////////////////////////////////////////////////////////////////
// h264_dpb_reset_dpb ()
//
// Used to reset the contents of dpb
// Must calculate memory (aligned) pointers for each of the possible frame stores
//
// Also want to calculate possible max dpb size in terms of frames
// We should have an active SPS when we call this ftn to calc bumping level
//////////////////////////////////////////////////////////////////////////////
void h264_dpb_reset_dpb(h264_Info * pInfo,int32_t PicWidthInMbs, int32_t FrameHeightInMbs, int32_t SizeChange, int32_t no_output_of_prior_pics_flag)
{
    h264_DecodedPictureBuffer *p_dpb = &pInfo->dpb;

    int32_t num_ref_frames = pInfo->active_SPS.num_ref_frames;


    // If half way through a frame then Frame in progress will still be high,
    // so mark the previous field as a dangling field. This is also needed to
    // keep cs7050_sif_dpb_disp_numb_ptr correct. Better to reset instead?
    if (p_dpb->used_size)
    {
        int32_t idx;
        idx = p_dpb->used_size-1;
        if (p_dpb->fs_dpb_idc[idx] != MPD_DPB_FS_NULL_IDC)
        {
            h264_dpb_set_active_fs(p_dpb, p_dpb->fs_dpb_idc[idx]);

            if (viddec_h264_get_is_used(p_dpb->active_fs) != 3)
                h264_dpb_mark_dangling_field(p_dpb, p_dpb->active_fs->fs_idc);       //, DANGLING_TYPE_DPB_RESET
        }
    }

    // initialize software DPB
    if (p_dpb->active_fs) {
        viddec_h264_set_dec_structure(p_dpb->active_fs, INVALID);
    }
    h264_dpb_idr_memory_management(pInfo, &pInfo->active_SPS, no_output_of_prior_pics_flag);  // implied no_output_of_prior_pics_flag==1


    // May always be a size change which calls this function now...
    // could eliminate below branch
    if (SizeChange)
    {

        /***
        Note : 21/03/2005 14:16
        Danger asociated with resetting curr_alloc_mem as it would allow the FW top reallocate
        frame stores from 0 -> NUM_FRAME_STORES again - could lead to queue overflow and corruption

        Placed in size change condition in the hope that this will only ensure dpb is empty
        and thus this behaviour is valid before continuing again
        ***/


        p_dpb->PicWidthInMbs      = PicWidthInMbs;
        p_dpb->FrameHeightInMbs   = FrameHeightInMbs;

        p_dpb->fs_dec_idc = MPD_DPB_FS_NULL_IDC;
        //Flush the current DPB.
        h264_dpb_flush_dpb(pInfo, 1,0,num_ref_frames);
    }

    return;
} ///// End of reset DPB

/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
// ---------------------------------------------------------------------------
// Note that if an 'missing_pip_fb' condition exists, the message will
// sent to the host each time setup_free_fb is called. However, since this
// condition is not expected to happen if pre-defined steps are followed, we let
// it be for now and will change it if required. Basically, as long as host
// enables PiP after adding PiP buffers and disables PiP before removing buffers
// and matches PiP fb_id's with normal decode fb_id's this condition should
// not occur.
// ---------------------------------------------------------------------------
int32_t dpb_setup_free_fb( h264_DecodedPictureBuffer *p_dpb, uint8_t* fb_id, pip_setting_t* pip_setting )
{
    uint8_t  idx;

    //remove warning
    pip_setting = pip_setting;


    for (idx = 0; idx < NUM_DPB_FRAME_STORES; idx++)
    {
        if (p_dpb->fs[idx].fs_idc == MPD_DPB_FS_NULL_IDC)
        {
            *fb_id = idx;
            break;
        }
    }

    if (idx == NUM_DPB_FRAME_STORES)
        return 1;

    p_dpb->fs[idx].fs_idc = idx;

    return 0;

}

/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
//////////////////////////////////////////////////////////////////////////////
// h264_dpb_assign_frame_store ()
//
// may need a non-existing option parameter
//

int32_t h264_dpb_assign_frame_store(h264_Info * pInfo, int32_t NonExisting)
{
    uint8_t idc = MPD_DPB_FS_NULL_IDC;
    pip_setting_t pip_setting;
    h264_DecodedPictureBuffer *p_dpb = &pInfo->dpb;


    while ( dpb_setup_free_fb(p_dpb, &idc, &pip_setting)  != 0 ) {
        ///
        /// Generally this is triggered a error case, no more frame buffer avaliable for next
        /// What we do here is just remove one with min-POC before get more info
        ///

        int32_t pos = 0, poc = 0, existing = 1;

        // find smallest non-output POC
        h264_dpb_get_smallest_poc(p_dpb, &poc, &pos);
        if (pos != MPD_DPB_FS_NULL_IDC)
        {
            // put it into the output queue
            h264_dpb_frame_output(pInfo, p_dpb->fs_dpb_idc[pos], 0, &existing);
            p_dpb->last_output_poc = poc;
            h264_dpb_remove_frame_from_dpb(p_dpb, pos);	 // Remove dpb.fs_dpb_idc[pos]
        }
    }


    if (NonExisting) {
        p_dpb->fs_non_exist_idc = idc;
    } else {
        p_dpb->fs_dec_idc = idc;
    }

    //add to support frame relocation interface to host
    if (!NonExisting)
    {
        p_dpb->frame_numbers_need_to_be_allocated = 1;
        p_dpb->frame_id_need_to_be_allocated = p_dpb->fs_dec_idc;
    }


    ///////////////////////////////h264_dpb_reset_fs();
    h264_dpb_set_active_fs(p_dpb, idc);
    p_dpb->active_fs->fs_flag_1 = 0;
    p_dpb->active_fs->fs_flag_2 = 0;
    viddec_h264_set_is_non_existent(p_dpb->active_fs, NonExisting);
    viddec_h264_set_is_output(p_dpb->active_fs, (NonExisting?1:0));

    p_dpb->active_fs->pic_type = ((FRAME_TYPE_INVALID<<FRAME_TYPE_TOP_OFFSET)|(FRAME_TYPE_INVALID<<FRAME_TYPE_BOTTOM_OFFSET));			//----

    // Only put members in here which will not be reset somewhere else
    // and which could be used before they are overwritten again with
    // new valid values
    // eg ->is_used is reset on removal from dpb, no need for it here
    //    ->poc would only be changed when we overwrite on insert_Picture_in_dpb()
    //    but would be used by get_smallest_poc()
    //    ->top.poc would also not be overwritten until a new valid value comes along,
    //    but I don't think it is used before then so no need to reset
    //p_dpb->active_fs->is_long_term    = 0;
    p_dpb->active_fs->frame.used_for_reference    = 0;
    p_dpb->active_fs->frame.poc			= 0;

    return 1;
}


/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
//////////////////////////////////////////////////////////////////////////////
// h264_dpb_update_queue_dangling_field (h264_Info * pInfo)
//
// Update DPB for Dangling field special case
//
void h264_dpb_update_queue_dangling_field(h264_Info * pInfo)
{
    h264_DecodedPictureBuffer *dpb_ptr = &pInfo->dpb;
    int32_t prev_pic_unpaired_field = 0;

    if (dpb_ptr->used_size > dpb_ptr->BumpLevel)
    {
        if (dpb_ptr->fs_dpb_idc[dpb_ptr->used_size-1] != MPD_DPB_FS_NULL_IDC)
        {
            h264_dpb_set_active_fs(dpb_ptr, dpb_ptr->fs_dpb_idc[dpb_ptr->used_size-1]);
            if (viddec_h264_get_is_used(dpb_ptr->active_fs) != 3)
            {
                prev_pic_unpaired_field = 1;
            }
        }

        if (pInfo->img.structure != FRAME)
        {
            // To prove this is the second field,
            // 1) The previous picture is an (as yet) unpaired field
            if (prev_pic_unpaired_field)
            {
                // If we establish the previous pic was an unpaired field and this picture is not
                // its complement, the previous picture was a dangling field
                if (pInfo->img.second_field == 0) {
                    while (dpb_ptr->used_size > dpb_ptr->BumpLevel)
                        h264_dpb_queue_update(pInfo, 1, 0, 0,pInfo->active_SPS.num_ref_frames); // flush a frame
                }
            }
        }
        else if (prev_pic_unpaired_field) {
            while (dpb_ptr->used_size > dpb_ptr->BumpLevel)
                h264_dpb_queue_update(pInfo, 1, 0, 0,pInfo->active_SPS.num_ref_frames); // flush a frame
        }
    }


    return;
}	///// End of init Frame Store


/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
//////////////////////////////////////////////////////////////////////////////
// h264_dpb_init_frame_store (h264_Info * pInfo)
//
// Set the frame store to be used in decoding the picture
//

void h264_dpb_init_frame_store(h264_Info * pInfo)
{
    h264_DecodedPictureBuffer *dpb_ptr = &pInfo->dpb;

    int32_t free_fs_found;
    int32_t idx = 0;
    int32_t prev_pic_unpaired_field = 0;
    int32_t prev_idc = MPD_DPB_FS_NULL_IDC;
    int32_t structure = pInfo->img.structure;

    if (dpb_ptr->used_size)
    {
        idx = dpb_ptr->used_size-1;
        prev_idc = dpb_ptr->fs_dpb_idc[idx];
    }

    if (prev_idc != MPD_DPB_FS_NULL_IDC)
    {
        h264_dpb_set_active_fs(dpb_ptr, dpb_ptr->fs_dpb_idc[dpb_ptr->used_size-1]);
        if (viddec_h264_get_is_used(dpb_ptr->active_fs) != 3)
        {
            //PRINTF(MFD_NONE, " FN: %d p_dpb->active_fs->is_used = %d \n", (h264_frame_number+1), p_dpb->active_fs->is_used);
            prev_pic_unpaired_field = 1;
        }
    }

    //if ((pInfo->img.curr_has_mmco_5) || (pInfo->img.idr_flag))   curr_fld_not_prev_comp = 1;

    if (structure != FRAME)
    {

        // To prove this is the second field,
        // 1) The previous picture is an (as yet) unpaired field
        if (prev_pic_unpaired_field)
        {
            // If we establish the previous pic was an unpaired field and this picture is not
            // its complement, the previous picture was a dangling field
            if (pInfo->img.second_field == 0)
                h264_dpb_mark_dangling_field(dpb_ptr, dpb_ptr->active_fs->fs_idc);  //, DANGLING_TYPE_FIELD
        }
    }
    else if (prev_pic_unpaired_field) {
        h264_dpb_mark_dangling_field(dpb_ptr, dpb_ptr->active_fs->fs_idc);		//, DANGLING_TYPE_FRAME
    }

    free_fs_found = 0;

    // If this is not a second field, we must find a free space for the current picture
    if (!(pInfo->img.second_field))
    {
        dpb_ptr->fs_dec_idc = MPD_DPB_FS_NULL_IDC;
        free_fs_found = h264_dpb_assign_frame_store(pInfo, 0);
        //h264_frame_number++;
        //PRINTF(MFD_NONE, " FN: %d (inc) fs_idc =  %d \n", (h264_frame_number+1), dpb.fs_dec_idc);
    }

    h264_dpb_set_active_fs(dpb_ptr, dpb_ptr->fs_dec_idc);

    ////////////// TODO: THe following init
#if 1
    if ( pInfo->img.second_field) {
        //p_dpb->active_fs->second_dsn = pInfo->img.dsn;
        //p_dpb->active_fs->prev_dsn = pInfo->img.prev_dsn;
        if (dpb_ptr->active_fs->pic_type == FRAME_TYPE_IDR ||
                dpb_ptr->active_fs->pic_type == FRAME_TYPE_I) {

            viddec_h264_set_first_field_intra(dpb_ptr->active_fs, 1);
        } else {
            viddec_h264_set_first_field_intra(dpb_ptr->active_fs, 0);
        }

    }
    else {
        //p_dpb->active_fs->first_dsn = pInfo->img.dsn;
        //p_dpb->active_fs->prev_dsn = pInfo->img.prev_dsn;
        viddec_h264_set_first_field_intra(dpb_ptr->active_fs, 0);
    }

    if (pInfo->img.structure == FRAME) {
        //dpb_ptr->active_fs->second_dsn = 0x0;
    }

    if ( pInfo->sei_information.broken_link_pic )
    {
        viddec_h264_set_broken_link_picture(dpb_ptr->active_fs, 1);
        pInfo->sei_information.broken_link_pic = 0;
    }

    if ((pInfo->img.frame_num == pInfo->sei_information.recovery_frame_num)&&(pInfo->SliceHeader.nal_ref_idc != 0))
        viddec_h264_set_recovery_pt_picture(dpb_ptr->active_fs, 1);

    //if ((( gRestartMode.aud ) || ( gRestartMode.sei )) && ( !gRestartMode.idr))
    if (pInfo->img.recovery_point_found == 6)
    {
        viddec_h264_set_open_gop_entry(dpb_ptr->active_fs, 1);
        pInfo->dpb.SuspendOutput         = 1;
    }
#endif

    if ((pInfo->img.second_field) || (free_fs_found))
    {
        viddec_h264_set_dec_structure(dpb_ptr->active_fs, pInfo->img.structure);
        viddec_h264_set_is_output(dpb_ptr->active_fs, 0);

        switch (pInfo->img.structure)
        {
        case (FRAME)     : {
            dpb_ptr->active_fs->frame.pic_num = pInfo->img.frame_num;
            dpb_ptr->active_fs->frame.long_term_frame_idx = 0;
            dpb_ptr->active_fs->frame.long_term_pic_num = 0;
            dpb_ptr->active_fs->frame.used_for_reference = 0;
            dpb_ptr->active_fs->frame.is_long_term = 0;
            //dpb_ptr->active_fs->frame.structure = pInfo->img.structure;
            dpb_ptr->active_fs->frame.poc = pInfo->img.framepoc;
        }
        break;
        case (TOP_FIELD) : {
            dpb_ptr->active_fs->top_field.pic_num = pInfo->img.frame_num;
            dpb_ptr->active_fs->top_field.long_term_frame_idx = 0;
            dpb_ptr->active_fs->top_field.long_term_pic_num = 0;
            dpb_ptr->active_fs->top_field.used_for_reference = 0;
            dpb_ptr->active_fs->top_field.is_long_term = 0;
            //dpb_ptr->active_fs->top_field.structure = pInfo->img.structure;
            dpb_ptr->active_fs->top_field.poc = pInfo->img.toppoc;
        }
        break;
        case(BOTTOM_FIELD) : {
            dpb_ptr->active_fs->bottom_field.pic_num = pInfo->img.frame_num;
            dpb_ptr->active_fs->bottom_field.long_term_frame_idx = 0;
            dpb_ptr->active_fs->bottom_field.long_term_pic_num = 0;
            dpb_ptr->active_fs->bottom_field.used_for_reference = 0;
            dpb_ptr->active_fs->bottom_field.is_long_term = 0;
            //dpb_ptr->active_fs->bottom_field.structure = pInfo->img.structure;
            dpb_ptr->active_fs->bottom_field.poc = pInfo->img.bottompoc;
        }
        break;
        }
    }
    else
    {
        // Need to drop a frame or something here
    }

    return;
}	///// End of init Frame Store


/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
//////////////////////////////////////////////////////////////////////////////
// Decoding POC for current Picture
// 1) pic_order_cnt_type (0, 1, 2)
//
//////////////////////////////////////////////////////////////////////////////

void h264_hdr_decoding_poc (h264_Info * pInfo,int32_t NonExisting, int32_t frame_num)
{
    int32_t MaxPicOrderCntLsb = (1<<(pInfo->active_SPS.log2_max_pic_order_cnt_lsb_minus4+4));
    int32_t delta_pic_order_count[2];
    int32_t MaxFrameNum = 1 << (pInfo->active_SPS.log2_max_frame_num_minus4 + 4);

    int32_t AbsFrameNum =0;
    int32_t ExpectedDeltaPerPicOrderCntCycle =0;
    int32_t PicOrderCntCycleCnt = 0;
    int32_t FrameNumInPicOrderCntCycle =0;
    int32_t ExpectedPicOrderCnt =0;

    int32_t actual_frame_num =0;



    if (NonExisting)    actual_frame_num = frame_num;
    else               actual_frame_num = pInfo->img.frame_num;

    switch (pInfo->active_SPS.pic_order_cnt_type)
    {
    case 0:
        if (NonExisting != 0) break;

        if (pInfo->SliceHeader.idr_flag)
        {
            pInfo->img.PicOrderCntMsb = 0;
            pInfo->img.PrevPicOrderCntLsb = 0;
        }
        else if (pInfo->img.last_has_mmco_5)
        {
            if (pInfo->img.last_pic_bottom_field)
            {
                pInfo->img.PicOrderCntMsb     = 0;
                pInfo->img.PrevPicOrderCntLsb = 0;
            }
            else
            {
                pInfo->img.PicOrderCntMsb     = 0;
                pInfo->img.PrevPicOrderCntLsb = pInfo->img.toppoc;
            }
        }

        // Calculate the MSBs of current picture
        if ((pInfo->img.pic_order_cnt_lsb < pInfo->img.PrevPicOrderCntLsb)  &&
                ((pInfo->img.PrevPicOrderCntLsb - pInfo->img.pic_order_cnt_lsb )>=(MaxPicOrderCntLsb>>1)) )
        {
            pInfo->img.CurrPicOrderCntMsb = pInfo->img.PicOrderCntMsb + MaxPicOrderCntLsb;
        } else if ((pInfo->img.pic_order_cnt_lsb  >  pInfo->img.PrevPicOrderCntLsb)  &&
                   ((pInfo->img.pic_order_cnt_lsb - pInfo->img.PrevPicOrderCntLsb ) > (MaxPicOrderCntLsb>>1)) )
        {
            pInfo->img.CurrPicOrderCntMsb = pInfo->img.PicOrderCntMsb - MaxPicOrderCntLsb;
        } else
        {
            pInfo->img.CurrPicOrderCntMsb = pInfo->img.PicOrderCntMsb;
        }

        // 2nd

        if (pInfo->img.field_pic_flag==0)
        {
            //frame pix
            pInfo->img.toppoc = pInfo->img.CurrPicOrderCntMsb + pInfo->img.pic_order_cnt_lsb;
            pInfo->img.bottompoc = pInfo->img.toppoc + pInfo->img.delta_pic_order_cnt_bottom;
            pInfo->img.ThisPOC = pInfo->img.framepoc = (pInfo->img.toppoc < pInfo->img.bottompoc)? pInfo->img.toppoc : pInfo->img.bottompoc; // POC200301
        }
        else if (pInfo->img.bottom_field_flag==0)
        {  //top field
            pInfo->img.ThisPOC= pInfo->img.toppoc = pInfo->img.CurrPicOrderCntMsb + pInfo->img.pic_order_cnt_lsb;
        }
        else
        {  //bottom field
            pInfo->img.ThisPOC= pInfo->img.bottompoc = pInfo->img.CurrPicOrderCntMsb + pInfo->img.pic_order_cnt_lsb;
        }
        pInfo->img.framepoc=pInfo->img.ThisPOC;

        if ( pInfo->img.frame_num != pInfo->old_slice.frame_num)
            pInfo->img.PreviousFrameNum = pInfo->img.frame_num;

        if (pInfo->SliceHeader.nal_ref_idc)
        {
            pInfo->img.PrevPicOrderCntLsb = pInfo->img.pic_order_cnt_lsb;
            pInfo->img.PicOrderCntMsb = pInfo->img.CurrPicOrderCntMsb;
        }

        break;
    case 1: {
        if (NonExisting)
        {
            delta_pic_order_count[0] = 0;
            delta_pic_order_count[1] = 0;
        }
        else
        {
            delta_pic_order_count[0] = ( pInfo->img.delta_pic_order_always_zero_flag ) ? 0 : pInfo->img.delta_pic_order_cnt[0];
            delta_pic_order_count[1] = ( pInfo->img.delta_pic_order_always_zero_flag ) ? 0 :
                                       ( (!pInfo->active_PPS.pic_order_present_flag)  && (!(pInfo->img.field_pic_flag))) ? 0 :
                                       pInfo->img.delta_pic_order_cnt[1];
        }

        // this if branch should not be taken during processing of a gap_in_frame_num pic since
        // an IDR picture cannot produce non-existent frames...
        if (pInfo->SliceHeader.idr_flag)
        {
            pInfo->img.FrameNumOffset         = 0;
#ifdef VBP
#ifdef SW_ERROR_CONCEALEMNT
            if (pInfo->img.frame_num)
            {
                pInfo->sw_bail = 1;
            }
#endif
#endif
        }
        else
        {

            if (actual_frame_num < pInfo->img.PreviousFrameNum)
            {
                pInfo->img.FrameNumOffset = pInfo->img.PreviousFrameNumOffset + MaxFrameNum;
            }
            else
            {
                pInfo->img.FrameNumOffset = pInfo->img.PreviousFrameNumOffset;
            }
        }

        // pInfo->img.num_ref_frames_in_pic_order_cnt_cycle set from SPS
        // so constant between existent and non-existent frames
        if (pInfo->img.num_ref_frames_in_pic_order_cnt_cycle)
            AbsFrameNum = pInfo->img.FrameNumOffset + actual_frame_num;
        else
            AbsFrameNum = 0;

        // pInfo->img.disposable_flag should never be true for a non-existent frame since these are always
        // references...
        if ((pInfo->SliceHeader.nal_ref_idc == 0) && (AbsFrameNum > 0)) AbsFrameNum = AbsFrameNum - 1;

        // 3rd
        ExpectedDeltaPerPicOrderCntCycle = pInfo->active_SPS.expectedDeltaPerPOCCycle;

        if (AbsFrameNum)
        {
            // Rem: pInfo->img.num_ref_frames_in_pic_order_cnt_cycle takes max value of 255 (8 bit)
            // Frame NUm may be 2^16 (17 bits)
            // I guess we really have to treat AbsFrameNum as a 32 bit number
            uint32_t temp = 0;
            int32_t i=0;
            int32_t offset_for_ref_frame[MAX_NUM_REF_FRAMES_IN_PIC_ORDER_CNT_CYCLE];

            if (pInfo->img.num_ref_frames_in_pic_order_cnt_cycle)
                PicOrderCntCycleCnt = ldiv_mod_u((uint32_t)(AbsFrameNum-1), (uint32_t)pInfo->img.num_ref_frames_in_pic_order_cnt_cycle, &temp);

            ExpectedPicOrderCnt = mult_u((uint32_t)PicOrderCntCycleCnt, (uint32_t)ExpectedDeltaPerPicOrderCntCycle);

            FrameNumInPicOrderCntCycle = temp;

            //ExpectedPicOrderCnt +=pInfo->active_SPS.expectedDeltaPerPOCCycle;
#ifndef USER_MODE
            h264_Parse_Copy_Offset_Ref_Frames_From_DDR(pInfo, offset_for_ref_frame, pInfo->active_SPS.seq_parameter_set_id);
            for (i = 0; i <= FrameNumInPicOrderCntCycle; i++)
                ExpectedPicOrderCnt += offset_for_ref_frame[i];
#else
            for (i = 0; i <= FrameNumInPicOrderCntCycle; i++)
                ExpectedPicOrderCnt += pInfo->active_SPS.offset_for_ref_frame[i];
#endif
        }
        else {
            ExpectedPicOrderCnt = 0;
        }

        if (pInfo->SliceHeader.nal_ref_idc == 0)
            ExpectedPicOrderCnt += pInfo->img.offset_for_non_ref_pic;

        if (!(pInfo->img.field_pic_flag))
        {
            pInfo->img.toppoc = ExpectedPicOrderCnt + delta_pic_order_count[0];
            pInfo->img.bottompoc = pInfo->img.toppoc + pInfo->img.offset_for_top_to_bottom_field + delta_pic_order_count[1];
            pInfo->img.framepoc = (pInfo->img.toppoc < pInfo->img.bottompoc)? pInfo->img.toppoc : pInfo->img.bottompoc;
            pInfo->img.ThisPOC = pInfo->img.framepoc;
        }
        else if (!(pInfo->img.bottom_field_flag))
        {
            //top field
            pInfo->img.toppoc = ExpectedPicOrderCnt + delta_pic_order_count[0];
            pInfo->img.ThisPOC = pInfo->img.toppoc;
            pInfo->img.bottompoc = 0;
        }
        else
        {
            //bottom field
            pInfo->img.toppoc = 0;
            pInfo->img.bottompoc = ExpectedPicOrderCnt + pInfo->img.offset_for_top_to_bottom_field + delta_pic_order_count[0];
            pInfo->img.ThisPOC = pInfo->img.bottompoc;
        }

        //CONFORMANCE_ISSUE
        pInfo->img.framepoc=pInfo->img.ThisPOC;

        //CONFORMANCE_ISSUE
        pInfo->img.PreviousFrameNum=pInfo->img.frame_num;
        pInfo->img.PreviousFrameNumOffset=pInfo->img.FrameNumOffset;

    }
    break;
    case 2: {     // POC MODE 2
        if (pInfo->SliceHeader.idr_flag)
        {
            pInfo->img.FrameNumOffset = 0;
            pInfo->img.framepoc = 0;
            pInfo->img.toppoc = 0;
            pInfo->img.bottompoc = 0;
            pInfo->img.ThisPOC = 0;
#ifdef VBP
#ifdef SW_ERROR_CONCEALEMNT
            if (pInfo->img.frame_num)
            {
                pInfo->sw_bail = 1;
            }
#endif
#endif
        }
        else
        {
            if (pInfo->img.last_has_mmco_5)
            {
                pInfo->img.PreviousFrameNum = 0;
                pInfo->img.PreviousFrameNumOffset = 0;
            }
            if (actual_frame_num < pInfo->img.PreviousFrameNum)
                pInfo->img.FrameNumOffset = pInfo->img.PreviousFrameNumOffset + MaxFrameNum;
            else
                pInfo->img.FrameNumOffset = pInfo->img.PreviousFrameNumOffset;

            AbsFrameNum = pInfo->img.FrameNumOffset + actual_frame_num;
            if (pInfo->SliceHeader.nal_ref_idc == 0) pInfo->img.ThisPOC = (AbsFrameNum<<1) - 1;
            else                     pInfo->img.ThisPOC = (AbsFrameNum<<1);

            if (!(pInfo->img.field_pic_flag))
            {
                pInfo->img.toppoc    = pInfo->img.ThisPOC;
                pInfo->img.bottompoc = pInfo->img.ThisPOC;
                pInfo->img.framepoc  = pInfo->img.ThisPOC;
            }
            else if (!(pInfo->img.bottom_field_flag))
            {
                pInfo->img.toppoc   = pInfo->img.ThisPOC;
                pInfo->img.framepoc = pInfo->img.ThisPOC;
            }
            else
            {
                pInfo->img.bottompoc = pInfo->img.ThisPOC;
                pInfo->img.framepoc  = pInfo->img.ThisPOC;
            }
        }

        //CONFORMANCE_ISSUE
        pInfo->img.PreviousFrameNum = pInfo->img.frame_num;
        pInfo->img.PreviousFrameNumOffset = pInfo->img.FrameNumOffset;
    }
    break;
    default:
        break;
    }

    return;
}  //// End of decoding_POC

/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------------------------ */
//////////////////////////////////////////////////////////////////////////////
// h264_hdr_post_poc ()
//
//////////////////////////////////////////////////////////////////////////////

void h264_hdr_post_poc(h264_Info* pInfo, int32_t NonExisting, int32_t frame_num, int32_t use_old)
{
    int32_t actual_frame_num = (NonExisting)? frame_num :
                               (use_old)?	pInfo->old_slice.frame_num :
                               pInfo->img.frame_num;

    int32_t disposable_flag = (use_old)?(pInfo->old_slice.nal_ref_idc == 0) :
                              (pInfo->SliceHeader.nal_ref_idc == 0);

    switch (pInfo->img.pic_order_cnt_type)
    {
    case  0: {
        pInfo->img.PreviousFrameNum   = actual_frame_num;
        if ((disposable_flag == 0) && (NonExisting == 0))
        {
            pInfo->img.PrevPicOrderCntLsb = (use_old)? pInfo->old_slice.pic_order_cnt_lsb :
                                            pInfo->SliceHeader.pic_order_cnt_lsb;
            pInfo->img.PicOrderCntMsb     = pInfo->img.CurrPicOrderCntMsb;
        }
    }
    break;
    case  1: {
        pInfo->img.PreviousFrameNum       = actual_frame_num;
        pInfo->img.PreviousFrameNumOffset = pInfo->img.FrameNumOffset;
    }
    break;
    case  2: {
        pInfo->img.PreviousFrameNum       = actual_frame_num;
        pInfo->img.PreviousFrameNumOffset = pInfo->img.FrameNumOffset;

    }
    break;

    default: {
    } break;
    }

    return;
} ///// End of h264_hdr_post_poc


