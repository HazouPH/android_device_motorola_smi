/*
 * Copyright (C) 2008 The Android Open Source Project
 * Copyright (C) 2017 The LineageOS Project
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <hardware/lights.h>

static struct light_state_t g_lights[3];

/**
 * device methods
 */

static int
set_light_backlight(__attribute__((unused)) struct light_device_t* dev,
        struct light_state_t const* state)
{
    FILE *fd;
    int brightness = (state->color & 0xff) * 100 / 0xff;
    fd = fopen("/sys/class/backlight/psb-bl/brightness", "w");
    if (fd == NULL)
        return -errno;
    fprintf(fd, "%i", brightness);
    fclose(fd);
    return 0;
}

static int update_rgb_leds() {
    FILE *fd;
    struct light_state_t* current_state;
    if (g_lights[0].color & 0x00ffffff)
        current_state = &g_lights[0];
    else if (g_lights[1].color & 0x00ffffff)
        current_state = &g_lights[1];
    else
        current_state = &g_lights[2];
    fd = fopen("/sys/class/leds/rgb/control", "w");
    if (fd == NULL)
        return -errno;
    fprintf(fd, "%06X %d %d %d %d", current_state->color, current_state->flashOnMS, current_state->flashOffMS, 50, 50);
    fclose(fd);
    return 0;
}

static int
set_light_notifications(__attribute__((unused)) struct light_device_t* dev,
        struct light_state_t const* state)
{
    g_lights[1] = *state;
    return update_rgb_leds();
}

static int
set_light_attention(__attribute__((unused)) struct light_device_t* dev,
        struct light_state_t const* state)
{
    g_lights[0] = *state;
    return update_rgb_leds();
}

static int
set_light_battery(__attribute__((unused)) struct light_device_t* dev,
        struct light_state_t const* state)
{
    g_lights[2] = *state;
    return update_rgb_leds();
}

static int set_light_noop(__attribute__((unused)) struct light_device_t *dev,
            __attribute__((unused)) struct light_state_t const *state)
{
    return 0;
}

/** Close the lights device */
static int
close_lights(struct light_device_t *dev)
{
    if (dev) {
        free(dev);
    }
    return 0;
}

/******************************************************************************/

/**
 * module methods
 */

/** Open a new instance of a lights device using name */
static int open_lights(const struct hw_module_t* module, char const* name,
        struct hw_device_t** device)
{
    int (*set_light)(struct light_device_t* dev,
            struct light_state_t const* state);

    if (0 == strcmp(LIGHT_ID_BACKLIGHT, name))
        set_light = set_light_backlight;
    else if (0 == strcmp(LIGHT_ID_NOTIFICATIONS, name))
        set_light = set_light_notifications;
    else if (0 == strcmp(LIGHT_ID_ATTENTION, name))
        set_light = set_light_attention;
    else if (0 == strcmp(LIGHT_ID_BATTERY, name))
        set_light = set_light_battery;
    else if (0 == strcmp(LIGHT_ID_BUTTONS, name))
        set_light = set_light_noop;
    else
        return -EINVAL;

    struct light_device_t *dev = malloc(sizeof(struct light_device_t));
    memset(dev, 0, sizeof(*dev));

    dev->common.tag = HARDWARE_DEVICE_TAG;
    dev->common.version = 0;
    dev->common.module = (struct hw_module_t*)module;
    dev->common.close = (int (*)(struct hw_device_t*))close_lights;
    dev->set_light = set_light;

    *device = (struct hw_device_t*)dev;
    return 0;
}

static struct hw_module_methods_t lights_module_methods = {
    .open =  open_lights,
};

/*
 * The lights Module
 */
struct hw_module_t HAL_MODULE_INFO_SYM = {
    .tag = HARDWARE_MODULE_TAG,
    .version_major = 1,
    .version_minor = 0,
    .id = LIGHTS_HARDWARE_MODULE_ID,
    .name = "razr-i Lights Module",
    .author = "The LineageOS Project",
    .methods = &lights_module_methods,
};
