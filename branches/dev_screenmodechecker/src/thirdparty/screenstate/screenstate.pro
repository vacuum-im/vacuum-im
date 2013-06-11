include(../../make/config.inc)

TARGET         = screenstate
TEMPLATE       = lib
CONFIG        += staticlib
DESTDIR        = ../../libs

unix:!macx:!haiku {
  DEFINES     += HAVE_XSS
}

include(screenstate.pri)
