include(../plugins.inc)

QT += network xml

LIBS += -l../../libs/utils

TARGET = xmppstreams

include(xmppstreams.pri)