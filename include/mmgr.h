/* Modem Manager (MMGR) - external include file
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

#ifndef __MMGR_EXTERNAL_HEADER_FILE__
#define __MMGR_EXTERNAL_HEADER_FILE__

#define MMGR_SOCKET_NAME "mmgr"
#define CLIENT_NAME_LEN 64
#define FUSE_LEN 9

/* Please read README file to have useful information about
   MMGR requests */

#define MMGR_REQUESTS \
    X(SET_NAME),\
    X(SET_EVENTS),\
    /* Resource allocation: Clients -> MMGR */ \
    X(RESOURCE_ACQUIRE),\
    X(RESOURCE_RELEASE),\
    /* Requests: Clients -> MMGR */ \
    X(REQUEST_MODEM_RECOVERY),\
    X(REQUEST_MODEM_RESTART),\
    X(REQUEST_FORCE_MODEM_SHUTDOWN),\
    /* ACK: Clients -> MMGR */ \
    X(ACK_MODEM_COLD_RESET),\
    X(ACK_MODEM_SHUTDOWN),\
    /* flashing request */ \
    X(REQUEST_MODEM_FUSE_INFO),\
    X(REQUEST_MODEM_GET_HW_ID),\
    X(REQUEST_MODEM_BACKUP_PRODUCTION),\
    /* fake requests */ \
    X(REQUEST_FAKE_DOWN), \
    X(REQUEST_FAKE_UP), \
    X(REQUEST_FAKE_AP_RESET), \
    X(REQUEST_FAKE_SELF_RESET), \
    X(REQUEST_FAKE_MODEM_SHUTDOWN), \
    X(REQUEST_FAKE_MODEM_OUT_OF_SERVICE), \
    X(REQUEST_FAKE_CORE_DUMP), \
    X(REQUEST_FAKE_CORE_DUMP_COMPLETE), \
    X(REQUEST_FAKE_PLATFORM_REBOOT), \
    X(REQUEST_FAKE_ERROR), \
    X(NUM_REQUESTS)

#define MMGR_EVENTS \
    /* Events notification: MMGR -> Clients */ \
    X(EVENT_MODEM_DOWN),\
    X(EVENT_MODEM_UP),\
    X(EVENT_MODEM_OUT_OF_SERVICE),\
    /* Notifications: MMGR -> Clients */ \
    X(NOTIFY_MODEM_WARM_RESET),\
    X(NOTIFY_MODEM_COLD_RESET),\
    X(NOTIFY_MODEM_SHUTDOWN),\
    X(NOTIFY_PLATFORM_REBOOT),\
    X(NOTIFY_CORE_DUMP),\
    /* ACK: MMGR -> Clients */ \
    X(ACK),\
    X(NACK),\
    /* Notifications for crashtool */\
    X(NOTIFY_CORE_DUMP_COMPLETE),\
    X(NOTIFY_AP_RESET),\
    X(NOTIFY_SELF_RESET),\
    X(NOTIFY_ERROR),\
    /* flashing notifications */ \
    X(RESPONSE_MODEM_HW_ID),\
    X(RESPONSE_MODEM_FW_RESULT),\
    X(RESPONSE_FUSE_INFO),\
    X(NUM_EVENTS)

#define CORE_DUMP_STATE \
    X(SUCCEED),\
    X(SUCCEED_WITHOUT_PANIC_ID),\
    X(FAILED),\
    X(FAILED_WITH_PANIC_ID)

typedef enum e_mmgr_requests {
#undef X
#define X(a) E_MMGR_##a
    MMGR_REQUESTS
} e_mmgr_requests_t;

typedef enum e_mmgr_events {
#undef X
#define X(a) E_MMGR_##a
    MMGR_EVENTS
} e_mmgr_events_t;

typedef enum e_core_dump_state {
#undef X
#define X(a) E_CD_##a
    CORE_DUMP_STATE
} e_core_dump_state_t;

typedef struct mmgr_cli_core_dump {
    e_core_dump_state_t state;
    int panic_id;
    size_t len;
    char *path;
} mmgr_cli_core_dump_t;

#define MMGR_CLI_MAX_RECOVERY_CAUSES 5
#define MMGR_CLI_MAX_RECOVERY_CAUSE_LEN 512

typedef struct mmgr_cli_recovery_cause {
    size_t len;
    char *cause;
} mmgr_cli_recovery_cause_t;

/* Note: 'recovery_causes' array (if present) is used to describe why
 *       client 'name' requested a modem recovery procedure.
 */
typedef struct mmgr_cli_ap_reset {
    size_t len;
    char *name;
    size_t num_causes;
    /* Size of 'recovery_causes' array is given in 'num_causes' */
    mmgr_cli_recovery_cause_t *recovery_causes;
} mmgr_cli_ap_reset_t;

typedef struct mmgr_cli_error {
    int id;
    size_t len;
    char *reason;
} mmgr_cli_error_t;

#ifdef MMGR_FW_OPERATIONS
#include <sys/types.h>

#define FW_ERROR \
    X(SUCCEED),\
    X(TOO_OLD),\
    X(READY_TIMEOUT),\
    X(SECURITY_CORRUPTED),\
    X(SW_CORRUPTED),\
    X(BAD_FAMILY),\
    X(ERROR_UNSPECIFIED),\
    X(NUM)

typedef enum e_modem_fw_error {
#undef X
#define X(a) E_MODEM_FW_##a
    FW_ERROR
} e_modem_fw_error_t;

typedef struct mmgr_cli_fw_update_result {
    e_modem_fw_error_t id;
} mmgr_cli_fw_update_result_t;

typedef struct mmgr_cli_fuse_info {
    char id[FUSE_LEN];
} mmgr_cli_fuse_info_t;

typedef struct mmgr_cli_hw_id {
    size_t len;
    char *id;
} mmgr_cli_hw_id_t;

#endif                          /* MMGR_FW_OPERATIONS */

#endif                          /* __MMGR_EXTERNAL_HEADER_FILE__ */
