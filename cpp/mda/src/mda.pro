QT += core
QT -= gui

CONFIG += c++11

DESTDIR = ../bin
OBJECTS_DIR = ../build
MOC_DIR=../build
TARGET = mda
TEMPLATE = app

HEADERS +=

SOURCES += mdamain.cpp

CONFIG += mlcommon
include(../../installbin.pri)
