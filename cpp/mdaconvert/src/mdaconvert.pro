QT += core network
QT -= gui

CONFIG += c++11

DESTDIR = ../bin
OBJECTS_DIR = ../build
MOC_DIR=../build
TARGET = mdaconvert
TEMPLATE = app

HEADERS += \
    mdaconvert.h

SOURCES += mdaconvertmain.cpp \
    mdaconvert.cpp

CONFIG += mlcommon
include(../../installbin.pri)
