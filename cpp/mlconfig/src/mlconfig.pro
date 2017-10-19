QT += core
QT -= gui

CONFIG += c++11
CONFIG += mlcommon

DESTDIR = ../bin
OBJECTS_DIR = ../build
MOC_DIR= ../build
TARGET = mlconfig
TEMPLATE = app

HEADERS += \
    mlconfigpage.h \
    mlconfigquestion.h

SOURCES += mlconfigmain.cpp \
    mlconfigpage.cpp \
    mlconfigquestion.cpp


include(../../installbin.pri)
