/*
 * Copyright (c) 2017 The LineageOS Project
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

#define LOG_TAG "yamaha_audio_wrapper"
/* #define LOG_NDEBUG 0  */

#include <errno.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <pthread.h>
#include <fcntl.h>

#include <cutils/log.h>

#include <hardware/hardware.h>
#include <system/audio.h>
#include <hardware/audio.h>

#include "audiowrapper.h"

/* Set this variable to 1 to enable ALOGI */
int logwrapped = 0;

/* Input */
struct wrapper_in_stream {
    struct audio_stream_in *stream_in;
    struct lp_audio_stream_in *lp_stream_in;
    int in_use;
    pthread_mutex_t in_use_mutex;
    pthread_cond_t in_use_cond;
};

static struct wrapper_in_stream *in_streams = NULL;
static int n_in_streams = 0;
static pthread_mutex_t in_streams_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Output */
struct wrapper_out_stream {
    struct audio_stream_out *stream_out;
    struct lp_audio_stream_out *lp_stream_out;
    int in_use;
    pthread_mutex_t in_use_mutex;
    pthread_cond_t in_use_cond;
};

static struct wrapper_out_stream *out_streams = NULL;
static int n_out_streams = 0;
static pthread_mutex_t out_streams_mutex = PTHREAD_MUTEX_INITIALIZER;

/* HAL */
static struct lp_audio_hw_device *lp_hw_dev = NULL;
static void *dso_handle = NULL;
static int in_use = 0;
static pthread_mutex_t in_use_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t in_use_cond = PTHREAD_COND_INITIALIZER;

#define WAIT_FOR_FREE(in_use) do { pthread_mutex_lock(&(in_use ## _mutex)); \
                             while (in_use) { \
                                 pthread_cond_wait(&(in_use ## _cond), &(in_use ## _mutex)); \
                             } } while(0)

#define UNLOCK_FREE(in_use) do { pthread_cond_signal(&(in_use ## _cond)); \
                           pthread_mutex_unlock(&(in_use ## _mutex)); } while (0)

/* Generic wrappers for streams */
#define _WRAP_STREAM_LOCKED(name, function, direction, rettype, err, prototype, parameters, log) \
    static rettype wrapper_ ## direction ## _ ## name  prototype \
    { \
        rettype ret = err; \
        struct lp_audio_stream *lpstream; \
        struct lp_audio_stream_ ## direction *lpstream_ ## direction; \
        int i; \
    \
        if (logwrapped == 1) ALOGI log; \
        pthread_mutex_lock(& direction ## _streams_mutex); \
        for (i = 0; i < n_ ## direction ## _streams; i++) { \
            if (direction ## _streams[i].stream_ ## direction == (struct audio_stream_ ## direction*)stream) { \
                WAIT_FOR_FREE(direction ## _streams[i].in_use); \
                lpstream = (struct lp_audio_stream *)direction ## _streams[i].lp_stream_ ## direction; \
                lpstream_ ## direction = direction ## _streams[i].lp_stream_ ## direction; \
                ret = lpstream_ ## direction ->function parameters; \
                UNLOCK_FREE(direction ## _streams[i].in_use); \
                break; \
            } \
        } \
        pthread_mutex_unlock(& direction ## _streams_mutex); \
    \
        return ret; \
    }

#define WRAP_STREAM_LOCKED(name, direction, rettype, err, prototype, parameters, log) \
        _WRAP_STREAM_LOCKED(name, name, direction, rettype, err, prototype, parameters, log)


#define WRAP_STREAM_LOCKED_COMMON(name, direction, rettype, err, prototype, parameters, log) \
_WRAP_STREAM_LOCKED(name, common.name, direction, rettype, err, prototype, parameters, log)

/* Generic wrappers for HAL */
#define _WRAP_HAL_LOCKED(name, function, prototype, parameters, log) \
    static int wrapper_ ## name  prototype \
    { \
        int ret; \
    \
        if (logwrapped == 1) ALOGI log; \
        pthread_mutex_lock(&out_streams_mutex); \
        pthread_mutex_lock(&in_streams_mutex); \
    \
        WAIT_FOR_FREE(in_use); \
        ret = lp_hw_dev->function parameters; \
        UNLOCK_FREE(in_use); \
    \
        pthread_mutex_unlock(&in_streams_mutex); \
        pthread_mutex_unlock(&out_streams_mutex); \
    \
        return ret; \
    }

#define WRAP_HAL_LOCKED(name, prototype, parameters, log) \
        _WRAP_HAL_LOCKED(name, name, prototype, parameters, log)

#define _WRAP_HAL(name, function, rettype, prototype, parameters, log) \
    static rettype wrapper_ ## name  prototype \
    { \
        if (logwrapped == 1) ALOGI log; \
    \
        return lp_hw_dev->function parameters; \
    }

#define WRAP_HAL(name, rettype, prototype, parameters, log) \
        _WRAP_HAL(name, name, rettype, prototype, parameters, log)

/* Unused parameters */
#define unused_audio_hw_device	__attribute__((unused)) struct audio_hw_device

/* Input stream */

WRAP_STREAM_LOCKED(read, in, ssize_t, -ENODEV, (struct audio_stream_in *stream, void* buffer, size_t bytes),
            (lpstream_in, buffer, bytes), ("in_read"))

WRAP_STREAM_LOCKED(set_gain, in, int, -ENODEV, (struct audio_stream_in *stream, float gain),
            (lpstream_in, gain), ("in_set_gain: %f", gain))

WRAP_STREAM_LOCKED_COMMON(standby, in, int, -ENODEV, (struct audio_stream *stream),
            (lpstream), ("in_standby"))

WRAP_STREAM_LOCKED_COMMON(set_parameters, in, int, -ENODEV, (struct audio_stream *stream, const char *kv_pairs),
(lpstream, kv_pairs), ("in_set_parameters: %s", kv_pairs))

WRAP_STREAM_LOCKED_COMMON(get_sample_rate, in, uint32_t, 0, (const struct audio_stream *stream),
            (lpstream), ("in_get_sample_rate"))

WRAP_STREAM_LOCKED_COMMON(set_sample_rate, in, int, -ENODEV, (struct audio_stream *stream, uint32_t rate),
            (lpstream, rate), ("in_set_sample_rate: %u", rate))

WRAP_STREAM_LOCKED_COMMON(get_buffer_size, in, size_t, 0, (const struct audio_stream *stream),
            (lpstream), ("in_get_buffer_size"))

WRAP_STREAM_LOCKED_COMMON(get_channels, in, audio_channel_mask_t, 0, (const struct audio_stream *stream),
            (lpstream), ("in_get_channels"))

WRAP_STREAM_LOCKED_COMMON(get_format, in, audio_format_t, 0, (const struct audio_stream *stream),
            (lpstream), ("in_get_format"))

WRAP_STREAM_LOCKED_COMMON(set_format, in, int, -ENODEV, (struct audio_stream *stream, audio_format_t format),
            (lpstream, format), ("in_set_format: %u", format))

WRAP_STREAM_LOCKED_COMMON(dump, in, int, -ENODEV, (const struct audio_stream *stream, int fd),
            (lpstream, fd), ("in_dump: %d", fd))

WRAP_STREAM_LOCKED_COMMON(get_device, in, audio_devices_t, 0, (const struct audio_stream *stream),
            (lpstream), ("in_get_device"))

WRAP_STREAM_LOCKED_COMMON(set_device, in, int, -ENODEV, (struct audio_stream *stream, audio_devices_t device),
            (lpstream, device), ("in_set_device: %d", device))

WRAP_STREAM_LOCKED_COMMON(get_parameters, in, char*, NULL, (const struct audio_stream *stream, const char *keys),
            (lpstream, keys), ("in_get_parameters: %s", keys))

WRAP_STREAM_LOCKED_COMMON(add_audio_effect, in, int, -ENODEV, (const struct audio_stream *stream, effect_handle_t effect),
            (lpstream, effect), ("in_add_audio_effect"))

WRAP_STREAM_LOCKED_COMMON(remove_audio_effect, in, int, -ENODEV, (const struct audio_stream *stream, effect_handle_t effect),
            (lpstream, effect), ("in_remove_audio_effect"))

static void wrapper_close_input_stream(unused_audio_hw_device *dev,
                                       struct audio_stream_in *stream_in)
{
    struct lp_audio_stream_in *lp_stream_in = NULL;
    int i;

    pthread_mutex_lock(&in_streams_mutex);
    for (i = 0; i < n_in_streams; i++) {
        if (in_streams[i].stream_in == stream_in) {
            WAIT_FOR_FREE(in_streams[i].in_use);
            UNLOCK_FREE(in_streams[i].in_use);
            lp_stream_in = in_streams[i].lp_stream_in;
            free(in_streams[i].stream_in);
            pthread_mutex_destroy(&(in_streams[i].in_use_mutex));
            pthread_cond_destroy(&(in_streams[i].in_use_cond));
            n_in_streams--;
            memmove(in_streams + i,
                    in_streams + i + 1,
                    sizeof(struct wrapper_in_stream) * (n_in_streams - i));
            in_streams = realloc(in_streams,
                                  sizeof(struct wrapper_in_stream) * n_in_streams);
            if (logwrapped == 1) ALOGI("Closed wrapped input stream");
            break;
        }
    }
    if (lp_stream_in) {
        WAIT_FOR_FREE(in_use);
        lp_hw_dev->close_input_stream(lp_hw_dev, lp_stream_in);
        UNLOCK_FREE(in_use);
    }

    pthread_mutex_unlock(&in_streams_mutex);
}

uint32_t wrapper_get_input_frames_lost(__attribute__((unused))struct audio_stream_in *stream)
{
        return 0;
}

static int wrapper_get_capture_position(__attribute__((unused))const struct audio_stream_in *stream,
                                   __attribute__((unused))int64_t *frames, __attribute__((unused))int64_t *time)
{
    return 0;
}

static int wrapper_open_input_stream(unused_audio_hw_device *dev,
                                     audio_io_handle_t handle,
                                     audio_devices_t devices,
                                     struct audio_config *config,
                                     struct audio_stream_in **stream_in,
                                     __attribute__((unused)) audio_input_flags_t flags,
                                     __attribute__((unused)) const char *address,
                                     __attribute__((unused)) audio_source_t source)
{
    struct lp_audio_stream_in *lp_stream_in;
    int ret;

    pthread_mutex_lock(&in_streams_mutex);

    WAIT_FOR_FREE(in_use);
    ret = lp_hw_dev->open_input_stream(lp_hw_dev, handle, devices,
                                         config, &lp_stream_in);
    UNLOCK_FREE(in_use);

    if (ret == 0) {
        struct wrapper_in_stream *new_in_streams;

        new_in_streams = realloc(in_streams,
                              sizeof(struct wrapper_in_stream) * (n_in_streams + 1));
        if (!new_in_streams) {
            ALOGE("Can't allocate memory for wrapped stream, not touching original!");
            pthread_mutex_unlock(&in_streams_mutex);
            return -ENOMEM;
        }
        in_streams = new_in_streams;
        memset(&in_streams[n_in_streams], 0, sizeof(struct wrapper_in_stream));

        in_streams[n_in_streams].lp_stream_in = lp_stream_in;
        in_streams[n_in_streams].stream_in = malloc(sizeof(struct audio_stream_in));
        if (!in_streams[n_in_streams].stream_in) {
            ALOGE("Can't allocate memory for stream_in!");
            pthread_mutex_unlock(&in_streams_mutex);
            return -ENOMEM;
        }
        memset(in_streams[n_in_streams].stream_in, 0, sizeof(struct audio_stream_in));
        *stream_in = in_streams[n_in_streams].stream_in;

        (*stream_in)->common.get_sample_rate = wrapper_in_get_sample_rate;
        (*stream_in)->common.set_sample_rate = wrapper_in_set_sample_rate;
        (*stream_in)->common.get_buffer_size = wrapper_in_get_buffer_size;
        (*stream_in)->common.get_channels = wrapper_in_get_channels;
        (*stream_in)->common.get_format = wrapper_in_get_format;
        (*stream_in)->common.set_format = wrapper_in_set_format;
        (*stream_in)->common.standby = wrapper_in_standby;
        (*stream_in)->common.dump = wrapper_in_dump;
        (*stream_in)->common.get_device = wrapper_in_get_device;
        (*stream_in)->common.set_device = wrapper_in_set_device;
        (*stream_in)->common.set_parameters = wrapper_in_set_parameters;
        (*stream_in)->common.get_parameters = wrapper_in_get_parameters;
        (*stream_in)->common.add_audio_effect = wrapper_in_add_audio_effect;
        (*stream_in)->common.remove_audio_effect = wrapper_in_remove_audio_effect;

        (*stream_in)->set_gain = wrapper_in_set_gain;
        (*stream_in)->read = wrapper_in_read;
        (*stream_in)->get_input_frames_lost = wrapper_get_input_frames_lost;
        (*stream_in)->get_capture_position = wrapper_get_capture_position;

        in_streams[n_in_streams].in_use = 0;
        pthread_mutex_init(&(in_streams[n_in_streams].in_use_mutex), NULL);
        pthread_cond_init(&(in_streams[n_in_streams].in_use_cond), NULL);

        if (logwrapped == 1){
			ALOGI("Wrapped an input stream: rate %d, channel_mask: %x, format: %d, addr: %p/%p",
                  config->sample_rate, config->channel_mask, config->format, *stream_in, lp_stream_in);
		}
        n_in_streams++;
    }
    pthread_mutex_unlock(&in_streams_mutex);

    return ret;
}

/* Output stream */

WRAP_STREAM_LOCKED(write, out, int, -ENODEV, (struct audio_stream_out *stream, const void* buffer, size_t bytes),
            (lpstream_out, buffer, bytes), ("out_write"))

WRAP_STREAM_LOCKED(set_volume, out, int, -ENODEV, (struct audio_stream_out *stream, float left, float right),
            (lpstream_out, left, right), ("set_out_volume: %f/%f", left, right))

WRAP_STREAM_LOCKED_COMMON(standby, out, int, -ENODEV, (struct audio_stream *stream),
            (lpstream), ("out_standby"))

WRAP_STREAM_LOCKED_COMMON(set_parameters, out, int, -ENODEV, (struct audio_stream *stream, const char *kv_pairs),
(lpstream, kv_pairs), ("out_set_parameters: %s", kv_pairs))

WRAP_STREAM_LOCKED_COMMON(get_sample_rate, out, uint32_t, 0, (const struct audio_stream *stream),
            (lpstream), ("out_get_sample_rate"))

WRAP_STREAM_LOCKED_COMMON(set_sample_rate, out, int, -ENODEV, (struct audio_stream *stream, uint32_t rate),
            (lpstream, rate), ("out_set_sample_rate: %u", rate))

WRAP_STREAM_LOCKED_COMMON(get_buffer_size, out, size_t, 0, (const struct audio_stream *stream),
            (lpstream), ("out_get_buffer_size"))

WRAP_STREAM_LOCKED_COMMON(get_channels, out, audio_channel_mask_t, 0, (const struct audio_stream *stream),
            (lpstream), ("out_get_channels"))

WRAP_STREAM_LOCKED_COMMON(get_format, out, audio_format_t, 0, (const struct audio_stream *stream),
            (lpstream), ("out_get_format"))

WRAP_STREAM_LOCKED_COMMON(set_format, out, int, -ENODEV, (struct audio_stream *stream, audio_format_t format),
            (lpstream, format), ("out_set_format: %u", format))

WRAP_STREAM_LOCKED_COMMON(dump, out, int, -ENODEV, (const struct audio_stream *stream, int fd),
            (lpstream, fd), ("out_dump: %d", fd))

WRAP_STREAM_LOCKED_COMMON(get_device, out, audio_devices_t, 0, (const struct audio_stream *stream),
            (lpstream), ("out_get_device"))

WRAP_STREAM_LOCKED_COMMON(set_device, out, int, -ENODEV, (struct audio_stream *stream, audio_devices_t device),
            (lpstream, device), ("out_set_device: %d", device))

WRAP_STREAM_LOCKED_COMMON(get_parameters, out, char*, NULL, (const struct audio_stream *stream, const char *keys),
            (lpstream, keys), ("out_get_parameters: %s", keys))

WRAP_STREAM_LOCKED_COMMON(add_audio_effect, out, int, -ENODEV, (const struct audio_stream *stream, effect_handle_t effect),
            (lpstream, effect), ("out_add_audio_effect"))

WRAP_STREAM_LOCKED_COMMON(remove_audio_effect, out, int, -ENODEV, (const struct audio_stream *stream, effect_handle_t effect),
            (lpstream, effect), ("out_remove_audio_effect"))

WRAP_STREAM_LOCKED(get_latency, out, uint32_t, 0, (const struct audio_stream_out *stream),
            (lpstream_out), ("out_get_latency"))

WRAP_STREAM_LOCKED(get_render_position, out, int, -ENODEV, (const struct audio_stream_out *stream, uint32_t *dsp_frames),
            (lpstream_out, dsp_frames), ("out_get_render_position"))

WRAP_STREAM_LOCKED(get_next_write_timestamp, out, int, -ENODEV, (const struct audio_stream_out *stream, int64_t *timestamp),
            (lpstream_out, timestamp), NULL)

int wrapper_get_presentation_position(__attribute__((unused)) const struct audio_stream_out *stream,
                __attribute__((unused)) uint64_t *frames, __attribute__((unused)) struct timespec *timestamp)
{
        return -1;
}

static void wrapper_close_output_stream(unused_audio_hw_device *dev,
                            struct audio_stream_out* stream_out)
{
    struct lp_audio_stream_out *lp_stream_out = NULL;
    int i;

    pthread_mutex_lock(&out_streams_mutex);
    for (i = 0; i < n_out_streams; i++) {
        if (out_streams[i].stream_out == stream_out) {
            WAIT_FOR_FREE(out_streams[i].in_use);
            UNLOCK_FREE(out_streams[i].in_use);
            lp_stream_out = out_streams[i].lp_stream_out;
            free(out_streams[i].stream_out);
            pthread_mutex_destroy(&(out_streams[i].in_use_mutex));
            pthread_cond_destroy(&(out_streams[i].in_use_cond));
            n_out_streams--;
            memmove(out_streams + i,
                    out_streams + i + 1,
                    sizeof(struct wrapper_out_stream) * (n_out_streams - i));
            out_streams = realloc(out_streams,
                                  sizeof(struct wrapper_out_stream) * n_out_streams);
            if (logwrapped == 1) ALOGI("Closed wrapped output stream");
            break;
        }
    }

    if (lp_stream_out) {
        WAIT_FOR_FREE(in_use);
        lp_hw_dev->close_output_stream(lp_hw_dev, lp_stream_out);
        UNLOCK_FREE(in_use);
    }

    pthread_mutex_unlock(&out_streams_mutex);
}

static int wrapper_open_output_stream(unused_audio_hw_device *dev,
                                      audio_io_handle_t handle,
                                      audio_devices_t devices,
                                      audio_output_flags_t flags,
                                      struct audio_config *config,
                                      struct audio_stream_out **stream_out,
                                      __attribute__((unused)) const char *address)
{
    struct lp_audio_stream_out *lp_stream_out;
    int ret;

    pthread_mutex_lock(&out_streams_mutex);

    WAIT_FOR_FREE(in_use);
    ret = lp_hw_dev->open_output_stream(lp_hw_dev, handle, devices,
                                          flags, config, &lp_stream_out);
    UNLOCK_FREE(in_use);

    if (ret == 0) {
        struct wrapper_out_stream *new_out_streams;

        new_out_streams = realloc(out_streams,
                              sizeof(struct wrapper_out_stream) * (n_out_streams + 1));
        if (!new_out_streams) {
            ALOGE("Can't allocate memory for wrapped stream, not touching original!");
            pthread_mutex_unlock(&out_streams_mutex);
            return -ENOMEM;
        }
        out_streams = new_out_streams;
        memset(&out_streams[n_out_streams], 0, sizeof(struct wrapper_out_stream));

        out_streams[n_out_streams].lp_stream_out = lp_stream_out;
        out_streams[n_out_streams].stream_out = malloc(sizeof(struct audio_stream_out));
        if (!out_streams[n_out_streams].stream_out) {
            ALOGE("Can't allocate memory for stream_out!");
            pthread_mutex_unlock(&out_streams_mutex);
            return -ENOMEM;
        }
        memset(out_streams[n_out_streams].stream_out, 0, sizeof(struct audio_stream_out));
        *stream_out = out_streams[n_out_streams].stream_out;

        (*stream_out)->common.get_sample_rate = wrapper_out_get_sample_rate;
        (*stream_out)->common.set_sample_rate = wrapper_out_set_sample_rate;
        (*stream_out)->common.get_buffer_size = wrapper_out_get_buffer_size;
        (*stream_out)->common.get_channels = wrapper_out_get_channels;
        (*stream_out)->common.get_format = wrapper_out_get_format;
        (*stream_out)->common.set_format = wrapper_out_set_format;
        (*stream_out)->common.standby = wrapper_out_standby;
        (*stream_out)->common.dump = wrapper_out_dump;
        (*stream_out)->common.get_device = wrapper_out_get_device;
        (*stream_out)->common.set_device = wrapper_out_set_device;
        (*stream_out)->common.set_parameters = wrapper_out_set_parameters;
        (*stream_out)->common.get_parameters = wrapper_out_get_parameters;
        (*stream_out)->common.add_audio_effect = wrapper_out_add_audio_effect;
        (*stream_out)->common.remove_audio_effect = wrapper_out_remove_audio_effect;

        (*stream_out)->get_latency = wrapper_out_get_latency;
        (*stream_out)->set_volume = wrapper_out_set_volume;
        (*stream_out)->write = wrapper_out_write;
        (*stream_out)->get_render_position = wrapper_out_get_render_position;
        (*stream_out)->get_next_write_timestamp = wrapper_out_get_next_write_timestamp;
        (*stream_out)->set_callback = NULL;
        (*stream_out)->pause = NULL;
        (*stream_out)->resume = NULL;
        (*stream_out)->drain = NULL;
        (*stream_out)->flush = NULL;
        (*stream_out)->get_presentation_position = wrapper_get_presentation_position;

        out_streams[n_out_streams].in_use = 0;
        pthread_mutex_init(&(out_streams[n_out_streams].in_use_mutex), NULL);
        pthread_cond_init(&(out_streams[n_out_streams].in_use_cond), NULL);

        if (logwrapped == 1) {
			ALOGI("Wrapped an output stream: rate %d, channel_mask: %x, format: %d, addr: %p/%p",
                  config->sample_rate, config->channel_mask, config->format, *stream_out, lp_stream_out);
		}
        n_out_streams++;
    }
    pthread_mutex_unlock(&out_streams_mutex);

    return ret;
}

/* Generic HAL */

WRAP_HAL_LOCKED(set_master_volume, (unused_audio_hw_device *dev, float volume),
                (lp_hw_dev, volume), ("set_master_volume: %f", volume))

WRAP_HAL_LOCKED(set_voice_volume, (unused_audio_hw_device *dev, float volume),
                (lp_hw_dev, volume), ("set_voice_volume: %f", volume))

WRAP_HAL_LOCKED(set_mic_mute, (unused_audio_hw_device *dev, bool state),
                (lp_hw_dev, state), ("set_mic_mute: %d", state))

WRAP_HAL_LOCKED(set_mode, (unused_audio_hw_device *dev, audio_mode_t mode),
                (lp_hw_dev, mode), ("set_mode: %d", mode))

WRAP_HAL_LOCKED(set_parameters, (unused_audio_hw_device *dev, const char *kv_pairs),
                (lp_hw_dev, kv_pairs), ("set_parameters: %s", kv_pairs))

WRAP_HAL(get_supported_devices, uint32_t, (const unused_audio_hw_device *dev),
         (lp_hw_dev), ("get_supported_devices"))

WRAP_HAL(init_check, int, (const unused_audio_hw_device *dev),
         (lp_hw_dev), ("init_check"))

WRAP_HAL(get_master_volume, int, (unused_audio_hw_device *dev, float *volume),
         (lp_hw_dev, volume), ("get_master_volume"))

WRAP_HAL(get_mic_mute, int, (const unused_audio_hw_device *dev, bool *state),
         (lp_hw_dev, state), ("get_mic_mute"))

WRAP_HAL(get_parameters, char*, (const unused_audio_hw_device *dev, const char *keys),
         (lp_hw_dev, keys), ("get_parameters: %s", keys))

WRAP_HAL(get_input_buffer_size, size_t, (const unused_audio_hw_device *dev, const struct audio_config *config),
         (lp_hw_dev, config), ("get_input_buffer_size"))

WRAP_HAL(dump, int, (const unused_audio_hw_device *dev, int fd),
         (lp_hw_dev, fd), ("dump"))

static int wrapper_close(hw_device_t *device)
{
    int ret;

    pthread_mutex_lock(&out_streams_mutex);
    pthread_mutex_lock(&in_streams_mutex);

    WAIT_FOR_FREE(in_use);

    ret = lp_hw_dev->common.close(device);

    dlclose(dso_handle);
    dso_handle = NULL;
    free(lp_hw_dev);
    lp_hw_dev = NULL;

    if (out_streams) {
        free(out_streams);
        out_streams = NULL;
        n_out_streams = 0;
    }

    if (in_streams) {
        free(in_streams);
        in_streams = NULL;
        n_in_streams = 0;
    }

    UNLOCK_FREE(in_use);
    pthread_mutex_unlock(&in_streams_mutex);
    pthread_mutex_unlock(&out_streams_mutex);

    return ret;
}

static int wrapper_open(__attribute__((unused)) const hw_module_t* module,
                             __attribute__((unused)) const char* name,
                             hw_device_t** device)
{
    struct hw_module_t *hmi;
    struct audio_hw_device *adev;
    int ret;

    ALOGI("Initializing wrapper for Yamahas's YMU831 audio-HAL");
    if (lp_hw_dev) {
        ALOGE("Audio HAL already opened!");
        return -ENODEV;
    }

    dso_handle = dlopen("/system/lib/hw/audio.primary.vendor.smi.so", RTLD_NOW);
    if (dso_handle == NULL) {
        char const *err_str = dlerror();
        ALOGE("wrapper_open: %s", err_str ? err_str : "unknown");
        return -EINVAL;
    }

    const char *sym = HAL_MODULE_INFO_SYM_AS_STR;
    hmi = (struct hw_module_t *)dlsym(dso_handle, sym);
    if (hmi == NULL) {
        ALOGE("wrapper_open: couldn't find symbol %s", sym);
        dlclose(dso_handle);
        dso_handle = NULL;
        return -EINVAL;
    }

    hmi->dso = dso_handle;

    ret = audio_hw_device_open(hmi, (struct audio_hw_device**)&lp_hw_dev);
    ALOGE_IF(ret, "%s couldn't open audio module in %s. (%s)", __func__,
                 AUDIO_HARDWARE_MODULE_ID, strerror(-ret));
    if (ret) {
        dlclose(dso_handle);
        dso_handle = NULL;
        return ret;
    }

    *device = malloc(sizeof(struct audio_hw_device));
    if (!*device) {
        ALOGE("Can't allocate memory for device, aborting...");
        dlclose(dso_handle);
        dso_handle = NULL;

		return -ENOMEM;
    }

    memset(*device, 0, sizeof(struct audio_hw_device));

    adev = (struct audio_hw_device*)*device;

    /* HAL */
    adev->common.tag = HARDWARE_DEVICE_TAG;
    adev->common.version = AUDIO_DEVICE_API_VERSION_MIN;
    adev->common.module = (struct hw_module_t *) module;
    adev->common.close = wrapper_close;

    adev->get_supported_devices = wrapper_get_supported_devices;
    adev->init_check = wrapper_init_check;
    adev->set_voice_volume = wrapper_set_voice_volume;
    adev->set_master_volume = wrapper_set_master_volume;
    adev->get_master_volume = wrapper_get_master_volume;
    adev->set_mic_mute = wrapper_set_mic_mute;
    adev->get_mic_mute = wrapper_get_mic_mute;
    adev->get_parameters = wrapper_get_parameters;
    adev->get_input_buffer_size = wrapper_get_input_buffer_size;
    adev->set_mode = wrapper_set_mode;
    adev->set_parameters = wrapper_set_parameters;

    /* Output */
    adev->open_output_stream = wrapper_open_output_stream;
    adev->close_output_stream = wrapper_close_output_stream;

    /* Input */
    adev->open_input_stream = wrapper_open_input_stream;
    adev->close_input_stream = wrapper_close_input_stream;

    adev->dump = wrapper_dump;
    adev->set_master_mute = NULL;
    adev->get_master_mute = NULL;
    adev->create_audio_patch = NULL;
    adev->release_audio_patch = NULL;
    adev->get_audio_port = NULL;
    adev->set_audio_port_config = NULL;

    return 0;
}

static struct hw_module_methods_t wrapper_module_methods = {
    .open = wrapper_open,
};

struct audio_module HAL_MODULE_INFO_SYM = {
    .common = {
        .tag = HARDWARE_MODULE_TAG,
        .version_major = 1,
        .version_minor = 0,
        .id = AUDIO_HARDWARE_MODULE_ID,
        .name = "Yamaha YMU831 AUDIO-HAL wrapper",
        .author = "The LineageOS Project (Martin Bouchet)",
        .methods = &wrapper_module_methods,
    },
};
