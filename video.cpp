#include "server.h"  // мой заголовочныйный файл
#include "xml.h"


// Получение псевдонимов видеоустройств

string getAliasesVideoDevice()
{
    cout << "Получение псевдонимов видеоустройств...\n";
    string videoDevicesString = "";
    FILE *fp = popen("v4l2-ctl --list-devices", "r");


    char videoInfoBuf[255] = {0};
    // в цикле получаем псевдонимы всех видеоустройств
    while (fgets(videoInfoBuf,sizeof(videoInfoBuf),fp))
    {
        string videoInfoString = videoInfoBuf;
        if (videoInfoString.find("usb") == std::string::npos) continue;

        for(char *ptr=videoInfoBuf ;*ptr!='\0';ptr++)
            if (*ptr=='\n') *ptr='\0';  // заменяем символ новой строки в конце имени камеры нулем

        videoDevicesString += xmlStringToTag("videoDevice", videoInfoBuf);
    }
    pclose(fp);   // закрываем поток

    return videoDevicesString;
}



// получение имени видеоустройства в виде: "/dev/video" по его псевдониму

string getVideoDeviceName(string aAlias){

    cout << "Получение имени видеоустройства по его псевдониму...\n";
    vector<string> videoInfoList;
    FILE *fp = popen("v4l2-ctl --list-devices", "r");

    char videoInfoBuf[255] = {0};
    // в цикле получаем строки из списка псевдонимов и имен устройств
    while (fgets(videoInfoBuf,sizeof(videoInfoBuf),fp))
    {
        for(char *ptr=videoInfoBuf ;*ptr!='\0';ptr++)
            if (*ptr=='\n') *ptr='\0';  // заменяем символ новой строки нулем

        videoInfoList.push_back(videoInfoBuf);
    }
    pclose(fp);   // закрываем поток

    // проходим в цикле по всем записанным строкам
    for(vector<string>::const_iterator it = videoInfoList.begin(); it !=videoInfoList.end(); ++it){

        if (*it == aAlias){
            int index = it - videoInfoList.begin();
            string name = trim(videoInfoList[index + 1]);
            cout << "Псевдоним: " << aAlias << " Имя: " << name << endl;
            return name;
        }

    }


    cout << "Не удалось получить имя видеоустройства по его псевдониму\n";

    return "";

}


