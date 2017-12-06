/*
 * Copyright (C) 2011 The Android Open Source Project
 * Copyright (c) 2011-2014, The Linux Foundation. All rights reserved.
 * Not a Contribution.
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


/* common audio stream parameters and operations */
struct lp_audio_stream {

    /**
     * Return the sampling rate in Hz - eg. 44100.
     */
    uint32_t (*get_sample_rate)(const struct lp_audio_stream *stream);

    /* currently unused - use set_parameters with key
     *    AUDIO_PARAMETER_STREAM_SAMPLING_RATE
     */
    int (*set_sample_rate)(struct lp_audio_stream *stream, uint32_t rate);

    /**
     * Return size of input/output buffer in bytes for this stream - eg. 4800.
     * It should be a multiple of the frame size.  See also get_input_buffer_size.
     */
    size_t (*get_buffer_size)(const struct lp_audio_stream *stream);

    /**
     * Return the channel mask -
     *  e.g. AUDIO_CHANNEL_OUT_STEREO or AUDIO_CHANNEL_IN_STEREO
     */
    audio_channel_mask_t (*get_channels)(const struct lp_audio_stream *stream);

    /**
     * Return the audio format - e.g. AUDIO_FORMAT_PCM_16_BIT
     */
    audio_format_t (*get_format)(const struct lp_audio_stream *stream);

    /* currently unused - use set_parameters with key
     *     AUDIO_PARAMETER_STREAM_FORMAT
     */
    int (*set_format)(struct lp_audio_stream *stream, audio_format_t format);

    /**
     * Put the audio hardware input/output into standby mode.
     * Driver should exit from standby mode at the next I/O operation.
     * Returns 0 on success and <0 on failure.
     */
    int (*standby)(struct lp_audio_stream *stream);

    /** dump the state of the audio input/output device */
    int (*dump)(const struct lp_audio_stream *stream, int fd);

    /** Return the set of device(s) which this stream is connected to */
    audio_devices_t (*get_device)(const struct lp_audio_stream *stream);

    /**
     * Currently unused - set_device() corresponds to set_parameters() with key
     * AUDIO_PARAMETER_STREAM_ROUTING for both input and output.
     * AUDIO_PARAMETER_STREAM_INPUT_SOURCE is an additional information used by
     * input streams only.
     */
    int (*set_device)(struct lp_audio_stream *stream, audio_devices_t device);

    /**
     * set/get audio stream parameters. The function accepts a list of
     * parameter key value pairs in the form: key1=value1;key2=value2;...
     *
     * Some keys are reserved for standard parameters (See AudioParameter class)
     *
     * If the implementation does not accept a parameter change while
     * the output is active but the parameter is acceptable otherwise, it must
     * return -ENOSYS.
     *
     * The audio flinger will put the stream in standby and then change the
     * parameter value.
     */
    int (*set_parameters)(struct lp_audio_stream *stream, const char *kv_pairs);

    /*
     * Returns a pointer to a heap allocated string. The caller is responsible
     * for freeing the memory for it using free().
     */
    char * (*get_parameters)(const struct lp_audio_stream *stream,
                             const char *keys);
    int (*add_audio_effect)(const struct lp_audio_stream *stream,
                             effect_handle_t effect);
    int (*remove_audio_effect)(const struct lp_audio_stream *stream,
                             effect_handle_t effect);
};
typedef struct lp_audio_stream lp_audio_stream_t;

struct lp_audio_stream_out {
    struct lp_audio_stream common;

    /**
     * Return the audio hardware driver estimated latency in milliseconds.
     */
    uint32_t (*get_latency)(const struct lp_audio_stream_out *stream);

    /**
     * Use this method in situations where audio mixing is done in the
     * hardware. This method serves as a direct interface with hardware,
     * allowing you to directly set the volume as apposed to via the framework.
     * This method might produce multiple PCM outputs or hardware accelerated
     * codecs, such as MP3 or AAC.
     */
    int (*set_volume)(struct lp_audio_stream_out *stream, float left, float right);

    /**
     * Write audio buffer to driver. Returns number of bytes written, or a
     * negative status_t. If at least one frame was written successfully prior to the error,
     * it is suggested that the driver return that successful (short) byte count
     * and then return an error in the subsequent call.
     *
     * If set_callback() has previously been called to enable non-blocking mode
     * the write() is not allowed to block. It must write only the number of
     * bytes that currently fit in the driver/hardware buffer and then return
     * this byte count. If this is less than the requested write size the
     * callback function must be called when more space is available in the
     * driver/hardware buffer.
     */
    ssize_t (*write)(struct lp_audio_stream_out *stream, const void* buffer,
                     size_t bytes);

    /* return the number of audio frames written by the audio dsp to DAC since
     * the output has exited standby
     */
    int (*get_render_position)(const struct lp_audio_stream_out *stream,
                               uint32_t *dsp_frames);

    /**
     * get the local time at which the next write to the audio driver will be presented.
     * The units are microseconds, where the epoch is decided by the local audio HAL.
     */
    int (*get_next_write_timestamp)(const struct lp_audio_stream_out *stream,
                                    int64_t *timestamp);
};
typedef struct lp_audio_stream_out lp_audio_stream_out_t;

struct lp_audio_stream_in {
    struct lp_audio_stream common;

    /** set the input gain for the audio driver. This method is for
     *  for future use */
    int (*set_gain)(struct lp_audio_stream_in *stream, float gain);

    /** Read audio buffer in from audio driver. Returns number of bytes read, or a
     *  negative status_t. If at least one frame was read prior to the error,
     *  read should return that byte count and then return an error in the subsequent call.
     */
    ssize_t (*read)(struct lp_audio_stream_in *stream, void* buffer,
                    size_t bytes);

    /**
     * Return the amount of input frames lost in the audio driver since the
     * last call of this function.
     * Audio driver is expected to reset the value to 0 and restart counting
     * upon returning the current value by this function call.
     * Such loss typically occurs when the user space process is blocked
     * longer than the capacity of audio driver buffers.
     *
     * Unit: the number of input audio frames
     */
    uint32_t (*get_input_frames_lost)(struct lp_audio_stream_in *stream);

    /**
     * Return a recent count of the number of audio frames received and
     * the clock time associated with that frame count.
     *
     * frames is the total frame count received. This should be as early in
     *     the capture pipeline as possible. In general,
     *     frames should be non-negative and should not go "backwards".
     *
     * time is the clock MONOTONIC time when frames was measured. In general,
     *     time should be a positive quantity and should not go "backwards".
     *
     * The status returned is 0 on success, -ENOSYS if the device is not
     * ready/available, or -EINVAL if the arguments are null or otherwise invalid.
     */
    int (*get_capture_position)(const struct audio_stream_in *stream,
                                int64_t *frames, int64_t *time);
};
typedef struct lp_audio_stream_in lp_audio_stream_in_t;

struct lp_audio_hw_device {
    struct hw_device_t common;

    /**
     * used by audio flinger to enumerate what devices are supported by
     * each lp_audio_hw_device implementation.
     *
     * Return value is a bitmask of 1 or more values of audio_devices_t
     *
     * NOTE: audio HAL implementations starting with
     * AUDIO_DEVICE_API_VERSION_2_0 do not implement this function.
     * All supported devices should be listed in audio_policy.conf
     * file and the audio policy manager must choose the appropriate
     * audio module based on information in this file.
     */
    uint32_t (*get_supported_devices)(const struct lp_audio_hw_device *dev);

    /**
     * check to see if the audio hardware interface has been initialized.
     * returns 0 on success, -ENODEV on failure.
     */
    int (*init_check)(const struct lp_audio_hw_device *dev);

    /** set the audio volume of a voice call. Range is between 0.0 and 1.0 */
    int (*set_voice_volume)(struct lp_audio_hw_device *dev, float volume);

    /**
     * set the audio volume for all audio activities other than voice call.
     * Range between 0.0 and 1.0. If any value other than 0 is returned,
     * the software mixer will emulate this capability.
     */
    int (*set_master_volume)(struct lp_audio_hw_device *dev, float volume);

    /**
     * Get the current master volume value for the HAL, if the HAL supports
     * master volume control.  AudioFlinger will query this value from the
     * primary audio HAL when the service starts and use the value for setting
     * the initial master volume across all HALs.  HALs which do not support
     * this method may leave it set to NULL.
     */
    int (*get_master_volume)(struct lp_audio_hw_device *dev, float *volume);

    /**
     * set_mode is called when the audio mode changes. AUDIO_MODE_NORMAL mode
     * is for standard audio playback, AUDIO_MODE_RINGTONE when a ringtone is
     * playing, and AUDIO_MODE_IN_CALL when a call is in progress.
     */
    int (*set_mode)(struct lp_audio_hw_device *dev, audio_mode_t mode);

    /* mic mute */
    int (*set_mic_mute)(struct lp_audio_hw_device *dev, bool state);
    int (*get_mic_mute)(const struct lp_audio_hw_device *dev, bool *state);

    /* set/get global audio parameters */
    int (*set_parameters)(struct lp_audio_hw_device *dev, const char *kv_pairs);

    /*
     * Returns a pointer to a heap allocated string. The caller is responsible
     * for freeing the memory for it using free().
     */
    char * (*get_parameters)(const struct lp_audio_hw_device *dev,
                             const char *keys);

    /* Returns audio input buffer size according to parameters passed or
     * 0 if one of the parameters is not supported.
     * See also get_buffer_size which is for a particular stream.
     */
    size_t (*get_input_buffer_size)(const struct lp_audio_hw_device *dev,
                                    const struct audio_config *config);

    /** This method creates and opens the audio hardware output stream */
    int (*open_output_stream)(struct lp_audio_hw_device *dev,
                              audio_io_handle_t handle,
                              audio_devices_t devices,
                              audio_output_flags_t flags,
                              struct audio_config *config,
                              struct lp_audio_stream_out **stream_out);

    void (*close_output_stream)(struct lp_audio_hw_device *dev,
                                struct lp_audio_stream_out* stream_out);

    /** This method creates and opens the audio hardware input stream */
    int (*open_input_stream)(struct lp_audio_hw_device *dev,
                             audio_io_handle_t handle,
                             audio_devices_t devices,
                             struct audio_config *config,
                             struct lp_audio_stream_in **stream_in);

    void (*close_input_stream)(struct lp_audio_hw_device *dev,
                               struct lp_audio_stream_in *stream_in);

    /** This method dumps the state of the audio hardware */
    int (*dump)(const struct lp_audio_hw_device *dev, int fd);

    /**
     * set the audio mute status for all audio activities.  If any value other
     * than 0 is returned, the software mixer will emulate this capability.
     */
    int (*set_master_mute)(struct lp_audio_hw_device *dev, bool mute);

    /**
     * Get the current master mute status for the HAL, if the HAL supports
     * master mute control.  AudioFlinger will query this value from the primary
     * audio HAL when the service starts and use the value for setting the
     * initial master mute across all HALs.  HALs which do not support this
     * method may leave it set to NULL.
     */
    int (*get_master_mute)(struct lp_audio_hw_device *dev, bool *mute);
};
typedef struct lp_audio_hw_device lp_audio_hw_device_t;
