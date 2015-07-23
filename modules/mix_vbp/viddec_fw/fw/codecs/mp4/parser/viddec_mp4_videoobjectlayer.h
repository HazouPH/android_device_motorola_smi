#ifndef VIDDEC_MP4_VIDEOOBJECTLAYER_H
#define VIDDEC_MP4_VIDEOOBJECTLAYER_H
#include "viddec_fw_debug.h"
#include "viddec_parser_ops.h"
#include "viddec_mp4_parse.h"

void mp4_ResetVOL(mp4_Info_t *pInfo);

mp4_Status_t mp4_InitVOL(mp4_Info_t *pInfo);

mp4_Status_t mp4_FreeVOL(mp4_Info_t *pInfo);

mp4_Status_t mp4_Parse_VideoObjectLayer(void *parent, viddec_mp4_parser_t *cxt);



#endif
