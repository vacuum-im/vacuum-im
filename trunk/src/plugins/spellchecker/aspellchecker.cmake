add_definitions(-DHAVE_ASPELL) 

if (WIN32)
	#FIXME: ${ASPELL_DEV_PATH}/lib <- setup aditional lib directory
	set(ADD_LIBS aspell-15)
	include_directories(AFTER "${ASPELL_DEV_PATH}/include")
else (WIN32)
	set(ADD_LIBS aspell)
endif (WIN32)

set(SOURCES ${SOURCES} "aspellchecker.cpp")
set(HEADERS ${HEADERS} "aspellchecker.h")
