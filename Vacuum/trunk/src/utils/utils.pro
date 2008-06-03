TEMPLATE = lib

CONFIG += dll

QT += network xml

LIBS += -l../libs/minizip
LIBS += -l../libs/idn

DEFINES += UTILS_DLL

DLLDESTDIR = ../..

DESTDIR = ../libs

TARGET = utils

include(utils.pri)