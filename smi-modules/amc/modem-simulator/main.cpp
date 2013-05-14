#include <iostream>
#include "ModemSimulator.h"
#include <semaphore.h>
#include <stdlib.h>
#include <stdint.h>

using namespace std;

const char* gpcModemTty = "/tmp/tty2";

// To start from a shell
// socat -v -x PTY,link=/tmp/tty1 PTY,link=/tmp/tty2

int main(int argc, char *argv[])
{
    CModemSimulator modemSimulator;
    uint32_t uiAnswerBehavior;

    if (argc < 2) {

        cerr << "Missing argument: Answer behavior" << endl;

        goto failed;
    }

    // Get desired answer behavior
    uiAnswerBehavior = strtoul(argv[1], NULL, 0);

    if (uiAnswerBehavior >= CModemSimulator::EMaxAnswerBehavior) {

        cerr << "Invalid argument: Answer behavior" << endl;

        goto failed;
    }
    // Turn on unsollicited
    //modemSimulator.issueUnsolicited(true);

    // Start
    if (!modemSimulator.start(gpcModemTty)) {

        goto failed;
    }

    modemSimulator.setAnswerBehavior((CModemSimulator::AnswerBehavior)uiAnswerBehavior);

    //modemSimulator.setStatus(false);

    // Block here
    sem_t sem;

    sem_init(&sem, false, 0);

    sem_wait(&sem);

    sem_destroy(&sem);

failed:
    modemSimulator.stop();

    return 0;
}
