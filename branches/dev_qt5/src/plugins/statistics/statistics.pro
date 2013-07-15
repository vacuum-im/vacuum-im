QT += webkit

isEqual(QT_MAJOR_VERSION, 5) { QT += webkitwidgets }

QT -= phonon xmlpatterns
TARGET = statistics 
include(statistics.pri) 
include(../plugins.inc) 
