/*
 * Copyright (C) 2012 The Android Open Source Project
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
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <fcntl.h>
#include<time.h>
#include <limits.h>


#define LOG_TAG "Intel PowerHAL"
#include <utils/Log.h>

#include <hardware/hardware.h>
#include <hardware/power.h>

#define TIMER_RATE_SYSFS    "/sys/devices/system/cpu/cpufreq/interactive/timer_rate"
#define BOOST_PULSE_SYSFS    "/sys/devices/system/cpu/cpufreq/interactive/boostpulse"
#define TOUCHBOOST_PULSE_SYSFS    "/sys/devices/system/cpu/cpufreq/interactive/touchboostpulse"
#define TOUCHBOOST_SYSFS    "/sys/devices/system/cpu/cpufreq/interactive/touchboost_freq"

#define EARLY_SUSPEND_SYSFS_DIR    "/sys/power/early_suspend"
#define EARLY_SUSPEND_SYSFS_NAME   "early_suspend"
#define EARLY_SUSPEND_OFF          "0"
#define EARLY_SUSPEND_ON           "1"

/*
 * This parameter is to identify continuous touch/scroll events.
 * Any two touch hints received between a 20 interval ms is
 * considered as a scroll event.
 */
#define SHORT_TOUCH_TIME 20

/*
 * This parameter is to identify first touch events.
 * Any two touch hints received after 100 ms is considered as
 * a first touch event.
 */
#define LONG_TOUCH_TIME 100

/*
 * This parameter defines the number of vsync boost to be
 * done after the finger release event.
 */
#define VSYNC_BOOST_COUNT 4

/*
 * This parameter defines the time between a touch and a vsync
 * hint. the time if is > 30 ms, we do a vsync boost.
 */
#define VSYNC_TOUCH_TIME 30

struct intel_power_module{
    struct power_module container;
    int touchboost_disable;
    int timer_set;
    int vsync_boost;
};

static void sysfs_write(char *path, char *s)
{
    char buf[80];
    int len;
    int fd = open(path, O_WRONLY);

    if (fd < 0) {
        strerror_r(errno, buf, sizeof(buf));
        ALOGE("Error opening %s: %s\n", path, buf);
        return;
    }

    len = write(fd, s, strlen(s));
    if (len < 0) {
        strerror_r(errno, buf, sizeof(buf));
        ALOGE("Error writing to %s: %s\n", path, buf);
    }

    close(fd);
}
static int sysfs_read(char *path, char *s, int num_bytes)
{
    char buf[80];
    int count;
    int ret = 0;
    int fd = open(path, O_RDONLY);
     if (fd < 0) {
        strerror_r(errno, buf, sizeof(buf));
        ALOGE("Error reading from %s: %s\n", path, buf);
        return -1;
    }
    if ((count = read(fd, s, (num_bytes - 1))) < 0) {
        strerror_r(errno, buf, sizeof(buf));
        ALOGE("Error reading from  %s: %s\n", path, buf);
        ret = -1;
    } else {
        s[count] = '\0';
    }
    close(fd);
    return ret;
}

static int filter_non_suspend(const struct dirent *d)
{
    return !(strcmp(d->d_name, EARLY_SUSPEND_SYSFS_NAME));
}

static void free_dir_list(struct dirent **dir_list, int entries)
{
    int index;

    for (index = 0; index < entries; index++)
        free(dir_list[index]);

    free(dir_list);
}

static int select_suspend_dir(const struct dirent *d)
{
    struct dirent **device_list = NULL;
    int i, entries = 0;
    char path[PATH_MAX];

    /* filter non dir except symlink*/
    if (!((d->d_type & DT_DIR) || (d->d_type & DT_LNK)))
        return 0;

    /* filter dot dir */
    if (!(strcmp(d->d_name, "..") && strcmp(d->d_name, ".")))
        return 0;

    snprintf(path, PATH_MAX, "%s/%s", EARLY_SUSPEND_SYSFS_DIR, d->d_name);

    /* scan every dir and look for suspend file */
    entries = scandir(path, &device_list, filter_non_suspend, NULL);

    /* free device_list */
    free_dir_list(device_list, entries);

    /* return 0 for non suspend dir*/
    if (entries < 0)
        return 0;

    return 1;
}

static int get_early_suspend_devices(struct dirent ***dir_list)
{
    struct dirent **device_list = NULL;
    int i, entries = 0;

    entries = scandir(EARLY_SUSPEND_SYSFS_DIR, &device_list,
                    select_suspend_dir, NULL);

    if ( entries < 0 ) {
        ALOGE("Error scanning early suspend sysfs\n");
        goto out;
    }

    *dir_list = device_list;

out:
    return entries;
}

static void handle_device_suspend(struct dirent **device_list,
                    int entries, int on)
{
    int i;
    char path[PATH_MAX];

    for (i = 0; i < entries; i++) {
        snprintf(path, PATH_MAX, "%s/%s/%s",
                EARLY_SUSPEND_SYSFS_DIR,
                device_list[i]->d_name,
                EARLY_SUSPEND_SYSFS_NAME);

        if (on)
            sysfs_write(path, EARLY_SUSPEND_OFF);
        else
            sysfs_write(path, EARLY_SUSPEND_ON);
    }
}

static void intel_power_init(struct power_module *module)
{
    ALOGW("**Intel Power HAL initialisation**\n");

#if NO_TOUCH_BOOST
#if (POWERHAL_MFLD || POWERHAL_MRFLD)
    sysfs_write(TOUCHBOOST_SYSFS, "1333000");
#endif

#if POWERHAL_BYT
    sysfs_write(TOUCHBOOST_SYSFS, "1463000");
#endif

#if POWERHAL_CLV
    sysfs_write(TOUCHBOOST_SYSFS, "933000");
#endif
#endif
}

static void intel_power_set_interactive(struct power_module *module, int on)
{
#ifdef EARLY_SUSPEND_SUPPORT
    struct dirent **device_list = NULL;
    int entries = 0;

    entries = get_early_suspend_devices(&device_list);

    handle_device_suspend(device_list, entries, on);

    free_dir_list(device_list, entries);
#endif
}

static void intel_power_hint(struct power_module *module, power_hint_t hint,
                       void *data) {
    struct intel_power_module *intel = (struct intel_power_module *) module;
    char sysfs_val[80];
    static clock_t curr_time, prev_time , vsync_time;
    float diff;
    static int vsync_count;
    static int consecutive_touch_int;

    switch (hint) {
        case POWER_HINT_INTERACTION:
            curr_time = clock();
            diff = (((float)curr_time -
                            (float)prev_time) / CLOCKS_PER_SEC ) * 1000;
            prev_time = curr_time;
            if(diff < SHORT_TOUCH_TIME)
                consecutive_touch_int ++;
            else if (diff > LONG_TOUCH_TIME) {
                intel->vsync_boost = 0;
                intel->timer_set = 0;
                intel->touchboost_disable = 0;
                vsync_count = 0;
                consecutive_touch_int = 0;
            }
            /* Simple touch: timer rate need not be changed here */
            if((diff < SHORT_TOUCH_TIME) && (intel->touchboost_disable == 0)
                            && (consecutive_touch_int > 4))
                intel->touchboost_disable = 1;
            /*
            *Scrolling: timer rate reduced to increase sensitivity. No more touch
            *boost after this
            */
            if((intel->touchboost_disable == 1) && (consecutive_touch_int > 15)
                            && (intel->timer_set == 0)) {
                intel->timer_set = 1;
            }
            if (!intel->touchboost_disable) {
                sysfs_write(TOUCHBOOST_PULSE_SYSFS,"1");
            }
            break;
        case POWER_HINT_VSYNC:
            if(intel->touchboost_disable == 1) {
                vsync_time = clock();
                diff = (((float)vsync_time -
                            (float)curr_time) / CLOCKS_PER_SEC ) * 1000;
                if(diff > VSYNC_TOUCH_TIME) {
                    intel->timer_set = 0;
                    intel->vsync_boost = 1;
                    intel->touchboost_disable = 0;
                    vsync_count = VSYNC_BOOST_COUNT;
                }
            }
            if(intel->vsync_boost) {
                if((data == 1) && (vsync_count > 0)) {
                    sysfs_write(TOUCHBOOST_PULSE_SYSFS,"1");
                    vsync_count -- ;
                if(vsync_count == 0)
                   intel->vsync_boost = 0;
                }
            }
            break;
            default:
            break;
    }
}

static struct hw_module_methods_t power_module_methods = {
    .open = NULL,
};

struct intel_power_module HAL_MODULE_INFO_SYM = {
    container:{
            common: {
            tag: HARDWARE_MODULE_TAG,
            module_api_version: POWER_MODULE_API_VERSION_0_2,
            hal_api_version: HARDWARE_HAL_API_VERSION,
            id: POWER_HARDWARE_MODULE_ID,
            name: "Intel Power HAL",
            author: "Power Management Team",
            methods: &power_module_methods,
        },
    init: intel_power_init,
    setInteractive: intel_power_set_interactive,
#if NO_TOUCH_BOOST
    //powerHint: intel_power_hint,
#endif
    },
        touchboost_disable: 0,
        timer_set: 0,
        vsync_boost: 0,
};
