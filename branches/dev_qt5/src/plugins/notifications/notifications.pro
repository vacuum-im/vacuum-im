TARGET = notifications

isEqual(QT_MAJOR_VERSION, 5) { QT += multimedia }

include(notifications.pri)
include(../plugins.inc)
