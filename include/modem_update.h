/* Modem Manager (MMGR) - external modem update API include file
**
** Copyright (C) Intel 2012
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
**
*/

#ifndef __MMGR_MUP_HEADER__
#define __MMGR_MUP_HEADER__

#include <stddef.h>
#include <stdbool.h>
#include "mmgr.h"

#define MUP_STATE \
    X(SUCCEED), \
    X(BAD_PARAMETER), \
    X(FAILED), \
    X(FW_OUTDATED), \
    X(FW_CORRUPTED), \
    X(FW_RESTRICTED)

typedef enum e_mup_err {
#undef X
#define X(a) E_MUP_ ## a
    MUP_STATE
} e_mup_err_t;

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*mup_ap_log_callback_t) (const char *msg, ...);

typedef struct mup_interface {
    void *context;
    int last_error;
    mup_ap_log_callback_t ap_log_callback;
} mup_interface_t;

typedef struct mup_fw_update_params {
    mup_interface_t *handle;
    const char *mdm_com_port;
    bool channel_hw_sw;
    const char *fw_file_path;
    size_t fw_file_path_len;
    int erase_all;
} mup_fw_update_params_t;

typedef struct mup_rndcert_read_params {
    mup_interface_t *handle;
    char *mdm_com_port;
    char *dest_file_name;
    size_t dest_file_name_len;
} mup_rndcert_read_params_t;

typedef struct mup_rndcert_erase_params {
    mup_interface_t *handle;
    char *mdm_com_port;
} mup_rndcert_erase_params_t;

typedef struct mup_rndcert_update_params {
    mup_interface_t *handle;
    char *mdm_com_port;
    char *src_file_path;
    size_t src_file_path_len;
} mup_rndcert_update_params_t;

typedef struct mup_fuse_get_params {
    mup_interface_t *handle;
    char *mdm_com_port;
    char *dest_buffer;
    size_t dest_buffer_size;
} mup_fuse_get_params_t;

typedef struct mup_hw_id_get_params {
    mup_interface_t *handle;
    char *mdm_com_port;
    char *dest_buffer;
    size_t dest_buffer_size;
} mup_hw_id_get_params_t;

typedef struct mup_nvm_update_params {
    mup_interface_t *handle;
    char *mdm_com_port;
    char *nvm_file_path;
    size_t nvm_file_path_len;
} mup_nvm_update_params_t;

typedef struct mup_nvm_read_id_params {
    mup_interface_t *handle;
    char *mdm_com_port;
    char *dest_buffer;
    size_t dest_buffer_size;
} mup_nvm_read_id_params_t;

int mup_get_last_error(mup_interface_t *handle);

e_mup_err_t mup_initialize(mup_interface_t **handle,
                           mup_ap_log_callback_t ap_log_callback);

e_mup_err_t mup_check_fw_version(char *path, char *version);

e_mup_err_t mup_update_fw(mup_fw_update_params_t *params);

e_mup_err_t mup_update_rnd(mup_rndcert_update_params_t *params);

e_mup_err_t mup_read_rnd(mup_rndcert_read_params_t *params);

e_mup_err_t mup_erase_rnd(mup_rndcert_erase_params_t *params);

e_mup_err_t mup_get_fuse(mup_fuse_get_params_t *params);

e_mup_err_t mup_get_hw_id(mup_hw_id_get_params_t *params);

e_mup_err_t mup_update_nvm(mup_nvm_update_params_t *params);

e_mup_err_t mup_get_nvm_id(mup_nvm_read_id_params_t *params);

e_mup_err_t mup_dispose(mup_interface_t *handle);

e_mup_err_t mup_toggle_hsi_flashing_mode(bool flashing_mode);

e_mup_err_t mup_open_device(mup_fw_update_params_t *param);

e_mup_err_t mup_configure_secur_channel(mup_interface_t *handle, void *func,
                                        char *rnd_path, size_t len);
e_mup_err_t mup_gen_fls(const char *fls_in, const char *fls_out,
                        const char *nvm_path, const char *cert_path,
                        const char *sec_path);

#ifdef __cplusplus
}
#endif
#endif                          /* __MMGR_MUP_HEADER__ */
