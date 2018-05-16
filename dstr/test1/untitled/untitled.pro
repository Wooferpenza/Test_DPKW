TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += main.cpp

DISTFILES += \
    ../include/conv_utf8

HEADERS += \
    ../include/791.h \
    ../include/791cmd.h \
    ../include/e140.h \
    ../include/e140cmd.h \
    ../include/e154.h \
    ../include/e154cmd.h \
    ../include/e440.h \
    ../include/e440cmd.h \
    ../include/e2010.h \
    ../include/e2010cmd.h \
    ../include/guiddef.h \
    ../include/ifc_ldev.h \
    ../include/ioctl.h \
    ../include/ldevbase.h \
    ../include/pcicmd.h \
    ../include/plx.h \
    ../include/stubs.h

QMAKE_LFLAGS += -L/usr/lib/nptl\
 -ldl\
 -lpthread
