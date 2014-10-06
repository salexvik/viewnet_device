#ifndef SERVER_H
#define SERVER_H

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <algorithm>
#include <set>
#include <termios.h> /* Объявления управления POSIX-терминалом */
#include <iostream>
#include <sstream>
#include <string.h>  /* Объявления строковых функций */
#include <signal.h>
#include <errno.h>
#include <wait.h>
#include <time.h>
#include <stdlib.h>
#include <pthread.h>
#include <bcm2835.h>
#include <getopt.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <map>
#include <fstream>
#include <vector>

#include <arpa/inet.h>
#include <netdb.h>

using namespace std;

    // объявления функций

void sig_chld(int signo);
int openTcpPort(string aPort);
void sendClient(string aOutData);
void sendAllClients(string  aOutData);
string getAliasesVideoDevice();
string getAliasesAudioDevice();
bool getSocketPacket(string &aPacketRef, string &aSocketDataRef);
bool getControllerPacket(string &aPacketRef, string &aComPortData);
void log(string msg);
int openUartPort(string portName);
int startRtspServer(string aMediaStreams);
string getMyName();
void terminate(int dummy);
bool startSendSmsThread(string aMsg);
bool startSendGcmThread(string aMsg);
string getConfig();
bool saveConfig();
bool changePassword(string aUsername, string aPassword);
bool redirectSocket(int aFdSock, int aTargetPort);
string trim(string aStringTrim);
bool existFileConfig();
string getDateTime(string aFormat);
string getDate(string aFormat);
string getTime(string aFormat);
bool startRecord(string aMediaStreamId);
string intToString(int i);
void sendMediaStreamsDataToView();
void sendMediaStreamsDataToRecord();
void setRecordStatus(string aMediaStreamId, string aRecordStatus);
void killRecord(string aMediaStreamId);
void changePhones(string aData);
void sendController(string aOutData);
bool startTimerThreads();
string getVideoDeviceName(string aAlias);
string getAudioDeviceName(string aAlias);
void internetConnect();
string getPopenString(string popenLine, bool newLine);
vector<string> getPopenList(string popenLine, bool newLine);
void getSimInfo();
void sendSimInfo();
void autoStartRecord();
bool checkAvailabilityDevices(string aMediaStream, string &aBusyMediaStreamId);
bool startControllerReadPortThread();
bool startRtspReadCannelThread();
bool startUssdRequestThread(string aUssdCommand);
void modemMutexInit();
string getRandomString(size_t N);
void sendNotification(string aTypeMsg, string aMsg);


    // объявления глобальных констант

const int PACKSIZE = 1024; // размер массива для данных из сокета

const float MINSERVOVALUE = 0.040f; // не меньше нуля
const float MAXSERVOVALUE = 0.109f; // не больше единицы


const string START_TAG = "<packet>";
const string END_TAG = "</packet>";
const int START_TAG_SIZE = 8;
const int END_TAG_SIZE = 9;

 // объявление глобальных переменных

extern string pathConfig; // путь к файлу конфигурации
extern string config; // строка с конфигом сервера в формате XML
extern int fd_listener;  // дескриптор слушающего сокета
extern set<int> fd_clients;  // массив дескрипторов клиентов
extern int fd_currentClient; // дескриптор текущего клиента
extern map<string, int> recordPidMap; // массив для соответствия идентификатора строки настроек записи и PID процесса записи медиаданных
extern map<string, string> recordStatusMap; // массив статусов медиапотоков записи



#endif // SERVER_H
