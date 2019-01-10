#include <stdio.h>
#include <string.h>
#include <hardware/hardware.h>
//#include <hardware/intelvibrator.h>" FOR STOCK/VENDOR
#include <hardware/vibrator.h>

#define LOG_TAG "Vibrator Test"
#include <cutils/log.h>

// NEWNEWNEWNEWNEWNEW
static hw_module_t *gVibraModule = NULL;
static vibrator_device_t *gVibraDevice = NULL;

static void vibratorInit()
{
    ALOGI("%s", __FUNCTION__);
    if (gVibraModule != NULL) {
        return;
    }

    int err = hw_get_module_by_class("vibrator", "wrapper",
            (hw_module_t const**)&gVibraModule);

    if (err) {
        ALOGE("Couldn't load %s module (%s)", VIBRATOR_HARDWARE_MODULE_ID, strerror(-err));
    } else {
        if (gVibraModule) {
            vibrator_open(gVibraModule, &gVibraDevice);
        }
    }
}

static int vibratorExists()
{
    ALOGI("%s", __FUNCTION__);
    if (gVibraModule && gVibraDevice) {
    ALOGI("it does");
    return 0;
    } else {
    ALOGI("Damn it doesn't");
    return 1;
    }
}

static void vibratorOn(int timeout_ms)
{
    ALOGI("%s", __FUNCTION__);
    if (gVibraDevice) {
        int err = gVibraDevice->vibrator_on(gVibraDevice, timeout_ms);
        if (err != 0) {
            ALOGE("The hw module failed in vibrator_on: %s", strerror(-err));
        }
    } else {
        ALOGW("Tried to vibrate but there is no vibrator device.");
    }
}

static void vibratorOff()
{
    ALOGI("%s", __FUNCTION__);
    if (gVibraDevice) {
        int err = gVibraDevice->vibrator_off(gVibraDevice);
        if (err != 0) {
            ALOGE("The hw module failed in vibrator_off(): %s", strerror(-err));
        }
    } else {
        ALOGW("Tried to stop vibrating but there is no vibrator device.");
    }
}

int
main (void)
{
    ALOGI("Vibrator Wrapper Test");
    vibratorInit();
    vibratorOff();
    vibratorExists();
    vibratorOn(1000);
    sleep(2);
    return 0;
}


/* FOR STOCK/VENDOR
static vibrator_module_t *gVendorModule = 0;

int check_vendor_module()
{
    int rv = 0;

    ALOGI("Vibrator Vendor Load Test");

    rv = hw_get_module_by_class("vibrator", "vendor",
            (const hw_module_t **)&gVendorModule);

    if (rv)
        ALOGE("failed to open vendor vibrator module");
    return rv;
}

int intelVibraExists()
{
    ALOGI("%s", __FUNCTION__);
    if (check_vendor_module())
        return 0;
    return gVendorModule->vibrator_exists();
}

int intelVibraOn(int timeout_ms)
{
    ALOGI("%s", __FUNCTION__);
    if (check_vendor_module())
        return 0;
    return gVendorModule->vibrator_on(timeout_ms);
}

int intelVibraOff()
{
    ALOGI("%s", __FUNCTION__);
    if (check_vendor_module())
        return 0;
    return gVendorModule->vibrator_off();
}

int
main (void)
{
    ALOGI("Vibrator Test");
    intelVibraExists();
    intelVibraOn(1000);
    intelVibraOff();
    return 0;
}*/
