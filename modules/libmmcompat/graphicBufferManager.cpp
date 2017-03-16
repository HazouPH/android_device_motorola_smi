#include <stdint.h>

#include <hardware/gralloc.h>

/* frameworks/native/include/ui/Rect.h */
class Rect;

/*
 * status_t GraphicBufferMapper::lock(buffer_handle_t handle,
 * uint32_t usage, const Rect& bounds, void** vaddr)
 */
extern "C" int _ZN7android19GraphicBufferMapper4lockEPK13native_handlejRKNS_4RectEPPv(
        buffer_handle_t handle, uint32_t usage, const Rect& bounds, void** vaddr);

/*
 * status_t GraphicBufferMapper::lock(buffer_handle_t handle,
 * int usage, const Rect& bounds, void** vaddr)
 */
extern "C" int _ZN7android19GraphicBufferMapper4lockEPK13native_handleiRKNS_4RectEPPv(
        buffer_handle_t handle, int usage, const Rect& bounds, void** vaddr)
{
    return _ZN7android19GraphicBufferMapper4lockEPK13native_handlejRKNS_4RectEPPv(
            handle, static_cast<uint32_t>(usage), bounds, vaddr);
}
