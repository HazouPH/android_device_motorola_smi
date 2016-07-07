////////////////////////////////////////////////////////////////////////////
// channel_nd.cpp
//
// Copyright 2009 Intrinsyc Software International, Inc.  All rights reserved.
// Patents pending in the United States of America and other jurisdictions.
//
//
// Description:
//    Implements the CChannel class, which provides the
//    infrastructure for the various AT Command channels.
//
/////////////////////////////////////////////////////////////////////////////

#include <stdio.h>

#include "types.h"
#include "rillog.h"
#include "thread_ops.h"
#include "command.h"
#include "response.h"
#include "cmdcontext.h"
#include "reset.h"
#include "channel_nd.h"
#include "te.h"
#include "rril_OEM.h"

//  This is for socket-related calls.
#include <sys/ioctl.h>
#include <errno.h>
#include <linux/termios.h>

CChannel::CChannel(UINT32 uiChannel)
: CChannelBase(uiChannel),
  m_pResponse(NULL)
{
    RIL_LOG_VERBOSE("CChannel::CChannel() - Enter/Exit\r\n");
}

CChannel::~CChannel()
{
    RIL_LOG_VERBOSE("CChannel::~CChannel() - Enter\r\n");

    delete m_pResponse;
    m_pResponse = NULL;

    RIL_LOG_VERBOSE("CChannel::~CChannel() - Exit\r\n");
}

//
// Function to get ABORT timeout (in ms) from RIL request ID
//
UINT32 GetAbortTimeout(UINT32 reqID)
{
    const UINT32 DEFAULT_ABORT_TIMEOUT = 500;  //  Default ABORT timeout
    UINT32 timeout = DEFAULT_ABORT_TIMEOUT;

    switch(reqID)
    {
        case RIL_REQUEST_QUERY_NETWORK_SELECTION_MODE:  // Read COPS?
        case RIL_REQUEST_QUERY_AVAILABLE_NETWORKS:      // Test COPS=?
            timeout = 22000;
            break;

        case RIL_REQUEST_OPERATOR:                      // Read XCOPS
        case RIL_REQUEST_SET_NETWORK_SELECTION_AUTOMATIC:   // Set COPS=
        case RIL_REQUEST_SET_NETWORK_SELECTION_MANUAL:  // Set COPS=
            timeout = 42000;
            break;

        case RIL_REQUEST_QUERY_CALL_FORWARD_STATUS: // Read CCFC
        case RIL_REQUEST_SET_CALL_FORWARD:          // Set CCFC=
            timeout = 37000;
            break;

        case RIL_REQUEST_QUERY_CLIP:                // Read CLIP
            timeout = 37000;
            break;

        case RIL_REQUEST_GET_CLIR:                  // Read CLIR
        case RIL_REQUEST_SET_CLIR:                  // Set CLIR
            timeout = 37000;
            break;

        case RIL_REQUEST_DATA_CALL_LIST:            // Read CGACT
            timeout = 2000;
            break;

        case RIL_REQUEST_DEACTIVATE_DATA_CALL:      // Set CGACT
            timeout = 2000;
            break;

        default:
            timeout = DEFAULT_ABORT_TIMEOUT;
            break;
    }

    return timeout;
}

//
// Is request ID an abortable command?
//
BOOL IsReqIDAbortable(UINT32 reqID)
{
    BOOL bIsAbortable = FALSE;

    switch(reqID)
    {
        case RIL_REQUEST_QUERY_NETWORK_SELECTION_MODE:  // Read COPS?
        case RIL_REQUEST_QUERY_AVAILABLE_NETWORKS:      // Test COPS=?
        case RIL_REQUEST_OPERATOR:                      // Read XCOPS
        case RIL_REQUEST_QUERY_CALL_FORWARD_STATUS:     // Read CCFC
        case RIL_REQUEST_SET_CALL_FORWARD:              // Set CCFC=
        case RIL_REQUEST_QUERY_CLIP:                    // Read CLIP
        case RIL_REQUEST_GET_CLIR:                      // Read CLIR
        case RIL_REQUEST_SET_CLIR:                      // Set CLIR
        case RIL_REQUEST_DATA_CALL_LIST:                // Read CGACT
        case RIL_REQUEST_DEACTIVATE_DATA_CALL:          // Set CGACT
            bIsAbortable = TRUE;
            break;

        default:
            bIsAbortable = FALSE;
            break;
    }
    return bIsAbortable;
}


//
//  request ID for command that requires modem ping
//  on different channel
//
BOOL ReqIDSpecialPing(UINT32 reqID)
{
    BOOL bSpecialPing = FALSE;

    switch(reqID)
    {
        case RIL_REQUEST_SET_NETWORK_SELECTION_AUTOMATIC: // Set COPS=0
        case RIL_REQUEST_SET_NETWORK_SELECTION_MANUAL:  // Set COPS=1
            bSpecialPing = TRUE;
            break;

        default:
            bSpecialPing = FALSE;
            break;
    }
    return bSpecialPing;
}

//
// Send a RIL command
//
BOOL CChannel::SendCommand(CCommand*& rpCmd)
{
    RIL_LOG_VERBOSE("CChannel::SendCommand() - Enter\r\n");

    char*           pATCommand = NULL;
    char*           pATCommand2 = NULL;
    CResponse*      pResponse = NULL;
    RIL_RESULT_CODE resCode = RRIL_E_UNKNOWN_ERROR;
    BOOL            bResult = FALSE;
    PFN_TE_POSTCMDHANDLER postCmdHandler = NULL;
    POST_CMD_HANDLER_DATA data;

    if (NULL == rpCmd)
    {
        RIL_LOG_CRITICAL("CChannel::SendCommand() - chnl=[%d] NULL params\r\n", m_uiRilChannel);
        // command null case should not happen.
        delete pResponse;
        pResponse = NULL;
        return FALSE;
    }

    RIL_LOG_VERBOSE("CChannel::SendCommand() - DEBUG: chnl=[%d] Executing command with ID="
            "[0x%x,%d]\r\n", m_uiRilChannel, rpCmd->GetRequestID(), rpCmd->GetRequestID());

    if (NULL == rpCmd->GetATCmd1())
    {
        // noop operation
        bResult = TRUE;
        goto Error;
    }
    else
    {
        // process command here
        int numRetries = 0;
        UINT32 uiCommandTimeout = rpCmd->GetTimeout();
        UINT32 uiTimeoutThresholdForRetry = CTE::GetTE().GetTimeoutThresholdForRetry();

        //  Compare command timeout of threshold timeout
        if (0 == uiTimeoutThresholdForRetry)
        {
            //  A threshold value of 0 means no retries.
            numRetries = 0;
        }
        else if (uiCommandTimeout <= uiTimeoutThresholdForRetry)
        {
            //  AT Command timeout is <= threshold time, retry ONCE
            numRetries = 1;
        }
        else
        {
            //  AT command timeout is > threshold time.  No retries.
            numRetries = 0;
        }

        pATCommand = (char*) rpCmd->GetATCmd1();
        pATCommand2 = (char*) rpCmd->GetATCmd2();
        UINT32 nCmd1Length = (NULL == pATCommand) ? 0 : strlen(pATCommand);
        UINT32 nCmd2Length = (NULL == pATCommand2) ? 0 : strlen(pATCommand2);
        CRLFExpandedString cmd1(pATCommand, nCmd1Length);
        CRLFExpandedString cmd2(pATCommand2, nCmd2Length);

        // GetString will return a pointer to the actual command buffer or "NULL"
        const char* pPrintStr = cmd1.GetString();
        const char* pPrintCmd2Str = cmd2.GetString();

        if (CRilLog::IsFullLogBuild())
            RIL_LOG_INFO("CChannel::SendCommand() -"
                    " chnl=[%d] RILReqID=[%d] retries=[%d] Timeout=[%d]"
                    " cmd1=[%s] cmd2=[%s]\r\n", m_uiRilChannel, rpCmd->GetRequestID(), numRetries,
                    rpCmd->GetTimeout(), pPrintStr, pPrintCmd2Str);
        else
            RIL_LOG_INFO("CChannel::SendCommand() - chnl=[%d] RILReqID=[%d] retries=[%d]"
                    " Timeout=[%d]\r\n", m_uiRilChannel, rpCmd->GetRequestID(), numRetries,
                    rpCmd->GetTimeout());

        do
        {
            UINT32 uiBytesWritten = 0;

            pATCommand = (char*) rpCmd->GetATCmd1();

            CMutex::Lock(m_pResponseObjectAccessMutex);

            // set flag for response thread
            SetCmdThreadBlockedOnRxQueue();

            // empty the queue before sending the command
            g_pRxQueue[m_uiRilChannel]->MakeEmpty();
            if (NULL != m_pResponse)
                m_pResponse->FreeData();

            CMutex::Unlock(m_pResponseObjectAccessMutex);

            nCmd1Length = (NULL == pATCommand) ? 0 : strlen(pATCommand);
            BOOL bSuccess = WriteToPort(pATCommand, nCmd1Length, uiBytesWritten);
            // write the command out to the com port
            if (!bSuccess)
            {
                // write() = -1, error.
                if (CRilLog::IsFullLogBuild())
                    RIL_LOG_CRITICAL("CChannel::SendCommand() - write() = -1, chnl=[%d] Error"
                            " writing command: %s\r\n", m_uiRilChannel, pPrintStr);
                else
                    RIL_LOG_CRITICAL("CChannel::SendCommand() - write() = -1, chnl=[%d] Error"
                            " writing requestId: %d\r\n", m_uiRilChannel, rpCmd->GetRequestID());

                DO_REQUEST_CLEAN_UP(); // Reason saved in WriteToPort
            }

            if (nCmd1Length != uiBytesWritten)
            {
                if (CRilLog::IsFullLogBuild())
                    RIL_LOG_CRITICAL("CChannel::SendCommand() - chnl=[%d] Only wrote [%d] chars of"
                            " command to port: %s\r\n", m_uiRilChannel, uiBytesWritten, pPrintStr);
                else
                    RIL_LOG_CRITICAL("CChannel::SendCommand() - chnl=[%d] Only wrote [%d] chars of"
                            " command to port, RequestId=[%d]\r\n", m_uiRilChannel, uiBytesWritten,
                            rpCmd->GetRequestID());
            }

            // retrieve response from modem
            resCode = GetResponse(rpCmd, pResponse);

            // response received, clear flag
            ClearCmdThreadBlockedOnRxQueue();

            if (!pResponse)
            {
                if (CRilLog::IsFullLogBuild())
                    RIL_LOG_CRITICAL("CChannel::SendCommand() - chnl=[%d] No response received to"
                            " TX [%s]\r\n", m_uiRilChannel, pPrintStr);
                else
                    RIL_LOG_CRITICAL("CChannel::SendCommand() - chnl=[%d] No response received for"
                            " requestId [%d]\r\n", m_uiRilChannel, rpCmd->GetRequestID());
                goto Error;
            }

            if (!pResponse->IsTimedOutFlag())
            {
                //  Our response is complete!
                break;
            }
            else if (resCode == RRIL_E_MODEM_RESET)
            {
                //  The modem was reset in the GetResponse call, exit the loop, no need to attempt
                //  a retry.
                break;
            }
            else
            {
                // Waiting indefinitely for modem response. In order for the command not to be
                // abandoned, the ping has to be done on a different DLC.
                if (ReqIDSpecialPing(rpCmd->GetRequestID()))
                {
                    if (CRilLog::IsFullLogBuild())
                        RIL_LOG_INFO("CChannel::SendCommand() - chnl=[%u] Response timeout"
                                " for cmd=[%s]. Waiting indefinitely while modem"
                                " active\r\n", m_uiRilChannel, pPrintStr);
                    else
                        RIL_LOG_INFO("CChannel::SendCommand() - chnl=[%u] Response timeout"
                                " for requestId [%d]. This is a special case, so waiting "
                                " indefinitely while modem active. \r\n", m_uiRilChannel,
                                rpCmd->GetRequestID());

                    WaitForResponse(rpCmd, pResponse);
                    break;
                }

                // Our response timed out, retry if we can
                if (numRetries > 0)
                {
                    RIL_LOG_INFO("CChannel::SendCommand() - ***** chnl=[%d] Attempting retry"
                            "  numRetries remaining=[%d] *****\r\n", m_uiRilChannel, numRetries);
                }
                else
                {
                    //  If this was last attempt and we timed out, and this was an init command,
                    //  then reset modem.  Signal clean up to MMGR.
                    if (rpCmd->IsInitCommand())
                    {
                        RIL_LOG_CRITICAL("CChannel::SendCommand() - ***** chnl=[%d] Init command"
                                " timed-out. Reset modem! *****\r\n", m_uiRilChannel);
                        // In this case, it's always AT command 1 that is sent
                        RequestCleanUpOnCommandTimeout(rpCmd, 1 /* Cmd Index */);
                        goto Error;
                    }
                }
            }
        } while (--numRetries >= 0);
    }

    if (NULL == pResponse)
    {
        RIL_LOG_CRITICAL("CChannel::SendCommand() : No response\r\n");
        goto Error;
    }

    if (pResponse->IsTimedOutFlag())
    {
        RIL_LOG_CRITICAL("CChannel::SendCommand() - Command Timedout\r\n");
        goto Error;
    }

    // Handle the response
    if (!ParseResponse(rpCmd, pResponse))
    {
        goto Error;
    }

    bResult = TRUE;

Error:
    postCmdHandler = rpCmd->GetPostCmdHandlerFcn();
    memset(&data, 0, sizeof(POST_CMD_HANDLER_DATA));

    if (postCmdHandler)
    {
        data.uiChannel = rpCmd->GetChannel();
        data.pRilToken = rpCmd->GetToken();
        data.pContextData = rpCmd->GetContextData();
        data.uiContextDataSize = rpCmd->GetContextDataSize();
        data.requestId = rpCmd->GetRequestID();
        data.uiResultCode = RRIL_RESULT_OK;

        if (NULL != pResponse)
        {
             data.uiErrorCode = pResponse->GetErrorCode();
             data.uiResultCode = pResponse->GetResultCode();
             pResponse->GetData(data.pData, data.uiDataSize);
        }

        /*
         * Call the Post command handler function which will either complete
         * the RIL request after doing Post command cleanup or send another
         * command.
         */
        (CTE::GetTE().*postCmdHandler)(data);
    }
    else
    {
        data.uiChannel = rpCmd->GetChannel();
        data.pRilToken = rpCmd->GetToken();
        data.pContextData = rpCmd->GetContextData();
        data.uiContextDataSize = rpCmd->GetContextDataSize();
        data.requestId = rpCmd->GetRequestID();
        data.uiResultCode = RRIL_RESULT_ERROR;

        if (NULL != pResponse)
        {
             data.uiErrorCode = pResponse->GetErrorCode();
             data.uiResultCode = pResponse->GetResultCode();
             pResponse->GetData(data.pData, data.uiDataSize);
        }

        /*
         * This function will only complete the RIL request. Use this in case
         * of no cleanup required.
         */
        CTE::GetTE().PostCmdHandlerCompleteRequest(data);
    }

    if (!bResult)
    {
        RIL_LOG_CRITICAL("CChannel::SendCommand() Failed");
    }

    delete rpCmd;
    rpCmd = NULL;

    delete pResponse;
    pResponse = NULL;

    RIL_LOG_VERBOSE("CChannel::SendCommand() - Exit\r\n");
    return bResult;
}

//
// Get modem response from queue and send 2nd AT command if present
//
RIL_RESULT_CODE CChannel::GetResponse(CCommand*& rpCmd, CResponse*& rpResponse)
{
    RIL_RESULT_CODE resCode = RIL_E_GENERIC_FAILURE;
    char*           pATCommand = NULL;

    RIL_LOG_VERBOSE("CChannel::GetResponse() - Enter\r\n");

    // Get the response out of the Response Queue
    resCode = ReadQueue(rpResponse, rpCmd->GetTimeout());

#if defined(SIMULATE_MODEM_RESET)
    //  This is for testing purposes only. (radio reboot)
    static int nCount = 0;
    if (RIL_REQUEST_SIGNAL_STRENGTH == rpCmd->GetRequestID())
    {
        nCount++;
        RIL_LOG_INFO("COUNT = %d\r\n", nCount);
        if (nCount == 4)
        {
            DO_REQUEST_CLEAN_UP(1, "Radio reboot test");
            goto Error;
        }
    }
#endif // SIMULATE_MODEM_RESET

    pATCommand = (char*) rpCmd->GetATCmd1();

    if (rpResponse && rpResponse->IsTimedOutFlag())
    {
        if ((CRilLog::IsFullLogBuild()) && (pATCommand != NULL))
        {
            CRLFExpandedString cmd(pATCommand, strlen(pATCommand));
            const char* pPrintStr = cmd.GetString();

            RIL_LOG_CRITICAL("CChannel::GetResponse() - ***** Command timed out chnl=[%d]!"
                    " timeout=[%d]ms No response to TX [%s] *****\r\n", m_uiRilChannel,
                    rpCmd->GetTimeout(), pPrintStr);
        }
        else
            RIL_LOG_CRITICAL("CChannel::GetResponse() - ***** Command timed out chnl=[%d]!"
                    " timeout=[%d]ms No response for requestId TX [%d] *****\r\n",
                    m_uiRilChannel, rpCmd->GetTimeout(), rpCmd->GetRequestID());

        if (HandleTimeout(rpCmd, rpResponse, 1 /* Cmd Index */))
        {
            resCode = RRIL_E_MODEM_RESET;
        }
        goto Error;
    }
    else if (RIL_E_SUCCESS != resCode)
    {
        RIL_LOG_CRITICAL("CChannel::GetResponse() - Failed read from queue\r\n");
        goto Error;
    }
    else
    {
        // We got a valid response.
    }


    //  Send 2nd phase of command
    //  Only send if response to first command was OK (RIL_E_SUCCESS) OR
    //  SEEK for Android request returned CME ERROR (need to get error code
    //   for custom SEEK response code)
    if ((NULL != rpCmd->GetATCmd2()) &&
        (NULL != rpResponse) &&
        (SendCommandPhase2(rpResponse->GetResultCode(), rpCmd->GetRequestID())))
    {
        pATCommand = (char*) rpCmd->GetATCmd2();
        UINT32 uiBytesWritten = 0;

        // send 2nd phase of command
        SetCmdThreadBlockedOnRxQueue();

        UINT32 cmdStrLen = (NULL == pATCommand) ? 0 : strlen(pATCommand);
        CRLFExpandedString cmd(pATCommand, cmdStrLen);
        const char* pPrintStr = cmd.GetString();

        BOOL bSuccess = WriteToPort(pATCommand, cmdStrLen, uiBytesWritten);
        if (!bSuccess)
        {
            if (CRilLog::IsFullLogBuild())
                RIL_LOG_CRITICAL("CChannel::GetResponse() - chnl=[%d] Error sending 2nd command:"
                        " %s\r\n", m_uiRilChannel, pPrintStr);
            else
                RIL_LOG_CRITICAL("CChannel::GetResponse() - chnl=[%d] Error sending 2nd command"
                        " with request ID=[%d]\r\n", m_uiRilChannel, rpCmd->GetRequestID());

            // ignore error and wait for a modem response, or time out
        }

        if (cmdStrLen != uiBytesWritten)
        {
            if (CRilLog::IsFullLogBuild())
                RIL_LOG_CRITICAL("CChannel::GetResponse() - chnl=[%d] Could only write [%d] chars"
                        " of 2nd command: %s\r\n", m_uiRilChannel, uiBytesWritten, pPrintStr);
            else
                RIL_LOG_CRITICAL("CChannel::GetResponse() - chnl=[%d] Could only write [%d] chars"
                        " of 2nd command RequestID=[%d]\r\n",
                        m_uiRilChannel, uiBytesWritten, rpCmd->GetRequestID());
        }

        // wait for the secondary response
        delete rpResponse;
        rpResponse = NULL;
        resCode = ReadQueue(rpResponse, rpCmd->GetTimeout());

        if (rpResponse && rpResponse->IsTimedOutFlag())
        {
            if (CRilLog::IsFullLogBuild())
                RIL_LOG_CRITICAL("CChannel::GetResponse() - ***** Command2 timed out chnl=[%d] !"
                        " timeout=[%d]ms No response to TX [%s] *****\r\n",
                        m_uiRilChannel, rpCmd->GetTimeout(), pPrintStr);
            else
                RIL_LOG_CRITICAL("CChannel::GetResponse() - ***** Command2 timed out chnl=[%d] !"
                        " timeout=[%d]ms No response to requestID [%d] *****\r\n",
                        m_uiRilChannel, rpCmd->GetTimeout(), rpCmd->GetRequestID());

            if (HandleTimeout(rpCmd, rpResponse, 2 /* Cmd Index */))
            {
                resCode = RRIL_E_MODEM_RESET;
            }
            goto Error;
        }
        else if (RIL_E_SUCCESS != resCode)
        {
            RIL_LOG_CRITICAL("CChannel::GetResponse() - chnl=[%d] Failed read from queue for"
                    " command's second response\r\n", m_uiRilChannel);
            goto Error;
        }
        else
        {
            // We got valid response.
        }
    }

Error:
    RIL_LOG_VERBOSE("CChannel::GetResponse() - Exit  resCode=[%d]\r\n", resCode);
    return resCode;
}

//  Helper function to determine whether to send phase 2 of a command
BOOL CChannel::SendCommandPhase2(const UINT32 uiResCode,
                                    const int reqID) const
{
    BOOL bSendPhase2 = TRUE;

    //  Is our request ID in the special list?
    switch (reqID)
    {
        case RIL_REQUEST_SEND_USSD:
        case RIL_REQUEST_GET_CLIR:
        case RIL_REQUEST_SET_CLIR:
        case RIL_REQUEST_QUERY_CALL_FORWARD_STATUS:
        case RIL_REQUEST_SET_CALL_FORWARD:
        case RIL_REQUEST_QUERY_CALL_WAITING:
        case RIL_REQUEST_SET_CALL_WAITING:
        case RIL_REQUEST_QUERY_FACILITY_LOCK:
        case RIL_REQUEST_SET_FACILITY_LOCK:
        case RIL_REQUEST_CHANGE_BARRING_PASSWORD:
        case RIL_REQUEST_QUERY_CLIP:
        case RIL_REQUEST_SETUP_DATA_CALL:
            if (RIL_E_SUCCESS == uiResCode)
                bSendPhase2 = FALSE;
            else
                bSendPhase2 = TRUE;
            break;
        default:
            if (RIL_E_SUCCESS == uiResCode)
                bSendPhase2 = TRUE;
            else
                bSendPhase2 = FALSE;
            break;
    }

    return bSendPhase2;
}

// Helper function to request modem restart due to command time-out
void CChannel::RequestCleanUpOnCommandTimeout(CCommand* rpCmd, UINT32 uiCmdIndex)
{
    const char* pszCmdString = uiCmdIndex == 1 ?
            rpCmd->GetATCmd1() :
            rpCmd->GetATCmd2();
    char szTimeOut[MAX_STRING_SIZE_FOR_INT] = { '\0' };
    char szChannel[MAX_STRING_SIZE_FOR_INT] = { '\0' };
    if ((!CRilLog::IsFullLogBuild()) || (pszCmdString == NULL))
    {
        pszCmdString  = "<AT cmd>";
    }
    CRLFExpandedString cmd(pszCmdString, strlen(pszCmdString));
    snprintf(szTimeOut, sizeof(szTimeOut), "%u", rpCmd->GetTimeout());
    snprintf(szChannel, sizeof(szChannel), "%u", m_uiRilChannel);
    DO_REQUEST_CLEAN_UP(3, cmd.GetString(), szTimeOut, szChannel);
}

//
//  This function handles the timeout mechanism.
//  If timeout, send ABORT.  Then send PING.
//
//  Function returns TRUE if the modem has been reset.
BOOL CChannel::HandleTimeout(CCommand*& rpCmd, CResponse*& /*rpResponse*/,
        UINT32 uiCmdIndex)
{
    RIL_LOG_VERBOSE("CChannel::HandleTimeout() - Enter\r\n");

    //  -If there is AT command timeout, check to see if AT command is abortable or not.
    //   If AT command is abortable:
    //     -Send ABORT command, wait for ABORT response.
    //        -If ABORT timeout, assume modem is dead. Request clean up from MMGR. Quit processing.
    //        -If ABORT response received, continue with Ping processing.
    //  -"Ping" the modem with simple AT command.
    //     -If "Ping" timeout, assume modem is dead.  Request clean up from MMGR. Quit processing.
    //     -If "Ping" response received, assume modem is alive.
    //  - If command can be retried and no MMGR clean-up requested, re-send original AT command.

    BOOL bNeedModemCleanUp = FALSE;
    const UINT32 PING_TIMEOUT = 3000;  // PING timeout in ms.
    char szABORTCmd[] = "AT\x1b\r";  //  AT<esc>\r
    char szPINGCmd[] = "ATE0V1;+CMEE=1\r";
    char szSimplePINGCmd[] = "ATE0V1\r";

    UINT32 uiBytesWritten = 0;
    CResponse* pRspTemp = NULL;
    RIL_RESULT_CODE resTmp = RIL_E_SUCCESS;

    //  Is AT command abortable?  If so, send ABORT command.
    if ( IsReqIDAbortable(rpCmd->GetRequestID()) )
    {
        // Send ABORT command
        UINT32 uiAbortTimeout = GetAbortTimeout(rpCmd->GetRequestID());
        RIL_LOG_INFO("CChannel::HandleTimeout() - Sending ABORT Command on chnl=[%d],"
                " timeout=[%d]ms\r\n", m_uiRilChannel, uiAbortTimeout);
        WriteToPort(szABORTCmd, strlen(szABORTCmd), uiBytesWritten);

        resTmp = ReadQueue(pRspTemp, uiAbortTimeout); //  wait for ABORTED response

        //  Did the ABORT command timeout?
        if (pRspTemp && pRspTemp->IsTimedOutFlag())
        {
            //  ABORTED timeout
            RIL_LOG_CRITICAL("CChannel::HandleTimeout() - chnl=[%d] ABORT command timed out!!\r\n",
                    m_uiRilChannel);
        }
        else if (RIL_E_SUCCESS != resTmp)
        {
            RIL_LOG_CRITICAL("CChannel::HandleTimeout() - chnl=[%d] Failed read from queue during"
                    " ABORTED !\r\n", m_uiRilChannel);

            bNeedModemCleanUp = TRUE;

            goto Error;
        }
        else
        {
            //  Received response to ABORTED
            RIL_LOG_INFO("CChannel::HandleTimeout() - chnl=[%d] Received response to ABORT"
                    " command!!\r\n", m_uiRilChannel);
        }
        delete pRspTemp;
        pRspTemp = NULL;
    }

    //  If AT command allows simple PING, send PING command.
    if (!ReqIDSpecialPing(rpCmd->GetRequestID())) {

        SetCmdThreadBlockedOnRxQueue();  //  Tell response thread that response is pending

        // empty the queue before sending the command
        g_pRxQueue[m_uiRilChannel]->MakeEmpty();

        FlushResponse();

        uiBytesWritten = 0;
        RIL_LOG_INFO("CChannel::HandleTimeout() - Sending PING Command on chnl=[%d],"
                " timeout=[%d]ms\r\n", m_uiRilChannel, PING_TIMEOUT);
        WriteToPort(szPINGCmd, strlen(szPINGCmd), uiBytesWritten);

        resTmp = ReadQueue(pRspTemp, PING_TIMEOUT);  // Wait for PING response

        //  Did the PING command timeout?
        if (pRspTemp && pRspTemp->IsTimedOutFlag())
        {
            //  PING timeout
            //  Assume modem is dead!  Signal MMGR to request cleanup.
            RIL_LOG_CRITICAL("CChannel::HandleTimeout() - chnl=[%d] PING attempt timed out!!"
                    "  Assume MODEM IS DEAD!\r\n", m_uiRilChannel);
            RIL_LOG_CRITICAL("CChannel::HandleTimeout() - chnl=[%d] request clean up\r\n",
                    m_uiRilChannel);

            bNeedModemCleanUp = TRUE;
        }
        else if (RIL_E_SUCCESS != resTmp)
        {
            RIL_LOG_CRITICAL("CChannel::HandleTimeout() - chnl=[%d] Failed read from queue during"
                    " PING !\r\n", m_uiRilChannel);

            bNeedModemCleanUp = TRUE;
        }
        else
        {
            //  Received response to PING
            RIL_LOG_INFO("CChannel::HandleTimeout() - chnl=[%d] Received response to PING"
                    " command!!\r\n", m_uiRilChannel);

            //  Modem is alive.  Let calling function handle the retry attempt.
        }
    }
    else //  If AT command requires PING command to be sent on different channel.
    {
        CCommand* pCmd = NULL;
        char szCmd[MAX_BUFFER_SIZE];

        RIL_LOG_INFO("CChannel::HandleTimeout() - Sending PING Command on chnl=[%d],"
                " timeout=[%d]ms\r\n", RIL_CHANNEL_ATCMD, PING_TIMEOUT);

        if (!CopyStringNullTerminate(szCmd, szSimplePINGCmd, sizeof(szCmd)))
        {
            RIL_LOG_CRITICAL("CChannel::HandleTimeout() - Could not"
                    " make modem ping command.\r\n");
            bNeedModemCleanUp = TRUE;
            goto Error;
        }

        if (RIL_CHANNEL_ATCMD != m_uiRilChannel)
        {
            pCmd = new CCommand(RIL_CHANNEL_ATCMD, 0, REQ_ID_NONE, szCmd);
        }
        else
        {
            RIL_LOG_CRITICAL("CChannel::HandleTimeout() - Unable to send special"
                    " PING command. Same channel number\r\n");
            bNeedModemCleanUp = TRUE;
            goto Error;
        }

        if (pCmd)
        {
            pCmd->SetTimeout(PING_TIMEOUT);
            pCmd->SetHighPriority();
            if (!CCommand::AddCmdToQueue(pCmd))
            {
                RIL_LOG_CRITICAL("CChannel::HandleTimeout() - Could not queue"
                        "modem ping command.\r\n");
                delete pCmd;
                pCmd = NULL;
                bNeedModemCleanUp = TRUE;
                goto Error;
             }
        }

        RIL_LOG_INFO("CChannel::HandleTimeout() - PING Command added to queue");

    }

Error:
    if (bNeedModemCleanUp)
    {
        RequestCleanUpOnCommandTimeout(rpCmd, uiCmdIndex);
    }

    delete pRspTemp;
    pRspTemp = NULL;

    RIL_LOG_VERBOSE("CChannel::HandleTimeout() - Exit  bRet=[%d]\r\n", bNeedModemCleanUp);
    return bNeedModemCleanUp;
}


BOOL CChannel::ProcessNoop(CResponse*& rpResponse)
{
    // no-op command, nothing to send - processed here
    // allocate a new response
    rpResponse = new CResponse(this);
    if (NULL == rpResponse)
    {
        // signal critical error for low memory
        DO_REQUEST_CLEAN_UP(1, "Out of memory");
        return FALSE;
    }

    // fake an OK response for the no-op command
    rpResponse->SetResultCode(RIL_E_SUCCESS);
    rpResponse->SetUnsolicitedFlag(FALSE);

    return TRUE;
}

//
// Handle response to an AT command
//
BOOL CChannel::ParseResponse(CCommand*& rpCmd, CResponse*& rpRsp)
{
    RIL_LOG_VERBOSE("CChannel::ParseResponse() - Enter\r\n");

    BOOL bResult = FALSE;

    if ((NULL == rpCmd) || (NULL == rpRsp))
    {
        RIL_LOG_CRITICAL("CChannel::ParseResponse() : Invalid arguments\r\n");
        goto Error;
    }

    // parse the response if successful, or if client requires parse in all circumstances
    if ((RIL_E_SUCCESS == rpRsp->GetResultCode()) || rpCmd->IsAlwaysParse())
    {
        (void) rpRsp->ParseResponse(rpCmd);
    }

    // call the context
    if (rpCmd->GetContext())
    {
        rpCmd->GetContext()->Execute(
                RIL_E_SUCCESS == rpRsp->GetResultCode(), rpRsp->GetErrorCode());
    }

    bResult = TRUE;

Error:
    RIL_LOG_VERBOSE("CChannel::ParseResponse() - Exit\r\n");
    return bResult;
}

int CChannel::FindIdenticalRequestsAndSendResponses(int reqID,
                                                        UINT32 uiResultCode,
                                                        void* pResponse,
                                                        size_t responseLen,
                                                        int callId)
{
    RIL_LOG_VERBOSE("CChannel::FindIdenticalRequestsAndSendResponses() - Enter\r\n");

    CCommand** pCmdArray = NULL;
    int numOfCommands = 0;
    int nCompletedRequests = 0;

    //  The following function returns us an array of CCommands, and the size of the returned array.
    g_pTxQueue[m_uiRilChannel]->GetAllQueuedObjects(pCmdArray, numOfCommands);

    for (int i = 0; i < numOfCommands; i++)
    {
        if (pCmdArray[i]->GetRequestID() == reqID
                && pCmdArray[i]->GetCallId() == callId)
        {
            //  Dequeue the object, send the response and then free the CCommand.
            //  Do this ONLY if the command is still present in the channel queue
            if (g_pTxQueue[m_uiRilChannel]->DequeueByObj(pCmdArray[i]))
            {
                RIL_Token rilToken = pCmdArray[i]->GetToken();
                if (NULL != rilToken)
                {
                    nCompletedRequests++;
                    RIL_onRequestComplete(rilToken, (RIL_Errno) uiResultCode,
                            pResponse, responseLen);
                }

                delete pCmdArray[i];
            }
            pCmdArray[i] = NULL;
        }
    }

    delete []pCmdArray;
    pCmdArray = NULL;

    RIL_LOG_VERBOSE("CChannel::FindIdenticalRequestsAndSendResponses() - Exit\r\n");
    return nCompletedRequests;
}

BOOL CChannel::ProcessModemData(char* szRxBytes, UINT32 uiRxBytesSize)
{
    RIL_LOG_VERBOSE("CChannel::ProcessModemData() - Enter\r\n");
    BOOL bResult = FALSE;
    BOOL bUnlockMutex = FALSE;

    if (!szRxBytes || uiRxBytesSize == 0)
    {
        RIL_LOG_CRITICAL("CChannel::ProcessModemData() - Invalid params [0x%08x] [%d]\r\n",
                szRxBytes, uiRxBytesSize);
        goto Error;
    }

    RIL_LOG_INFO("CChannel::ProcessModemData() - INFO: chnl=[%d] size=[%d] RX [%s]\r\n",
            m_uiRilChannel, uiRxBytesSize,
            CRLFExpandedString(szRxBytes,uiRxBytesSize).GetString());

    CMutex::Lock(m_pResponseObjectAccessMutex);

    bUnlockMutex = TRUE;

    if (!m_pResponse)
    {
        m_pResponse = new CResponse(this);
        if (!m_pResponse)
        {
            // critically low on memory
            DO_REQUEST_CLEAN_UP(1, "Out of memory");
            goto Error;
        }
    }

    // append data to our response buffer
    if (!m_pResponse->Append(szRxBytes, uiRxBytesSize))
    {
        RIL_LOG_CRITICAL("CChannel::ProcessModemData() - chnl=[%d] Append failed\r\n",
                m_uiRilChannel);
        goto Error;
    }

    if (m_bTimeoutWaitingForResponse)
    {
        m_pResponse->SetCorruptFlag(TRUE);
        m_bTimeoutWaitingForResponse = FALSE;
    }

    // process the data received thus far
    while (m_pResponse->IsCompleteResponse())
    {
        // yes; transfer data to a new buffer; this leaves data in the member variable
        //  buffer if we have more than a complete response
        CResponse* pResponse = NULL;
        if (!CResponse::TransferData(m_pResponse, pResponse))
        {
            RIL_LOG_CRITICAL("CChannel::ProcessModemData() - chnl=[%d] TransferData failed\r\n",
                    m_uiRilChannel);
            goto Error;
        }

        // process the response
        if (!ProcessResponse(pResponse))
        {
            RIL_LOG_CRITICAL("CChannel::ProcessModemData() - chnl=[%d] ProcessResponse failed\r\n",
                    m_uiRilChannel);
            goto Error;
        }
    }

    bResult = TRUE;

Error:
    if (bUnlockMutex)
    {
        CMutex::Unlock(m_pResponseObjectAccessMutex);
    }

    RIL_LOG_VERBOSE("CChannel::ProcessModemData() - Exit [%d]\r\n", bResult);
    return bResult;
}

BOOL CChannel::ProcessResponse(CResponse*& rpResponse)
{
    BOOL bResult = FALSE;

    RIL_LOG_VERBOSE("CChannel::ProcessResponse : chnl=[%d] Enter\r\n", m_uiRilChannel);

    if (rpResponse->IsIgnoreFlag())
    {
        RIL_LOG_INFO("CChannel::ProcessResponse : chnl=[%u] Ignoring %s\r\n", m_uiRilChannel,
                CRLFExpandedString(rpResponse->Data(), rpResponse->Size()).GetString());
    }
    else if (rpResponse->IsUnrecognizedFlag())
    {
        // garbage in buffer, discard
        RIL_LOG_INFO("CChannel::ProcessResponse : chnl=[%d] Unidentified response, size [%d]"
                       "  %s\r\n", m_uiRilChannel, rpResponse->Size(),
                       CRLFExpandedString(rpResponse->Data(), rpResponse->Size()).GetString());
    }
    else if (rpResponse->IsUnsolicitedFlag())
    {
        // unsolicited notification -> broadcast to all clients
        if ((RRIL_RESULT_OK != rpResponse->GetResultCode()))
        {
            void*   pData;
            UINT32  uiDataSize;

            rpResponse->GetData(pData, uiDataSize);
            RIL_LOG_INFO("CChannel::ProcessResponse : chnl=[%d] unsolicited notification"
                    " resultcode=[%d]\r\n", m_uiRilChannel,
                    rpResponse->GetResultCode());
            RIL_onUnsolicitedResponse(rpResponse->GetResultCode(), pData, uiDataSize);
            rpResponse->FreeData();
        }
    }
    else
    {
        // command result
        if (IsCmdThreadBlockedOnRxQueue())
        {
            // response expected, stop waiting for response and queue it
            ClearCmdThreadBlockedOnRxQueue();

            //  Make sure the command thread is listening for a response in proper state.
            //  Sometimes, the Rx Queue IsEmpty() is TRUE, but we enqueue and signal just before
            //  the RxQueueEvent Reset gets called in CChannel::ReadQueue().
            //const unsigned int dwSleep = 75;
            //RIL_LOG_INFO("CChannel::ProcessResponse() - BEGIN SLEEP %u\r\n", dwSleep);
            //Sleep(dwSleep);
            //RIL_LOG_INFO("CChannel::ProcessResponse() - END SLEEP %u\r\n", dwSleep);

            //RIL_LOG_INFO("CChannel::ProcessResponse() - Enqueue response  resultcode=[%d]\r\n",
            //        rpResponse->GetResultCode() );

            // Queue the command response
            if (!g_pRxQueue[m_uiRilChannel]->Enqueue(rpResponse))
            {
                RIL_LOG_CRITICAL("CChannel::ProcessResponse() - Unable to Enqueue response\r\n");
                goto Error;
            }

            // Queue has ownership of this now
            rpResponse = NULL;

            // signal Tx thread
            //RIL_LOG_INFO("CChannel::ProcessResponse : Signal g_RxQueueEvent BEGIN\r\n");
            (void) CEvent::Signal(g_RxQueueEvent[m_uiRilChannel]);
            //(void) CEvent::Reset(g_RxQueueEvent[m_uiRilChannel]);
            //RIL_LOG_INFO("CChannel::ProcessResponse : Signal g_RxQueueEvent END\r\n");

        }
        else
        {
            RIL_LOG_INFO("CChannel::ProcessResponse : chnl=[%d] Non recognized response:"
                    " [%d] %s\r\n", m_uiRilChannel, rpResponse->Size(),
                    CRLFExpandedString(rpResponse->Data(), rpResponse->Size()).GetString());
        }
    }

    bResult = TRUE;

Error:
    // we took ownership of the buffer, free it
    delete rpResponse;
    rpResponse = NULL;

    RIL_LOG_VERBOSE("CChannel::ProcessResponse : chnl=[%d] Exit\r\n", m_uiRilChannel);
    return bResult;
}


RIL_RESULT_CODE CChannel::ReadQueue(CResponse*& rpResponse, UINT32 uiTimeout)
{
    RIL_RESULT_CODE resCode = RIL_E_GENERIC_FAILURE;

    if (g_pRxQueue[m_uiRilChannel]->IsEmpty())
    {
        CEvent* pCancelWaitEvent = CSystemManager::GetInstance().GetCancelWaitEvent();
        CEvent* rgpEvents[] = {g_RxQueueEvent[m_uiRilChannel], pCancelWaitEvent};

        // wait for response
        //RIL_LOG_INFO("CChannel::ReadQueue() - QUEUE EMPTY, WAITING FOR RxQueueEvent...\r\n");
        //CEvent::Reset(g_RxQueueEvent[m_uiRilChannel]);
        UINT32 uiRet = CEvent::WaitForAnyEvent(2, rgpEvents, uiTimeout);
        //CEvent::Reset(g_RxQueueEvent[m_uiRilChannel]);
        switch(uiRet)
        {
            case WAIT_EVENT_0_SIGNALED:
                //RIL_LOG_INFO("CChannel::ReadQueue() - chnl=[%d]"
                //        " RxQueueEvent WAIT_EVENT_0_SIGNALED\r\n", m_uiRilChannel);
                // object pushed to queue
                break;

            case WAIT_EVENT_0_SIGNALED + 1:
                // cancel event signalled
                RIL_LOG_CRITICAL("CChannel::ReadQueue() : chnl=[%d] Cancel signalled\r\n",
                        m_uiRilChannel);
                goto Error;
                break;  // unreachable

            case WAIT_TIMEDOUT:
                // command timed-out
                RIL_LOG_VERBOSE("CChannel::ReadQueue() : chnl=[%d] Setting timed out flag!\r\n",
                        m_uiRilChannel);
                rpResponse = new CResponse(this);
                if (!rpResponse)
                {
                    RIL_LOG_CRITICAL("CChannel::ReadQueue() - chnl=[%d] Failed to allocate memory"
                            " for response!\r\n", m_uiRilChannel);
                    goto Error;
                }

                rpResponse->SetResultCode(resCode);
                rpResponse->SetUnsolicitedFlag(FALSE);
                rpResponse->SetTimedOutFlag(TRUE);

                goto Error;
                break;  // unreachable

            default:
                RIL_LOG_INFO("CChannel::ReadQueue() : chnl=[%d] unknown error\r\n", m_uiRilChannel);
                goto Error;
                break;  // unreachable
        }
    }

    //RIL_LOG_INFO("CChannel::ReadQueue() : chnl=[%d] g_RxQueue Dequeue BEGIN\r\n", m_uiRilChannel);
    // if we get here, there is something in the queue; get it
    //CEvent::Reset(g_RxQueueEvent[m_uiRilChannel]);
    if (!g_pRxQueue[m_uiRilChannel]->Dequeue(rpResponse))
    {
        RIL_LOG_CRITICAL("CChannel::ReadQueue() : Dequeue failed\r\n", m_uiRilChannel);
        goto Error;
    }
    else
    {
        CEvent::Reset(g_RxQueueEvent[m_uiRilChannel]);
    }
    //RIL_LOG_INFO("CChannel::ReadQueue() : chnl=[%d] g_RxQueue Dequeue END\r\n", m_uiRilChannel);

    resCode = RIL_E_SUCCESS;

Error:
    RIL_LOG_VERBOSE("CChannel::ReadQueue() - Exit [%d]\r\n", resCode);
    return resCode;
}

void CChannel::FlushResponse()
{
    void* pData = NULL;
    UINT32 uiDataSize = 0;

    if (NULL == m_pResponse)
    {
        return;
    }

    m_pResponse->GetData(pData, uiDataSize);
    // Flush unrecognized data
    if (uiDataSize)
    {
        m_pResponse->FreeData();
    }
    // Flush any data not yet processed
    if (m_pResponse->Data())
    {
        m_pResponse->Flush();
    }
}

//
// Block the read thread on demand, then flush the channel upon argument level
// Flush level 0 : No flush
// Flush level 1 : Flush the channel only
// Flush level 2 : Flush the TTY port only
// Flush level 3 : Flush both channel and TTY
// Block level 0 : No block
// Block level 1 : Block read thread
// Block level 2 : Active TTY control flow
// Block level 3 : Block both read thread and TTY
//
BOOL CChannel::BlockAndFlushChannel(BLOCK_CHANNEL blockLevel, FLUSH_CHANNEL flushLevel)
{
    int fd, ret, flags;
    BOOL bRet = RRIL_RESULT_ERROR;

    fd = this->GetFD();

    if (BLOCK_CHANNEL_NO_BLOCK != blockLevel)
    {
        if ((BLOCK_CHANNEL_BLOCK_ALL == blockLevel) || (BLOCK_CHANNEL_BLOCK_THREAD == blockLevel))
        {
            RIL_LOG_INFO("BlockAndFlushChannel() - ***** Blocking read thread on "
                    "channel=[%d] *****\r\n",
                    this->GetRilChannel());
            this->BlockReadThread();
        }


        if ((BLOCK_CHANNEL_BLOCK_ALL == blockLevel) || (BLOCK_CHANNEL_BLOCK_TTY == blockLevel))
        {
            // Blocking TTY flow.
            // Third security level in order to avoid IP data in response buffer.
            // Not mandatory.
            if (fd >= 0)
            {
                RIL_LOG_INFO("BlockAndFlushChannel() - ***** Activating flow control on"
                        " channel=[%d] *****\r\n",
                        this->GetRilChannel());
                ret = ioctl(fd, TIOCMGET, &flags);       // Get current flags
                flags &= ~TIOCM_RTS;
                if (ret < 0)
                {
                    RIL_LOG_CRITICAL("BlockAndFlushChannel() - Could not get flags on"
                            " Channel chnl=[%d] fd=[%d]  %s\r\n",
                            this->GetRilChannel(), fd, strerror(errno));
                    goto Error;
                }

                ret = ioctl(fd, TIOCMSET, &flags);       // Set flags
                if (ret < 0)
                {
                    RIL_LOG_CRITICAL("BlockAndFlushChannel() - Could not set flow control on"
                            " Channel chnl=[%d] fd=[%d]  %s\r\n",
                            this->GetRilChannel(), fd, strerror(errno));
                    goto Error;
                }
            }
            else
            {
                //  No FD.
                RIL_LOG_CRITICAL("BlockAndFlushChannel() - Could not unset flow control on"
                        " Channel chnl=[%d] fd=[%d].\r\n",
                        this->GetRilChannel(), fd);
                goto Error;
            }
        }
    }

    if (FLUSH_CHANNEL_NO_FLUSH != flushLevel)
    {
        if ((FLUSH_CHANNEL_FLUSH_TTY == flushLevel) || (FLUSH_CHANNEL_FLUSH_ALL == flushLevel))
        {
            if (fd >= 0)
            {
                // Flush TTY buffer.
                // Third security level in order to avoid IP data in response buffer.
                // Not mandatory.
                RIL_LOG_INFO("BlockAndFlushChannel() - ***** Flushing TTY on"
                        " channel=[%d] *****\r\n", this->GetRilChannel());

                // Flushing the tty IN and OUT.
                // No need to re-flush the channel once we blocked reading.
                ret = ioctl(fd, TCFLSH, 2);
                if (ret < 0)
                {
                    RIL_LOG_CRITICAL("BlockAndFlushChannel() - Unable to flush the tty"
                            " chnl=[%d] fd=[%d]  %s\r\n",
                            this->GetRilChannel(), fd, strerror(errno));

                    RIL_LOG_INFO("BlockAndFlushChannel() - Exit bRet=[0]\r\n");
                    goto Error;
                }
            }
            else
            {
                //  No FD.
                RIL_LOG_CRITICAL("BlockAndFlushChannel() - Could not flush TTY on"
                        " Channel chnl=[%d] fd=[%d].\r\n", this->GetRilChannel(), fd);

                RIL_LOG_INFO("BlockAndFlushChannel() - Exit bRet=[0]\r\n");
                goto Error;
            }
        }

        if ((FLUSH_CHANNEL_FLUSH_BUFFER == flushLevel) || (FLUSH_CHANNEL_FLUSH_ALL == flushLevel))
        {
            // Flush the response buffer. First security level in order to avoid IP data in response buffer.
            // Mandatory.
            // We ishould be in a stable state before the flush.
            // Any data received by the RRIL during the DATA MODE should be trashed.
            // Once read thread was blocked until then, this may by useless.
            RIL_LOG_INFO("BlockAndFlushChannel() - ***** Flushing response buffer on"
                    " channel=[%d] *****\r\n", this->GetRilChannel());

            this->FlushResponse();
        }
    }

    bRet = RRIL_RESULT_OK;

Error:
    return bRet;
}

//
// Waiting for a response from modem indefinitely
//
void CChannel::WaitForResponse(CCommand*& rpCmd, CResponse*& pResponse)
{
        RIL_LOG_VERBOSE("CChannel::WaitForResponse : Enter\r\n");

        // Waiting indefinitely for a response
        do
        {
            // set flag for response thread
            SetCmdThreadBlockedOnRxQueue();

            // retrieve response from modem
            GetResponse(rpCmd, pResponse);

            // response received, clear flag
            ClearCmdThreadBlockedOnRxQueue();

            if (!pResponse)
            {
                break;
            }
        }
        while (pResponse->IsTimedOutFlag());

     RIL_LOG_VERBOSE("CChannel::WaitForResponse : Exit\r\n");
}

//
// Flush the channel upon argument level, and then unblock read thread
// Flush level 0 : no flush
// Flush level 1 : Flush the channel only
// Flush level 2 : Flush the tty port only
// Flush level 3 : Flush both channel and tty
// Unblock level 0 : No unblock
// Unblock level 1 : Unblock read thread
// Unblock level 2 : Deactive TTY control flow
// Unblock level 3 : Unblock both
//
BOOL CChannel::FlushAndUnblockChannel(UNBLOCK_CHANNEL unblockLevel, FLUSH_CHANNEL flushLevel)
{
    int fd, ret, flags;
    BOOL bRet = RRIL_RESULT_ERROR;

    fd = this->GetFD();

    if (FLUSH_CHANNEL_NO_FLUSH != flushLevel)
    {
        if ((FLUSH_CHANNEL_FLUSH_ALL == flushLevel) || (FLUSH_CHANNEL_FLUSH_TTY == flushLevel))
        {
            if (fd >= 0)
            {
                // Flush TTY buffer.
                // Third security level in order to avoid IP data in response buffer.
                // Not mandatory.
                RIL_LOG_INFO("FlushAndUnblockChannel() - ***** Flushing TTY on"
                        " channel=[%d] *****\r\n", this->GetRilChannel());

                // Flushing the tty IN and OUT.
                // No need to re-flush the channel once we blocked reading.
                ret = ioctl(fd, TCFLSH, 2);
                if (ret < 0)
                {
                    RIL_LOG_CRITICAL("FlushAndUnblockChannel() - Unable to flush the tty"
                            " chnl=[%d] fd=[%d]  %s\r\n",
                            this->GetRilChannel(), fd, strerror(errno));
                    RIL_LOG_INFO("FlushAndUnblockChannel() - Exit bRet=[%u]\r\n", bRet);
                    goto Error;
                }
            }
            else
            {
                //  No FD.
                RIL_LOG_CRITICAL("FlushAndUnblockChannel() - Could not flush TTY on"
                        " Channel chnl=[%d] fd=[%d].\r\n",
                        this->GetRilChannel(), fd);
                RIL_LOG_INFO("FlushAndUnblockChannel() - Exit bRet=[%u]\r\n", bRet);
                goto Error;
            }
        }

        if ((FLUSH_CHANNEL_FLUSH_ALL == flushLevel) || (FLUSH_CHANNEL_FLUSH_BUFFER == flushLevel))
        {
            // Flush the response buffer. First security level in order to avoid IP data in response buffer.
            // Mandatory.
            // We are in a stable state.
            // Any data received by the RRIL during the DATA MODE should be trashed.
            // Once read thread was blocked until then, this may by useless.
            RIL_LOG_INFO("FlushAndUnblockChannel() - ***** Flushing response buffer on"
                    " channel=[%d] *****\r\n", this->GetRilChannel());
            this->FlushResponse();
        }
    }

    bRet = RRIL_RESULT_OK;

Error:
    if (UNBLOCK_CHANNEL_NO_UNBLOCK != unblockLevel)
    {
        if ((UNBLOCK_CHANNEL_UNBLOCK_ALL == unblockLevel)
                || (UNBLOCK_CHANNEL_UNBLOCK_TTY == unblockLevel))
        {
            // Unlocking TTY flow. Third security level in order to avoid IP data in response buffer.
            // Not mandatory.
            if (fd >= 0)
            {
                RIL_LOG_INFO("FlushAndUnblockChannel() - ***** Deactivating flow control on"
                        " channel=[%d] *****\r\n", this->GetRilChannel());
                ret = ioctl(fd, TIOCMGET, &flags);       // Get current flags
                flags |= TIOCM_RTS;
                if (ret < 0)
                {
                    RIL_LOG_CRITICAL("FlushAndUnblockChannel() - Could not get flags on"
                            " Channel chnl=[%d] fd=[%d]  %s\r\n",
                            this->GetRilChannel(), fd, strerror(errno));
                    goto Unblock;
                }

                ret = ioctl(fd, TIOCMSET, &flags);       // Set flags
                if (ret < 0)
                {
                    RIL_LOG_CRITICAL("FlushAndUnblockChannel() - Could not unset flag on"
                            " Channel chnl=[%d] fd=[%d]  %s\r\n",
                            this->GetRilChannel(), fd, strerror(errno));
                    goto Unblock;
                }
            }
            else
            {
                //  No FD.
                RIL_LOG_CRITICAL("FlushAndUnblockChannel() - Could not unset flow control on"
                        " Channel chnl=[%d] fd=[%d].\r\n", this->GetRilChannel(), fd);
                goto Unblock;
            }
        }

Unblock:
        if ((UNBLOCK_CHANNEL_UNBLOCK_ALL == unblockLevel)
                || (UNBLOCK_CHANNEL_UNBLOCK_THREAD == unblockLevel))
        {
            // Unblock the read thread
            this->UnblockReadThread();
            RIL_LOG_INFO("FlushAndUnblockChannel() - ***** Unblocking read thread on"
                    " channel=[%d] *****\r\n", this->GetRilChannel());
        }
    }

    return bRet;
}
