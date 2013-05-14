#include "ATCommand.h"
#include "ATManager.h"
#include <iostream>
#include <semaphore.h>

using namespace std;

// Singleton access
CATManager* getInstance()
{
    static CATManager amcInstance;
    return &amcInstance;
}

int
main(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    // Send AT command
    CATCommand atCommand("AT");

    cout << "Starting AMC..." << endl;

    AT_CMD_STATUS eStatus = getInstance()->start("/tmp/tty1", 1);

    cout << "Result: " << eStatus << endl;

    if (eStatus != AT_CMD_OK) {

        return -1;
    }
    cout << "Sending AT " << atCommand.getCommand() << "..." << endl;

    eStatus = getInstance()->sendCommand(&atCommand, true);

    cout << "Result: " << eStatus << ", response: " << atCommand.getAnswer() << endl;


    //////////////////////////////

    CATCommand atXdrvCommand("AT+XDRV 1,2,3,4", "AT+XDRV");

    cout << "Sending AT " << atXdrvCommand.getCommand() << "..." << endl;

    eStatus = getInstance()->sendCommand(&atXdrvCommand, true);

    cout << "Result: " << eStatus << ", response: " << atXdrvCommand.getAnswer() << endl;

    //////////////////////////////

    CATCommand atXdrvCommand2("AT+XDRV 0,2,3,4", "AT+XDRV");

    cout << "Sending AT " << atXdrvCommand2.getCommand() << "..." << endl;

    eStatus = getInstance()->sendCommand(&atXdrvCommand2, true);

    cout << "Result: " << eStatus << ", response: " << atXdrvCommand2.getAnswer() << endl;

    // Block here
    sem_t sem;

    sem_init(&sem, false, 0);

    sem_wait(&sem);

    sem_destroy(&sem);

    return 0;
}
