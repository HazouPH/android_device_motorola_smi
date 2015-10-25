
#include "viddec_pm.h"
#include "viddec_fw_debug.h"
#include "viddec_fw_common_defs.h"
#include "viddec_pm_tags.h"
/*
  Overview of tag association:

  Contribution flags:
  The current list has all the buffers which contribute to this particular workload. So we walkthrough the
  list and throw buf done for all the buffers which were consumed. This can be deduced from total bytes we
  in list which represents the bytes that were used for this acces unit.
  For buffers which were partially used and this can only be the last buffer we throw continued tag. The
  Parser manager tells us when to throw a continued tag. This will only happen when parser Manager detects
  that we reached end of current frame.

  Association Tags:
  These are the tags that FW generates which indicates how to associate metadata with Frames.
  The policy to determine which tag belongs to which frame is based on sc prefix position. If ES buffer starts with
  or has a sc prefix its associated to next decodable frame(based on first slice or header depending on codec).
  We use three state variables to determine where the frame starts and ends.
    frame_start_found: Indicates we saw the beggining of frame in current list of ES buffers(which represent current acces unit).
                       This is decremented on workload done since it normally means we detected frame end.
    found_fm_st_in_current_au:Indicates we saw the first slice in current access unit. Its mainly used to decide whether the first buffer
                              belongs to current frame or next frame. Its reset after its use.
    Frame Done: Indicates we detected end of frame pointed by current workload.

   Basic algo:
   If we find frame start and if first buffer doesn't start with SC prefix Every consumed buffer belongs to Next frame. If first buffer
   starts with SC prefix on that buffer belongs to Current frame.
   If we haven't found frame start every buffer belongs to current frame.

   TODO: Check for return codes from emitter
*/


/*
  This function generates contribution tags current workload by walking through list of consumed buffers.
  If frame is done(ignore_partial is false) we generate continue tags for the last item in list(if its not completely consumed).
  This is used for all codecs except H264.
 */
uint32_t viddec_pm_generic_generate_contribution_tags(void *parent, uint32_t ignore_partial)
{
    uint32_t ret = PM_SUCCESS;
    viddec_pm_cxt_t *cxt = (viddec_pm_cxt_t *)parent;
    viddec_pm_utils_list_t *list = &(cxt->list);

    if (list->num_items != 0)
    {
        if (!cxt->late_frame_detect)
        {
            uint32_t num_items = 0;
            while ((num_items < list->num_items) && (list->data[num_items].edpos <= (uint32_t)list->total_bytes))
            {/* Walkthrough Consumed buffers and dump the tags */
                viddec_emit_contr_tag(&(cxt->emitter), &(list->sc_ibuf[num_items]), false, false);
                num_items++;
            }
            /* Dump incomplete tags if required */
            if (!ignore_partial)
            {/* check to see if last item is not consumed and dump continued flag */
                if ((num_items < list->num_items)
                        && (list->data[num_items].edpos >= (uint32_t)list->total_bytes))
                {
                    viddec_emit_contr_tag(&(cxt->emitter), &(list->sc_ibuf[num_items]), true, false);
                }
            }
        }
        else
        {
            /* Only happens for dangling fields in MP2 Field pictures, in which case we find out the current frame was done in
               last access unit, which is similar to H264 */
            ret = viddec_pm_lateframe_generate_contribution_tags(parent, ignore_partial);
            cxt->late_frame_detect = false;
        }
    }
    return ret;
}

/*
  For H264 when a frame is done it really means current frame was done in last access unit. The current access unit represnted
  by list belongs to next frame. ignore_partial is false for frame done.
  When frame is not done we dump all consumed buffers into next workload else they go to current workload.
  If frame is done we throw a continued flag for first buffer in current workload if it was used in last access unit.
 */
uint32_t viddec_pm_lateframe_generate_contribution_tags(void *parent, uint32_t ignore_partial)
{
    uint32_t ret = PM_SUCCESS;
    viddec_pm_cxt_t *cxt = (viddec_pm_cxt_t *)parent;
    viddec_pm_utils_list_t *list = &(cxt->list);

    if (list->num_items != 0)
    {
        uint32_t num_items = 0;
        /* If start offset is not 0 then it was partially used in last access unit. !ignore_partial means frame done*/
        if ((list->start_offset!= 0) && !ignore_partial)
        {/* Emit continue in current if necessary. */
            viddec_emit_contr_tag(&(cxt->emitter), &(list->sc_ibuf[num_items]), true, false);
        }

        while ((num_items < list->num_items) && (list->data[num_items].edpos <= (uint32_t)list->total_bytes))
        {  /* Walkthrough Consumed buffers and dump the tags to current or Next*/
            viddec_emit_contr_tag(&(cxt->emitter), &(list->sc_ibuf[num_items]), false, !ignore_partial);
            num_items++;
        }
    }
    return ret;
}

/*
  This function dumps tags from temporary array into a workload(we indicate either current or next from using_next).
*/
uint32_t viddec_pm_generate_missed_association_tags(viddec_pm_cxt_t *cxt, uint32_t using_next)
{
    uint32_t i=0, ret = PM_SUCCESS;

    while ((i < MAX_IBUFS_PER_SC) && (cxt->pending_tags.pending_tags[i] != INVALID_ENTRY))
    {
        viddec_emit_assoc_tag(&(cxt->emitter), cxt->pending_tags.pending_tags[i], using_next);
        cxt->pending_tags.pending_tags[i] = INVALID_ENTRY;
        i++;
    }
    return ret;
}

/* This function adds current list of es buffer to pending list. ignore_first when set tells us to ignore the first
   buffer in list.
*/
void viddec_pm_add_tags_to_pendinglist(viddec_pm_cxt_t *cxt, uint32_t ignore_first)
{
    viddec_pm_utils_list_t *list = &(cxt->list);
    vidded_pm_pending_tags_t *pend = &(cxt->pending_tags);
    uint32_t index=0, t_index=0;

    if (!ignore_first && (list->start_offset == 0))
    {/* If start offset is 0 we are saying that first buffer in list starts with start code */
        pend->first_buf_aligned = true;
    }
    else
    {/* We are ignoring first item in list since we already threw a tag for this buffer */
        index++;
        pend->first_buf_aligned  = false;
    }

    while ( (index < list->num_items) && (list->data[index].edpos <= (uint32_t)list->total_bytes))
    {
        if (t_index < MAX_IBUFS_PER_SC)
        { /* walk through consumed buffers and buffer id's in pending list */
            pend->pending_tags[t_index] = list->sc_ibuf[index].id;
            index++;
            t_index++;
        }
    }
    if ( (index < list->num_items) && (list->data[index].stpos < (uint32_t)list->total_bytes))
    {/* If last item is partially consumed still add it to pending tags since tag association is based on start of ES buffer */
        pend->pending_tags[t_index] = list->sc_ibuf[index].id;
    }
}

/* Helper function to emit a association tag from pending list and resetting the value to invalid entry */
static inline void viddec_pm_emit_pending_tag_item(viddec_emitter *emit, vidded_pm_pending_tags_t *pend, uint32_t index, uint32_t using_next)
{
    viddec_emit_assoc_tag(emit, pend->pending_tags[index], using_next);
    pend->pending_tags[index] = INVALID_ENTRY;
}

/*
  Tag association for mpeg2:
  start frame is detected in pict header extension, but pict header represents start of frame.
  To handle this we always store current AU list in temporary pending list. At the start of function
  we look to see if a frame start was found, if we did we start dumping items from pending list based
  on byte position of sc in first buffer of pending list. At the end we copy current list items to
  pending list.
  Limitation With Dangling fields: If we have AF1 AF2 BF1 CF1 CF2 as the sequence of fields
  Tag assocaiation will be fine for A & B, However the first buffer tag on C will fall into B
  We donot want to fix this issue right now as it means doubling size of pending list which
  increases memory usage. Normally dangling fields are thrown away so worst case we will miss
  one original PTS, So its OK not to fix it right now.
 */
uint32_t viddec_mpeg2_add_association_tags(void *parent)
{
    uint32_t ret = PM_SUCCESS;
    viddec_pm_cxt_t *cxt = (viddec_pm_cxt_t *)parent;
    vidded_pm_pending_tags_t *pend = &(cxt->pending_tags);
    uint32_t first_slice = false, index = 0;
    /* check to see if we found a frame start in current access unit */
    first_slice = cxt->frame_start_found && cxt->found_fm_st_in_current_au;
    cxt->found_fm_st_in_current_au = false;
    /* If we found frame start and first item in pending tags is start with start code
       then it needs to go to current frame. */
    if (first_slice && pend->first_buf_aligned && (pend->pending_tags[index] != INVALID_ENTRY))
    {
        viddec_pm_emit_pending_tag_item(&(cxt->emitter), pend, index, false);
        index++;
    }
    /* rest of list goes to current if frame start is not found else next frame */
    while ((index < MAX_IBUFS_PER_SC) && (pend->pending_tags[index] != INVALID_ENTRY))
    {
        viddec_pm_emit_pending_tag_item(&(cxt->emitter), pend, index, cxt->frame_start_found);
        index++;
    }
    /* Copy items to temporary List */
    viddec_pm_add_tags_to_pendinglist(cxt, false);
    return ret;
}

/*
  Tag association for h264:
  In this case when we get frame done it means current frame was done in last access unit. The data in current list belongs
  to next frame. To handle this we always dump the buffered tags from last list and throw them in current/next frame based on pend state.
  If the first item in current list is on sc boundary, it has to go into next so we always throw that tag in next.
  For rest of items we store them in pending tags array and store inforamtion on where these stored tags should go into for
  next run. Thi is detemined by start frame. we do this because at this state our next should be current and "next next" should
  be next.
 */
uint32_t viddec_h264_add_association_tags(void *parent)
{
    uint32_t ret = PM_SUCCESS;
    viddec_pm_cxt_t *cxt = (viddec_pm_cxt_t *)parent;
    viddec_pm_utils_list_t *list = &(cxt->list);
    vidded_pm_pending_tags_t *pend = &(cxt->pending_tags);
    uint32_t first_slice = false, index = 0;

    /* Throw tags for items from pending list based on stored state  from last run */
    viddec_pm_generate_missed_association_tags(cxt, pend->using_next);
    first_slice = cxt->frame_start_found && cxt->found_fm_st_in_current_au;
    cxt->found_fm_st_in_current_au = false;
    /* If we saw frame start and first buffer is aligned to start code throw it into next */
    if (first_slice && (list->start_offset == 0))
    {
        viddec_emit_assoc_tag(&(cxt->emitter), list->sc_ibuf[index].id, cxt->frame_start_found && cxt->pending_tags.frame_done);
        index++;
    }
    /* add tags to pending list */
    viddec_pm_add_tags_to_pendinglist(cxt, (index != 0));
    /* We want to figure out where these buffers should go into. There are three possible cases
       current: If no frame start found these should go into next.
       next: If one frame start is found and frame is not done then it should go to next.
             if a frame is done then pm will push current out and next time we come here previous next is current.
       next next: If two frame starts are found then we want it to be next next workload, which is what next will be
                  when we get called next time.
    */
    pend->using_next = (!cxt->pending_tags.frame_done && (cxt->frame_start_found == 1)) || (cxt->frame_start_found > 1);
    return ret;
}

/*
  Tag association for vc1:
  Frame header represents start of new frame. If we saw a frame start in current access unit and the buffer starts
  with start code it needs to go to current frame.  Rest of items go to next if frame start found else current frame.
 */
uint32_t viddec_generic_add_association_tags(void *parent)
{
    uint32_t ret = PM_SUCCESS;
    viddec_pm_cxt_t *cxt = (viddec_pm_cxt_t *)parent;
    viddec_pm_utils_list_t *list = &(cxt->list);
    uint32_t not_first_slice = false, index = 0;

    /* We check to see if this access unit is not the first one with frame start. This evaluates to true in that case */
    not_first_slice = cxt->frame_start_found && !cxt->found_fm_st_in_current_au;
    cxt->found_fm_st_in_current_au = false;
    if (list->start_offset == 0)
    {/* If start offset is 0, we have start code at beggining of buffer. If frame start was detected in this
        access unit we put the tag in current else it goes to next */
        viddec_emit_assoc_tag(&(cxt->emitter), list->sc_ibuf[index].id, not_first_slice);
    }
    /* Skip first item always, for start_offset=0 its already been handled above*/
    index++;
    while ( (index < list->num_items) && (list->data[index].edpos <= (uint32_t)list->total_bytes))
    {/* Walkthrough Consumed buffers and dump the tags to current or next*/
        viddec_emit_assoc_tag(&(cxt->emitter), list->sc_ibuf[index].id, cxt->frame_start_found);
        index++;
    }
    if ( (index < list->num_items) && (list->data[index].stpos < (uint32_t)list->total_bytes))
    {/* Dump last item if it was partially consumed */
        viddec_emit_assoc_tag(&(cxt->emitter), list->sc_ibuf[index].id, cxt->frame_start_found);
    }
    return ret;
}

/*
  This function throws tags for buffers which were not used yet during flush.
 */
void viddec_pm_generate_tags_for_unused_buffers_to_flush(viddec_pm_cxt_t *cxt)
{
    viddec_pm_utils_list_t *list;
    uint32_t index=0;

    list = &(cxt->list);
    /* Generate association tags from temporary pending array */
    viddec_pm_generate_missed_association_tags(cxt, false);
    if (list->num_items > 0)
    {
        /* Throw contribution flag for first item as done */
        viddec_emit_contr_tag(&(cxt->emitter), &(list->sc_ibuf[index]), false, false);
        if (cxt->list.start_offset == 0)
        {/* Throw association for first item if it was not done already */
            viddec_emit_assoc_tag(&(cxt->emitter), list->sc_ibuf[index].id, false);
        }
        index++;
        while (index < list->num_items)
        {/* Walk through list and throw contribution and association flags */
            viddec_emit_contr_tag(&(cxt->emitter), &(list->sc_ibuf[index]), false, false);
            viddec_emit_assoc_tag(&(cxt->emitter), list->sc_ibuf[index].id, false);
            index++;
        }
    }
    /* Not required to re init list structure as flush takes care of it */
}

