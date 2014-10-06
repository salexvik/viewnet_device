#include "server.h"  // мой заголовочныйный файл


int openTcpPort(string aPort)
{

    struct sockaddr_in addr;    // структура сокета
    int optval;
    size_t optlen;

    fd_listener = socket(AF_INET, SOCK_STREAM, 0);
    if(fd_listener < 0)
    {

        perror("socket");
        exit(0);
    }

    fcntl(fd_listener, F_SETFL, O_NONBLOCK);

    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(aPort.c_str()));
    addr.sin_addr.s_addr = INADDR_ANY;

    optval = 1;
    optlen = sizeof(int);
    setsockopt(fd_listener, SOL_SOCKET, SO_REUSEADDR, &optval, optlen);
    if (bind(fd_listener, (struct sockaddr *)&addr, sizeof(addr)) < 0) // пробуем открыть порт
    {
        perror("bind");
        exit(0);
    }

    listen(fd_listener, 2);  // начинаем слушать порт
    cout << "Open TCP port: " << aPort << endl;
    return 1;
}




// Отправка данных клиенту

void sendClient(string  aOutData)
{    

    if (fd_currentClient <= 0) return; // выходим, если нет клиента

    string pack;
    int packSize = 0;


    pack = START_TAG + aOutData + END_TAG;
    packSize = pack.size();


    int n = send(fd_currentClient, pack.c_str(), packSize, 0); // отправляем в сокет
    if(n <= 0){
        cout << "Ошибка при отправке данных. Закрываю клиентский сокет: fd = " << fd_currentClient << endl;
        close(fd_currentClient); // закрываем сокет в случае ошибки передачи данных
        fd_currentClient = -1;
        fd_clients.erase(fd_currentClient); // удаляем закрытый сокет из списка
    }
}


// Отправка данных всем клиентам

void sendAllClients(string  aOutData)
{
    string pack;
    int packSize = 0;


    pack = START_TAG + aOutData + END_TAG;
    packSize = pack.size();


    // проходим в цикле по всем клиентским сокетам
    for(set<int>::iterator it = fd_clients.begin(); it != fd_clients.end(); it++)
    {
        int n = send(*it,pack.c_str(), packSize, 0); // отправляем в сокет
        if(n <= 0){
            cout << "Ошибка при отправке данных. Закрываю клиентский сокет: fd = " << *it << endl;
            close(*it); // закрываем сокет в случае ошибки передачи данных
            fd_clients.erase(*it); // удаляем закрытый сокет из списка
        }
    }
}




// Обработка данных принятых из сокета


bool getSocketPacket(string &aPacketRef, string &aSocketDataRef){

    static string saveBuf(""); // буфер для хранения склеенных или неполных пакетов
    int posStartTag = 0;
    int posEndTag = 0;


    // обработка вновь принятых данных...

    if (aSocketDataRef != ""){ // если это первая итерация в цикле вызова функции...
        saveBuf += aSocketDataRef; // помещаем принятые данные в буфер хранения
        aSocketDataRef = "";

    }

    // обработка данных в буфере хранения...
    posStartTag = saveBuf.find(START_TAG);
    posEndTag = saveBuf.find(END_TAG);
    // если обнаружен стартовый и завершающий тэг...
    if(posStartTag != -1 && posEndTag != -1){
        if (posStartTag < posEndTag){
            aPacketRef = saveBuf.substr(posStartTag + START_TAG_SIZE, posEndTag - (posStartTag + START_TAG_SIZE)); // извлекаем пакет
            saveBuf.erase(0, posEndTag + END_TAG_SIZE); // удаляем извлеченный пакет и исе данные перед ним
            return true;
        }
        else saveBuf.erase(0, posEndTag + END_TAG_SIZE); // удаляем данные с нарушенной структурой
    }
    return false;

}







