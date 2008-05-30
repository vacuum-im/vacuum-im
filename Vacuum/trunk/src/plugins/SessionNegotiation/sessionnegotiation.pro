include(../plugins.inc)

QT += network xml

LIBS += -l../../libs/utils

TARGET = sessionnegotiation

include(sessionnegotiation.pri)