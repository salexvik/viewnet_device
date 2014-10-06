#include "server.h"  // мой заголовочныйный файл
#include "xml.h"




// автостарт записи
void autoStartRecord(){
    string mediaStreams = xmlGetString(config, "record");
    vector<string> arrayMediaStreams = xmlGetArrayTags(mediaStreams, "mediaStream");
    static string mediaStreamId;

    for(vector<string>::const_iterator it = arrayMediaStreams.begin(); it !=arrayMediaStreams.end(); ++it){
        if (xmlGetBool(*it, "autoStartRecord")){
            mediaStreamId = xmlGetString(*it, "mediaStreamId");
            startRecord(mediaStreamId);
        }
    }
}




// отправка данных для отображения медиапотоков записи
void sendMediaStreamsDataToRecord(){
    string mediaStreams =   xmlGetString(config, "record");
    string outDataXML = xmlStringToTag("id", "responseMediastreamsDataToRecord");
    outDataXML += xmlGetTags(mediaStreams, "mediaStreamName");
    outDataXML += xmlGetTags(mediaStreams, "mediaStreamId");

    vector<string> arrayRecordStatus = xmlGetArrayTags(mediaStreams, "mediaStream");
    string mediaStreamsRecordStatus;
    for(vector<string>::const_iterator it = arrayRecordStatus.begin(); it !=arrayRecordStatus.end(); ++it){
        string recordStatus = recordStatusMap[xmlGetString(*it, "mediaStreamId")];
        if (recordStatus == "") mediaStreamsRecordStatus += xmlStringToTag("mediaStreamRecordStatus", "stop");
        else mediaStreamsRecordStatus += xmlStringToTag("mediaStreamRecordStatus", recordStatus);
    }
    outDataXML += mediaStreamsRecordStatus;
    sendClient(outDataXML);   // отправляяем клиенту
}






void setRecordStatus(string aMediaStreamId, string aRecordStatus){

    recordStatusMap[aMediaStreamId] = aRecordStatus; // сохраняем статус записи в массиве

    sendAllClients(xmlStringToTag("id", "recordStatus")
                   + xmlStringToTag("mediaStreamId", aMediaStreamId)
                   + xmlStringToTag("recordStatus", aRecordStatus));
}

void killRecord(string aMediaStreamId){

    int recordPid = recordPidMap[aMediaStreamId];
    if (recordPid > 0) kill(recordPid, SIGTERM);
    recordPidMap[aMediaStreamId] = 0;
}



bool createDir(string aDir){

    if (access(aDir.c_str(), F_OK) !=0 ){
        if (mkdir(aDir.c_str(), 0777) !=0){
            cout << "Ошибка при создании каталога\n";
            return false;
        } else cout << "Каталог создан\n";
    } else cout << "Каталог уже существует\n";

    return true;
}



bool checkFreeSize(int aMinFreeSizeFlashKB,  string aMountName, string aRootRecordDir, int aNumberFilesToDeleted){

    int freeSizeFlashKB = 0;
    int oldFreeSizeFlashKB = -1;
    while(true){

        string popenLine = "df | awk '/'" +aMountName  +"/' {print $4}'";
        FILE *fp = popen(popenLine.c_str(), "r");
        char freeSizeBuf[10] = {0};
        fgets(freeSizeBuf,sizeof(freeSizeBuf),fp);
        pclose(fp);

        freeSizeFlashKB = atoi(freeSizeBuf);

        if (freeSizeFlashKB == oldFreeSizeFlashKB){
            cout << "После удаления файлов, размер свободгого места не изменился, запись остановлена\n";
            return false;
        }

        if (freeSizeFlashKB <= aMinFreeSizeFlashKB){
            cout << "Недостаточно свободного места для записи\n";
            cout << "Размер свободного места: " << freeSizeFlashKB << " kB\n";
            cout << "Минимальный размер свободного места: " << aMinFreeSizeFlashKB << " kB\n";
            cout << "Удаление старых файлов...\n";

            oldFreeSizeFlashKB = freeSizeFlashKB; // запоминаем размер свободного места до удаления файлов

            string popenLine = "find " +aRootRecordDir  +" -type f -name \"*\" -exec ls -1rt \"{}\" +";
            FILE *fp = popen(popenLine.c_str(), "r");
            char fileNameToRemove[256] = {0};

            for (int i=0;i<aNumberFilesToDeleted;i++){

                if(!fgets(fileNameToRemove,sizeof(fileNameToRemove),fp)) break; // если дошли до конца списка, выходим

                char *pos = strrchr(fileNameToRemove, '\n');
                if (pos) fileNameToRemove[pos-fileNameToRemove] = 0;

                cout << "Удаление файла: " << fileNameToRemove << endl;

                if (unlink(fileNameToRemove) != 0){
                    cout << "Ошибка при удалении файла\n";
                }
            }
            pclose(fp);

            // удаляем все пустые файлы и каталоги в директории записи на флешке, если таковые обнаружатся...
            string systemLine = "find " +aRootRecordDir +" -mindepth 1 -name \"*\" -empty -delete";
            system(systemLine.c_str());
        }

        if (freeSizeFlashKB > aMinFreeSizeFlashKB){
            cout << "Размер свободного места достаточен для записи\n";
            return true;
        }
    }
}

// проверка файлов после окончания записи и удаление если размер равен 0

void removeEmptyFile(string aFileName)
{
    cout << "Проверка файла после завершения записи: " << aFileName << endl;
    struct stat file_stat;
    stat(aFileName.c_str(), &file_stat);
    cout << "Размер файла: " << file_stat.st_size << endl;
    if (file_stat.st_size == 0){
        if(remove(aFileName.c_str()) !=0){
            cout << "Ошибка при удалении файла\n";
        }
        else cout << "Файл удален\n";
    }
    else cout << "Файл не пустой, удаление не требуется\n";
}



// выполняется в отдельном потоке

void *handle_startRecord(void *aMediaStreamId) {
    cout << "record: Старт потока для записи медиаданных в файл\n";

    string mediaStreamId = (const char*)aMediaStreamId;

    setRecordStatus(mediaStreamId, "started"); // отправляем клиентам статус записи

    string mountName = xmlGetString(config, "mountName");
    string rootRecordDir = xmlGetString(config, "mountDir") + "/" + mountName + "/" + xmlGetString(config, "rootRecordName") + "/";

    int numberFilesToDeleted = xmlGetInt(config, "numberFilesToDeleted");
    int minFreeSizeFlashKB = xmlGetInt(config, "minFreeSizeFlashMB") * 1024; // лимит минимального свободного места на флешке

    string mediaStreams = xmlGetString(config, "record");
    string mediaStream = xmlGetTagById(mediaStreams, "mediaStream", mediaStreamId);
    string mediaStreamNameDir = rootRecordDir + xmlGetString(mediaStream, "mediaStreamName") + "/";

    time_t startTime, currentTime;

    string gstLaunchPath = xmlGetString(config, "gstLaunchPath");
    string gstLaunchName = xmlGetString(config, "gstLaunchName");

    cout << "mediaStream = " << mediaStream << endl;

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
    int recordingTimeSec = xmlGetInt(mediaStream, "recordingTime") * 60 ;

    // выполняем запись в цикле

    while(true){

        // проверяем статус записи...
        if(recordStatusMap[mediaStreamId] == "stop"){
            cout << "Процесс записи медиаданных завершен пользователем, выход из потока записи\n";
            setRecordStatus(mediaStreamId, "stop");
            return 0;
        }



        cout << "Создание процесса для записи медиаданных в файл\n";
        cout << "mediaStreamId = " << mediaStreamId << endl;

        cout << "Проверка свободного места на флешке...\n";
        if (!checkFreeSize(minFreeSizeFlashKB, mountName, rootRecordDir, numberFilesToDeleted)){
            setRecordStatus(mediaStreamId, "starting"); // отправляем клиентам статус записи
            sleep(10);
            continue;
        }

        cout << "Создание корневого каталога...\n";
        if (!createDir(rootRecordDir)){
            setRecordStatus(mediaStreamId, "starting"); // отправляем клиентам статус записи
            sleep(10);
            continue;
        }

        cout << "Создание каталога с именем медиапотока...\n";
        if (!createDir(mediaStreamNameDir)){
            setRecordStatus(mediaStreamId, "starting"); // отправляем клиентам статус записи
            sleep(10);
            continue;
        }

        cout << "Создание каталога с текущей датой...\n";
        string currentDateDir = mediaStreamNameDir + getDate("%e.%m.%Y") + "/";
        if (!createDir(currentDateDir)){
            setRecordStatus(mediaStreamId, "starting"); // отправляем клиентам статус записи
            sleep(10);
            continue;
        }


        time(&startTime); // получаем время старта процесса

        string recordingFile = currentDateDir + getTime("%H-%M-%S") + ".ts";
        string location = "location=" + recordingFile; // получаем имя для файла
        cout << "Записываемый файл: " << recordingFile << endl;

        int recordPid = fork();   // создаем дочерний процесс

        if (recordPid == 0){   // если находимся в дочернем процессе...

            // только видео

            if(videoEnable  && !audioEnable){

                if(codec == "h264"){

                    cout << "Выбрана запись видео без звука с аппаратным кодером устройства h264\n";

                    string videoDeviceNameParam = "device=" + videoDevice;
                    string videoFormatParam = "video/x-raw,format=I420,width=" +resolutionW  +",height=" +resolutionH +",framerate=" +fps +"/1";
                    string targetBitrate = "target-bitrate=" +bitrate;

                    execl(gstLaunchPath.c_str(), gstLaunchName.c_str(), "v4l2src", videoDeviceNameParam.c_str(),
                          "!", "videorate", "!", videoFormatParam.c_str(), "!", "omxh264enc", targetBitrate.c_str(), "control-rate=variable", "!",
                          "video/x-h264,profile=high", "!", "queue", "max-size-bytes=10000000", "!", "h264parse", "!",  "avimux",
                          "!", "filesink", location.c_str(), (const char*)0); // запускаем код в новом процессе...
                }

                // сжатие с помощью аппаратного кодера камеры h264

                if(codec == "h264 camera support"){
                    cout << "Выбрана запись видео без звука с аппаратным кодером камеры h264\n";


                    string videoDeviceNameParam = "device=" + videoDevice;
                    string videoFormatParam = "video/x-h264,width=" +resolutionW  +",height=" +resolutionH +",framerate=" +fps +"/1,profile=constrained-baseline";
                    string initialBitrate = "initial-bitrate=" + bitrate;

                    execl(gstLaunchPath.c_str(), gstLaunchName.c_str(), "uvch264src", "rate-control=cbr", initialBitrate.c_str(), "iframe-period=2000",
                          videoDeviceNameParam.c_str(), "name=src", "auto-start=true",
                          "src.vfsrc", "!", "queue", "!", "video/x-raw,width=320,height=240,framerate=30/1", "!", "fakesink",
                          "src.vidsrc", "!", "queue", "!", videoFormatParam.c_str(), "!", "queue", "!", "h264parse", "!", "mpegtsmux", "!", "filesink",
                          location.c_str(), (const char*)0); // запускаем код в новом процессе...
                }

                // сжатие с помощью аппаратного кодера камеры mjpeg                

                if(codec == "mjpeg camera support"){

                    // gst-launch-1.0 v4l2src device=/dev/video0 ! videorate ! image/jpeg,framerate=10/1,width=640,height=480 ! queue max-size-bytes=10000000 ! avimux ! filesink location=/mnt/flash/mjpg.ts

                    cout << "Выбрана запись видео без звука с кодером камеры mjpeg\n";

                    string videoDeviceNameParam = "device=" + videoDevice;
                    string videoFormatParam = "image/jpeg,width=" +resolutionW  +",height=" +resolutionH +",framerate=" +fps +"/1";


                    execl(gstLaunchPath.c_str(), gstLaunchName.c_str(), "v4l2src", videoDeviceNameParam.c_str(),
                          "!", "videorate", "!", videoFormatParam.c_str(), "!", "queue", "max-size-bytes=10000000", "!", "avimux",
                          "!", "filesink", location.c_str(), (const char*)0); // запускаем код в новом процессе...

                }

            }

            // видео и аудио

            if(videoEnable && audioEnable){

                if(codec == "h264"){

                    cout << "Выбрана запись видео со звуком с аппаратным кодером устройства h264\n";

                    string videoDeviceNameParam = "device=" + videoDevice;
                    string audioDeviceParam = "device=" + audioDevice;
                    string videoFormatParam = "video/x-raw,format=I420,width=" +resolutionW  +",height=" +resolutionH +",framerate=" +fps +"/1";
                    string targetBitrate = "target-bitrate=" +bitrate;
                    string audioFormatParam = "audio/x-raw,format=S16LE,rate=48000,channels=" + channel;

                    execl(gstLaunchPath.c_str(), gstLaunchName.c_str(), "v4l2src", videoDeviceNameParam.c_str(),
                          "!", "videorate", "!", videoFormatParam.c_str(), "!", "omxh264enc", targetBitrate.c_str(), "control-rate=variable", "!",
                          "video/x-h264,profile=high", "!", "h264parse", "!", "queue", "max-size-bytes=10000000", "!", "avimux", "name=mux",
                          "alsasrc", audioDeviceParam.c_str(), "!", "volume", "volume=10.0", "!", "audioresample", "!", audioFormatParam.c_str(), "!",
                          "queue", "!", "voaacenc", "!", "aacparse", "!", "queue", "!", "mux.", "mux.", "!",
                          "filesink", location.c_str(), (const char*)0); // запускаем код в новом процессе...
                }

                // сжатие с помощью аппаратного кодера камеры h264

                if(codec == "h264 camera support"){
                    cout << "Выбрана запись видео со звуком с аппаратным кодером камеры h264\n";


                    string videoDeviceNameParam = "device=" + videoDevice;
                    string audioDeviceParam = "device=" + audioDevice;
                    string videoFormatParam = "video/x-h264,width=" +resolutionW  +",height=" +resolutionH +",framerate=" +fps +"/1,profile=constrained-baseline";
                    string audioFormatParam = "audio/x-raw,format=S16LE,rate=48000,channels=" + channel;
                    string initialBitrate = "average-bitrate=" + bitrate;


                    execl(gstLaunchPath.c_str(), gstLaunchName.c_str(), "uvch264src", "rate-control=cbr", initialBitrate.c_str(), "iframe-period=2000", videoDeviceNameParam.c_str(), "name=src", "auto-start=true", "src.vidsrc", "!", "queue", "!",
                          videoFormatParam.c_str(), "!", "queue", "!", "h264parse", "!", "avimux", "name=mux",
                          "alsasrc", audioDeviceParam.c_str(), "!", "volume", "volume=10.0", "!", "audioresample", "!", audioFormatParam.c_str(), "!",
                          "queue", "!", "voaacenc", "!", "aacparse", "!", "queue", "!", "mux.", "mux.", "!",
                          "filesink", location.c_str(), (const char*)0); // запускаем код в новом процессе...
                }



                // сжатие с помощью аппаратного кодера камеры mjpeg

                if(codec == "mjpeg camera support"){

                    // gst-launch-1.0 v4l2src device=/dev/video0 ! videorate ! image/jpeg,framerate=10/1,width=640,height=480 !
                    // queue max-size-bytes=10000000 ! avimux name=mux alsasrc device=hw:1,0 ! volume volume=10.0 ! audioresample !
                    // audio/x-raw,rate=48000,channels=2 ! queue ! voaacenc bitrate=16000 ! aacparse ! queue ! mux. mux. ! filesink location=/mnt/flash/mjpg_and_audio.ts

                    cout << "Выбрана запись видео со звуком с кодером камеры mjpeg\n";

                    string videoDeviceNameParam = "device=" + videoDevice;
                    string videoFormatParam = "image/jpeg,width=" +resolutionW  +",height=" +resolutionH +",framerate=" +fps +"/1";
                    string audioDeviceParam = "device=" + audioDevice;
                    string audioFormatParam = "audio/x-raw,format=S16LE,rate=48000,channels=" + channel;


                    execl(gstLaunchPath.c_str(), gstLaunchName.c_str(), "v4l2src", videoDeviceNameParam.c_str(),
                          "!", "videorate", "!", videoFormatParam.c_str(), "!", "queue", "max-size-bytes=10000000", "!", "avimux", "name=mux",
                          "alsasrc", audioDeviceParam.c_str(), "!", "volume", "volume=10.0", "!", "audioresample", "!", audioFormatParam.c_str(), "!",
                          "queue", "!", "voaacenc", "!", "aacparse", "!", "queue", "!", "mux.", "mux.", "!",
                          "filesink", location.c_str(), (const char*)0); // запускаем код в новом процессе...

                }



            }

            // только аудио

            if(!videoEnable  && audioEnable){
                cout << "Выбрана запись звука\n";

                string audioDeviceParam = "device=" + audioDevice;
                string audioFormatParam = "audio/x-raw,format=S16LE,rate=48000,channels=" + channel;

                execl(gstLaunchPath.c_str(), gstLaunchName.c_str(), "alsasrc", audioDeviceParam.c_str(), "!", "volume", "volume=10.0", "!",
                      "audioresample", "!", audioFormatParam.c_str(), "!", "queue", "!", "voaacenc", "!",
                      "aacparse", "!", "queue", "!", "mpegtsmux", "!", "filesink", location.c_str(), (const char*)0); // запускаем код в новом процессе...
            }


            // следующий код сработает если произойдет ошибка при вызове execv
            cout << "Ошибка запуска записи медиаданных в новом процессе\n";
            perror("exec");
            exit(1); // завершаем процесс
        }
        else{
            // если находимся в родительском процессе...

            recordPidMap[mediaStreamId] = recordPid; // заносим идентификатор дочернего процесса в массив

            // ожидаем завершение процесса...

            for (int i=0; i< recordingTimeSec; i++){

                if(waitpid(recordPid, NULL, WNOHANG) != 0 ){ // если процесс уже завершился

                    recordPidMap[mediaStreamId] = 0; // зануляем идентификатор процесса записи

                    if(recordStatusMap[mediaStreamId] == "started" || recordStatusMap[mediaStreamId] == "starting"){
                        cout << "В процессе записи файла произошла ошибка\n";
                        removeEmptyFile(recordingFile);
                        setRecordStatus(mediaStreamId, "starting");
                        cout << "Перезапуск процесса записи файла будет произведен через 10 секунд\n";
                        sleep(10);
                        break;
                    }


                    if(recordStatusMap[mediaStreamId] == "stop"){
                        cout << "Процесс записи медиаданных завершен пользователем, выход из потока записи\n";
                        setRecordStatus(mediaStreamId, "stop");
                        removeEmptyFile(recordingFile);
                        return 0;
                    }

                }
                else{ // если процесс еще выполняется...

                    time(&currentTime); // получаем текущее время
                    int workingTime = difftime(currentTime, startTime); // вычисляем время в секундах, которое отработал процесс

                    if(workingTime > 10){
                        if (recordStatusMap[mediaStreamId] == "starting"){
                            cout << "Процесс записи файла продолжается более 10 секунд. Смена статуса записи на \"Started\"\n";
                            setRecordStatus(mediaStreamId, "started"); // отправляем клиентам статус записи
                        }
                    }

                    sleep(1); // ждем одну секунду до следующей проверки на окончание процесса
                }
            }

            if (recordStatusMap[mediaStreamId] == "started"){
                cout << "Время записи файла истекло, уничтожаем процесс, и начинаем запись нового файла в новом процессе...\n";
                killRecord(mediaStreamId); // убиваем процесс записи медиаданных
            }

        }
    }

    return 0;
}


// Создание потока

bool startRecord(string aMediaStreamId) {
    pthread_t thread;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    return !pthread_create(&thread, &attr, handle_startRecord, (void*)aMediaStreamId.c_str());
}
