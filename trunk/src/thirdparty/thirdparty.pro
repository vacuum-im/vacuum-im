TEMPLATE    = subdirs
unix:!macx {
  SUBDIRS   = minizip idn qtlockedfile
} else {
  SUBDIRS   = zlib minizip idn qtlockedfile
}
