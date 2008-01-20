include(../plugins.inc)

QT += network xml

LIBS += -l../../libs/utils

TARGET = servicediscovery

include(servicediscovery.pri)
