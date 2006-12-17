TEMPLATE = lib

CONFIG += dll

QT += network xml

DEFINES += UTILS_DLL

DLLDESTDIR = ../..

DESTDIR = ../libs

TARGET = utils

include(utils.pri)