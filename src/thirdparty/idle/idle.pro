include(../../config.inc)

TARGET         = idle
TEMPLATE       = lib
CONFIG        += staticlib warn_off
DESTDIR        = ../../libs
unix:!macx {
  DEFINES     += HAVE_XSS
}
include(idle.pri)
