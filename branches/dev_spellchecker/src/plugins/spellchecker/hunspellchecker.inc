DEFINES       += HAVE_HUNSPELL
DEFINES       += HUNSPELL_STATIC
LIBS          += -L../../libs
LIBS          += -lhunspell
HEADERS       += hunspellchecker.h
SOURCES       += hunspellchecker.cpp
INCLUDEPATH   += ../../thirdparty/hunspell
