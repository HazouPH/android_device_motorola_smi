#ifndef VIDDEC_MP4_VISUALOBJECT_H
#define VIDDEC_MP4_VISUALOBJECT_H
#include "viddec_fw_debug.h"
#include "viddec_parser_ops.h"
#include "viddec_mp4_parse.h"

mp4_Status_t mp4_Parse_VisualSequence(void *parent, viddec_mp4_parser_t *parser);

mp4_Status_t mp4_Parse_VisualObject(void *parent, viddec_mp4_parser_t *parser);

mp4_Status_t mp4_Parse_UserData(void *parent, viddec_mp4_parser_t *parser);

#endif
