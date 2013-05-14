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

#include <stdio.h>
#include <sys/poll.h>
#define _POSIX_C_SOURCE

#include <alsa/asoundlib.h>

static int snd_pcm_hook_ctl_voice_hw_params(snd_pcm_hook_t *hook)
{
    snd_sctl_t *h = snd_pcm_hook_get_private(hook);
    return snd_sctl_install(h);
}

static int snd_pcm_hook_ctl_voice_hw_free(snd_pcm_hook_t *hook)
{
    snd_sctl_t *h = snd_pcm_hook_get_private(hook);
    return snd_sctl_remove(h);
}

static int snd_pcm_hook_ctl_voice_close(snd_pcm_hook_t *hook)
{
    snd_sctl_t *h = snd_pcm_hook_get_private(hook);
    int err = snd_sctl_free(h);
    snd_pcm_hook_set_private(hook, NULL);
    return err;
}

/**
 * \brief Install CTL settings using hardware associated with PCM handle
 * \param pcm PCM handle
 * \param conf Configuration node with CTL settings
 * \return zero on success otherwise a negative error code
 */
int _snd_pcm_hook_ctl_voice_install(snd_pcm_t *pcm, snd_config_t *conf)
{
    int err;
    int card;
    snd_pcm_info_t *info;
    char ctl_name[16];
    snd_ctl_t *ctl;
    snd_sctl_t *sctl = NULL;
    snd_config_t *pcm_conf = NULL;
    snd_pcm_hook_t *h_hw_params = NULL, *h_hw_free = NULL, *h_close = NULL;
    assert(conf);
    assert(snd_config_get_type(conf) == SND_CONFIG_TYPE_COMPOUND);

    sprintf(ctl_name, "voice");
    err = snd_ctl_open(&ctl, ctl_name, 0);
    if (err < 0) {
        SNDERR("Cannot open CTL %s", ctl_name);
        return err;
    }
    err = snd_config_imake_pointer(&pcm_conf, "pcm_handle", pcm);
    if (err < 0)
        goto _err;
    err = snd_sctl_build(&sctl, ctl, conf, pcm_conf, 0);
    if (err < 0)
        goto _err;
    err = snd_pcm_hook_add(&h_hw_params, pcm, SND_PCM_HOOK_TYPE_HW_PARAMS,
                           snd_pcm_hook_ctl_voice_hw_params, sctl);
    if (err < 0)
        goto _err;
    err = snd_pcm_hook_add(&h_hw_free, pcm, SND_PCM_HOOK_TYPE_HW_FREE,
                           snd_pcm_hook_ctl_voice_hw_free, sctl);
    if (err < 0)
        goto _err;
    err = snd_pcm_hook_add(&h_close, pcm, SND_PCM_HOOK_TYPE_CLOSE,
                           snd_pcm_hook_ctl_voice_close, sctl);
    if (err < 0)
        goto _err;
    snd_config_delete(pcm_conf);
    return 0;
_err:
    if (h_hw_params)
        snd_pcm_hook_remove(h_hw_params);
    if (h_hw_free)
        snd_pcm_hook_remove(h_hw_free);
    if (h_close)
        snd_pcm_hook_remove(h_close);
    if (sctl)
        snd_sctl_free(sctl);
    if (pcm_conf)
        snd_config_delete(pcm_conf);
    return err;
}
#ifndef DOC_HIDDEN
SND_DLSYM_BUILD_VERSION(_snd_pcm_hook_ctl_voice_install, SND_PCM_DLSYM_VERSION);
#endif

