#include "viddec_mp4_decodevideoobjectplane.h"

mp4_Status_t mp4_DecodeVideoObjectPlane(mp4_Info_t* pInfo)
{
    mp4_Status_t status = MP4_STATUS_OK;
    uint32_t vop_time=0;
//    mp4_VisualObject_t *vo = &(pInfo->VisualObject);
    mp4_VideoObjectLayer_t *vol = &(pInfo->VisualObject.VideoObject);
    mp4_GroupOfVideoObjectPlane_t *gvop = &(pInfo->VisualObject.VideoObject.GroupOfVideoObjectPlane);
    mp4_VideoObjectPlane_t *vop = &(pInfo->VisualObject.VideoObject.VideoObjectPlane);

    // set VOP time
    if (vol->short_video_header)
    {
        vop_time = vol->vop_sync_time +
                   pInfo->VisualObject.VideoObject.VideoObjectPlaneH263.temporal_reference * 1001;

//        if (vo->currentFrame.time > vop_time)
        {
            vol->vop_sync_time += 256 * 1001;
            vop_time += 256 * 1001;
        }
    }
    else
    {
        if (vop->vop_coding_type == MP4_VOP_TYPE_B)
        {
            vop_time = vol->vop_sync_time_b + vop->modulo_time_base * vol->vop_time_increment_resolution + vop->vop_time_increment;
        }
        else
        {
            if (gvop->time_base > vol->vop_sync_time)
                vol->vop_sync_time = gvop->time_base;

            vop_time = vol->vop_sync_time + vop->modulo_time_base * vol->vop_time_increment_resolution + vop->vop_time_increment;

            if (vol->vop_sync_time_b < vol->vop_sync_time)
                vol->vop_sync_time_b = vol->vop_sync_time;

            if (vop->modulo_time_base != 0)
                vol->vop_sync_time = vop_time - vop->vop_time_increment;
        }
    }

    if (vop->vop_coded)
    {
        switch (vop->vop_coding_type)
        {
        case MP4_VOP_TYPE_S:
            if (vol->sprite_enable != MP4_SPRITE_GMC)
                break;
            // Deliberate fall-through from this case
        case MP4_VOP_TYPE_I:
        case MP4_VOP_TYPE_P:
            // set past and future time for B-VOP
            vol->pastFrameTime = vol->futureFrameTime;
            vol->futureFrameTime = vop_time;
            break;
        default:
            break;
        }
    }

    if (vop->vop_coded)
//     || (vop_time != vo->currentFrame.time && vop_time != vo->pastFrame.time && vop_time != vo->futureFrame.time) )
    {
        if (vop->vop_coding_type == MP4_VOP_TYPE_B)
        {
            if (!vol->Tframe)
                vol->Tframe = (int) (vop_time); // - vo->pastFrame.time);

            if (vop->vop_coded)
            {
                vol->TRB = (int) (vop_time - vol->pastFrameTime);
                vol->TRD = (int) (vol->futureFrameTime - vol->pastFrameTime);

                // defense from bad streams when B-VOPs are before Past and/or Future
                if (vol->TRB <= 0)
                    vol->TRB = 1;

                if (vol->TRD <= 0)
                    vol->TRD = 2;

                if (vol->TRD <= vol->TRB)
                {
                    vol->TRB = 1;
                    vol->TRD = 2;
                }

                if (vol->Tframe >= vol->TRD)
                    vol->Tframe = vol->TRB;
            }
        }
    }

    return status;
} // mp4_DecodeVideoObjectPlane

