#include "server.h"  // мой заголовочныйный файл


        // функция открытия порта UART

int openUartPort(string portName)
{
    int fd_uart; // дескриптор ком порта

    struct termios options;  // структура опций компорта

    // открытие порта

    fd_uart = open(portName.c_str(), O_RDWR | O_NOCTTY);
    if(fd_uart == -1)
    {
        perror(portName.c_str());
        return 0;
    }
    else
    {
        tcgetattr(fd_uart, &options);  // получение текущих опций
        options.c_cflag = B9600 | CS8 | CLOCAL | CREAD;
        options.c_iflag = IGNPAR;
        options.c_oflag = 0;
        options.c_lflag = 0;
        options.c_cc[VMIN] = 5;      /* block untill n bytes are received */
        options.c_cc[VTIME] = 0;     /* block untill a timer expires (n * 100 mSec.) */
        tcsetattr(fd_uart, TCSANOW, &options); // установка опций
    }
    // cout << portName << " открыт, fd = " << fd_uart << endl;
    return fd_uart;
}




