include(../../make/config.inc)

TARGET         = idle
TEMPLATE       = lib
CONFIG        += staticlib warn_off
DESTDIR        = ../../libs
unix:!macx:!haiku {
  DEFINES     += HAVE_XSS
  INCLUDEPATH += ../../
}
include(idle.pri)
