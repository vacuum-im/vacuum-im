QT += webkit

isEqual(QT_MAJOR_VERSION, 5) { QT += webkitwidgets }

QT -= phonon xmlpatterns
TARGET = adiummessagestyle 
include(adiummessagestyle.pri) 
include(../plugins.inc) 
