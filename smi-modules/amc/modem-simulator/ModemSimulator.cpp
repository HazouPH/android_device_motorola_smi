#include "ModemSimulator.h"
#include "EventThread.h"
#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include "stmd.h"
#include <string.h>
#include <errno.h>
#include "ATParser.h"
#include <assert.h>
#include "TtyHandler.h"

static const char* gpcAnswerBehavior[CModemSimulator::EMaxAnswerBehavior] = {
    "RegularBehavior",
    "NoAnswerBehavior",
    "AnswerErrorBehavior",
    "FloodingBehavior",
    "StallingBehavior",
    "IncompleteAnswerBehavior",
    "GoDown",
    "EColdReset"
};

static const char* gpcModemStatus[] = {
    "MODEM_DOWN",
    "MODEM_UP",
    "MODEM_COLD_RESET"
};

CModemSimulator::CModemSimulator()
    : _bStarted(false), _eStatus(MODEM_DOWN), _eAnswerBehavior(ERegularBehavior), _bIssueUnsolicited(false),
      _pEventThread(new CEventThread(this)),
      _pATParser(new CATParser)
{
    // Simulated implementation relies on named pipe
    char acDeviceName[256];

    // Build pipe name
    strcpy(acDeviceName, "/tmp/");
    strcat(acDeviceName, SOCKET_NAME_MODEM_STATUS);

    // Make FIFO
    mkfifo(acDeviceName, 0777);

    // Record device name
    _strStmdDeviceName = acDeviceName;
}

CModemSimulator::~CModemSimulator()
{
    // Stop
    stop();

    // Thread
    delete _pEventThread;

    // AT parser
    delete _pATParser;

    // Remove FIFO
    unlink(_strStmdDeviceName.c_str());
}

// Start
bool CModemSimulator::start(const char* pcModemTty)
{
    // Open FDs
    int iFd;

    /// EFromModem
    iFd = CTtyHandler::openTty(pcModemTty, O_WRONLY|CLOCAL);
    if (iFd < 0) {

        cerr << "Unable to open modem device for writing" << endl;

        stop();

        return false;
    }
    // Add & Listen
    _pEventThread->addOpenedFd(EFromModem, iFd);

    /// EToModem
    iFd = CTtyHandler::openTty(pcModemTty, O_RDONLY|CLOCAL);
    if (iFd < 0) {

        cerr << "Unable to open modem device for reading" << endl;

        stop();

        return false;
    }
    // Add & Listen
    _pEventThread->addOpenedFd(EToModem, iFd, true);

    /// EStmd
    iFd = open(_strStmdDeviceName.c_str(), O_RDWR);
    if (iFd < 0) {

        cerr << "Unable to open modem status pipe for writing" << endl;

        stop();

        return false;
    }
    // Add & Listen
    _pEventThread->addOpenedFd(EStmd, iFd);

    // Push first status
    if (!setStatus(MODEM_UP)) {

        stop();

        return false;
    }

    // Start thread
    if (!_pEventThread->start()) {

        cerr << "Failed to create event thread" << endl;

        stop();

        return false;
    }

    cout << "Success!" << endl;

    // State
    _bStarted = true;

    return true;
}

// Stop
void CModemSimulator::stop()
{
    // Stop Thread
    _pEventThread->stop();

    // Close descriptors
    _pEventThread->closeAndRemoveFd(EStmd);
    _pEventThread->closeAndRemoveFd(EToModem);
    _pEventThread->closeAndRemoveFd(EFromModem);

    // AT parser
    _pATParser->clear();

    // Record state
    _bStarted = false;
}

// Unsollicited
void CModemSimulator::issueUnsolicited(bool bIssueUnsollicited)
{
    _bIssueUnsolicited = bIssueUnsollicited;
}

// Status control
bool CModemSimulator::setStatus(ModemStatus eStatus)
{
    if (eStatus != _eStatus) {

        // Store
        _eStatus = eStatus;

        // Send status change
        uint32_t uiStatus = (uint32_t)_eStatus;

        if (write(_pEventThread->getFd(EStmd), &uiStatus, sizeof(uiStatus)) != sizeof(uiStatus)) {

            cerr << "Unable to send modem status" << endl;

            return false;
        }
    }
    cout << "Modem status: going " << gpcModemStatus[_eStatus] << endl;

    return true;
}

// Status
ModemStatus CModemSimulator::getStatus() const
{
    return _eStatus;
}

// Set error behavior
void CModemSimulator::setAnswerBehavior(CModemSimulator::AnswerBehavior eErrorBehavior)
{
    assert(eErrorBehavior < EMaxAnswerBehavior);

    _eAnswerBehavior = eErrorBehavior;

    cout << "Answer behavior: " << gpcAnswerBehavior[_eAnswerBehavior] << endl;
}

// AT channel
bool CModemSimulator::onEvent(int iFd)
{
    if (iFd == _pEventThread->getFd(EToModem)) {

        // Received AT command
        readCommand();
    }
    return false;
}

bool CModemSimulator::onError(int iFd)
{
    (void)iFd;
    // Should not happen in this context
    return false;
}

bool CModemSimulator::onHangup(int iFd)
{
    (void)iFd;
    // Should not happen in this context
    return false;
}


void CModemSimulator::onTimeout()
{
    cerr << "Timeout!" << endl;
}

void CModemSimulator::onPollError()
{
    cerr << "Poll error!" << endl;
}

void CModemSimulator::onProcess()
{

}

// Read received command
void CModemSimulator::readCommand()
{
    if (_pATParser->receive(_pEventThread->getFd(EToModem))) {

        string strCommand;

        // Get command
        while(_pATParser->extractReceivedSentence(strCommand)) {

            // log
            cout << "Received command: " << strCommand << endl;

            if (!_eStatus) {

                // Nothing to do
                return;
            }
            // Unsollicited
            sendUnsolicited("Oh boy...");

            // Is alive?
            if (strCommand == "AT") {

                sendAnswer("OK");
            } else {
                // Regular command
                switch(_eAnswerBehavior) {
                case ENoAnswerBehavior:
                    break;
                case EAnswerErrorBehavior:
                    // Send ERROR
                    sendAnswer("ERROR");
                    break;
                case ERegularBehavior:
                case EFloodingBehavior:
                case EStallingBehavior:
                case EIncompleteAnswerBehavior:
                    acknowledge(strCommand);
                    break;
                case EGoDown:
                    sendString("\r\n");
                    // Echo command and go down
                    sendAnswer(strCommand);

                    // Go down
                    setStatus(MODEM_DOWN);

                    // Sleep a little
                    usleep(100000);

                    // Go up again
                    setStatus(MODEM_UP);
                    break;
                case EColdReset:
                    sendString("\r\n");
                    // Echo command and cold reset
                    sendAnswer(strCommand);

                    // Go down
                    setStatus(MODEM_DOWN);

                    // Sleep a little
                    usleep(100000);

                    // Go down
                    setStatus(MODEM_COLD_RESET);

                    // Sleep a little
                    usleep(100000);

                    // Go up again
                    setStatus(MODEM_UP);
                    break;
                default:
                    assert(0);
                }
            }
            // Unsollicited
            sendUnsolicited("That's amazing!");
        }
    }
}

// Acknowledge
void CModemSimulator::acknowledge(const string& strCommand)
{
    // Send some control chars
    sendString("\r\n");

    // Echo command
    if ((_eAnswerBehavior == EStallingBehavior || _eAnswerBehavior == EIncompleteAnswerBehavior) && (strCommand.length() > 1)) {

        uint32_t uiCut = strCommand.length() / 2;

        // Send first part
        sendString(strCommand.substr(0, uiCut));

        // Bail out here for incmplete answer
        if (_eAnswerBehavior == EIncompleteAnswerBehavior) {

            return;
        }
        // Sleep 100 ms
        usleep(100000);

        // Send second part
        sendString(strCommand.substr(uiCut, strCommand.length() - uiCut));

        // Terminate
        sendString("\r\n");
    } else {
        // Regular
        // Echo command
        sendAnswer(strCommand);
    }
    // Unsollicited
    sendUnsolicited("Do you believe that?");

    // Flood
    if (_eAnswerBehavior == EFloodingBehavior) {
        cout << "Flooding..." << endl;
        uint32_t uiNbSends = 1000;

        while (uiNbSends--) {

            sendString("AA");
        }
        sendString("\r\n");
    }
    // Send OK
    sendAnswer("OK");

    // Flood
    if (_eAnswerBehavior == EFloodingBehavior) {
        cout << "Flooding..." << endl;
        uint32_t uiNbSends = 1000;

        while (uiNbSends--) {

            sendString("BB");
        }
        sendString("\r\n");
    }
}

// Send unsolicited
void CModemSimulator::sendUnsolicited(const string& strAnswer)
{
    // Issue only zhen relevant
    if (_bIssueUnsolicited && _eAnswerBehavior != EIncompleteAnswerBehavior) {

        sendAnswer(strAnswer);
    }
}

// Send answer
bool CModemSimulator::sendAnswer(const string& strAnswer)
{
    cout << "Sending: " << strAnswer << endl;
    // Send
    return sendString(strAnswer) && sendString("\r\n");
}

// Send string
bool CModemSimulator::sendString(const string& strString)
{
    // Send
    int iNbWrittenChars = write(_pEventThread->getFd(EFromModem), strString.c_str(), strString.length());

    // Check for success
    if (iNbWrittenChars != (int)strString.length()) {

        // log
        cerr << "Unable to send full answer: " << iNbWrittenChars << endl;

        return false;
    }
    // Flush
    fsync(_pEventThread->getFd(EFromModem));

    return true;
}
