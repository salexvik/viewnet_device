#include "server.h"  // мой заголовочныйный файл
#include "xml.h"



// Получение псевдонимов видеоустройств

string getAliasesAudioDevice(){

    cout << "Получение псевдонимов аудиоустройств...\n";

    vector<string> videoAliasList;
    vector<string> audioInfoList;
    string audioDevicesString = "";
    FILE *fp = popen("cat /proc/asound/cards", "r");


    char audioInfoBuf[255] = {0};
    // в цикле получаем псевдонимы всех аудиоустройств
    while (fgets(audioInfoBuf,sizeof(audioInfoBuf),fp))
    {
        for(char *ptr=audioInfoBuf ;*ptr!='\0';ptr++)
            if (*ptr=='\n') *ptr='\0';  // заменяем символ новой строки нулем

        audioInfoList.push_back(audioInfoBuf);
    }
    pclose(fp);   // закрываем поток


    videoAliasList = xmlGetArrayValues(getAliasesVideoDevice(), "videoDevice");

    for(vector<string>::const_iterator it = videoAliasList.begin(); it !=videoAliasList.end(); ++it){

        string videoAlias = *it;
        int posFirstSep = videoAlias.find("usb-bcm2708");
        int posLastSep = videoAlias.find("):", posFirstSep);
        if (posFirstSep == -1 || posLastSep == -1) continue;
        string videoAliasUsbPath = videoAlias.substr(posFirstSep, posLastSep - posFirstSep);

        for(vector<string>::const_iterator it = audioInfoList.begin(); it !=audioInfoList.end(); ++it){
            string audioInfo = *it;
            int posFirstSep = audioInfo.find("usb-bcm2708");
            int posLastSep = audioInfo.find(",", posFirstSep);
            if (posFirstSep == -1 || posLastSep == -1) continue;
            string audioInfoUsbPath = audioInfo.substr(posFirstSep, posLastSep - posFirstSep);

            if (audioInfoUsbPath == videoAliasUsbPath){
                audioDevicesString += xmlStringToTag("audioDevice", videoAlias);
                break;
            }
        }

    }

    return audioDevicesString;
}



// получение имени аудиоустройства в виде: "hw:*,*" по его псевдониму

string getAudioDeviceName(string aAlias){

    cout << "Получение имени аудиоустройства по его псевдониму...\n";
    vector<string> audioInfoList;
    FILE *fp = popen("cat /proc/asound/cards", "r");

    char audioInfoBuf[255] = {0};
    // в цикле получаем строки из списка псевдонимов и имен устройств
    while (fgets(audioInfoBuf,sizeof(audioInfoBuf),fp))
    {
        for(char *ptr=audioInfoBuf ;*ptr!='\0';ptr++)
            if (*ptr=='\n') *ptr='\0';  // заменяем символ новой строки нулем

        audioInfoList.push_back(audioInfoBuf);
    }
    pclose(fp);   // закрываем поток


    // выделяем USB path из преобразуемого в имя псевдонима

    int posFirstSep = aAlias.find("usb-bcm2708");
    int posLastSep = aAlias.find("):", posFirstSep);
    if (posFirstSep == -1 || posLastSep == -1){
        cout << "Не удалось получить имя аудиоустройства по его псевдониму\n";
        return "";
    }
    string aliasUsbPath = aAlias.substr(posFirstSep, posLastSep - posFirstSep);


    for(vector<string>::const_iterator it = audioInfoList.begin(); it !=audioInfoList.end(); ++it){

        string audioInfo = *it;
        int posFirstSep = audioInfo.find("usb-bcm2708");
        int posLastSep = audioInfo.find(",", posFirstSep);
        if (posFirstSep == -1 || posLastSep == -1) continue;
        string audioInfoUsbPath = audioInfo.substr(posFirstSep, posLastSep - posFirstSep);



        if (audioInfoUsbPath == aliasUsbPath){
            int index = it - audioInfoList.begin();
            string stringNumberCard = audioInfoList[index - 1];
            int posSep = stringNumberCard.find('[');
            if (posSep != -1){

                string numberCard = trim(stringNumberCard.substr(0, posSep));
                string name = "hw:" + numberCard + ",0";
                cout << "Псевдоним: " << aAlias << " Имя: " << name << endl;
                return name;
            }
        }

    }


    cout << "Не удалось получить имя аудиоустройства по его псевдониму\n";

    return "";

}




