#include "server.h"  // мой заголовочныйный файл
#include "xml.h"





// Сброс таймера контроллера для перезагрузки устройства

void *handleResetRebootTimerThread(void * arg) {

    while(true){

        sleep(25);
        //cout << "Сброс таймера перезагрузки устройства...\n";
        sendController(xmlStringToTag("command", "resetRebootDeviceTimer"));

    }

    return 0;
}



// Проверка подключения к интернету

void *handleCheckInternetConnectThread(void * arg) {

    while (true){


        sleep(60);


        cout << "Проверка доступа к интернету...\n";
        char *hostname;
        struct hostent *hostinfo;

        hostname = (char *)"google.com";
        hostinfo = gethostbyname (hostname);

        if (hostinfo != NULL){
            cout << "Доступ к интернету имеется\n";
        }
        else {
            cout << "Доступ к интернету отсутствует\n";

            internetConnect();
        }


    }

    return 0;
}






// поток для получения информации о SIM карте

void *handleGetSimInfoThread(void * arg){

    while(true){
        getSimInfo();
    }
    return 0;
}





// Создание потоков

bool startTimerThreads() {
    pthread_t resetRebootTimerThread, checkInternetConnectThread, getSimInfoThread;
    pthread_attr_t attr;
    pthread_attr_init(&attr);

    pthread_create(&resetRebootTimerThread, &attr, handleResetRebootTimerThread, NULL);
//    pthread_create(&checkInternetConnectThread, &attr, handleCheckInternetConnectThread, NULL);
    pthread_create(&getSimInfoThread, &attr, handleGetSimInfoThread, NULL);

    return true;
}
