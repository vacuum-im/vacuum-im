include(../plugins.inc)

QT += network xml

LIBS += -l../../libs/utils

TARGET = saslbind

include(saslbind.pri)