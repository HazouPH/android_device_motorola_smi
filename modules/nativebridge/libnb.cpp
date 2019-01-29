/*
 * Copyright (C) 2015-2017 The Android-x86 Open Source Project
 *
 * by Chih-Wei Huang <cwhuang@linux.org.tw>
 *
 * Licensed under the GNU General Public License Version 2 or later.
 * You may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.gnu.org/licenses/gpl.html
 *
 */

#define LOG_TAG "libnb"

#include <string.h>
#include <dlfcn.h>
#include <cutils/log.h>
#include "nativebridge/native_bridge.h"

namespace android {
    
extern "C" NativeBridgeCallbacks NativeBridgeItf;

static void *native_handle = nullptr;

// NativeBridgeCallbacks implementations
static bool native_bridge2_initialize(const NativeBridgeRuntimeCallbacks *art_cbs,
                                      const char *app_code_cache_dir,
                                      const char *isa)
{
    ALOGV("enter native_bridge2_initialize %s %s", app_code_cache_dir, isa);
    NativeBridgeCallbacks *callbacks;
    
    if (!native_handle) {
        native_handle = dlopen("/system/lib/libhoudini.so", RTLD_LAZY);
        if (!native_handle) {
            ALOGE("Unable to open /system/lib/libhoudini.so: %s", dlerror());
            return false;
        }
    }
    callbacks = reinterpret_cast<NativeBridgeCallbacks *>(dlsym(native_handle, "NativeBridgeItf"));
    ALOGI("Found %s version %u", "/system/lib/libhoudini.so", callbacks ? callbacks->version : 0);
    if (callbacks) {
        // v1
        NativeBridgeItf.loadLibrary = callbacks->loadLibrary;
        NativeBridgeItf.getTrampoline = callbacks->getTrampoline;
        NativeBridgeItf.isSupported = callbacks->isSupported;
        NativeBridgeItf.getAppEnv = callbacks->getAppEnv;
        // v2
        NativeBridgeItf.getSignalHandler = callbacks->getSignalHandler;
        return callbacks->initialize(art_cbs, app_code_cache_dir, isa);
    }
    ALOGW("libhoudini loaded but callbacks not found");
    return false;
}

static bool native_bridge2_isCompatibleWith(uint32_t version)
{
    ALOGV("enter native_bridge2_isCompatibleWith %u", version);
    return version <= 3;
}

static int native_bridge3_unloadLibrary(void *handle)
{
    ALOGV("enter native_bridge3_unloadLibrary %p", handle);
    return -1;
}

static const char *native_bridge3_getError()
{
    ALOGV("enter native_bridge3_getError");
    return "unknown";
}

static bool native_bridge3_isPathSupported(const char *path)
{
    const char *supported;
    const char *substr;
    ALOGV("enter native_bridge3_isPathSupported %s", path);

    supported = "armeabi-v7a";
    substr = path + strlen(path) - strlen(supported);
    if(substr > path && !strcmp(substr, supported))
        return true;
    supported = "armeabi";
    substr = path + strlen(path) - strlen(supported);
    if(substr > path && !strcmp(substr, supported))
        return true;

    ALOGV("%s is not supported", path);
    return false;
}

static bool native_bridge3_initAnonymousNamespace(const char *public_ns_sonames,
                                                  const char *anon_ns_library_path)
{
    ALOGV("enter native_bridge3_initAnonymousNamespace %s, %s", public_ns_sonames, anon_ns_library_path);
    return true;
}

static native_bridge_namespace_t *
native_bridge3_createNamespace(const char *name,
                               const char *ld_library_path,
                               const char *default_library_path,
                               uint64_t type,
                               const char *permitted_when_isolated_path,
                               native_bridge_namespace_t *parent_ns)
{
    ALOGV("enter native_bridge3_createNamespace %s, %s, %s, %s", name, ld_library_path, default_library_path, permitted_when_isolated_path);
    //returning nullptr would be seen as error, so return 1
    return (native_bridge_namespace_t *)1;
}

static bool native_bridge3_linkNamespaces(native_bridge_namespace_t *from,
                                          native_bridge_namespace_t *to,
                                          const char *shared_libs_soname)
{
    ALOGV("enter native_bridge3_linkNamespaces %s", shared_libs_soname);
    return true;
}

static void *native_bridge3_loadLibraryExt(const char *libpath,
                                           int flag,
                                           native_bridge_namespace_t *ns)
{
    ALOGV("enter native_bridge3_loadLibraryExt %s, %d, %p", libpath, flag, ns);
    void *result = NativeBridgeItf.loadLibrary(libpath, flag);
    ALOGV("native_bridge3_loadLibraryExt: %p", result);
    return result;
}

static void __attribute__ ((destructor)) on_dlclose()
{
    if (native_handle) {
        dlclose(native_handle);
        native_handle = nullptr;
    }
}

extern "C" NativeBridgeCallbacks NativeBridgeItf = {
    // v1
    .version = 3,
    .initialize = native_bridge2_initialize,
    .loadLibrary = nullptr,
    .getTrampoline = nullptr,
    .isSupported = nullptr,
    .getAppEnv = nullptr,
    // v2
    .isCompatibleWith = native_bridge2_isCompatibleWith,
    .getSignalHandler = nullptr,
    // v3
    .unloadLibrary = native_bridge3_unloadLibrary,
    .getError = native_bridge3_getError,
    .isPathSupported = native_bridge3_isPathSupported,
    .initAnonymousNamespace = native_bridge3_initAnonymousNamespace,
    .createNamespace = native_bridge3_createNamespace,
    .linkNamespaces = native_bridge3_linkNamespaces,
    .loadLibraryExt = native_bridge3_loadLibraryExt,
    // v4
    .getVendorNamespace = nullptr,
};

} // namespace android
