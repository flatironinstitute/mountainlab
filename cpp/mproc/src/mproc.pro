QT = core qml

CONFIG += c++11

CONFIG += mlcommon

DESTDIR = ../bin
OBJECTS_DIR = ../build
MOC_DIR=../build
TARGET = mproc
TEMPLATE = app

HEADERS += mprocmain.h \
    processormanager.h \
    processresourcemonitor.h \
    handle_request.h mllogmaster.h
SOURCES += mprocmain.cpp \
    processormanager.cpp \
    processresourcemonitor.cpp \
    handle_request.cpp mllogmaster.cpp

EXTRA_INSTALLS = # (clear it out) used by installbin.pri
EXTRA_INSTALLS += "$$PWD/../bin_extra/*"
include(../../installbin.pri)
