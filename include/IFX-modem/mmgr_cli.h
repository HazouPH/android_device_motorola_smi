/* Modem Manager - client library external include file
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

#ifndef __MMGR_C_CLI__
#define __MMGR_C_CLI__

#ifdef __cplusplus
extern "C" {
#endif

#include "mmgr.h"

#define MMGR_CLI_ERR \
    X(SUCCEED), \
    X(FAILED), \
    X(ALREADY_LOCK), \
    X(ALREADY_UNLOCK), \
    X(BAD_HANDLE), \
    X(TIMEOUT), \
    X(REJECTED), \
    X(BAD_CNX_STATE)

typedef enum e_err_mmgr_cli {
#undef X
#define X(a) E_ERR_CLI_ ## a
    MMGR_CLI_ERR
} e_err_mmgr_cli_t;

typedef struct mmgr_cli_event {
    e_mmgr_events_t id;
    size_t len;
    void *data;
    void *context;
} mmgr_cli_event_t;

/* For requests where id == REQUEST_MODEM_RECOVERY, data can point to an
 * array of up to MMGR_CLI_MAX_RECOVERY_CAUSES mmgr_cli_recovery_cause_t
 * structures.
 * For X entries, len should be 'X * sizeof(mmgr_cli_recovery_cause_t))'. */
typedef struct mmgr_cli_request {
    e_mmgr_requests_t id;
    size_t len;
    void *data;
} mmgr_cli_requests_t;

/**
 * Macro to be used to initialize client requests before sending to MMGR.
 * Useful to guarantee backwards-compatibility of client => MMGR API.
 *
 * @param [in, out] request to be initialized.
 * @param [in] req_id of the given request.
 */
#define MMGR_CLI_INIT_REQUEST(request, req_id) \
    do { memset(&(request), 0, sizeof(request)); \
         request.id = (req_id); } while (0)

typedef int (*event_handler) (mmgr_cli_event_t *);

typedef void *mmgr_cli_handle_t;

/**
 * Create mmgr client library handle. This function should be called first.
 * To avoid memory leaks *handle must be set to NULL by the caller.
 *
 * @param [out] handle library handle
 * @param [in] client_name name of the client
 * @param [in] context pointer to a struct that shall be passed to function
 *             context handle can be NULL if unused.
 *
 * @return E_ERR_CLI_FAILED if client_name is NULL or handle creation failed
 * @return E_ERR_CLI_BAD_HANDLE if handle is already created
 * @return E_ERR_CLI_SUCCEED
 */
e_err_mmgr_cli_t mmgr_cli_create_handle(mmgr_cli_handle_t **handle,
                                        const char *client_name, void *context);

/**
 * Delete mmgr client library handle
 *
 * @param [in, out] handle library handle
 *
 * @return E_ERR_CLI_BAD_HANDLE if handle is invalid or handle already
 *         deleted
 * @return E_ERR_CLI_BAD_CNX_STATE if client is connected
 * @return E_ERR_CLI_SUCCEED
 */
e_err_mmgr_cli_t mmgr_cli_delete_handle(mmgr_cli_handle_t *handle);

/**
 * Subscribe to an event. This function shall only be invoked on a valid
 * unconnected handle. Clients must not block the callback. Callback must
 * be used only for a short processing time, otherwise we do not guarantee
 * the responsiveness of the library and events should be received with
 * delay. Also, in the callback function, the client should send no message
 * to the MMGR. This has to be done in another thread/context.
 * NB: users can't subscribe to ACK or NACK events.
 *
 * @param [in,out] handle library handle
 * @param [in] func function pointer to the handle
 * @param [in] id event to subscribe to
 *
 * @return E_ERR_CLI_BAD_HANDLE if handle is invalid
 * @return E_ERR_CLI_BAD_CNX_STATE if connected
 * @return E_ERR_CLI_FAILED event already configured or func is NULL or
 *         unknown/invalid event
 * @return E_ERR_CLI_SUCCEED
 */
e_err_mmgr_cli_t mmgr_cli_subscribe_event(mmgr_cli_handle_t *handle,
                                          event_handler func,
                                          e_mmgr_events_t id);

/**
 * Unsubscribe to an event. This function shall only be invoked on a valid
 * unconnected handle.
 * NB: users can't subscribe to ACK or NACK events.
 *
 * @param [in, out] handle library handle
 * @param [in] id event to unsubscribe to
 *
 * @return E_ERR_CLI_BAD_HANDLE if handle is invalid
 * @return E_ERR_CLI_BAD_CNX_STATE if connected
 * @return E_ERR_CLI_FAILED unknown/invalid event
 * @return E_ERR_CLI_SUCCEED
 */
e_err_mmgr_cli_t mmgr_cli_unsubscribe_event(mmgr_cli_handle_t *handle,
                                            e_mmgr_events_t ev);

/**
 * Connect a previously not connected client to MMGR.
 * This function returns when the connection is achieved successfully,
 * or fail on error, or fail after a timeout of 20s.
 * This function shall only be invoked on a valid unconnected handle.
 *
 * @param [in] handle library handle
 *
 * @return E_ERR_CLI_BAD_HANDLE if handle is invalid
 * @return E_ERR_CLI_BAD_CNX_STATE if connected
 * @return E_ERR_CLI_TIMEOUT if MMGR is not responsive or after a timeout of
 *         20s
 * @return E_ERR_CLI_FAILED internal error
 * @return E_ERR_CLI_SUCCEED
 */
e_err_mmgr_cli_t mmgr_cli_connect(mmgr_cli_handle_t *handle);

/**
 * Disconnect from MMGR. If a lock is set, the unlock is done automatically.
 *
 * @param [in] handle library handle
 *
 * @return E_ERR_CLI_BAD_HANDLE if handle is invalid
 * @return E_ERR_CLI_BAD_CNX_STATE if already disconnected
 * @return E_ERR_CLI_FAILED internal error
 * @return E_ERR_CLI_SUCCEED
 */
e_err_mmgr_cli_t mmgr_cli_disconnect(mmgr_cli_handle_t *handle);

/**
 * Acquire the modem resource. This will start the modem if the modem is
 * currently down. This function returns when acquire request is parsed and
 * queueud by MMGR (but not processed yet). It could return an error
 * (see error description below). The lock is kept until explicit client
 * disconnection or request.
 *
 * @param [in] handle library handle
 *
 * @return E_ERR_CLI_BAD_HANDLE if handle is invalid
 * @return E_ERR_CLI_TIMEOUT if MMGR is not responsive or after a timeout of
 *         20s
 * @return E_ERR_CLI_REJECTED if this function is called under the callback
 * @return E_ERR_CLI_BAD_CNX_STATE if not connected
 * @return E_ERR_CLI_FAILED internal error
 * @return E_ERR_CLI_ALREADY_LOCK if the modem resource is already acquired
 * @return E_ERR_CLI_SUCCEED
 */
e_err_mmgr_cli_t mmgr_cli_lock(mmgr_cli_handle_t *handle);

/**
 * Release the modem resource. This function returns when release request is
 * parsed and queueud by MMGR (but not processed yet). This will stop the
 * modem if no client hold the resource. It could return an error
 * (see error description below).
 *
 * @param [in] handle library handle
 *
 * @return E_ERR_CLI_BAD_HANDLE if handle is invalid
 * @return E_ERR_CLI_TIMEOUT if MMGR is not responsive or after a timeout of
 *         20s
 * @return E_ERR_CLI_REJECTED if this function is called under the callback
 * @return E_ERR_CLI_BAD_CNX_STATE if not connected
 * @return E_ERR_CLI_FAILED internal error
 * @return E_ERR_CLI_ALREADY_UNLOCK if already unlocked
 * @return E_ERR_CLI_SUCCEED
 */
e_err_mmgr_cli_t mmgr_cli_unlock(mmgr_cli_handle_t *handle);

/**
 * Send a request to MMGR. This function returns when request is parsed and
 * queueud by MMGR (but not processed yet). It could return an error if MMGR
 * is not responsive or after a timeout of 20s.
 *
 * @param [in] handle library handle
 * @param [in] request request to send to the mmgr
 *
 * @return E_ERR_CLI_BAD_HANDLE if handle is invalid
 * @return E_ERR_CLI_TIMEOUT if MMGR is not responsive or after a timeout of
 *         20s
 * @return E_ERR_CLI_REJECTED if this function is called under the callback
 * @return E_ERR_CLI_BAD_CNX_STATE if not connected
 * @return E_ERR_CLI_FAILED internal error
 * @return E_ERR_CLI_FAILED if request is NULL or invalid request id
 * @return E_ERR_CLI_SUCCEED
 */
e_err_mmgr_cli_t mmgr_cli_send_msg(mmgr_cli_handle_t *handle,
                                   const mmgr_cli_requests_t *request);
/**
 * For debug purpose only. Do not use it in production
 *
 * Gets file descriptor
 *
 * @param [in] handle library handle
 *
 * @return file descriptor
 */
int mmgr_cli_get_fd(mmgr_cli_handle_t *handle);

/**
 * Example:
 *
 *   mmgr_create_handle
 *       mmgr_subscribe_event(E1)
 *       mmgr_subscribe_event(E2)
 *       mmgr_subscribe_event(E3)
 *           mmgr_connect (Listen E1, E2 and E3)
 *           mmgr_lock(); // Lock shall be sent once the client is connected
 *           mmgr_disconnect (Stop listening and automatic Unlock )
 *       mmgr_unsubscribe_event(E3)
 *           mmgr_connect (Listen E1 and E2)
 *           mmgr_disconnect(Stop listening)
 *   mmgr_delete_handle
 */

#ifdef __cplusplus
}
#endif
#endif                          /* __MMGR_C_CLI__ */
