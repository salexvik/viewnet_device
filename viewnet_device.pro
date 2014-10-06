TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt


INCLUDEPATH += /home/salexvik/VirtualBox_Shared/libs/bcm2835/bcm2835-1.15_for_pi/src

LIBS += -pthread
LIBS += -lrt
LIBS += -L/home/salexvik/VirtualBox_Shared/libs/bcm2835/bcm2835-1.15_for_pi/src -lbcm2835

SOURCES += main.cpp \
    socket.cpp \
    video.cpp \
    GPIO.cpp \
    md5.cpp \
    audio.cpp \
    XML.cpp \
    RTSP.cpp \
    recordMedia.cpp \
    GCM.cpp \
    phones.cpp \
    other.cpp \
    controller.cpp \
    uart.cpp \
    timers.cpp \
    modem.cpp

HEADERS += \
    server.h \
    md5.h \
    xml.h

