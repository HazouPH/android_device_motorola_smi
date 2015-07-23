#ifndef VIDDEC_MP4_SHORTHEADER_H
#define VIDDEC_MP4_SHORTHEADER_H
#include "viddec_fw_debug.h"
#include "viddec_parser_ops.h"
#include "viddec_mp4_parse.h"

mp4_Status_t mp4_Parse_VideoObjectPlane_svh(void *parent, viddec_mp4_parser_t *cxt);

mp4_Status_t mp4_Parse_VideoObject_svh(void *parent, viddec_mp4_parser_t *cxt);

#endif
