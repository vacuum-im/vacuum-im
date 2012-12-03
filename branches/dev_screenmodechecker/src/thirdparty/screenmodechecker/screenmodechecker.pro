include(../../make/config.inc)

TARGET         = screenmodechecker
TEMPLATE       = lib
CONFIG        += staticlib
DESTDIR        = ../../libs

unix:!macx:!haiku {
  DEFINES     += HAVE_XSS
}

include(screenmodechecker.pri)
