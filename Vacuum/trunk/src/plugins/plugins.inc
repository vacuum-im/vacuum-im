TEMPLATE = lib
CONFIG += plugin
QT += network xml
DLLDESTDIR = ../../../plugins
LIBS += -l../../libs/utils

#Translations
include(../translations.inc)