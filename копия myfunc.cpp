
#include <netinet/in.h>
#include <set>
#include <string.h>  /* Объявления строковых функций */
#include "server.h"  // мой заголовочныйный файл
#include <stdio.h>
#include <time.h>
#include <iostream>

extern char pathlog[100]; // путь к лог-файлу



char* getTime()
{ //
    time_t now;
    struct tm *ptr;
    static char tbuf[64];
    bzero(tbuf,64);
    time(&now);
    ptr = localtime(&now);
    strftime(tbuf,64, "%Y-%m-%e %H:%M:%S", ptr);
    return tbuf;
}


// Отправка данных клиентам

void sendsockdata(const char *data, set<int> clients, int _fd)
{
    int len = strlen(data); // узнаем размер данных для отправки
    unsigned char pack[1024] = {'#'};
    pack[1] = (len + 3) >> 8;
    pack[2] = len + 3;
    memcpy(&pack[3], data, len);

    if (_fd == 0) // если требуется отправка всем подключенным клиентам...
    {
         // проходим в цикле по всем клиентским сокетам
       for(set<int>::iterator it = clients.begin(); it != clients.end(); it++)
      {
         send(*it,pack,len+3, 0); // отправляем в сокет заголовок
      }
    }
    else    // если требуется отправка только одному клиенту...
    {
        send(_fd,pack, len+3, 0); // отправляем в сокет заголовок
    }
}



// Поиск видеоустройств

char *findcam(char *listCam)
{

    memset( listCam, 0, sizeof( listCam ));
    // открываем поток
    FILE *fp = popen("find /dev/video* | awk -F '/dev/' '{print $2}'", "r");

    // в цикле читаем имена всех текстовых файлов
    char s[100];
    while (fgets(s,sizeof(s),fp))
    {
        s[strlen(s)-1]=','; // вставляем разделитель
        strcat(listCam,s);
    }
    listCam[strlen(listCam)-1]='\0';

    // закрываем поток
    pclose(fp);
    return(listCam);
}



        // Обработка данных принятых из сокета


int parsingsockdata(unsigned char *sockdata, int *sockdatasize, char **arraycommands)
{
    static char savebuf[1024] = {0}; // буфер для хранения склеенных или неполных пакетов
    static int savebufsize = 0; // размер буфера хранения неполных пакетов
    static int offset = 0;     // смещение относительно начала буфера хранения
    int exdatasize = 0; // размер ожидаемых данных
    unsigned char onlydata[1024] = {0};  // массив для пакета без заголовка

    if (*sockdatasize !=0) // если имеются новые данные начинаем обработку
    {
        if (sockdata[0] == '#') // если обнаружен заголовог пакета...
        {
            if (savebufsize != 0) // если буфер хранения не пуст, то очищаем его
            {
                memset(savebuf, 0, sizeof(savebufsize));
                savebufsize = 0; // обнуляем размер буфера хранения
                exdatasize = 0; // обнуляем размер ожидаемых данных
                offset = 0; // обнуляем смещение
                writeLog("parsingsockdata: получен новый пакет, но предъидущий пакет принят не полностью");
                cout << "parsingsockdata: получен новый пакет, но предъидущий пакет принят не полностью" << endl;
            }
            exdatasize = sockdata[1] << 8 | sockdata[2]; // определяем размер ожидаемых данных
            if (exdatasize == *sockdatasize) // делаем вывод, что данные не склеены и пришли полностью
            {
                memcpy(onlydata, &sockdata[3], exdatasize-3); // копируем пакет данных без заголовка
            }
            else //если данные склеены или не полные, помещаем их в буфер хранения
            {
                memcpy(savebuf, sockdata, *sockdatasize);
                savebufsize = *sockdatasize; // запоминаем размер буфера хранения
            }
        }
        else  // если пришел пакет данных без заголовка, добавляем его в буфер хранения
        {
            memcpy(&savebuf[savebufsize+offset], sockdata, *sockdatasize);
            savebufsize += *sockdatasize; // увеличиваем размер буфера хранения
            writeLog("parsingsockdata: получена часть пакета без заголовка");
            cout << "parsingsockdata: получена часть пакета без заголовка" << endl;
        }
        *sockdatasize = 0; // указываем на то, что вновь пришедшие данные обработаны
    }


    if (savebufsize != 0) // если буфер хранения не пуст, обрабатываем имеющиеся там данные
    {
        if (savebuf[0+offset] != '#') // если нарушена структура данных в буфере хранения
        {
            memset(savebuf, 0, sizeof(savebufsize));
            savebufsize = 0; // обнуляем размер буфера хранения
            exdatasize = 0; // обнуляем размер ожидаемых данных
            offset = 0; // обнуляем смещение
            writeLog("parsingsockdata: нарушена структура данных в буфере хранения");
            cout << "parsingsockdata: нарушена структура данных в буфере хранения" << endl;
            return(0);
        }
        exdatasize = savebuf[1+offset] << 8 | savebuf[2+offset]; // определяем размер ожидаемых данных

        if (savebufsize == exdatasize)  // если буфер хранения стал равен размеру ожидаемых данных...
        {
            memcpy(onlydata, &savebuf[3+offset], exdatasize-3); // копируем пакет данных без заголовка
            memset(savebuf, 0, sizeof(savebuf)); // очищаем буфер хранения
            savebufsize = 0; // обнуляем размер буфера хранения
            exdatasize = 0; // обнуляем размер ожидаемых данных
            offset = 0; // обнуляем смещение
        }
        else if (savebufsize > exdatasize) // делаем вывод, что данные в буфере склеены
        {
            memcpy(onlydata, &savebuf[3+offset], exdatasize-3); // копируем пакет данных без заголовка
            offset += exdatasize;  // запоминаем смещение относительно начала буфера
            savebufsize-=exdatasize; // уменьшаем размер буфера хранения
            writeLog("parsingsockdata: обнаружена склейка пакетов");
            cout << "parsingsockdata: обнаружена склейка пакетов" << endl;
        }
        else if (savebufsize < exdatasize) // делаем вывод, что данные в буфере не полные
        {
            writeLog("parsingsockdata: получен неполный пакет");
            cout << "parsingsockdata: получен неполный пакет" << endl;
            return 0;
        }
    }


    if (onlydata[0] != '\0')  // если имеются данные для заполнения массива команд, заполняем его
    {
        int i = 0;
        for( char * p = strtok( onlydata, "|" ); p; p = strtok( NULL, "|" ) )
        {
            arraycommands[i++] = p;  //  заполняем массив команд
        }
        return 1;
    }

    return 0;
}


    // разбор полученной строки на параметры

char **getParam(unsigned char *strParams)
{
    char *arrayParams;
    int offset = 1;


    while (strParams[offset])
    {

    }



}

    // функция записи в лог файл

int writeLog(const char *msg)
{
    FILE * pLog;
    pLog = fopen(pathlog, "a");
    if(pLog == NULL)
    {
        return 1;
    }
    char str[312];
    bzero(str, 312);
    strcpy(str, getTime());
    strcat(str, " ");
    strcat(str, msg);
    strcat(str, "\n");
    fputs(str, pLog);
    fclose(pLog);
    return 0;
}

