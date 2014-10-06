#include "server.h"  // мой заголовочныйный файл
#include "xml.h"


pthread_mutex_t mutex;


string networkName; // имя сети сотовой связи
string signalLevel; // уровень сигнала
string simBalance; // баланс SIM-карты устройства
string simNumber; // номер SIM-карты



// Инициализация мьютекса

void modemMutexInit(){
    pthread_mutex_init(&mutex, NULL);
}



// отправка информации сим карты

void sendSimInfo(){
    sendClient(xmlStringToTag("id", "responseSimInfo")
               +xmlStringToTag("networkName", networkName)
               +xmlStringToTag("signalLevel", signalLevel)
               +xmlStringToTag("simBalance", simBalance)
               +xmlStringToTag("simNumber", simNumber));
}




// Выполнение AT команды

string executeAtCommand(string aCommandLine, int fd_modemCommandPort){

    fd_set readfds;
    struct timeval tv;
    int selectResult;
    string outString;


    write(fd_modemCommandPort, aCommandLine.c_str(), aCommandLine.length()); // отправляем команду

    // настраиваем select
    FD_ZERO (&readfds);
    FD_SET (fd_modemCommandPort, &readfds);
    tv.tv_sec = 5;
    tv.tv_usec = 0;

    while((selectResult = select(fd_modemCommandPort+1, &readfds, 0, 0, &tv)) > 0){ // ждем появление данных

        if (FD_ISSET (fd_modemCommandPort, &readfds)) {  // если получены данные, читаем их

            char outBuf[1024] = {'\0'}; // буфер для данных из порта uart
            int bytesRead = read(fd_modemCommandPort, outBuf, sizeof(outBuf)); // читаем из uart
            if(bytesRead > 0)
            {
                //cout << "Принято из порта модема: " << outBuf << endl;

                outString += (string)outBuf; // записываем принятый буфер в строку

                // если получили сообщение об ощибке...
                if (outString.find("ERROR") != string::npos){
                    cout << "Ошибка команды\n";
                    return "";
                }

                if (outString.find("OK") != string::npos || outString.find('>') != string::npos){
                    return outString;
                }
            }
        }
    }
    if (selectResult == 0) cout << "Закончился таймаут ожидания результата выполнения команды\n";
    if (selectResult == -1) cout << "Ошибка вызова select\n";
    return "";
}


// выполнение USSD запроса

string ussdRequest(string aUssdCommand, int fd_modemCommandPort, bool aNewLine){

    // cout << "USSD запрос...\n";

    fd_set readfds;
    struct timeval tv;
    int selectResult;
    string outString;
    size_t posStartId = string::npos;
    size_t posEndId= string::npos;

    // кодируем USSD запрос в 7-ми битную кодировку
    string encodeUssdCommand = getPopenString(xmlGetString(config, "modemUtilsPath") + " encodeUssd " + aUssdCommand, false);

    //cout << encodeUssdCommand << endl;

    string commandLine = "AT+CUSD=1," + encodeUssdCommand + ",15\r\n";

    write(fd_modemCommandPort, commandLine.c_str(), commandLine.length()); // отправляем команду

    // настраиваем select
    FD_ZERO (&readfds);
    FD_SET (fd_modemCommandPort, &readfds);
    tv.tv_sec = 10;
    tv.tv_usec = 0;

    while((selectResult = select(fd_modemCommandPort+1, &readfds, 0, 0, &tv)) > 0){ // ждем появление данных

        if (FD_ISSET (fd_modemCommandPort, &readfds)) {  // если получены данные, читаем их

            char outBuf[1024] = {'\0'}; // буфер для данных из порта uart
            int bytesRead = read(fd_modemCommandPort, outBuf, sizeof(outBuf)); // читаем из uart
            if(bytesRead > 0)
            {

                //cout << "Принято из порта модема: " << outBuf << endl;


                outString += (string)outBuf; // записываем принятый буфер в строку


                // если получили сообщение об ощибке...
                if (outString.find("ERROR") != string::npos){
                    cout << "Ошибка команды\n";
                    return "";
                }

                posStartId = outString.find(",\"");
                if (posStartId != string::npos){
                    posEndId = outString.find("\",", posStartId);
                    if (posStartId != string::npos && posEndId != string::npos){
                        string result = outString.substr(posStartId + 2, (posEndId - posStartId) - 2);

                        //cout << "результат: " << result << endl;

                        // удаляем управляющие символы
                        size_t DLE;
                        while((DLE = result.find('\u0010')) != string::npos){
                            result.erase(DLE, 1);
                        }

                        // декодируем результат
                        return getPopenString(xmlGetString(config, "modemUtilsPath") + " decodeUssd " + result, aNewLine);
                    }
                }
            }
        }
    }
    if (selectResult == 0) cout << "Закончился таймаут ожидания результата выполнения команды\n";
    if (selectResult == -1) cout << "Ошибка вызова select\n";
    return "";
}





void getNetworkName(){

    int fd_modemCommandPort = openUartPort(xmlGetString(config, "modemCommandPort"));
    string out = executeAtCommand("AT+COPS?\r\n", fd_modemCommandPort);
    close(fd_modemCommandPort);

    if (out.find("+COPS: ") != string::npos){
        size_t posStartId = out.find(",\"");
        if (posStartId != string::npos){
            size_t posEndId = out.find("\",", posStartId);
            if (posStartId != string::npos && posEndId != string::npos){
                networkName = out.substr(posStartId + 2, (posEndId - posStartId) - 2);
            }
        }
    }
    else networkName = "";
    //cout << "Имя сети: " << networkName<< endl;
}



void getSignalLevel(){

    int fd_modemCommandPort = openUartPort(xmlGetString(config, "modemCommandPort"));
    string out = executeAtCommand("AT+CSQ\r\n", fd_modemCommandPort);
    close(fd_modemCommandPort);


    size_t posStartId = out.find("+CSQ: ");
    if (posStartId != string::npos){
        size_t posEndId = out.find(",", posStartId);
        if (posStartId != string::npos && posEndId != string::npos){
            signalLevel = out.substr(posStartId + 6, (posEndId - posStartId) - 6);
        }
    }
    else signalLevel = "";
    //cout << "Имя сети: " << signalLevel<< endl;
}



void getBalance(){

    int fd_modemCommandPort = openUartPort(xmlGetString(config, "modemCommandPort"));

    if (networkName == "MegaFon"){

        string result = ussdRequest("*100#", fd_modemCommandPort, true);

        size_t pos; // ищем символ новой строки
        if ((pos = result.find("\n")) != string::npos) result = result.substr(0, pos);
        simBalance = result;
    }

    close(fd_modemCommandPort);
    //cout << "Баланс: " << simBalance << endl;
}




void getSimNumber(){


    int fd_modemCommandPort = openUartPort(xmlGetString(config, "modemCommandPort"));

    if (networkName == "MegaFon"){

        string result = ussdRequest("*205#", fd_modemCommandPort, false);

        size_t pos; // ищем символ "7"
        if ((pos = result.find('7')) != string::npos) result = result.substr(pos);

        simNumber = result;
    }

    close(fd_modemCommandPort);
    //cout << "Номер: " << simNumber << endl;
}



void getSms(){

    size_t posStartId, posEndId;

    int fd_modemCommandPort = openUartPort(xmlGetString(config, "modemCommandPort"));

    // устанавливаем формат передачи сообщения - PDU
    executeAtCommand("AT+CMGF=0\r\n", fd_modemCommandPort);

    int messageIndex = 1;
    string encodeMessages;

    // читаем и удаляем прочитанные сообщения, пока не достигнем максимального кол-ва сообщений
    // или число сообщений не станет равно 1
    while (messageIndex < 14){

        // получаем кол-во сообщений хранящихся на SIM-карте
        int messageCount = 0;
        string out = executeAtCommand("AT+CPMS=\"SM\"\r\n", fd_modemCommandPort);
        posStartId = out.find("+CPMS: ");
        if (posStartId != string::npos){
            posEndId = out.find(",", posStartId);
            if (posStartId != string::npos && posEndId != string::npos){
                messageCount =  atoi(out.substr(posStartId + 7, (posEndId - posStartId) - 7).c_str()) - 1;
            }
        }
        //cout << "Количество сообщений: " << messageCount << endl;

        // если число сообщеий стало равно 0, выходим из цикла
        if (messageCount == 0) break;


        //cout << "Индекс сообщения: " << messageIndex << endl;

        // читаем одно сообщение по его индексу
        string encodeMessage = executeAtCommand("AT+CMGR=" + intToString(messageIndex) + "\r\n", fd_modemCommandPort);

        // если нашли сообщение...
        posStartId = encodeMessage.find("+CMGR: ");
        if (posStartId != string::npos){

            encodeMessage = encodeMessage.substr(posStartId);
            encodeMessage = encodeMessage.substr(encodeMessage.find('\n') + 1);
            encodeMessage = encodeMessage.substr(0, encodeMessage.find('\n') + 1); // выделяем сообщение вместе с символом новой строки
            //cout << "Закодированное сообщение: " << encodeMessage << endl;
            encodeMessages += encodeMessage;

            // удаляем прочитанное сообщение
            executeAtCommand("AT+CMGD=" + intToString(messageIndex) + "\r\n", fd_modemCommandPort);
        }

        messageIndex++;
    }

    close(fd_modemCommandPort);


    //cout << "Принятые закодированные сообщения: " << encodeMessages << endl;

    // если есть принятые сообщения, декодируем их и парсим
    if (encodeMessages != ""){

        vector<string> decodeMessages = getPopenList(xmlGetString(config, "modemUtilsPath") + " decodeSms " + "\"" +  encodeMessages + "\"", false);


        // цикл по массиву принятых СМС сообщений
        for(vector<string>::const_iterator it = decodeMessages.begin(); it !=decodeMessages.end(); ++it){

            if (*it == "") continue;

            string decodeMessage = *it;


            cout << "Принятое СМС: " << decodeMessage << endl;

            // ищем результат запроса номера
            posStartId = decodeMessage.find("7");
            if (posStartId != string::npos){
                string balance = decodeMessage.substr(posStartId);
                cout << "Номер: " << balance << endl;
            }


        }
    }

}






// получение информации с СИМ карты

void getSimInfo(){

    pthread_mutex_lock(&mutex);

    //cout << "Старт потока получения ииформации SIM карты\n";

    static int callCount = 0; // счетчик вызова функции

    if (callCount == 0){

        // Выполняется только в начале периода:

        getNetworkName();
        getBalance();
        getSimNumber();
        getSignalLevel();
        getSms();
    }
    else{

        // Выполняется регулярно:

        getNetworkName();
        getSignalLevel();


        getSms();

        // Будет выполнено, если данные небыли получены:

        if (simBalance == "") getBalance();
        if (simNumber == "") getSimNumber();
    }


    callCount++;

    if (callCount == 100) callCount = 0;

    //sendSimInfo(); // отправляем полученные данные всем подключенным клиентам

    //cout << "Завершение потока получения ииформации SIM карты\n";

    pthread_mutex_unlock(&mutex);

    sleep(10);
}









// поток для отправки СМС

void *handleSendSmsThread(void *aMsg) {

    pthread_mutex_lock(&mutex);


    cout << "Старт потока отправки СМС\n";

    string smsMessage = (const char*)aMsg;


    // открываем порт модема
    int fd_modemCommandPort = openUartPort(xmlGetString(config, "modemCommandPort"));


    // получаем массив телефонов
    vector<string> phoneArray = xmlGetArrayTags(xmlGetString(config, "phones"), "phone");

    // цикл по массиву телефонов
    for(vector<string>::const_iterator it = phoneArray.begin(); it !=phoneArray.end(); ++it){

        // проверяем статус телефона...
        if (!xmlGetBool(*it, "phoneStatus")) continue; // пропускаем телефон, если он не активен


        // кодируем сообщение
        string popenLine = xmlGetString(config, "modemUtilsPath") + " encodeSms " + "\"" + smsMessage + "\"" + " " + xmlGetString(*it, "phoneNumber");
        vector<string> encodeSmsMessage = getPopenList(popenLine, false);

        // цикл по массиву строк с кусочками закодированного сообщения и их длины
        for(vector<string>::const_iterator it = encodeSmsMessage.begin(); it !=encodeSmsMessage.end(); ++it){

            if (*it == "") continue;

            string len = xmlGetString(*it, "len");
            string msg = xmlGetString(*it, "msg");

            // устанавливаем формат передачи сообщения - PDU
            executeAtCommand("AT+CMGF=0\r\n", fd_modemCommandPort);


            // подготавливаем модем - передаем ему длину отправляемой строки
            executeAtCommand("AT+CMGS=" + len + "\r\n", fd_modemCommandPort);

            // отправляем строку
            executeAtCommand(msg + "\32", fd_modemCommandPort);

            sleep(1);
        }
    }

    // закрываем порт модема
    close(fd_modemCommandPort);

    cout << "Завершение потока отправки СМС\n";

    pthread_mutex_unlock(&mutex);

    return 0;
}



// поток для выполнения USSD запроса

void *handleUssdRequestThread(void *aUssdCommand) {

    pthread_mutex_lock(&mutex);

    cout << "Старт потока выполнения USSD запроса\n";

    string ussdCommand = (const char*)aUssdCommand;

    // открываем порт модема
    int fd_modemCommandPort = openUartPort(xmlGetString(config, "modemCommandPort"));


    string result = ussdRequest(ussdCommand, fd_modemCommandPort, true);

    cout << "Результат USSD запроса: " << result << endl;

    sendClient(xmlStringToTag("id", "responseUssd")
               +xmlStringToTag("result", result));



    // закрываем порт модема
    close(fd_modemCommandPort);

    cout << "Завершение потока выполнения USSD запроса\n";

    pthread_mutex_unlock(&mutex);

    return 0;

}




// Создание потока выполняющего отправку СМС

bool startSendSmsThread(string aMsg) {

    static string msg;

    msg = aMsg;

    pthread_t thread;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    return !pthread_create(&thread, &attr, handleSendSmsThread, (void*)msg.c_str());
}




// Создание потока для выполнения USSD запроса

bool startUssdRequestThread(string aUssdCommand) {
    pthread_t thread;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    return !pthread_create(&thread, &attr, handleUssdRequestThread, (void*)aUssdCommand.c_str());
}









// Соединение с интернетом

void internetConnect(){

    // удаляем процесс wvdial
    system("sudo killall wvdial");

    sleep(1); // ждем завершения процесса

    string systemString = "sudo wvdial " + networkName;

    int wvdialPid = fork();   // создаем дочерний процесс для запуска wvdial

    if (wvdialPid == 0){   // если находимся в дочернем процессе...
        cout << "Запуск wvdial\n";
        system(systemString.c_str()); // запускаем wvdial
        cout << "Завершение wvdial\n";
        exit(0); // завершаем созданный процесс после выхода из wvdial

    }
}






















