#-------------------------------------------------
#
# Project created by QtCreator 2011-12-07T18:29:27
#
#-------------------------------------------------

QT       -= core gui

TARGET = at-modem-manager
TEMPLATE = lib

DEFINES += ATMODEMMANAGER_LIBRARY

QMAKE_CXXFLAGS += -Wno-unused-result

SOURCES += ModemAudioManager.cpp \
    DualSimModemAudioManager.cpp \
    ModemAudioManagerInstance.cpp \
    ../simulation/cutils/sockets.c

HEADERS += \
    ../simulation/utils/Log.h \
    ../simulation/cutils/log.h \
    DualSimModemAudioManager.h \
    ModemAudioManager.h \
    ModemAudioManagerFactory.h \
    ModemStatusNotifier.h \
    ../simulation/cutils/sockets.h \
    EventNotifier.h

INCLUDEPATH += ../simulation ../../utility/event-listener ../at-parser ../tty-handler ../at-manager ../audio-at-manager

CONFIG(debug, debug|release) {
    DESTDIR = ../build/debug
} else {
    DESTDIR = ../build/release
}

LIBS += -L$$DESTDIR -levent-listener -lat-parser -ltty-handler -lrt -lat-manager -laudio-at-manager
OTHER_FILES += \
    Android.mk \
























