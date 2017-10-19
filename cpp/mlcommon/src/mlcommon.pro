QT = core network

DEFINES += USE_REMOTE_READ_MDA
DEFINES += MOUNTAINLAB_SRC_PATH=\\\"$$PWD/../../..\\\"

CONFIG += c++11
CONFIG += staticlib

DESTDIR = ../lib
OBJECTS_DIR = ../build
MOC_DIR=../build
TARGET = mlcommon
TEMPLATE = lib

INCLUDEPATH += ../include
VPATH += ../include
HEADERS += mlcommon.h sumit.h \
    ../include/mda/mda32.h \
    ../include/mda/diskreadmda32.h \
    ../include/mda/mda_p.h \
    ../include/mliterator.h \
    ../include/mda/mdareader.h \
    ../include/mda/mdareader_p.h \
    ../include/objectregistry.h \
    ../include/mlprivate.h \
    ../include/icounter.h \
    ../include/qprocessmanager.h \
    ../include/signalhandler.h \
    ../include/mllog.h \
    ../include/tracing/tracing.h \
    ../include/mlvector.h \
    ../include/get_sort_indices.h

SOURCES += \
    mlcommon.cpp sumit.cpp \
    mda/mda32.cpp \
    mda/diskreadmda32.cpp \
    objectregistry.cpp \
    mda/mdareader.cpp \
    componentmanager/icomponent.cpp \
    icounter.cpp \
    qprocessmanager.cpp \
    signalhandler.cpp \
    mllog.cpp \
    tracing/tracing.cpp \
    mlvector.cpp \
    get_sort_indices.cpp

INCLUDEPATH += ../include/mda
VPATH += ../include/mda
VPATH += mda
HEADERS += diskreadmda.h diskwritemda.h mda.h mdaio.h remotereadmda.h usagetracking.h
SOURCES += diskreadmda.cpp diskwritemda.cpp mda.cpp mdaio.cpp remotereadmda.cpp usagetracking.cpp

INCLUDEPATH += ../include/cachemanager
VPATH += ../include/cachemanager
VPATH += cachemanager
HEADERS += cachemanager.h
SOURCES += cachemanager.cpp

INCLUDEPATH += ../include/taskprogress
VPATH += ../include/taskprogress
VPATH += taskprogress
HEADERS += taskprogress.h
SOURCES += taskprogress.cpp

INCLUDEPATH += ../include/mlnetwork
VPATH += ../include/mlnetwork
VPATH += mlnetwork
HEADERS += mlnetwork.h
SOURCES += mlnetwork.cpp

INCLUDEPATH += ../include/componentmanager
VPATH += ../include/componentmanager
VPATH += componentmanager
HEADERS += componentmanager.h icomponent.h
SOURCES += componentmanager.cpp


DISTFILES += \
    ../taskprogress.pri \
    ../mlnetwork.pri
