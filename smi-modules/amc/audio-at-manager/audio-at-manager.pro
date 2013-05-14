#-------------------------------------------------
#
# Project created by QtCreator 2011-12-07T18:29:27
#
#-------------------------------------------------

QT       -= core gui

TARGET = at-manager
TEMPLATE = lib

DEFINES += ATMANAGER_LIBRARY

QMAKE_CXXFLAGS += -Wno-unused-result

SOURCES += ATCommand.cpp \
    ATManager.cpp \
    PeriodicATCommand.cpp \
    UnsollicitedATCommand.cpp \
    ../simulation/cutils/sockets.c

HEADERS += ATManager.h \
    ../simulation/utils/Log.h \
    ../simulation/cutils/log.h \
    ../simulation/stmd.h \
    PeriodicATCommand.h \
    UnsollicitedATCommand.h \
    ../simulation/cutils/sockets.h \
    ATCommand.h \
    EventNotifier.h

INCLUDEPATH += ../simulation ../../utility/event-listener ../at-parser ../tty-handler

CONFIG(debug, debug|release) {
    DESTDIR = ../build/debug
} else {
    DESTDIR = ../build/release
}

LIBS += -L$$DESTDIR -levent-listener -lat-parser -ltty-handler -lrt
OTHER_FILES += \
    Android.mk \
























