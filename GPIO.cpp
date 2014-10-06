#include "server.h"  // мой заголовочныйный файл

#define GORIZONTSERVOPIN RPI_GPIO_P1_07



// Инициализация GPIO

//    if (!bcm2835_init()) cout << "Ошибка инициализации GPIO\n";
//    else cout << "GPIO инициализировано\n";

//    int id1, id2, result;
//    pthread_t thread1, thread2;

//    id1 = 1;
//    result = pthread_create(&thread1, NULL, thread_func, &id1);
//    if (result != 0) perror("Creating the first thread");
//    else cout << "Создан первый поток\n";

//    id2 = 2;
//    result = pthread_create(&thread2, NULL, thread_func, &id2);
//    if (result != 0) perror("Creating the second thread");
//    else cout << "Создан второй поток\n";


//    saveGorizontServoAngle = (MINSERVOANGLE + MAXSERVOANGLE)/2; // устанавливаем среднее положение сервы

//    cout << "setServoAnfle: " << (int)saveGorizontServoAngle <<endl;




//void * thread_func(void *arg)
//{

//    bcm2835_gpio_fsel(GORIZONTSERVOPIN, BCM2835_GPIO_FSEL_OUTP);     //Устанавливаем порт Р1_03 на вывод

//    int id = *(int *)arg;

//    while(1)
//    {
//        if (id == 1)
//        {
//            bcm2835_gpio_write(GORIZONTSERVOPIN, HIGH); // подаем импульс

//            delayMicroseconds((saveGorizontServoAngle * 11)+500);      // ждем. (пауза равна ширине импульса)

//            bcm2835_gpio_write(GORIZONTSERVOPIN, LOW); // снимаем импульс
//        }


//        delayMicroseconds(20000);      // ждём конца периода
//    }

//    return 0;
//}
