TARGET = utils
TEMPLATE = lib
CONFIG += dll
QT += xml
LIBS += -l../libs/idn
LIBS += -l../libs/minizip
DEFINES += UTILS_DLL
DLLDESTDIR = ../..
DESTDIR = ../libs
include(utils.pri)

#Translations
include(../translations.inc)