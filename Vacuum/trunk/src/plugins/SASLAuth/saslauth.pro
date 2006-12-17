include(../plugins.inc)

QT += network xml

LIBS += -l../../libs/utils

TARGET = saslauth

include(saslauth.pri)