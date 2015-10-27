LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_COPY_HEADERS_TO := libttm

LOCAL_COPY_HEADERS :=			\
		psb_ttm_fence_user.h	\
		psb_ttm_placement_user.h\

include $(BUILD_COPY_HEADERS)

include $(CLEAR_VARS)

LOCAL_COPY_HEADERS_TO := libttm/ttm

LOCAL_COPY_HEADERS :=			\
		ttm_placement.h\

include $(BUILD_COPY_HEADERS)
