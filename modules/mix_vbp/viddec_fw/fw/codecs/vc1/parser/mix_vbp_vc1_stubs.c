#include "vc1.h"

void vc1_start_new_frame (void *parent, vc1_viddec_parser_t   *parser )
{
    return;
}

void vc1_end_frame (vc1_viddec_parser_t *parser)
{
    return;
}


int32_t vc1_parse_emit_current_frame( void *parent,  vc1_viddec_parser_t   *parser )
{
    return(0);
}


void vc1_parse_emit_frame_start(void *parent, vc1_viddec_parser_t *parser)
{
}

void vc1_parse_emit_second_field_start(void *parent, vc1_viddec_parser_t *parser)
{
}

void vc1_parse_emit_current_slice(void *parent, vc1_viddec_parser_t *parser)
{
}
