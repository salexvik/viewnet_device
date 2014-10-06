#include "server.h"  // мой заголовочныйный файл
#include "xml.h"


// Получение имени компьютера на котором работаем

string getMyName()
{
    char _myName[50] = {'\0'};
    string myName;

    FILE *fp = popen("uname -n", "r");
    fgets(_myName,sizeof(_myName),fp);
    _myName[strlen(_myName)-1] = '\0'; // зануляем символ перевода строки в конце имени
    pclose(fp); // закрываем поток
    myName = _myName;

    return myName;
}



// получение даты и времени

string getDateTime(string aFormat){
    string result;
    time_t now;
    struct tm *ptr;
    static char tbuf[64];
    bzero(tbuf,64);
    time(&now);
    ptr = localtime(&now);
    strftime(tbuf,64, aFormat.c_str(), ptr);
    result = tbuf;
    return trim(result);
}


string getDate(string aFormat){
    string result;
    time_t now;
    struct tm *ptr;
    static char tbuf[64];
    bzero(tbuf,64);
    time(&now);
    ptr = localtime(&now);
    strftime(tbuf,64, aFormat.c_str(), ptr);
    result = tbuf;
    return trim(result);
}

string getTime(string aFormat){
    string result;
    time_t now;
    struct tm *ptr;
    static char tbuf[64];
    bzero(tbuf,64);
    time(&now);
    ptr = localtime(&now);
    strftime(tbuf,64, aFormat.c_str(), ptr);
    result = tbuf;
    return trim(result);
}



// функция проверки существования файла конфигурации

bool existFileConfig(){
    return ifstream(pathConfig.c_str()).good();
}




// Функция чтения настроек из файла конфигурации

string getConfig()
{   
    string configString = "";
    string result = "";

    ifstream configFileReading(pathConfig.c_str()); // открываем файл для чтения
    if (configFileReading.is_open()){  // если удалось открыть файл
        while(!configFileReading.eof()){ // пока не прочтем все строки...
            getline(configFileReading, configString); // получаем текущую строку конфига

            result += configString;
            result +="\n";
        }
        configFileReading.close();
    }
    return result;
}


// функция изменения файла конфигурации

bool saveConfig()
{
    // открываем файл для записи

    if (config == "") return false;

    ofstream configFileWriting(pathConfig.c_str());
    if (configFileWriting.is_open())   // если удалось открыть файл
    {
        configFileWriting << config; // записываем в файл
        configFileWriting.close(); //закрываем файл
        return true;
    }
    else return false;
}





// функция записи в лог файл и вывода в терминал

void log(string msg)
{
    string pathLog;
    pathLog = "server.log";

    FILE * pLog;
    pLog = fopen(pathLog.c_str(), "a");
    if(pLog == NULL)
    {
        perror("writeLog");
    }
    char str[512];
    bzero(str, 512);
    strcpy(str, getDateTime("%e.%m.%Y_%H-%M-%S").c_str());
    strcat(str, " ");
    strcat(str, msg.c_str());
    strcat(str, "\n");
    fputs(str, pLog);
    fclose(pLog);

    cout << getDateTime("%e.%m.%Y_%H-%M-%S") << " "  << msg << endl;
}


// смена пароля

bool changePassword(string aUsername, string aPassword){

    string param("echo \""+aUsername +":"+aPassword+"\" | chpasswd 2>&1");
    bool result = false;

    FILE *fp = popen(param.c_str(), "r");

    char str[256] = {0};

    if(fgets(str,sizeof(str),fp)){ // если что-то прочитали, значит произошла ошибка
        result = false;
    }
    else{
        result = true;
    }
    pclose(fp);   // закрываем поток

    return result;
}



// Удаление начальных и конечных пробелов из строки

string trim(string aStringTrim){
    while(std::isspace(*aStringTrim.begin()))
        aStringTrim.erase(aStringTrim.begin());

    while(std::isspace(*aStringTrim.rbegin()))
        aStringTrim.erase(aStringTrim.length()-1);

    return aStringTrim;
}



// преобразование числа в строку

string intToString(int i){
    std::stringstream ss;
    std::string s;
    ss << i;
    s = ss.str();

    return s;
}



// функция для получения результата выполнения команды popen

vector<string> getPopenList(string popenLine, bool newLine){

    vector<string> resultList;

    FILE *fp = popen(popenLine.c_str(), "r");


    char resultBuf[512] = {0};

    while (fgets(resultBuf,sizeof(resultBuf),fp))
    {
        if (!newLine){
            for(char *ptr=resultBuf ;*ptr!='\0';ptr++)
                if (*ptr=='\n') *ptr='\0';  // заменяем символ новой строки нулем
        }

        resultList.push_back(resultBuf);
    }
    pclose(fp);   // закрываем поток

    return resultList;
}



string getPopenString(string popenLine, bool newLine){

    string resultString;

    FILE *fp = popen(popenLine.c_str(), "r");


    char resultBuf[512] = {0};

    while (fgets(resultBuf,sizeof(resultBuf),fp))
    {

        if (!newLine){
            for(char *ptr=resultBuf ;*ptr!='\0';ptr++)
                if (*ptr=='\n') *ptr='\0';  // заменяем символ новой строки нулем
        }

        resultString += resultBuf;
    }
    pclose(fp);   // закрываем поток

    return resultString;
}





string getRandomString(size_t N){

    char * pass = new char[N + 1]; //выделяем память для строки пароля
    for (int i = 0; i < N; ++ i)
    {
        switch(rand() % 3) //генерируем случайное число от 0 до 2
        {
        case 0: //если ноль
            pass[i] = rand() % 10 + '0'; //вставляем в пароль случайную цифру
            break;
        case 1: //если единица
            pass[i] = rand() % 26 + 'A'; //вставляем случайную большую букву
            break;
        case 2: //если двойка
            pass[i] = rand() % 26 + 'a'; //вставляем случайную маленькую букву
        }
    }
    pass[N] = 0; //записываем в конец строки признак конца строки

    return pass;
}





void sendNotification(string aTypeMsg, string aMsg){

    string msg = xmlStringToTag("msg", aMsg);
    string idMessage = xmlStringToTag("idM", getRandomString(5));
    string idDevice =  xmlStringToTag("idD", xmlGetString(config, "idDevice"));
    string timestamp = xmlStringToTag("timestamp", getDateTime("%e.%m.%Y %H:%M:%S"));

    if (aTypeMsg == "SMS") startSendSmsThread(msg + idMessage + idDevice);
    if (aTypeMsg == "GCM") startSendGcmThread(msg + idMessage + idDevice + timestamp);

    if (aTypeMsg == "all"){
       startSendSmsThread(msg + idMessage + idDevice);
       startSendGcmThread(msg + idMessage + idDevice + timestamp);
    }

}

















