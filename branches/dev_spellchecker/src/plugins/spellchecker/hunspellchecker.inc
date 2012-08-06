USE_HUNSPELL {
  DEFINES       += HAVE_HUNSPELL
  DEFINES       += HUNSPELL_STATIC
  INCLUDEPATH   += ./thirdparty/
  LIBS          += -lhunspell
  HEADERS       += hunspellchecker.h
  SOURCES       += hunspellchecker.cpp

  debug {
    LIBS += -L./thirdparty/hunspell/debug
  } else {
    LIBS += -L./thirdparty/hunspell/release
  }

}
