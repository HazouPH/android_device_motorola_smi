#-------------------------------------------------
#
# Project created by QtCreator 2011-12-11T17:06:15
#
#-------------------------------------------------

QT       -= core gui

TARGET = tty-handler
TEMPLATE = lib

SOURCES += TtyHandler.cpp

HEADERS += TtyHandler.h

CONFIG(debug, debug|release) {
    DESTDIR = ../build/debug
} else {
    DESTDIR = ../build/release
}

LIBS += -L$$DESTDIR

OTHER_FILES += \
    Android.mk


