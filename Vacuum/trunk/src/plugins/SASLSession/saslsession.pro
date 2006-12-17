include(../plugins.inc)

QT += network xml

LIBS += -l../../libs/utils

TARGET = saslsession

include(saslsession.pri)