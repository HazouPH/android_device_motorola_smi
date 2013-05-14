#-------------------------------------------------
#
# Project created by QtCreator 2011-12-11T10:35:08
#
#-------------------------------------------------

QT       -= core gui

TARGET = event-listener
TEMPLATE = lib

QMAKE_CXXFLAGS += -Wno-unused-result

DEFINES += EVENTLISTENER_LIBRARY

SOURCES += \
    EventThread.cpp

HEADERS += \
    EventThread.h \
    EventListener.h

INCLUDEPATH += ../simulation

CONFIG(debug, debug|release) {
    DESTDIR = ../build/debug
} else {
    DESTDIR = ../build/release
}

LIBS += -L$$DESTDIR

OTHER_FILES += \
    Android.mk






