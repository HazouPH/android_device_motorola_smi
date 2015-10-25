# Copyright (C) 2013 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

#
# Android.mk for PCG libraries
#

LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_PREBUILT_LIBS := libpcg.a libirc_pcg.a libsvml_pcg.a
LOCAL_MODULE_TAGS := optional
include $(BUILD_MULTI_PREBUILT)

$(LOCAL_BUILT_MODULE) : check_libpcg_consistency
#
# libpcg light consistency check
#
define post_build
check_libpcg_consistency: $(1)/libpcg.log
	echo "Check libpcg package for consistency:"
	cd $(1); md5sum -c libpcg.log
endef
$(eval $(call post_build,$(LOCAL_PATH)))

#
# Build for the host.
#

ifeq ($(WITH_HOST_DALVIK),true)
    include $(CLEAR_VARS)
    LOCAL_PREBUILT_LIBS := libpcg_host.a libirc_pcg.a libsvml_pcg.a
    LOCAL_MODULE_TAGS := optional
    LOCAL_IS_HOST_MODULE := true
    include $(BUILD_MULTI_PREBUILT)
endif
