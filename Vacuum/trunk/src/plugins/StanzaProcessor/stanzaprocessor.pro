include(../plugins.inc)

QT += network xml

LIBS += -l../../libs/utils

TARGET = stanzaprocessor

include(stanzaprocessor.pri)