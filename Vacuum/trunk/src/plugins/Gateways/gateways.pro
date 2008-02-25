include(../plugins.inc)

QT += network xml

LIBS += -l../../libs/utils

TARGET = gateways

include(gateways.pri)