QT = core network
CONFIG += c++11

DESTDIR = ../bin
OBJECTS_DIR = ../build
MOC_DIR=../build
TARGET = prv
TEMPLATE = app

SOURCES += prvmain.cpp \
    prvfile.cpp

CONFIG += mlcommon taskprogress mlnetwork

#AUX.files=bin/prv-create bin/prv-download
#AUX.path=/bin
#INSTALLS += AUX

EXTRA_INSTALLS = # (clear it out) used by installbin.pri
EXTRA_INSTALLS += "$$PWD/../bin_extra/*"

include(../../installbin.pri)
