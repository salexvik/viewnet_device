#include "server.h"  // мой заголовочныйный файл
#include "xml.h"

void *handleSendGcmThread(void *aText) {

    // получение необходимых данных для отправки СМС

    string host = "elektroserver.webhop.net";
    string script = "GCM.php";
    string regId = "APA91bFbwYKoiBm06bMRAYFgtvZWi30bIyFQ6AlTuF09fs8gFP831MwFPnkd9UBRpiwvHQKdKOYX6Z0WwwjJDygExtfqiCHgHxEZteCDYSEYNpaa8IkF37P8G-D12OYdDAVDVFtHOyMzGozcDy3R_fmKL1n-OQHbfA";
    string msg = (const char*)aText;

    // заменяем пробелы знаками "+"
    std::replace( msg.begin(), msg.end(), ' ', '+');

    cout <<    "message: " << msg << endl;
    string getRequest = "GET http://" +host +"/" +script +"?regId=" + regId +"&msg=" +msg + " HTTP/1.1\r\n" +
            "Host: " + host + "\r\n" +
            "Connection: Close\r\n" +
            "\r\n";


    int sockfd, portno, n;
        struct sockaddr_in serv_addr;
        struct hostent *server;

        char buffer[512];
        portno = 80;
        /* Create a socket point */
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0)
        {
            perror("ERROR opening socket");
            return 0;
        }
        server = gethostbyname(host.c_str());
        if (server == NULL) {
            fprintf(stderr,"ERROR, no such host\n");
            close(sockfd);
            return 0;
        }

        bzero((char *) &serv_addr, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        bcopy((char *)server->h_addr,
               (char *)&serv_addr.sin_addr.s_addr,
                    server->h_length);
        serv_addr.sin_port = htons(portno);

        /* Now connect to the server */
        if (connect(sockfd, (const sockaddr*)&serv_addr,sizeof(serv_addr)) < 0)
        {
             perror("ERROR connecting");
             close(sockfd);
             return 0;
        }
        bzero(buffer,512);

        strcat(buffer, getRequest.c_str());


        /* Send message to the server */
        n = write(sockfd,buffer,strlen(buffer));
        if (n < 0)
        {
             perror("ERROR writing to socket");
             close(sockfd);
             return 0;
        }
        /* Now read server response */
        bzero(buffer,512);
        n = read(sockfd,buffer,511);
        if (n < 0)
        {
             perror("ERROR reading from socket");
             close(sockfd);
             return 0;
        }
        printf("%s\n",buffer);
        close(sockfd);
        return 0;
}







// Создание потока

bool startSendGcmThread(string aMsg) {
    static string msg;

    msg = aMsg;

    pthread_t thread;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    return !pthread_create(&thread, &attr, handleSendGcmThread, (void*)msg.c_str());
}
