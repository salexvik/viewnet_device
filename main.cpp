#include "server.h"  // мой заголовочныйный файл
#include "xml.h"
#include "md5.h"

string pathConfig = "/home/pi/server.conf"; // путь к файлу конфигурации
// string pathConfig = "/home/salexvik/VirtualBox_Shared/qt-proj/server/debug/server.conf";
string config; // строка с конфигом сервера в формате XML
int fd_listener = 0;  // дескриптор слушающего сокета
set<int> fd_clients;  // массив дескрипторов клиентов
int fd_currentClient = -1; // дескриптор текущего клиента
map<string, int> recordPidMap; // массив для соответствия идентификатора строки настроек записи и PID процесса записи медиаданных
map<string, string> recordStatusMap; // массив статусов медиапотоков записи



// main

int main(int argc, char *argv[]){

    // проверка параметров запуска

    for (int i=1; i < argc; i++){
        if (strcmp(argv[i], "-d") == 0){
            freopen("outserver.txt","w",stdout);
            if (daemon(0, 1) < 0) cout << "Ошибка демонизации\n";
            continue;
        }
        cout << "Неправильный параметр запуска: " << argv[i] << endl;
        exit(-1);
    }

    cout << "Server running\n";

    // проверяем существования файла конфигурации
    if (!existFileConfig()) exit(-1);

    config = getConfig();  // получаем конфигурацию

    srand(time(0)); // автоматическая рандомизация

    // инициализируем мьютекс для работы с модемом
    modemMutexInit();

    // включаем автоматическую запись с камер, если настроена
    autoStartRecord();

    // открываем TCP порт
    openTcpPort(xmlGetString(config, "controlPort"));

    // установка обработчика сигнала завершения дочернего процесса
    signal(SIGCHLD, sig_chld);

    // запускаем поток для чтения из порта контроллера
    startControllerReadPortThread();

    // запускаем таймеры
    startTimerThreads();

    // запускаем поток чтения из канала RSTP сервера
    startRtspReadCannelThread();


    while(1) // основной цикл программы выполняется бесконечно
    {

        // Заполняем множество дескрипторов
        fd_set readset; // множество файловых дескрипторов для функции "select"
        FD_ZERO(&readset);
        FD_SET(fd_listener, &readset);  // заносим дескриптор слушающего сокета в множество
        for(set<int>::iterator it = fd_clients.begin(); it != fd_clients.end(); it++)
            FD_SET(*it, &readset);  // заносим дескрипторы клиентских сокетов в множество


        // определяем максимальный дескриптор
        int max1 = *max_element(fd_clients.begin(), fd_clients.end());
        int max2 = max(max1, fd_listener);

        // Ждём события в одном из дискрипторов

        if(select(max2+1, &readset, NULL, NULL, NULL) <= 0)
        {
            if(errno == EINTR) continue; // не реагируем на полученный сигнал
            perror("select");
        }

        // Определяем тип события и выполняем соответствующие действия...

        if(FD_ISSET(fd_listener, &readset))
        {
            // Поступил новый запрос на соединение, присоединяем клиента

            fd_currentClient = accept(fd_listener, NULL, NULL);
            if (fd_currentClient >= 0){
                cout << "Подключился клиент: fd = " << fd_currentClient << endl;

                fd_clients.insert(fd_currentClient); // добавляем нового клиента в множество
            }
            else perror("accept");
        }


        // проверяем наличие данных в клиентских сокетах

        for(set<int>::iterator it = fd_clients.begin(); it != fd_clients.end(); it++){
            if(FD_ISSET(*it, &readset)){
                fd_currentClient = *it; // запоминаем дескриптор текущего клиента

                char socketDataBuf[PACKSIZE] = {'\0'}; // буфер для данных из сокета

                // Поступили данные от клиента, читаем их...
                int bytesRead = recv(fd_currentClient, socketDataBuf, PACKSIZE-1, 0);

                if(bytesRead <= 0){  // если соединение разорвано...
                    // Соединение разорвано, удаляем сокет из множества
                    cout << "Ошибка при чтении принятых данных. Закрываю клиентский сокет: fd = " << fd_currentClient << endl;
                    close(fd_currentClient);
                    fd_clients.erase(fd_currentClient);
                    break; // выходим из цикла
                }


                string socketData(socketDataBuf);        // строка с данными из сокета
                string & socketDataRef = socketData; // ссылка на строку с данными из сокета
                string socketPacket = ""; // пакет с данными из сокета
                string & socketPacketRef = socketPacket; // ссылка на пакет с данными из сокета

                while (getSocketPacket(socketPacketRef, socketDataRef)){ // выполняем разбор данных, пока не получим false

                    string id = xmlGetString(socketPacket, "id"); // получаем идентификатор принятых данных

                    // запрос проверки связи

                    if (id == "checkConnect"){
                        //cout << id << endl;
                        sendAllClients(xmlStringToTag("id", "checkConnectSuccessfully")); // отправляем всем подключенным клиентам
                        continue;
                    }


                    // Проверка идентификатора устройства

                    if (xmlGetString(socketPacket, "idDevice") != xmlGetString(config, "idDevice")){
                        cout << "Не правильный ID устройства, разрываем соединение с клиентом\n";
                        close(fd_currentClient);
                        fd_clients.erase(fd_currentClient);
                        break; // выходим из цикла
                    }


                    // Проверка пароля

                    if (xmlGetString(socketPacket, "password") != xmlGetString(config, "password")){
                        cout << "Аутентификация не пройдена\n";
                        string deniedRequest = xmlDeleteTag(socketPacket, "idDevice");
                        deniedRequest = xmlDeleteTag(deniedRequest, "password");
                        sendClient(xmlStringToTag("id", "authenticationFailed")
                                   +xmlStringToTag("deniedRequest", deniedRequest));
                        continue;
                    }


                    // Инициализация клиента

                    if (id == "initializationData"){
                        cout << "Получили данные для инициализации клиента\n";
                        sendClient(xmlStringToTag("id", "initializationSuccessful"));
                        continue;
                    }



                    // Команда на изменение настроек медиапотока

                    if (id == "changeMediaStreams"){
                        string mediaStreamType = xmlGetString(socketPacket, "mediaStreamType");
                        string operationId = xmlGetString(socketPacket, "operationId");
                        string mediaStreamId = xmlGetString(socketPacket, "mediaStreamId");

                        if (operationId == "renameMediaStream"){
                            string newName = xmlGetString(socketPacket, "newName");
                            config = xmlSetStringById(config, "mediaStream", mediaStreamId, "mediaStreamName", newName);
                        }

                        if (operationId == "deleteMediaStream"){
                            config = xmlDeleteTagById(config, "mediaStream", mediaStreamId);
                        }

                        if (operationId == "replaceMediastream"){
                            string newMediaStream = xmlGetTag(socketPacket, "mediaStream");
                            config = xmlSetTagById(config, "mediaStream", mediaStreamId, newMediaStream);
                        }

                        if (operationId == "addMediaStream"){
                            string newMediaStream = xmlGetTag(socketPacket, "mediaStream");
                            config = xmlAddTag(config, mediaStreamType, newMediaStream);
                        }


                        if (saveConfig()){ // сохраняем конфигурацию медиапотоков
                            if (operationId == "renameMediaStream" || operationId == "deleteMediaStream"){
                                if (mediaStreamType == "view") sendMediaStreamsDataToView();    // отправляем данные медиапотоков для просмотра
                                if (mediaStreamType == "record") sendMediaStreamsDataToRecord();  // отправляем данные медиапотоков для записи
                            }
                        }
                        continue;

                    }




                    // Команда на изменение настроек телефонных номеров для SMS оповещения

                    if (id == "changePhones"){
                        changePhones(socketPacket);
                        continue;
                    }


                    // Запрос телефонов

                    if (id == "requestPhones"){

                        sendClient(xmlStringToTag("id", "responsePhones")
                                   + xmlGetTag(config, "phones"));
                        continue;
                    }





                    // Запрос данных для редактирования медиапотока

                    if (id == "requestDataForEditMediaStream"){
                        string mediaStreamType = xmlGetString(socketPacket, "mediaStreamType");
                        string mediaStreamId = xmlGetTag(socketPacket, "mediaStreamId");
                        sendClient(xmlStringToTag("id", "responseDataForEditMediaStream")
                                   +xmlStringToTag("mediaStreamType", mediaStreamType)
                                   +xmlGetTagById(xmlGetString(config, mediaStreamType), "mediaStream", mediaStreamId)
                                   +xmlStringToTag("videoDevices", getAliasesVideoDevice())
                                   +xmlStringToTag("audioDevices", getAliasesAudioDevice()));   // отправляяем клиенту
                        continue;
                    }



                    // Запрос данных для создания нового медиапотока

                    if (id == "requestDataForAddNewMediaStream"){
                        sendClient(xmlStringToTag("id", "responseDataForAddNewMediaStream")
                                   +xmlStringToTag("videoDevices", getAliasesVideoDevice())
                                   +xmlStringToTag("audioDevices", getAliasesAudioDevice()));   // отправляяем клиенту
                        continue;
                    }


                    // Запрос на список имен и id медиапотоков просмотра

                    if (id == "requestMediastreamsDataToView"){
                        sendMediaStreamsDataToView();    // отправляем данные медиапотоков для просмотра
                        continue;
                    }


                    // Запрос на список имен, id и статусов медиапотоков  записи

                    if (id == "requestMediastreamsDataToRecord"){
                        sendMediaStreamsDataToRecord();    // отправляем данные медиапотоков для записи
                        continue;
                    }


                    // команда на смену пароля

                    if (id == "changePassword"){

                        config = xmlSetString(config, "password", xmlGetString(socketPacket, "newPassword"));

                        if (config != ""){
                            // если пароль изменен
                            saveConfig();

                            // отправляем подтверждение смены пароля и новый пароль
                            sendClient(xmlStringToTag("id", "changePasswordSuccessfully")
                                       +xmlStringToTag("newPassword", xmlGetString(config, "password")));
                        }
                        else{
                            // отправляем сообщение об ошибке смены пароля
                            sendClient(xmlStringToTag("id", "changePasswordFailed"));
                        }
                        continue;
                    }


                    // команда на запуск RTSP сервера

                    if (id == "startRtsp"){

                        static string mediaStreams = "";
                        string mediaStreamId = xmlGetString(socketPacket, "mediaStreamId");
                        string checkedMediaStream;

                        if (mediaStreamId != "testMediaStreamId"){
                            cout << "Получена команда на запуск RTSP сервера в обычном режиме\n";
                            mediaStreams = xmlGetString(config, "view");
                            checkedMediaStream = xmlGetTagById(mediaStreams, "mediaStream", mediaStreamId);
                        }
                        else{
                            cout << "Получена команда на запуск RTSP сервера в тестовом режиме\n";
                            mediaStreams = xmlGetTag(socketPacket, "mediaStream");
                            checkedMediaStream = mediaStreams;
                        }

                        // проверяем видеоустройство и аудиоустройство на доступность
                        string busyMediaStreamId;
                        string & busyMediaStreamIdLink = busyMediaStreamId;

                        if (!checkAvailabilityDevices(checkedMediaStream, busyMediaStreamIdLink)){

                            bool forcedStart = xmlGetBool(socketPacket, "forcedStart");

                            if (forcedStart){
                                cout << "Форсированный старт RTSP сервера, с освобождением занятых устройств\n";

                                do{
                                    killRecord(busyMediaStreamId); // убиваем процесс записи медиаданных занявший устройства
                                }
                                while (!checkAvailabilityDevices(checkedMediaStream, busyMediaStreamIdLink));
                            }
                            else{
                                cout << "Требуемое видео или аудио устройство занято записью. Отправка запроса на подтверждение подключения к трансляции\n";
                                sendClient(xmlStringToTag("id", "deviceBusy")
                                           + xmlStringToTag("oldData", socketPacket));
                                continue;
                            }
                        }


                        // запускаем RTSP сервер

                        int result = startRtspServer(mediaStreams);

                        if (result == 0){
                            cout << "Ошибка при запуске RTSP сервера\n";
                            // отправляем уведомление о ошибке RTSP сервера
                            sendClient(xmlStringToTag("id", "rtspServerError" ));
                        }
                        if (result == 1){
                            // отправляем уведомление о том, что RTSP сервер уже запущен
                            sendClient(xmlStringToTag("id", "rtspStarted" ));
                        }
                        if (result ==2){
                            // отправляем уведомление об ожидании запуска RTSP сервера
                            sendClient(xmlStringToTag("id", "rtspStarts" ));
                        }
                        continue;
                    }


                    // команда на начало записи медиаданных

                    if (id == "startRecord"){
                        cout << "Получена команда на запись медиаданных\n";
                        static string mediaStreamId = "";
                        mediaStreamId = xmlGetString(socketPacket, "mediaStreamId");

                        string recordStatus = recordStatusMap[mediaStreamId];
                        if (recordStatus == "stop" || recordStatus == ""){
                            startRecord(mediaStreamId);
                        }
                        else  cout << "Запись медиаданных уже производится\n";
                        continue;
                    }

                    // команда на останов записи медиаданных

                    if (id == "stopRecord"){
                        cout << "Получена команда на завершение записи медиаданных\n";
                        string mediaStreamId = xmlGetString(socketPacket, "mediaStreamId");
                        string recordStatus = recordStatusMap[mediaStreamId];
                        if(recordStatus != "stop"){ // если поток работает
                            setRecordStatus(mediaStreamId, "stop");
                            cout << "Уничтожение процесса записи медиаданных\n";
                            killRecord(mediaStreamId); // убиваем процесс записи медиаданных
                        }
                        else  cout << "Запись уже остановлена\n";
                        continue;
                    }


                    // запрос на список камер

                    if (id == "getCamList"){
                        sendClient(xmlStringToTag("id", "camList")
                                   +xmlStringToTag("videoDevices", getAliasesVideoDevice()));
                        continue;
                    }


                    // запрос баланса

                    if (id == "requestSimInfo"){
                        sendSimInfo();
                        continue;
                    }


                    // тестовая отправка СМС

                    if (id == "testSms"){
                        sendNotification("SMS", "test SMS notification");
                        continue;
                    }


                    // тестовая отправка GCM

                    if (id == "testGcm"){
                        sendNotification("GCM", "test GCM notification");
                        continue;
                    }


                    if (id == "testNotification"){
                        sendNotification("all", "test notification");
                        continue;
                    }



                    // команда для контроллера

                    if (id == "controller"){

                        // удаляем ненужные теги...
                        string packet = xmlDeleteTag(socketPacket, "id");
                        packet = xmlDeleteTag(packet, "auth");
                        sendController(packet);

                        //cout << "Команда для контроллера: " << packet << endl;
                        continue;
                    }


                    // команда для выполнения USSD запроса

                    if (id == "requestUssd"){
                        string ussdCommand = xmlGetString(socketPacket, "ussdCommand");
                        startUssdRequestThread(ussdCommand);
                        continue;
                    }



                }
            }
        }


    }
    return 0;
}



