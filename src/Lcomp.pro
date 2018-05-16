#-------------------------------------------------
#
# Project created by QtCreator 2016-07-12T14:14:02
#
#-------------------------------------------------

QT       += core gui
#
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
greaterThan(QT_MAJOR_VERSION, 4): QT += printsupport
TARGET = Lcomp
TEMPLATE = app

!win32::DEFINES+=LCOMP_LINUX
SOURCES += main.cpp\
           mainwindow.cpp \
           sensor.cpp \
           LCard.cpp \
           qcustomplot.cpp \

win32:SOURCES +=../include/create.cpp


HEADERS  += mainwindow.h \
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
    ../include/stubs.h \
    sensor.h \
    LCard.h \
    qcustomplot.h



FORMS    += mainwindow.ui \
            adc_setting.ui

DISTFILES += \
RESOURCES+= ../Res/Logo.ico
!win32:LIBS += -ldl
# -fPIC
