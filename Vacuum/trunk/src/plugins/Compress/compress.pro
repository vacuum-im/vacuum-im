include(../plugins.inc)
	
QT += network xml 

LIBS += -l../../libs/utils 

TARGET = compress 

include(compress.pri) 
