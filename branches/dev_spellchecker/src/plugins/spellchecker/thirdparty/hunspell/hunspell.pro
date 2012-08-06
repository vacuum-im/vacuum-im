TARGET   = hunspell
TEMPLATE = lib
CONFIG  -= qt
CONFIG  += staticlib warn_off
DEFINES += HUNSPELL_STATIC

debug {
DESTDIR = debug 
} else {
DESTDIR = release
}

SOURCES += \
    affentry.cxx \
	affixmgr.cxx \
	csutil.cxx \
	dictmgr.cxx \
	filemgr.cxx \
	hashmgr.cxx \
	hunspell.cxx \
	hunzip.cxx \
	phonet.cxx \
	replist.cxx \
	suggestmgr.cxx

HEADERS += \  
    affentry.hxx \
	affixmgr.hxx \
	atypes.hxx \
	baseaffix.hxx \
	config.h \
	csutil.hxx \
	dictmgr.hxx \
	filemgr.hxx \
	hashmgr.hxx \
	htypes.hxx \
	hunspell.hxx \
	hunspell.h \
	hunzip.hxx \
	langnum.hxx \
	phonet.hxx \
	replist.hxx \
	suggestmgr.hxx \
	w_char.hxx \
