/* ModemSimulator.h
 **
 ** Copyright 2011 Intel Corporation
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
 */
#pragma once

#include "EventListener.h"
#include <string>
#include "stmd.h"

using namespace std;

class CEventThread;
class CATParser;

class CModemSimulator : public IEventListener
{
    enum FileDesc {
        EFromModem,
        EToModem,
        EStmd,

        ENbFileDesc
    };

public:
    enum AnswerBehavior {
        ERegularBehavior,
        ENoAnswerBehavior,
        EAnswerErrorBehavior,
        EFloodingBehavior,
        EStallingBehavior,
        EIncompleteAnswerBehavior,
        EGoDown,
        EColdReset,

        EMaxAnswerBehavior
    };

    CModemSimulator();
    ~CModemSimulator();

    // Start
    bool start(const char* pcModemTty);

    // Stop
    void stop();

    // Status control
    bool setStatus(ModemStatus eStatus);

    // Status
    ModemStatus getStatus() const;

    // Set error behavior
    void setAnswerBehavior(AnswerBehavior eErrorBehavior);

    // Unsollicited
    void issueUnsolicited(bool bIssueUnsollicited);

private:
    // Event processing
    virtual bool onEvent(int iFd);
    virtual bool onError(int iFd);
    virtual bool onHangup(int iFd);
    virtual void onTimeout();
    virtual void onPollError();
    virtual void onProcess();
    // Read received command
    void readCommand();
    // Acknowledge
    void acknowledge(const string& strCommand);
    // Send unsolicited
    void sendUnsolicited(const string& strAnswer);
    // Send answer
    bool sendAnswer(const string& strAnswer);
    // Send string
    bool sendString(const string& strString);

    // State attributes
    bool _bStarted;
    ModemStatus _eStatus;
    AnswerBehavior _eAnswerBehavior;
    bool _bIssueUnsolicited;

    // Devices and Fds
    string _strStmdDeviceName;

    // Thread
    CEventThread* _pEventThread;
    // AT Parser
    CATParser* _pATParser;
};

