/**
 * viddec_mpeg2_frame_attr.c
 * -------------------------
 * This is a helper file for viddec_mpeg2_workload.c to translate the data
 * stored in the parser context into frame attributes in the workload.
 */

#include "viddec_mpeg2.h"

/* viddec_mpeg2_print_attr() - Prints collected frame attributes             */
static inline void viddec_mpeg2_print_attr(viddec_frame_attributes_t *attr)
{
    unsigned int index = 0;

    MPEG2_FA_DEB("Content_Size=%dx%d\n",        attr->cont_size.width,
                 attr->cont_size.height);
    MPEG2_FA_DEB("Repeat=%d\n",                 attr->mpeg2.repeat_first_field);
    MPEG2_FA_DEB("Frame_Type=%d\n",             attr->frame_type);
    MPEG2_FA_DEB("Temporal_Reference=%d\n",     attr->mpeg2.temporal_ref);
    MPEG2_FA_DEB("Top_Field_First=%d\n",        attr->mpeg2.top_field_first);
    MPEG2_FA_DEB("Progressive_Frame=%d\n",      attr->mpeg2.progressive_frame);
    MPEG2_FA_DEB("Picture_Struct=%d\n",         attr->mpeg2.picture_struct);
    MPEG2_FA_DEB("Pan_Scan_Offsets=%d\n",       attr->mpeg2.number_of_frame_center_offsets);

    for (index = 0; index < attr->mpeg2.number_of_frame_center_offsets; index++)
    {
        MPEG2_FA_DEB("\tPan_Scan_Offset_%d= %dx%d\n", index,
                     attr->mpeg2.frame_center_offset[index].horz,
                     attr->mpeg2.frame_center_offset[index].vert);
    }

    return;
}

/* viddec_mpeg2_set_default_values() - Resets attributes that are optional   */
/* in the bitstream to their default values.                                 */
static inline void viddec_mpeg2_set_default_values(viddec_frame_attributes_t *attrs)
{
    unsigned int index = 0;

    attrs->mpeg2.number_of_frame_center_offsets = 0;
    for (index = 0; index < MPEG2_MAX_VID_OFFSETS ; index++)
    {
        attrs->mpeg2.frame_center_offset[index].horz = 0;
        attrs->mpeg2.frame_center_offset[index].vert = 0;
    }

    return;
}

/* viddec_mpeg2_translate_attr() - Translates metadata parsed into frame     */
/* attributes in the workload                                                */
void viddec_mpeg2_translate_attr(void *parent, void *ctxt)
{
    /* Get MPEG2 Parser context */
    struct viddec_mpeg2_parser *parser = (struct viddec_mpeg2_parser *) ctxt;

    /* Get workload */
    viddec_workload_t *wl = viddec_pm_get_header( parent );

    /* Get attributes in workload */
    viddec_frame_attributes_t *attrs = &wl->attrs;

    /* Get the default values for optional attributes */
    viddec_mpeg2_set_default_values(attrs);

    /* Populate attributes from parser context */
    /* Content Size */
    attrs->cont_size.height         = ((parser->info.seq_ext.vertical_size_extension << 12)
                                       | parser->info.seq_hdr.vertical_size_value);
    attrs->cont_size.width          = ((parser->info.seq_ext.horizontal_size_extension << 12)
                                       | parser->info.seq_hdr.horizontal_size_value);

    /* Repeat field */
    attrs->mpeg2.repeat_first_field = parser->info.pic_cod_ext.repeat_first_field;

    /* Temporal Reference */
    attrs->mpeg2.temporal_ref       = parser->info.pic_hdr.temporal_reference;

    /* Top field first */
    attrs->mpeg2.top_field_first    = parser->info.pic_cod_ext.top_field_first;

    /* Progressive frame */
    attrs->mpeg2.progressive_frame  = parser->info.pic_cod_ext.progressive_frame;

    /* Picture Structure */
    attrs->mpeg2.picture_struct     = parser->info.pic_cod_ext.picture_structure;

    /* Populate the frame type */
    switch (parser->info.pic_hdr.picture_coding_type)
    {
    case MPEG2_PC_TYPE_I:
        attrs->frame_type = VIDDEC_FRAME_TYPE_I;
        break;
    case MPEG2_PC_TYPE_P:
        attrs->frame_type = VIDDEC_FRAME_TYPE_P;
        break;
    case MPEG2_PC_TYPE_B:
        attrs->frame_type = VIDDEC_FRAME_TYPE_B;
        break;
    default:
        attrs->frame_type = VIDDEC_FRAME_TYPE_INVALID;
    }

    /* Update PanScan data */
    if (parser->mpeg2_curr_frame_headers & MPEG2_HEADER_PIC_DISP_EXT)
    {
        unsigned int index = 0;
        attrs->mpeg2.number_of_frame_center_offsets = parser->mpeg2_num_pan_scan_offsets;
        for (index = 0; index < parser->mpeg2_num_pan_scan_offsets; index++)
        {
            attrs->mpeg2.frame_center_offset[index].horz = parser->info.pic_disp_ext.frame_center_horizontal_offset[index];
            attrs->mpeg2.frame_center_offset[index].vert = parser->info.pic_disp_ext.frame_center_vertical_offset[index];
        }
    }

    /* Print frame attributes */
    viddec_mpeg2_print_attr(attrs);

    return;
}
