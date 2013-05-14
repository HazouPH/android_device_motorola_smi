#include "TtyHandler.h"
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>


int CTtyHandler::openTty(const char* pcTty, int iFlags)
{

    int iFd = open(pcTty, iFlags);

    if (iFd < 0) {

        return -1;
    }
    struct termios iosconfig;

    // Get config
    if(tcgetattr(iFd, &iosconfig) < 0) {

        close(iFd);

        return -1;
    }

    // Local mode flags:
    iosconfig.c_cflag = B115200;

    // Control mode flags:
    iosconfig.c_cflag |= CS8 | CLOCAL | CREAD;

    // Input mode flags:
    iosconfig.c_iflag &= ~(INPCK | IGNPAR | PARMRK | ISTRIP | IXANY | ICRNL);

    // Output mode flags:
    iosconfig.c_oflag &= ~OPOST;

    // Special characters:
    iosconfig.c_cc[VMIN]  = 1;
    iosconfig.c_cc[VTIME] = 10;

    // set input mode (non-canonical, no echo,...)
    iosconfig.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);


    tcflush(iFd, TCIFLUSH);

    // Set config
    if(tcsetattr(iFd, TCSANOW, &iosconfig) < 0) {

        close(iFd);

        return -1;
    }

    return iFd;
}
