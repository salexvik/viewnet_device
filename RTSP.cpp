#include "server.h"  // мой заголовочныйный файл
#include "xml.h"

int fd_readRtsp = 0; // дескриптор для чтения из канала RTSP сервера
int rtspServerPid = 0; // идентификатор дочернего процесса RTSP сервера
string oldMediaStreams;
string oldPassword;





// обработка сигналов завершения дочерних процессов

void sig_chld(int signo){
    pid_t pid;
    int stat;
    while( ( pid = waitpid(-1, &stat, WNOHANG) ) > 0 ){
        // cout << "Получен сигнал: PID=" << pid << " signo=" <<signo <<endl;
        if(pid == rtspServerPid){
            cout << "Получен сигнал: неожиданное завершение процесса RTSP сервера\n";
            rtspServerPid = 0;  // зануляем дескриптор процесса
            close(fd_readRtsp); // закрываем канал RTSP сервера
            fd_readRtsp = 0;    // зануляем дескриптор канала
            // отправляем уведомление о ошибке RTSP сервера
            sendAllClients(xmlStringToTag("id", "rtspServerError" ));
        }
    }
}





// проверка на доступность видеоустройства и аудиоустройства перед подключением к медиапотоку
bool checkAvailabilityDevices(string aMediaStream, string &aBusyMediaStreamId){

    bool videoEnable;
    bool audioEnable;
    string targetVideoDevice;
    string targetAudioDevice;
    string mediaStreams;
    vector<string> arrayMediaStreams;


    videoEnable = xmlGetBool(aMediaStream, "videoEnable");
    audioEnable = xmlGetBool(aMediaStream, "audioEnable");

    targetVideoDevice = xmlGetString(aMediaStream, "videoDevice");
    targetAudioDevice = xmlGetString(aMediaStream, "audioDevice");

    // Получаем все медиапотоки для записи
    mediaStreams = xmlGetString(config, "record");
    arrayMediaStreams = xmlGetArrayTags(mediaStreams, "mediaStream");

    for(vector<string>::const_iterator it = arrayMediaStreams.begin(); it !=arrayMediaStreams.end(); ++it){

        string mediaStreamId = xmlGetString(*it, "mediaStreamId");
        string recordStatus = recordStatusMap[mediaStreamId];

        if (recordStatus == "started"){ // если запись активна
            if (videoEnable){
                if (xmlGetBool(*it, "videoEnable")){
                    if ( xmlGetString(*it, "videoDevice") == targetVideoDevice){
                        aBusyMediaStreamId = mediaStreamId;
                        return false;
                    }
                }
            }

            if (audioEnable){
                if (xmlGetBool(*it, "audioEnable")){
                    if ( xmlGetString(*it, "audioDevice") == targetAudioDevice){
                        aBusyMediaStreamId = mediaStreamId;
                        return false;
                    }
                }
            }
        }
    }

    return true;
}




// отправка данных для отображения медиапотоков просмотра
void sendMediaStreamsDataToView(){
    string mediaStreams = xmlGetString(config, "view");
    string outDataXML = xmlStringToTag("id", "responseMediastreamsDataToView");
    outDataXML += xmlGetTags(mediaStreams, "mediaStreamName");
    outDataXML += xmlGetTags(mediaStreams, "mediaStreamId");
    sendClient(outDataXML);   // отправляяем клиенту
}






// запуск RTSP сервера

int startRtspServer(string aMediaStreams)
{

    string password = xmlGetString(config, "password"); // получаем пароль из настроек

    // проверяем существование процесса RSTP сервера

    cout << "Проверка состояния RTSP сервера...\n";

    if (rtspServerPid > 0){ // если RTSP-сервер активен...

        cout << "RTSP сервер активен\n";

        // проверяем изменение параметров запуска...

        cout << "Проверка изменений параметров запуска...\n";

        if ((xmlDeleteTags(aMediaStreams, "mediaStreamName") != xmlDeleteTags(oldMediaStreams, "mediaStreamName")) || password != oldPassword){

            // если параметры запуска изменились...

            // запоминаем новые параметры запуска
            oldPassword = password;
            oldMediaStreams = aMediaStreams;

            cout << "Параметры запуска изменились, перезапуск RTSP сервера с новыми параметрами...\n";
            int tempPid = rtspServerPid;
            rtspServerPid = 0;
            close(fd_readRtsp); // закрываем канал RTSP сервера
            fd_readRtsp = 0;
            cout << "Завершение процесса RTSP сервера\n";
            kill(tempPid, SIGTERM); // убиваем старый процесс
            sleep(1);
        }
        else{

            // если параметры запуска не менялись...
            cout << "Параметры запуска не изменились, действие не требуется\n";
            return 1; // сервер запущен
        }
    }
    else cout << "RTSP сервер не активен\n";


    char *startLinesArray[20] = {(char*)0}; // массив параметров запуска медиапотоков для передачи RTSP серверу


    string rtspServerPath = xmlGetString(config, "rtspServerPath");
    string rtspServerName = xmlGetString(config, "rtspServerName");
    string idDevice = xmlGetString(config, "idDevice");
    string rtspServerPort = xmlGetString(config, "rtspServerPort");
    vector<string> vectorString;


    // создаем канал для связи с RTSP сервером
    int pipedes[2];
    pipe(pipedes);

    char fd_writeRtsp[10] = {0}; // строка с номером дескриптора для записи
    sprintf(fd_writeRtsp, "%d", pipedes[1]); // переводим дескриптор для записи в строку


    // заполняем массив основными параметрами командной строки...

    startLinesArray[0] = (char*)rtspServerName.c_str(); // имя запускаемой программы
    startLinesArray[1] = (char*)"--gst-debug-level=2"; // параметр для вывода лога
    startLinesArray[2] = (char*)fd_writeRtsp; // дескриптор для записи в канал
    startLinesArray[3] = (char*)idDevice.c_str(); // id устройства
    startLinesArray[4] = (char*)password.c_str(); // пароль
    startLinesArray[5] = (char*)rtspServerPort.c_str(); // порт

    // заполняем массив строками запуска медиапотоков

    vector<string> mediaStreamsList = xmlGetArrayValues(aMediaStreams, "mediaStream");
    int i = 6;
    for(vector<string>::const_iterator it = mediaStreamsList.begin(); it !=mediaStreamsList.end(); ++it){
        string mediaStream = *it;

        string videoDevice, resolution, resolutionW, resolutionH, fps, codec, bitrate, audioDevice, channel;

        bool videoEnable = xmlGetBool(mediaStream, "videoEnable");
        if (videoEnable){
            videoDevice = getVideoDeviceName(xmlGetString(mediaStream, "videoDevice"));
            resolution = xmlGetString(mediaStream, "resolution");
            resolutionW = resolution.substr(0, resolution.find("x"));
            resolutionH = resolution.substr(resolution.find("x")+1, resolution.length());
            fps = xmlGetString(mediaStream, "fps");
            codec = xmlGetString(mediaStream, "codec");
            bitrate = xmlGetString(mediaStream, "bitrate");

            // настройка камеры

            string systemLine = "sudo v4l2-ctl -d " + videoDevice + " --set-ctrl  focus_auto=0";
            system(systemLine.c_str());
            systemLine = "sudo v4l2-ctl -d " + videoDevice + " --set-ctrl  focus_absolute=0";
            system(systemLine.c_str());
            systemLine = "sudo v4l2-ctl -d " + videoDevice + " --set-ctrl  sharpness=255";
            system(systemLine.c_str());
            systemLine = "sudo v4l2-ctl -d " + videoDevice + " --set-ctrl  contrast=170";
            system(systemLine.c_str());

        }

        bool audioEnable = xmlGetBool(mediaStream, "audioEnable");
        if (audioEnable){
            audioDevice = getAudioDeviceName(xmlGetString(mediaStream, "audioDevice"));
            channel = xmlGetString(mediaStream, "channel");
        }



        string mediaStreamId = xmlGetString(mediaStream, "mediaStreamId");
        string startLine;

        // видео и аудио

        if(videoEnable && audioEnable){

            // сжатие с помощью аппаратного кодера устройства h264

            if(codec == "h264"){
                startLine = "( v4l2src device=" +videoDevice
                        +" ! video/x-raw, format=(string)I420, width=(int)"
                        +resolutionW +", height=(int)" +resolutionH +", framerate=(fraction)"
                        +fps +"/1 ! clockoverlay time-format=\"%d.%m.%Y   %H:%M:%S\" ! textoverlay text=\"video h264 and audio\""
                        +" ! omxh264enc target-bitrate=" +bitrate +" control-rate=variable ! queue ! video/x-h264, profile=(string)high, level=(string)4 ! "
                        +"rtph264pay name=pay0 pt=96 config-interval=2 alsasrc device="
                        +audioDevice +" ! volume volume=10.0 ! audio/x-raw,format=S16LE,rate=16000,channels=" +channel +" ! voaacenc bitrate=16000 ! rtpmp4apay name=pay1 pt=97 )"
                        +"url=/" +mediaStreamId;

            }

            // сжатие с помощью аппаратного кодера камеры h264

            if(codec == "h264 camera support"){
                startLine = "( uvch264src initial-bitrate=" +bitrate +" iframe-period=2000 device=" +videoDevice
                        +" name=src auto-start=true src.vidsrc ! queue ! video/x-h264, width=(int)"
                        +resolutionW +", height=(int)" +resolutionH +", framerate=(fraction)"
                        +fps +"/1 ! h264parse ! "
                        +"rtph264pay name=pay0 pt=96 config-interval=2 alsasrc device="
                        +audioDevice +" ! volume volume=10.0 ! audio/x-raw,format=S16LE,rate=16000,channels=" +channel +" ! voaacenc bitrate=16000 ! rtpmp4apay name=pay1 pt=97 )"
                        +"url=/" +mediaStreamId;

            }

            // сжатие с помощью аппаратного кодера камеры mjpeg

            if(codec == "mjpeg camera support"){
                startLine = "( v4l2src device=" +videoDevice
                        +" ! videorate ! image/jpeg, width=(int)" + resolutionW +", height=(int)" +resolutionH +", framerate=(fraction)"
                        +fps +"/5 ! queue ! rtpjpegpay name=pay0 pt=96 alsasrc device="
                        +audioDevice +" ! volume volume=10.0 ! audio/x-raw,format=S16LE,rate=16000,channels=" +channel +" ! voaacenc bitrate=16000 ! rtpmp4apay name=pay1 pt=97 )"
                        +"url=/" +mediaStreamId;
            }


        }
        else

            // только видео

            if(videoEnable && !audioEnable){

                // сжатие с помощью аппаратного кодера устройства h264

                if(codec == "h264"){
                    startLine = "( v4l2src  device=" +videoDevice
                            +" ! video/x-raw, format=I420, width=" +resolutionW
                            +", height=" +resolutionH +", framerate=(fraction)"
                            +fps +"/1  ! clockoverlay time-format=\"%d.%m.%Y   %H:%M:%S\" ! textoverlay text=\"video h264\""
                            +" ! omxh264enc target-bitrate=" +bitrate +" control-rate=variable ! "
                            +"video/x-h264, profile=(string)high, level=(string)4 ! rtph264pay name=pay0 pt=96 config-interval=2 )"
                            +"url=/" +mediaStreamId;
                }

                // сжатие с помощью аппаратного кодера камеры h264

                if(codec == "h264 camera support"){
                    startLine = "( uvch264src initial-bitrate=" +bitrate +" iframe-period=2000 device=" +videoDevice
                            +" name=src auto-start=true src.vidsrc ! queue ! video/x-h264, width=(int)"
                            +resolutionW +", height=(int)" +resolutionH +", framerate=(fraction)"
                            +fps +"/1 ! h264parse ! "
                            +"rtph264pay name=pay0 pt=96 config-interval=2 )"
                            +"url=/" +mediaStreamId;

                }

                // сжатие с помощью аппаратного кодера камеры mjpeg

                if(codec == "mjpeg camera support"){
                    startLine = "( v4l2src device=" +videoDevice
                            +" ! videorate ! image/jpeg, width=(int)" + resolutionW +", height=(int)" +resolutionH +", framerate=(fraction)"
                            +fps +"/1 ! queue ! rtpjpegpay name=pay0 pt=96 )url=/" +mediaStreamId;
                }

            }
            else

                // только аудио

                if(!videoEnable && audioEnable){

                    startLine = "( alsasrc device="
                            +audioDevice +" ! volume volume=10.0 ! audio/x-raw,format=S16LE,rate=16000,channels=" +channel +" ! voaacenc bitrate=16000 ! rtpmp4apay name=pay0 pt=96 )"
                            +"url=/" +mediaStreamId;

                }
                else return 0; // ошибка при запуске сервера



        vectorString.push_back(startLine);
        string tmp = vectorString.back();
        startLinesArray[i] = (char *)tmp.c_str(); // добавляем строку запуска медиапотока в массив параметров командной строки
        i++;
    }

    cout << "Создание процесса для RTSP сервера\n";
    rtspServerPid = fork();   // создаем дочерний процесс

    if (rtspServerPid == 0){   // если находимся в дочернем процессе...

        close(pipedes[0]); // закрываем дескриптор открытый для чтения

        // запускаем в дочернем процессе RTSP сервер
        cout << "Запуск RTSP сервера в новом процессе\n";
        execv(rtspServerPath.c_str(), startLinesArray);
        cout << "Ошибка запуска RTSP сервера в новом процессе\n";
        perror("exec"); // сработает, если произойдет ошибка
        exit(1); // завершаем процесс
    }

    close(pipedes[1]); // закрываем дескриптор открытый для записи
    fd_readRtsp = pipedes[0]; // запоминаем дескриптор для чтения из канала

    return 2; // сервер запускается
}





// обработчик потока чтения из канала RTSP сервера

void *handlerRtspReadCannelThread(void * arg){

    while (true){

        if (fd_readRtsp > 0){
            char readBuf[128] = {0};
            int bytesReadCount = read(fd_readRtsp, readBuf, sizeof(readBuf));
            if(bytesReadCount > 0){
                cout << "Прочли из канала: " << bytesReadCount << " байт "  << readBuf << endl;

                string messageStart = "RSTP server started";
                if(messageStart == readBuf){

                    // отправляем уведомление о запуске RTSP сервера
                    cout << "Отправляем уведомление о запуске RTSP сервера\n";
                    sendClient(xmlStringToTag("id", "rtspStarted" ));
                }
            }
        } else sleep(1);

    }

    return 0;

}



// Создание потока чтения из канала RTSP сервера

bool startRtspReadCannelThread() {
    pthread_t rtspReadCannelThread;
    pthread_attr_t attr;
    pthread_attr_init(&attr);

    pthread_create(&rtspReadCannelThread, &attr, handlerRtspReadCannelThread, NULL);
    return true;
}



