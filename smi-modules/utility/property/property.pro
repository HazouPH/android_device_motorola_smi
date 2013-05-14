#-------------------------------------------------
#
# Project created by QtCreator 2012-07-07T11:14:03
#
#-------------------------------------------------

QT -= core gui

TARGET = property
TEMPLATE = lib

QMAKE_CXXFLAGS += -Wno-unused-result

DEFINES +=

SOURCES += \
    PropertyBase.cpp \
    Property.cpp \
    simulation/cutils/properties.cpp

HEADERS += \
    PropertyBase.h \
    Property.h \
    simulation/cutils/properties.h \
    simulation/utils/Log.h \
    PropertyTemplateInstanciations.h

INCLUDEPATH += simulation
DEPENDPATH += simulation

CONFIG(debug, debug|release) {
    DESTDIR = ../property-build/debug
} else {
    DESTDIR = ../property-build/release
}

LIBS += -L$$DESTDIR

OTHER_FILES += \
    Android.mk \
    getProp














