include(../plugins.inc)

QT += network xml

LIBS += -l../../libs/utils

TARGET = messagearchiver

include(messagearchiver.pri)