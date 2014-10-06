#include "server.h"  // мой заголовочныйный файл
#include "xml.h"



int fd_controller = 0;


// Отправка данных в UART контроллера

void sendController(string aOutData){
//    cout << "Отправляем в контроллер: " << aOutData << endl;

    aOutData = aOutData + '\n';

    write(fd_controller, aOutData.c_str(), aOutData.length());
}





// Обработка данных принятых из UART контроллера


bool getControllerPacket(string &aPacketRef, string &aComPortDataRef){

    static string saveBuf(""); // буфер для хранения склеенных или неполных пакетов


    int posStartTag = 0;
    int posEndTag = 0;

    if (aComPortDataRef != ""){
        saveBuf += aComPortDataRef; // помещаем помещаем принятые данные в буфер хранения
        aComPortDataRef = "";
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



// обработчик потока чтения из порта контроллера

void *handlerControllerReadPortThread(void * arg){


    // открываем порт контроллера
    fd_controller = openUartPort(xmlGetString(config, "controllerPort"));

    while (true){

        char comDataBuf[16] = {'\0'}; // буфер для данных из ком-порта

        int bytesRead = read(fd_controller, comDataBuf,sizeof(comDataBuf)); // читаем из ком-порта
        if(bytesRead > 0){

            //cout << "Receive from controller: " << comDataBuf << endl;

            string comPortData(comDataBuf);        // строка с данными из ком порта
            string & comPortDataRef = comPortData; // ссылка на строку с данными из ком порта
            string comPortPacket = ""; // пакет с данными из ком порта
            string & comPortPacketRef = comPortPacket; // ссылка на пакет с данными из ком порта

            while (getControllerPacket(comPortPacketRef, comPortDataRef)){ // выполняем разбор данных, пока не получим false

                // cout << "Пакет из UART контроллера: " << comPortPacket << endl;

                string id = xmlGetString(comPortPacket, "id"); // получаем идентификатор принятых данных

                if (id == "externalBatteryVoltage"){
                    sendAllClients(comPortPacket); // пересылаем всем подключенным клиентам
                }



            }
        }
    }

    return 0;

}



// Создание потока чтения из порта контроллера

bool startControllerReadPortThread() {
    pthread_t controllerReadPortThread;
    pthread_attr_t attr;
    pthread_attr_init(&attr);

    pthread_create(&controllerReadPortThread, &attr, handlerControllerReadPortThread, NULL);
    return true;
}
