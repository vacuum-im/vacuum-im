TEMPLATE = lib
CONFIG += plugin
QT += network xml
LIBS += -L../../libs
LIBS += -lutils
DLLDESTDIR = ../../../plugins

#Translations
include(../translations.inc)