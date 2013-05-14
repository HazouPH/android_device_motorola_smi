#-------------------------------------------------
#
# Project created by QtCreator 2011-12-11T10:41:31
#
#-------------------------------------------------

QT       += core

QT       -= gui

TARGET = modem-simulator
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app


SOURCES += main.cpp \
    ModemSimulator.cpp

INCLUDEPATH += ../libamc ../../utility/event-listener ../simulation ../at-parser ../tty-handler

CONFIG(debug, debug|release) {
    DESTDIR = ../build/debug
} else {
    DESTDIR = ../build/release
}

LIBS += -L$$DESTDIR -levent-listener -lat-parser -ltty-handler


HEADERS += \
    ModemSimulator.h


