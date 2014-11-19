#include <dlfcn.h>
#include <cutils/properties.h>
#include <cutils/log.h>
#include <elf.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

/******************************************************************************
 * Init interface pointer between hook and libhoudini
 *****************************************************************************/
#define HOUDINI_PATH            "/system/lib/libhoudini.so"
#define HOUDINI_BUILD_PROP      "sys.app.houdini"

typedef int   (*HOUDINI_INIT)(void*);
typedef void* (*HOUDINI_DLOPEN)(const char*, int);
typedef void* (*HOUDINI_DLSYM)(void*, const char*);
typedef int   (*HOUDINI_NATIVE_HELPER)(bool, void*, unsigned char,
        void*, unsigned int, const unsigned char*, const void **);
typedef bool  (*HOUDINI_NEEDED)(void*);
typedef int   (*HOUDINI_CREATE_ACTIVITY)(void*, void*, void*, void*, size_t);

struct houdiniHook {
    /*libhoudini function pointers and init flag */
    void*                       handle;
    HOUDINI_INIT                dvm2hdInit;
    HOUDINI_DLOPEN              dvm2hdDlopen;
    HOUDINI_DLSYM               dvm2hdDlsym;
    HOUDINI_NATIVE_HELPER       dvm2hdNativeMethodHelper;
    HOUDINI_NEEDED              dvm2hdNeeded;
    HOUDINI_CREATE_ACTIVITY     androidrt2hdCreateActivity;
};

//Need this pointer to unify gHoudini in dalvik and native activity module
extern void* gHoudiniHook;

extern const char* dvmGetMethodShorty(void*);

static struct houdiniHook *gHoudini = NULL;

bool houdiniHookInit() {

    struct dvm2hdEnv {
        void *logger;
        void *getShorty;
    } env;

    char propBuf[PROPERTY_VALUE_MAX];
    property_get(HOUDINI_BUILD_PROP, propBuf, "");
    //setting HOUDINI_BUILD_PROP to "on" or do not set this
    //property will enable houdini
    if(strcmp(propBuf, "on") && strcmp(propBuf, "")) {
        return false;
    }

    env.logger = (void*)__android_log_print;
    env.getShorty = (void*)dvmGetMethodShorty;
    if (!gHoudiniHook) {
        gHoudini = (struct houdiniHook*)malloc(sizeof(struct houdiniHook));
        if (gHoudini == NULL) {
            ALOGE("libhoudini memory allocation failed!\n");
            return false;
        }

        //TODO: hard code the path currently
        void *handle = dlopen(HOUDINI_PATH,RTLD_NOW);
        if (handle == NULL) {
            return false;
        }
        gHoudini->handle = handle; // Record down the handle in global data structure
                                   // in case we need to close it later
        gHoudini->dvm2hdInit = (HOUDINI_INIT)dlsym(handle, "dvm2hdInit");
        if (gHoudini->dvm2hdInit == NULL) {
            ALOGE("Cannot find symbol dvm2hdInit, please check the "
                    "libhoudini library is correct: %s!\n", dlerror());
            return false;
        }
        if (!gHoudini->dvm2hdInit((void*)&env)) {
            ALOGE("libhoudini init failed!\n");
            return false;
        }

        gHoudini->dvm2hdDlopen = (HOUDINI_DLOPEN)dlsym(handle, "dvm2hdDlopen");
        gHoudini->dvm2hdDlsym = (HOUDINI_DLSYM)dlsym(handle, "dvm2hdDlsym");
        gHoudini->dvm2hdNeeded = (HOUDINI_NEEDED)dlsym(handle, "dvm2hdNeeded");
        gHoudini->dvm2hdNativeMethodHelper =
            (HOUDINI_NATIVE_HELPER)dlsym(handle, "dvm2hdNativeMethodHelper");
        gHoudini->androidrt2hdCreateActivity =
            (HOUDINI_CREATE_ACTIVITY)dlsym(handle, "androidrt2hdCreateActivity");
        if (!gHoudini->dvm2hdDlopen || !gHoudini->dvm2hdDlsym
                || !gHoudini->dvm2hdNeeded || !gHoudini->dvm2hdNativeMethodHelper
                || !gHoudini->androidrt2hdCreateActivity) {
            ALOGE("The library symbol is missing, please check the libhoudini "
                    "library is correct: %s!\n", dlerror());
            return false;
        }
        gHoudiniHook = (void*)gHoudini;
    } else {
        gHoudini = (struct houdiniHook*)gHoudiniHook;
    }

    return true;
}

/******************************************************************************
 * hook interfaces for dalvik and native activity
 *****************************************************************************/
namespace houdini {

/******************************************************************************
 * For register JNI method and JNI downcall
 *****************************************************************************/
bool hookCheckMethod(void *fnPtr) {
    if (gHoudini == NULL)
        gHoudini = (struct houdiniHook*)gHoudiniHook;

    if (gHoudini && gHoudini->dvm2hdNeeded(fnPtr))
        return true;
    else
        return false;
}

//Assume gHoudini has been initialized
void dvmHookPlatformInvoke(void* pEnv, void* clazz, int argInfo, int argc,
    const int* argv, const char* shorty, void* func, void* pReturn)
{
    const int kMaxArgs = argc+2;    /* +1 for env, maybe +1 for clazz */
    unsigned char types[kMaxArgs+1];
    void* values[kMaxArgs];
    char retType;
    char sigByte;
    int dstArg;

    types[0] = 'L';
    values[0] = &pEnv;

    types[1] = 'L';
    if (clazz != NULL) {
        values[1] = &clazz;
    } else {
        values[1] = (void*)argv++;
    }
    dstArg = 2;

    /*
     * Scan the types out of the short signature.  Use them to fill out the
     * "types" array.  Store the start address of the argument in "values".
     */
    retType = *shorty;
    while ((sigByte = *++shorty) != '\0') {
        types[dstArg] = sigByte;
        values[dstArg++] = (void*)argv++;
        if (sigByte == 'D' || sigByte == 'J') {
            argv++;
        }
    }
    types[dstArg] = '\0';

    if (gHoudini)
        gHoudini->dvm2hdNativeMethodHelper(false, func, retType, pReturn, dstArg,
            types, (const void**)values);
    else
        ALOGE("Houdini has not been initialized!");
}


/******************************************************************************
 * For hook dlopen, dlsym and ABI2 func execution
 *****************************************************************************/
static bool hookCheckABI2Header(const char *filename) {
    int fd = -1;
    unsigned char header[64];
    Elf32_Ehdr *hdr;

    if ((fd = open(filename, O_RDONLY)) == -1)
        return true; // open fail, probablly the file isn't exist, return true to keep align with bionic linker's implementation.
                     // The linker will check libname first instead of its existence

    if (lseek(fd, 0, SEEK_SET) < 0)
        goto fail;

    if (read(fd, &header[0], 64) < 0)
        goto fail;

    close(fd);

    hdr = (Elf32_Ehdr*)header;
    if (hdr->e_machine == EM_ARM)
        return true;
    else
        return false;

fail:
    if (fd != -1)
        close(fd);
    return false;
}

void* hookDlopen(const char *filename, int flag, bool* useHoudini) {
    void *handle = dlopen(filename, flag);
    *useHoudini = false;

    if (handle != NULL)
        return handle;

    //Houdini will not handle non-ARM libraries.
    if (!hookCheckABI2Header(filename)) {
        return NULL;
    }

    //houdiniHookInit will make sure gHoudini initialized
    *useHoudini = houdiniHookInit();
    if (*useHoudini == true)
        return gHoudini->dvm2hdDlopen(filename, flag);
    else
        return NULL;
}

//Assume gHoudini has been initialized
void* hookDlsym(bool useHoudini, void* handle, const char* symbol) {
    if (useHoudini) {
        if (gHoudini)
            return gHoudini->dvm2hdDlsym(handle, symbol);
        else {
            ALOGE("Houdini has not been initialized!");
            return NULL;
        }
    } else
        return dlsym(handle, symbol);
}

//typedef int (*OnLoadFunc)(JavaVM*, void*);
typedef int (*OnLoadFunc)(void*, void*);
//Assume gHoudini has been initialized
int hookJniOnload(bool useHoudini, void* func, void* jniVm, void* arg) {
    if (useHoudini) {
        int version = 0;
        const void* argv[] = {jniVm, NULL};//{gDvm.jniVm, NULL};
        if (gHoudini)
            gHoudini->dvm2hdNativeMethodHelper(true, func, 'I', (void*)&version,
                2, NULL, argv);
        else
            ALOGE("Houdini has not been initialized!");
        return version;
    } else {
        return (*(OnLoadFunc)func)(jniVm, NULL);
    }
}

//typedef void (*CreateActivityFunc)(ANativeActivity* activity, void* savedState, size_t savedStateSize);
typedef void (*CreateActivityFunc)(void* activity, void* savedState, size_t savedStateSize);
//Assume gHoudini has been initialized
void hookCreateActivity(bool useHoudini, void* createActivityFunc, void* activity,
        void*houdiniActivity, void* savedState, size_t savedStateSize) {
    if (useHoudini) {
        if (gHoudini)
            gHoudini->androidrt2hdCreateActivity(createActivityFunc, activity,
                houdiniActivity, savedState, savedStateSize);
        else
            ALOGE("Houdini has not been initialized!");
    } else {
        (*(CreateActivityFunc)createActivityFunc)(activity, savedState, savedStateSize);
    }
}

} //namespace houdini
