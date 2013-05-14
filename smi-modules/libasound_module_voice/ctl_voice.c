/*
 **
 ** Copyright 2011 Intel Corporation
 **
 ** Licensed under the Apache License, Version 2.0 (the "License");
 ** you may not use this file except in compliance with the License.
 ** You may obtain a copy of the License at
 **
 **      http://www.apache.org/licenses/LICENSE-2.0
 **
 ** Unless required by applicable law or agreed to in writing, software
 ** distributed under the License is distributed on an "AS IS" BASIS,
 ** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 ** See the License for the specific language governing permissions and
 ** limitations under the License.
 */

#define LOG_TAG "ALSAPlugInVoiceCTL"
#include <utils/Log.h>
#include <sys/poll.h>
#define _POSIX_C_SOURCE

#include <alsa/asoundlib.h>
#include <alsa/control_external.h>

#include "vpc_hardware.h"

typedef struct snd_ctl_voice {
    snd_ctl_ext_t ext;

    vpc_device_t *vpc;

    int subscribed;
    int updated;
} snd_ctl_voice_t;


#define VOICE_EARPIECE  "Voice Earpiece"
#define VOICE_SPEAKER   "Voice Speaker"
#define VOICE_HEADSET   "Voice Headset"
#define VOICE_BT        "Voice BT"
#define VOICE_HEADPHONE "Voice Headphone"

#define UPDATE_EARPIECE   0x01
#define UPDATE_SPEAKER    0x02
#define UPDATE_HEADSET    0x04
#define UPDATE_BT         0x08
#define UPDATE_HEADPHONE  0x10

typedef enum snd_ctl_ext_incall_key {
    VOICE_EARPIECE_INCALL = 0x0,
    VOICE_SPEAKER_INCALL,
    VOICE_HEADSET_INCALL,
    VOICE_BT_INCALL,
    VOICE_HEADPHONE_INCALL,
    EXT_INCALL
} snd_ctl_ext_incall_key_t;


static int voice_elem_count(snd_ctl_ext_t * ext)
{
    snd_ctl_voice_t *ctl = ext->private_data;
    int count = 0;

    assert(ctl);

    return count;
}

static int voice_elem_list(snd_ctl_ext_t * ext, unsigned int offset,
                           snd_ctl_elem_id_t * id)
{
    snd_ctl_voice_t *ctl = ext->private_data;
    int err = 0;

    assert(ctl);

    snd_ctl_elem_id_set_interface(id, SND_CTL_ELEM_IFACE_MIXER);

    return err;
}

static snd_ctl_ext_key_t voice_find_elem(snd_ctl_ext_t * ext,
        const snd_ctl_elem_id_t * id)
{
    const char *name;
    unsigned int numid;

    numid = snd_ctl_elem_id_get_numid(id);
    if (numid > 0 && numid <= EXT_INCALL)
        return numid - 1;

    name = snd_ctl_elem_id_get_name(id);
    ALOGD("%s : name = %s\n", __func__, name);

    if (strcmp(name, VOICE_EARPIECE) == 0)
        return VOICE_EARPIECE_INCALL;
    if (strcmp(name, VOICE_SPEAKER) == 0)
        return VOICE_SPEAKER_INCALL;
    if (strcmp(name, VOICE_HEADSET) == 0)
        return VOICE_HEADSET_INCALL;
    if (strcmp(name, VOICE_BT) == 0)
        return VOICE_BT_INCALL;
    if (strcmp(name, VOICE_HEADPHONE) == 0)
        return VOICE_HEADPHONE_INCALL;

    return SND_CTL_EXT_KEY_NOT_FOUND;
}

static int voice_get_attribute(snd_ctl_ext_t * ext, snd_ctl_ext_key_t key,
                               int *type, unsigned int *acc,
                               unsigned int *count)
{
    snd_ctl_voice_t *ctl = ext->private_data;
    int err = 0;

    if (key > EXT_INCALL + 1)
        return -EINVAL;

    assert(ctl);

    if (key & 1)
        *type = SND_CTL_ELEM_TYPE_BOOLEAN;
    else
        *type = SND_CTL_ELEM_TYPE_INTEGER;

    *acc = SND_CTL_EXT_ACCESS_READWRITE;

    *count = 1;

    return err;
}

static int voice_get_integer_info(snd_ctl_ext_t * ext,
                                  snd_ctl_ext_key_t key, long *imin,
                                  long *imax, long *istep)
{
    *istep = 1;
    *imin = 0;
    *imax = 100;

    return 0;
}

static int voice_read_integer(snd_ctl_ext_t * ext, snd_ctl_ext_key_t key,
                              long *value)
{
    snd_ctl_voice_t *ctl = ext->private_data;
    int err = 0;
    assert(ctl);

    switch (key) {
    case VOICE_EARPIECE_INCALL:
        break;
    case VOICE_SPEAKER_INCALL:
        break;
    case VOICE_HEADSET_INCALL:
        break;
    case VOICE_BT_INCALL:
        break;
    case VOICE_HEADPHONE_INCALL:
        break;
    default:
        err = -EINVAL;
        break;
    }

    return err;
}

static int voice_write_integer(snd_ctl_ext_t * ext, snd_ctl_ext_key_t key,
                               long *value)
{
    snd_ctl_voice_t *ctl = ext->private_data;
    int err = 1;

    assert(ctl);

    switch (key) {
    case VOICE_EARPIECE_INCALL:
        ALOGD("voice route to earpiece \n");
        err = ctl->vpc->route(VPC_ROUTE_OPEN);
        break;
    case VOICE_SPEAKER_INCALL:
        ALOGD("voice route to Speaker \n");
        err = ctl->vpc->route(VPC_ROUTE_OPEN);
        break;
    case VOICE_HEADSET_INCALL:
        ALOGD("voice route to headset \n");
       err = ctl->vpc->route(VPC_ROUTE_OPEN);
        break;
    case VOICE_BT_INCALL:
        ALOGD("voice route to BT \n");
        err = ctl->vpc->route(VPC_ROUTE_OPEN);
        break;
    case VOICE_HEADPHONE_INCALL:
        ALOGD("voice route to headphone \n");
        err = ctl->vpc->route(VPC_ROUTE_OPEN);
        break;
    default:
        err = -EINVAL;
        break;
    }

    return err;
}

static void voice_subscribe_events(snd_ctl_ext_t * ext, int subscribe)
{
    snd_ctl_voice_t *ctl = ext->private_data;

    assert(ctl);

    ctl->subscribed = !!(subscribe & SND_CTL_EVENT_MASK_VALUE);
}

static int voice_read_event(snd_ctl_ext_t * ext, snd_ctl_elem_id_t * id,
                            unsigned int *event_mask)
{
    snd_ctl_voice_t *ctl = ext->private_data;
    int err = 1;

    assert(ctl);

    if (ctl->updated & UPDATE_EARPIECE) {
        ctl->updated &= ~UPDATE_EARPIECE;
    } else if (ctl->updated & UPDATE_SPEAKER) {
        ctl->updated &= ~UPDATE_SPEAKER;
    } else if (ctl->updated & UPDATE_HEADSET) {
        ctl->updated &= ~UPDATE_HEADSET;
    } else if (ctl->updated & UPDATE_BT) {
        ctl->updated &= ~UPDATE_BT;
    } else if (ctl->updated & UPDATE_HEADPHONE) {
        ctl->updated &= ~UPDATE_HEADPHONE;
    }

    *event_mask = SND_CTL_EVENT_MASK_VALUE;

    return err;
}

static int voice_ctl_poll_revents(snd_ctl_ext_t * ext, struct pollfd *pfd,
                                  unsigned int nfds,
                                  unsigned short *revents)
{
    snd_ctl_voice_t *ctl = ext->private_data;
    int err = 0;

    assert(ctl);

    if (ctl->updated)
        *revents = POLLIN;
    else
        *revents = 0;

    return err;
}

static void voice_close(snd_ctl_ext_t * ext)
{
    snd_ctl_voice_t *ctl = ext->private_data;

    assert(ctl);

    ctl->vpc->route(VPC_ROUTE_CLOSE);

    free(ctl);
}

static const snd_ctl_ext_callback_t voice_ext_callback = {
    .elem_count = voice_elem_count,
    .elem_list = voice_elem_list,
    .find_elem = voice_find_elem,
    .get_attribute = voice_get_attribute,
    .get_integer_info = voice_get_integer_info,
    .read_integer = voice_read_integer,
    .write_integer = voice_write_integer,
    .subscribe_events = voice_subscribe_events,
    .read_event = voice_read_event,
    .poll_revents = voice_ctl_poll_revents,
    .close = voice_close,
};

SND_CTL_PLUGIN_DEFINE_FUNC(voice)
{
    snd_config_iterator_t i, next;
    int err;
    const char *device = NULL;
    hw_device_t *hw_device;
    snd_ctl_voice_t *ctl;

    snd_config_for_each(i, next, conf) {
        snd_config_t *n = snd_config_iterator_entry(i);
        const char *id;
        if (snd_config_get_id(n, &id) < 0)
            continue;
        if (strcmp(id, "comment") == 0 || strcmp(id, "type") == 0)
            continue;
        if (strcmp(id, "device") == 0) {
            if (snd_config_get_string(n, &device) < 0) {
                ALOGE("Invalid type for %s", id);
                return -EINVAL;
            }
            continue;
        }
        ALOGE("Unknown field %s", id);
        return -EINVAL;
    }

    ctl = calloc(1, sizeof(*ctl));
    if (!ctl)
        return -ENOMEM;

    hw_module_t *module;
    err = hw_get_module(VPC_HARDWARE_MODULE_ID, (hw_module_t const**)&module);

    if (err == 0) {
        err = module->methods->open(module, VPC_HARDWARE_NAME, &hw_device);
        if (err == 0) {
            ALOGD("VPC MODULE OK.");
            ctl->vpc = (vpc_device_t *) hw_device;
        }
        else {
            ALOGE("VPC Module not found");
            goto error;
        }
    }

    ctl->ext.version = SND_CTL_EXT_VERSION;
    ctl->ext.card_idx = 0;
    strncpy(ctl->ext.id, "voice", sizeof(ctl->ext.id) - 1);
    strncpy(ctl->ext.driver, "Voice plugin", sizeof(ctl->ext.driver) - 1);
    strncpy(ctl->ext.name, "Voice", sizeof(ctl->ext.name) - 1);
    strncpy(ctl->ext.longname, "Voice", sizeof(ctl->ext.longname) - 1);
    strncpy(ctl->ext.mixername, "Voice", sizeof(ctl->ext.mixername) - 1);

    ctl->ext.callback = &voice_ext_callback;
    ctl->ext.private_data = ctl;

    err = snd_ctl_ext_create(&ctl->ext, name, mode);
    if (err < 0)
        goto error;

    *handlep = ctl->ext.handle;
    return 0;

error:
    free(ctl);

    return err;
}

SND_CTL_PLUGIN_SYMBOL(voice);
