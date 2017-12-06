/*
 * Copyright (C) 2012, The CyanogenMod Project
 * Copyright (C) 2017, The Lineage Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
* @file VibratorWrapper.cpp
*
* This file wraps a vendor vibrator module.
*
*/

//#define LOG_NDEBUG 0
#define LOG_PARAMETERS

#define LOG_TAG "VibratorWrapper"
#include <cutils/log.h>

#include <utils/threads.h>
#include <utils/String8.h>
#include <hardware/hardware.h>
#include <hardware/intelvibrator.h>
#include <hardware/vibrator.h>

using namespace android;

static vibrator_module_t *gVendorModule = 0;

static int check_vendor_module()
{
    int rv = 0;

    if(gVendorModule)
        return 0;

    ALOGI("%s", __FUNCTION__);

    rv = hw_get_module_by_class("vibrator", "vendor",
            (const hw_module_t **)&gVendorModule);

    if (rv)
        ALOGE("failed to open vendor vibrator module");
    return rv;
}

/*******************************************************************
 * implementation of vibrator_device_ops functions
 *******************************************************************/

static int vibra_exists()
{
    ALOGI("%s", __FUNCTION__);
    if (check_vendor_module())
        return 0;
    return gVendorModule->vibrator_exists();
}

static int vibra_on(vibrator_device_t* vibradev __unused, unsigned int timeout_ms)
{
    ALOGI("%s", __FUNCTION__);
    if (check_vendor_module())
        return 0;
    return gVendorModule->vibrator_on(timeout_ms);
}

static int vibra_off(vibrator_device_t* vibradev __unused)
{
    ALOGI("%s", __FUNCTION__);
    if (check_vendor_module())
        return 0;
    return gVendorModule->vibrator_off();
}

static int vibra_close(hw_device_t *device)
{
    free(device);
    return 0;
}

static int vibra_open(const hw_module_t* module, const char* id __unused,
                      hw_device_t** device __unused) {
    if (!vibra_exists()) {
        ALOGE("Vibrator device does not exist. Cannot start vibrator");
        return -ENODEV;
    }

    vibrator_device_t *vibradev = (vibrator_device_t*)calloc(1, sizeof(vibrator_device_t));

    if (!vibradev) {
        ALOGE("Can not allocate memory for the vibrator device");
        return -ENOMEM;
    }

    vibradev->common.tag = HARDWARE_DEVICE_TAG;
    vibradev->common.module = (hw_module_t *) module;
    vibradev->common.version = HARDWARE_DEVICE_API_VERSION(1,0);
    vibradev->common.close = vibra_close;

    vibradev->vibrator_on = vibra_on;
    vibradev->vibrator_off = vibra_off;

    *device = (hw_device_t *) vibradev;

    return 0;
}

static struct hw_module_methods_t vibrator_module_methods = {
    .open = vibra_open,
};

struct hw_module_t HAL_MODULE_INFO_SYM = {
    .tag = HARDWARE_MODULE_TAG,
    .module_api_version = VIBRATOR_API_VERSION,
    .hal_api_version = HARDWARE_HAL_API_VERSION,
    .id = VIBRATOR_HARDWARE_MODULE_ID,
    .name = "Motorola Medfield Vibrator Wrapper",
    .author = "The LineageOS Project",
    .methods = &vibrator_module_methods,
};
