#-------------------------------------------------
#
# Project created by QtCreator 2011-12-11T14:11:11
#
#-------------------------------------------------

QT       -= core gui

TARGET = at-parser
TEMPLATE = lib

SOURCES += ATParser.cpp

HEADERS += ATParser.h

INCLUDEPATH += ../simulation

CONFIG(debug, debug|release) {
    DESTDIR = ../build/debug
} else {
    DESTDIR = ../build/release
}

LIBS += -L$$DESTDIR

OTHER_FILES += \
    Android.mk


