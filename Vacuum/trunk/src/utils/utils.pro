TEMPLATE = lib

CONFIG += dll

QT += network xml

LIBS += -l../libs/minizip

DEFINES += UTILS_DLL

DLLDESTDIR = ../..

DESTDIR = ../libs

TARGET = utils

include(utils.pri)