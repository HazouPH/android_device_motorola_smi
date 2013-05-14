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

#define LOG_TAG "hdmi_audio_hw"
//#define LOG_NDEBUG 0

#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <sys/time.h>
#include <stdlib.h>

#include <cutils/log.h>
#include <cutils/str_parms.h>
#include <cutils/properties.h>

#include <hardware/hardware.h>
#include <system/audio.h>
#include <hardware/audio.h>

#include <alsa/asoundlib.h>

/*Silence bytes are written to the hardware to keep the HDMI audio
sink active until valid data is obtained for MFLD/CTP */
#define WRITE_SILENCE

#define DEFAULT_PERIOD_TIME      23220 //in us
#define DEFAULT_PERIOD_SIZE      1024
#define DEFAULT_PERIOD_COUNT     4
#define DEFAULT_NUM_CHANNEL      2
#define DEFAULT_SAMPLING_RATE    44100
#define DEFAULT_PCM_FORMAT       SND_PCM_FORMAT_S16_LE

#define MAX_AGAIN_RETRY          2
#define WAIT_TIME_MS             20
#define WAIT_BEFORE_RETRY        10000 //10ms

/* Configuration for a stream */
struct pcm_config {
    unsigned int channels;
    unsigned int rate;
    unsigned int period_size;
    unsigned int period_count;
    unsigned int format;
    unsigned int buffer_size;
    /* Values to use for the ALSA start, stop and silence thresholds.  Setting
     * any one of these values to 0 will cause the default tinyalsa values to be
     * used instead.  Tinyalsa defaults are as follows.
     *
     * start_threshold   : period_count * period_size
     * stop_threshold    : period_count * period_size
     * silence_threshold : 0
     */
    unsigned int start_threshold;
    unsigned int stop_threshold;
    unsigned int silence_threshold;

    /* Minimum number of frames available before pcm_mmap_write() will actually
     * write into the kernel buffer. Only used if the stream is opened in mmap mode
     * (pcm_open() called with PCM_MMAP flag set).   Use 0 for default.
     */
    int avail_min;
};

struct pcm_config pcm_config_default = {
    .channels      = DEFAULT_NUM_CHANNEL,
    .rate          = DEFAULT_SAMPLING_RATE,
    .period_size   = DEFAULT_PERIOD_SIZE,
    .period_count  = DEFAULT_PERIOD_COUNT,
    .format        = DEFAULT_PCM_FORMAT,
};

struct audio_device {
    struct audio_hw_device hw_device;

    pthread_mutex_t lock;
};

struct hdmi_stream_out {
    struct audio_stream_out stream;

    pthread_mutex_t lock;
    bool standby;

/* ALSA Device Handle */
    snd_pcm_t  *handle;
/* PCM Stream Configurations */
    struct pcm_config pcm_config;
    uint32_t   channel_mask;

 /* ALSA PCM Configurations */
    uint32_t   sample_rate;
    uint32_t   buffer_size;
    uint32_t   channels;
    uint32_t   latency;
    bool       display_connected;

    struct audio_device *dev;
};

/*These global variables are used to keep track of the
opened pcm device.*/
/*Fix me: move the global variables to any structure*/
static int hdmi_device_opened = 0;

struct hdmi_stream_out *active_stream_out = NULL;

static int close_device(struct hdmi_stream_out *stream)
{
    int err = 0;
    struct hdmi_stream_out *out = (struct hdmi_stream_out *)stream;
    ALOGD("%s: Entered", __func__);

    if (out->handle) {
       err = snd_pcm_drain(out->handle);
       ALOGV("%s: Drain the samples and stop the stream %s",__func__,snd_strerror(err));
       err = snd_pcm_close(out->handle);
       ALOGD("%s: PCM output device closed", __func__);
       out->handle = NULL;
       active_stream_out = NULL;
    }

    return err;
}

static int set_hardware_params(struct hdmi_stream_out *out)
{
    snd_pcm_hw_params_t *hardware_params;

    int err                       = 0;
    snd_pcm_uframes_t buffer_size = out->buffer_size;
    unsigned int requested_rate   = out->pcm_config.rate;
    unsigned int channels         = out->pcm_config.channels;

    ALOGV("%s: Entered", __func__);

    if (snd_pcm_hw_params_malloc(&hardware_params) < 0) {
        ALOGE("%s: Failed to allocate ALSA hardware parameters!", __func__);
        return -ENODEV;
    }

    err = snd_pcm_hw_params_any(out->handle, hardware_params);
    if (err < 0) {
        ALOGE("Unable to configure hardware: %s", snd_strerror(err));
        goto error_exit;
    }

    // Set the interleaved read and write format.
    err = snd_pcm_hw_params_set_access(out->handle, hardware_params,
                                       SND_PCM_ACCESS_RW_INTERLEAVED);
    if (err < 0) {
        ALOGE("Unable to configure PCM read/write format: %s", snd_strerror(err));
        goto error_exit;
     }

    err = snd_pcm_hw_params_set_format(out->handle, hardware_params, out->pcm_config.format);
    if (err < 0) {
        goto error_exit;
    }

    err = snd_pcm_hw_params_set_channels(out->handle, hardware_params, channels);
    if (err < 0) {
        ALOGE("Unable to set channel count to %i: %s", channels, snd_strerror(err));
        goto error_exit;
    }
    ALOGV("Channels : %d\n", channels);

    err = snd_pcm_hw_params_set_rate_near(out->handle, hardware_params,
                                          &requested_rate, 0);
    if (err < 0) {
        ALOGE("Unable to set sample rate to %u: %s", out->pcm_config.rate,
                        snd_strerror(err));
        out->sample_rate = requested_rate;
    }
    // Setup buffers for latency

    buffer_size = out->pcm_config.period_size * out->pcm_config.period_count;

    //Ensure that the driver and ALSA lib allocates the requested buffer and period size.
    ALOGV("Requested buffer size: %d", (int)buffer_size);

    err = snd_pcm_hw_params_set_buffer_size_near(out->handle,
          hardware_params, &buffer_size);
    if (err < 0) {
        ALOGE("Unable to set the buffer size : %s", snd_strerror(err));
        goto error_exit;
    }

    ALOGV("Obtained buffer size: %d", (int)buffer_size);

    snd_pcm_uframes_t period_size = buffer_size / out->pcm_config.period_count;

    ALOGV("Requested period size: %d", (int)period_size);

    err = snd_pcm_hw_params_set_period_size_near(out->handle,
          hardware_params,
          &period_size,
          0);
    if (err < 0) {
        ALOGE("Unable to set the period size: %s", snd_strerror(err));
        goto error_exit;
    }

    ALOGV("Obtained period size: %d", (int)period_size);

    // Commit the hardware parameters back to the device.
    err = snd_pcm_hw_params(out->handle, hardware_params);
    if (err < 0)
        ALOGE("Unable to set hardware parameters: %s", snd_strerror(err));

    out->pcm_config.period_size = period_size;
    out->pcm_config.buffer_size = buffer_size;

error_exit:
    if(hardware_params)
       snd_pcm_hw_params_free(hardware_params);

    return err;
}

static int set_software_params(struct hdmi_stream_out *out)
{
    snd_pcm_sw_params_t * software_params;
    int err = 0;

    ALOGD("%s: Entered", __func__);

    snd_pcm_uframes_t buffer_size       = 0;
    snd_pcm_uframes_t period_size       = 0;
    snd_pcm_uframes_t start_threshold   = 0,
                      stop_threshold    = 0;
#ifdef WRITE_SILENCE
    snd_pcm_uframes_t silence_threshold = 0;
#endif //WRITE_SILENCE

    if (snd_pcm_sw_params_malloc(&software_params) < 0) {
        ALOGE("Failed to allocate ALSA software parameters!");
        return -ENODEV;
    }

    // Get the current software parameters
    err = snd_pcm_sw_params_current(out->handle, software_params);
    if (err < 0) {
        ALOGE("Unable to get software parameters: %s", snd_strerror(err));
        goto error_exit;
    }

    // Configure ALSA to start the transfer when the buffer is almost full.
    snd_pcm_get_params(out->handle, &buffer_size, &period_size);

    start_threshold   = period_size - 1;
    stop_threshold    = buffer_size;
    silence_threshold = period_size;

    err = snd_pcm_sw_params_set_start_threshold(out->handle, software_params,
            start_threshold);
    if (err < 0) {
        ALOGE("Unable to set start threshold to %lu frames: %s",
             start_threshold, snd_strerror(err));
        goto error_exit;
    }

    err = snd_pcm_sw_params_set_stop_threshold(out->handle, software_params,
            stop_threshold);
    if (err < 0) {
        ALOGE("Unable to set stop threshold to %lu frames: %s",
             stop_threshold, snd_strerror(err));
        goto error_exit;
    }

    err = snd_pcm_sw_params_set_silence_threshold(out->handle, software_params,
            silence_threshold);
    if (err < 0) {
        ALOGE("Unable to set silence threshold to %lu frames: %s",
             silence_threshold, snd_strerror(err));
        goto error_exit;
    }
    ALOGV("Set silence threshold = %d",(int)silence_threshold);

    //Allow the transfer to start when at least period_size samples can be
    //processed.
    err = snd_pcm_sw_params_set_avail_min(out->handle, software_params,
                                          period_size);
    if (err < 0) {
        ALOGE("Unable to configure available minimum to %lu: %s",
             period_size, snd_strerror(err));
        goto error_exit;
    }

    // Commit the software parameters back to the device.
    err = snd_pcm_sw_params(out->handle, software_params);
    if (err < 0)
       ALOGE("Unable to configure software parameters: %s", snd_strerror(err));

    out->pcm_config.start_threshold = start_threshold;
    out->pcm_config.stop_threshold = stop_threshold;
    out->pcm_config.avail_min = period_size;

error_exit:
    if(software_params)
       snd_pcm_sw_params_free(software_params);

    return err;
}

static int open_device(struct hdmi_stream_out *out)
{
    int err = 0;

    ALOGD("%s: opening AndroidPlayback_HDMI for %d channels", __func__, out->pcm_config.channels);


    // The PCM stream is opened in blocking mode, per ALSA defaults.  The
    // AudioFlinger seems to assume blocking mode too, so asynchronous mode
    // should not be used.
    if (out->handle != NULL) {
        ALOGE("%s: Closing ALSA device:", __func__);
        snd_pcm_close(out->handle);
        out->handle = NULL;
    }

    err = snd_pcm_open(&out->handle, "AndroidPlayback_HDMI", SND_PCM_STREAM_PLAYBACK,
                           SND_PCM_ASYNC);
    if (err < 0) {
        ALOGE("%s: Failed to open any ALSA device: %s", __func__, (char*)strerror(err));
        out->handle = NULL;
        goto err_open_device;
    }

    err = set_hardware_params(out);

    if (err == 0)
       err = set_software_params(out);

    if (err == 0) {
       ALOGI("%s: Initialized HDMI Audio device", __func__);
       return 0;
    }

    if (out->handle) {
        snd_pcm_close(out->handle);
        ALOGV("%s: PCM open device failed", __func__);
        out->handle = NULL;
    }

err_open_device:
    return -ENODEV;
}

/**
 * NOTE: when multiple mutexes have to be acquired, always respect the
 * following order: hw device > out stream
 */

/* Helper functions */

/* must be called with hw device and output stream mutexes locked */
static int start_output_stream(struct hdmi_stream_out *out)
{
    struct audio_device *hdmi_dev = out->dev;
    int i;

    ALOGV("%s: channels     :%d", __func__, out->pcm_config.channels);
    ALOGV("%s: rate         :%d", __func__, out->pcm_config.rate);
    ALOGV("%s: period_size  :%d", __func__, out->pcm_config.period_size);
    ALOGV("%s: period_count :%d", __func__, out->pcm_config.period_count);
    ALOGV("%s: format       :%d", __func__, out->pcm_config.format);

    if (open_device(out))  {
        ALOGE("%s: pcm_open() failed", __func__);
        return -ENOMEM;
    }

    active_stream_out = out;

    return 0;
}

/* API functions */

static uint32_t out_get_sample_rate(const struct audio_stream *stream)
{
    struct hdmi_stream_out *out = (struct hdmi_stream_out *)stream;

    ALOGV("%s : smaple_rate: %d", __func__, out->pcm_config.rate);

    return out->pcm_config.rate;
}

static int out_set_sample_rate(struct audio_stream *stream, uint32_t rate)
{
    ALOGV("%s unsupported", __func__);

    return 0;
}

static size_t out_get_buffer_size(const struct audio_stream *stream)
{
    struct hdmi_stream_out *out = (struct hdmi_stream_out *)stream;
    size_t buf_size;

//    buf_size = out->pcm_config.period_size *
  //         audio_stream_frame_size((struct audio_stream *)stream);


    buf_size = out->pcm_config.period_size *
               out->pcm_config.period_count *
               audio_stream_frame_size((struct audio_stream *)stream);

    ALOGV("%s : %d, period_size : %d, frame_size : %d",
        __func__,
        buf_size,
        out->pcm_config.period_size,
        audio_stream_frame_size((struct audio_stream *)stream));

    return buf_size;
}

static uint32_t out_get_channels(const struct audio_stream *stream)
{
    struct hdmi_stream_out *out = (struct hdmi_stream_out *)stream;

    ALOGV("%s: channel mask : %x", __func__, out->channel_mask);

    return out->channel_mask;
}

static audio_format_t out_get_format(const struct audio_stream *stream)
{
    ALOGV("%s", __func__);

    return AUDIO_FORMAT_PCM_16_BIT;
}

static int out_set_format(struct audio_stream *stream, audio_format_t format)
{
    ALOGV("%s unsupported", __func__);
    return 0;
}

static int out_standby(struct audio_stream *stream)
{
    struct hdmi_stream_out *out = (struct hdmi_stream_out *)stream;

    ALOGV("%s Entered: channel_mask : %d", __func__, out->channel_mask);

    pthread_mutex_lock(&out->dev->lock);
    pthread_mutex_lock(&out->lock);

    if (!out->standby) {
        close_device(out);
        out->standby = true;
    }

    pthread_mutex_unlock(&out->lock);
    pthread_mutex_unlock(&out->dev->lock);

    ALOGV("%s Exit", __func__);

    return 0;
}

static int out_dump(const struct audio_stream *stream, int fd)
{
    ALOGV("%s unsupported", __func__);
    return 0;
}

static int out_set_parameters(struct audio_stream *stream, const char *kvpairs)
{
    struct hdmi_stream_out *out = (struct hdmi_stream_out *)stream;
    struct str_parms *parms;
    char value[32];
    int ret = 0,
        val = 0;

    ALOGV("%s Entered", __func__);

    parms = str_parms_create_str(kvpairs);

    pthread_mutex_lock(&out->dev->lock);

    ret = str_parms_get_str(parms, AUDIO_PARAMETER_STREAM_ROUTING, value, sizeof(value));
    if (ret >= 0) {
       val = atoi(value);
       if (val & AUDIO_DEVICE_OUT_AUX_DIGITAL){
          //pthread_mutex_unlock(&out->dev->lock);
          /*we don't do anything here, so no need of standby. It will be needed
          if we want to change the configuration parameters
          CHECK - for scenarios where parameter setting is needed*/
          //out_standby(stream);
          //pthread_mutex_lock(&out->dev->lock);
       }
    }

    pthread_mutex_unlock(&out->dev->lock);
    str_parms_destroy(parms);

    return 0;
}

static char * out_get_parameters(const struct audio_stream *stream, const char *keys)
{
    ALOGV("%s unsupported", __func__);
    return strdup("");
}

static uint32_t out_get_latency(const struct audio_stream_out *stream)
{
    struct hdmi_stream_out *out = (struct hdmi_stream_out *)stream;
    uint32_t latency;

    latency =  (out->pcm_config.period_size * out->pcm_config.period_count * 1000) /
            out_get_sample_rate(&stream->common);

    ALOGV("%s: latency : %d", __func__, latency);
    out->latency = latency;

    return latency;
}

static int out_set_volume(struct audio_stream_out *stream, float left,
                          float right)
{
    ALOGV("%s unsupported", __func__);

    return -ENOSYS;
}

static ssize_t out_write(struct audio_stream_out *stream, const void* buffer,
                         size_t ip_bytes)
{
    struct hdmi_stream_out *out = (struct hdmi_stream_out *)stream;
    ssize_t ret = 0;
    size_t sent_bytes = 0;
    snd_pcm_sframes_t frames = 0;
    int it = 0;
    unsigned int totalSleepTime;

    ALOGV("%s out->standy = %d", __func__,out->standby);

    pthread_mutex_lock(&out->dev->lock);
    pthread_mutex_lock(&out->lock);

    if (out->standby) {
        ret = start_output_stream(out);
        if (ret != 0) {
           //pthread_mutex_unlock(&out->lock);
           //pthread_mutex_unlock(&out->dev->lock);
            ALOGE("%s: stream start failed", __func__);
            goto err_write;
        }
        ALOGV("%s: standby is set to false", __func__);
        out->standby = false;
    }
//    pthread_mutex_unlock(&out->dev->lock);

//if the pcm device is closed before the write can complete
// return silence. Audio policy must be changed to stop the
// already running stream
    if (out->handle == NULL) {
        pthread_mutex_unlock(&out->lock);
        pthread_mutex_unlock(&out->dev->lock);
        goto silence_write;
    }

    ALOGV("write data : channels : %d", out->channel_mask);

    frames = 0;

    do {
        frames = snd_pcm_writei(out->handle,
                           (char *)buffer + sent_bytes,
                           snd_pcm_bytes_to_frames(out->handle, ip_bytes - sent_bytes));

        if ((frames == -EAGAIN) ||
           ((frames >= 0) &&
           ((snd_pcm_frames_to_bytes(out->handle, frames) + sent_bytes) < ip_bytes))) {
            it++;
            if (it > MAX_AGAIN_RETRY){
                ALOGE("write err: EAGAIN breaking...");
                ret = -EAGAIN;  //What happens to the other condition?
                goto err_write;
            }
            snd_pcm_wait(out->handle, WAIT_TIME_MS);
        }
        else if (frames == -EBADFD) {
            ALOGE("write err: %s, TRY REOPEN...", snd_strerror(frames));
            ret = open_device(out);
            if(ret != 0) {
               ALOGE("Open device error %d", (int)ret);
               goto err_write;
            }
        }
        else if (frames == -ENODEV) {
            ALOGE("write err: %s, bailing out", snd_strerror(frames));
            ret = -ENODEV;
            goto err_write;
        }
        else if (frames < 0) {
            ALOGE("write err: %s", snd_strerror(frames));
            for (totalSleepTime = 0; totalSleepTime < out->latency; totalSleepTime += WAIT_BEFORE_RETRY) {
                ret = snd_pcm_recover(out->handle, frames, 1);
                if ((ret == -EAGAIN)) {
                    usleep(WAIT_BEFORE_RETRY);
                }
                else
                    break;
            }
            if(ret != 0) {
                ALOGE("pcm write recover error: %s", snd_strerror(frames));
                goto err_write;
            }
        }

        if(frames > 0) {
            sent_bytes += snd_pcm_frames_to_bytes(out->handle, frames);
        }

    } while (sent_bytes < ip_bytes);

    ALOGV("%s: frames %ld to bytes %ld",
        __func__,
        frames,
        snd_pcm_frames_to_bytes(out->handle, frames));

    ALOGV("%s: write size: ip_bytes : %d, frames : %ld, sent_bytes : %d",
        __func__,
        ip_bytes,
        frames,
        sent_bytes);

err_write:
    pthread_mutex_unlock(&out->lock);
    pthread_mutex_unlock(&out->dev->lock);

    return ret == 0 ? (ssize_t) sent_bytes : ret;

silence_write:
   if(ret !=0){
    uint64_t duration_ms = ((ip_bytes * 1000)/
                            (audio_stream_frame_size(&stream->common)) /
                            (out_get_sample_rate(&stream->common)));
    ALOGV("%s : silence written", __func__);
    usleep(duration_ms * 1000);
   }
    return ip_bytes;
}

static int out_get_render_position(const struct audio_stream_out *stream,
                                   uint32_t *dsp_frames)
{
    ALOGV("%s unsupported", __func__);
    return -EINVAL;
}

static int out_add_audio_effect(const struct audio_stream *stream, effect_handle_t effect)
{
    ALOGV("%s unsupported", __func__);
    return 0;
}

static int out_remove_audio_effect(const struct audio_stream *stream, effect_handle_t effect)
{
    ALOGV("%s unsupported", __func__);
    return 0;
}

static int out_get_next_write_timestamp(const struct audio_stream_out *stream,
                                        int64_t *timestamp)
{
    ALOGV("%s unsupported", __func__);
    return -EINVAL;
}

static int hdmi_dev_open_output_stream(struct audio_hw_device *dev,
                                   audio_io_handle_t handle,
                                   audio_devices_t devices,
                                   audio_output_flags_t flags,
                                   struct audio_config *config,
                                   struct audio_stream_out **stream_out)
{
    struct audio_device *hdmi_dev = (struct audio_device *)dev;
    struct hdmi_stream_out *out;
    int ret;

    ALOGV("%s: entered", __func__);

    out = (struct hdmi_stream_out *)calloc(1, sizeof(struct hdmi_stream_out));
    if (!out)
        return -ENOMEM;

    out->channel_mask = AUDIO_CHANNEL_OUT_STEREO;

    if (flags & AUDIO_OUTPUT_FLAG_DIRECT) {
        ALOGV("%s: HDMI Multichannel",__func__);
        if (config->sample_rate == 0)
            config->sample_rate = pcm_config_default.rate;
        if (config->channel_mask == 0)
            config->channel_mask = AUDIO_CHANNEL_OUT_5POINT1;
    } else {
        ALOGV("%s: HDMI Stereo",__func__);
        if (config->sample_rate == 0)
            config->sample_rate = pcm_config_default.rate;
        if (config->channel_mask == 0)
            config->channel_mask = AUDIO_CHANNEL_OUT_STEREO;
    }

    out->channel_mask                      = config->channel_mask;

    out->pcm_config.channels               = popcount(config->channel_mask);
    out->pcm_config.rate                   = config->sample_rate;
    out->pcm_config.period_size            = pcm_config_default.period_size;
    out->pcm_config.period_count           = pcm_config_default.period_count;
    out->pcm_config.format                 = pcm_config_default.format;

    out->stream.common.get_sample_rate     = out_get_sample_rate;
    out->stream.common.set_sample_rate     = out_set_sample_rate;
    out->stream.common.get_buffer_size     = out_get_buffer_size;
    out->stream.common.get_channels        = out_get_channels;
    out->stream.common.get_format          = out_get_format;
    out->stream.common.set_format          = out_set_format;
    out->stream.common.standby             = out_standby;
    out->stream.common.dump                = out_dump;
    out->stream.common.set_parameters      = out_set_parameters;
    out->stream.common.get_parameters      = out_get_parameters;
    out->stream.common.add_audio_effect    = out_add_audio_effect;
    out->stream.common.remove_audio_effect = out_remove_audio_effect;

    out->stream.get_latency                = out_get_latency;
    out->stream.set_volume                 = out_set_volume;
    out->stream.write                      = out_write;
    out->stream.get_render_position        = out_get_render_position;
    out->stream.get_next_write_timestamp   = out_get_next_write_timestamp;

    out->dev        = hdmi_dev;
    out->latency    = DEFAULT_PERIOD_TIME * out->pcm_config.period_count;

    out->standby  = true;

    pthread_mutex_lock(&out->dev->lock);

//if one more request to open the pcm device comes, close
// the already opened device. Audio Policy update needed
// to support a single stream being fed to renderer
    if (active_stream_out != NULL && active_stream_out->handle != NULL) {
       ALOGV("%s: Closing already opened stream %x",__func__,active_stream_out->handle);
       snd_pcm_close(active_stream_out->handle);
       active_stream_out->handle = NULL;
    }

    ret = start_output_stream(out);
    if (ret != 0) {
            ALOGV("%s: stream start failed", __func__);
            goto err_open;
    }

    out->standby = false;

    *stream_out = &out->stream;

    pthread_mutex_unlock(&out->dev->lock);

    ALOGV("%s: Successful", __func__);
    return 0;

err_open:
    ALOGE("%s: Failed", __func__);
    pthread_mutex_unlock(&out->dev->lock);
    free(out);
    *stream_out = NULL;
    return ret;
}

static void hdmi_dev_close_output_stream(struct audio_hw_device *dev,
                                     struct audio_stream_out *stream)
{
    struct hdmi_stream_out *out = (struct hdmi_stream_out *)(&stream->common);
    ALOGV("%s Entered %0x",__func__,(unsigned)out);


    out->standby = false;
    out_standby(&stream->common);
    free(stream);
    ALOGV("%s Exit",__func__);
}

static int hdmi_dev_set_parameters(struct audio_hw_device *dev, const char *kvpairs)
{
    ALOGV("%s unsupported", __func__);
    return 0;
}

static char * hdmi_dev_get_parameters(const struct audio_hw_device *dev,
                                  const char *keys)
{
    ALOGV("%s unsupported", __func__);
    return strdup("");
}

static int hdmi_dev_init_check(const struct audio_hw_device *dev)
{
    ALOGV("%s unsupported", __func__);
    return 0;
}

static int hdmi_dev_set_voice_volume(struct audio_hw_device *dev, float volume)
{
    ALOGV("%s unsupported", __func__);
    return -ENOSYS;
}

static int hdmi_dev_set_master_volume(struct audio_hw_device *dev, float volume)
{
    ALOGV("%s unsupported", __func__);
    return -ENOSYS;
}

static int hdmi_dev_set_mode(struct audio_hw_device *dev, audio_mode_t mode)
{
    ALOGV("%s unsupported", __func__);
    return 0;
}

static int hdmi_dev_set_mic_mute(struct audio_hw_device *dev, bool state)
{
    ALOGV("%s unsupported", __func__);
    return -ENOSYS;
}

static int hdmi_dev_get_mic_mute(const struct audio_hw_device *dev, bool *state)
{
    ALOGV("%s unsupported", __func__);
    return -ENOSYS;
}

static size_t hdmi_dev_get_input_buffer_size(const struct audio_hw_device *dev,
                                         const struct audio_config *config)
{
    ALOGV("%s unsupported", __func__);
    return 0;
}

static int hdmi_dev_open_input_stream(struct audio_hw_device *dev,
                                  audio_io_handle_t handle,
                                  audio_devices_t devices,
                                  struct audio_config *config,
                                  struct audio_stream_in **stream_in)
{
    ALOGV("%s unsupported", __func__);
    return -ENOSYS;
}

static void hdmi_dev_close_input_stream(struct audio_hw_device *dev,
                                   struct audio_stream_in *stream)
{
    ALOGV("%s unsupported", __func__);
}

static int hdmi_dev_dump(const audio_hw_device_t *device, int fd)
{
    ALOGV("%s unsupported", __func__);
    return 0;
}

static int hdmi_dev_close(hw_device_t *device)
{
    struct audio_device *hdmi_dev = (struct audio_device *)device;
    ALOGV("%s ", __func__);
    free(hdmi_dev);
    return 0;
}

static uint32_t hdmi_dev_get_supported_devices(const struct audio_hw_device *dev)
{
    return AUDIO_DEVICE_OUT_AUX_DIGITAL;
}

static int hdmi_dev_open(const hw_module_t* module, const char* name,
                     hw_device_t** device)
{
    struct audio_device *hdmi_dev;
    int ret;

    ALOGV("%s Entered",__func__);

    if (strcmp(name, AUDIO_HARDWARE_INTERFACE) != 0)
        return -EINVAL;

    hdmi_dev = calloc(1, sizeof(struct audio_device));
    if (!hdmi_dev)
        return -ENOMEM;

    hdmi_dev->hw_device.common.tag            = HARDWARE_DEVICE_TAG;
    hdmi_dev->hw_device.common.version        = AUDIO_DEVICE_API_VERSION_1_0;
    hdmi_dev->hw_device.common.module         = (struct hw_module_t *) module;
    hdmi_dev->hw_device.common.close          = hdmi_dev_close;

    hdmi_dev->hw_device.get_supported_devices = hdmi_dev_get_supported_devices;
    hdmi_dev->hw_device.init_check            = hdmi_dev_init_check;
    hdmi_dev->hw_device.set_voice_volume      = hdmi_dev_set_voice_volume;
    hdmi_dev->hw_device.set_master_volume     = hdmi_dev_set_master_volume;
    hdmi_dev->hw_device.set_mode              = hdmi_dev_set_mode;
    hdmi_dev->hw_device.set_mic_mute          = hdmi_dev_set_mic_mute;
    hdmi_dev->hw_device.get_mic_mute          = hdmi_dev_get_mic_mute;
    hdmi_dev->hw_device.set_parameters        = hdmi_dev_set_parameters;
    hdmi_dev->hw_device.get_parameters        = hdmi_dev_get_parameters;
    hdmi_dev->hw_device.get_input_buffer_size = hdmi_dev_get_input_buffer_size;
    hdmi_dev->hw_device.open_output_stream    = hdmi_dev_open_output_stream;
    hdmi_dev->hw_device.close_output_stream   = hdmi_dev_close_output_stream;
    hdmi_dev->hw_device.open_input_stream     = hdmi_dev_open_input_stream;
    hdmi_dev->hw_device.close_input_stream    = hdmi_dev_close_input_stream;
    hdmi_dev->hw_device.dump                  = hdmi_dev_dump;

    *device = &hdmi_dev->hw_device.common;

    return 0;
}

static struct hw_module_methods_t hal_module_methods = {
    .open = hdmi_dev_open,
};

struct audio_module HAL_MODULE_INFO_SYM = {
    .common = {
        .tag = HARDWARE_MODULE_TAG,
        .module_api_version = AUDIO_MODULE_API_VERSION_0_1,
        .hal_api_version = HARDWARE_HAL_API_VERSION,
        .id = AUDIO_HARDWARE_MODULE_ID,
        .name = "HDMI audio HW HAL",
        .author = "Intel :The Android Open Source Project",
        .methods = &hal_module_methods,
    },
};
