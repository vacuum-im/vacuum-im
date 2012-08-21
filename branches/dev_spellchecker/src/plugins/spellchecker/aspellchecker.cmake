add_definitions(-DHAVE_ASPELL) 

set(SOURCES ${SOURCES} "aspellchecker.cpp") 
set(HEADERS ${HEADERS} "aspellchecker.h")
set(MOC_HEADERS ${MOC_HEADERS} "aspellchecker.h") 


if (IS_ENABLED)
	if (WIN32)
		#FIXME: ${ASPELL_DEV_PATH}/lib <- setup aditional lib directory
		target_link_libraries(spellchecker aspell-15)
		include_directories(AFTER "${ASPELL_DEV_PATH}/include") 
	else (WIN32)
		target_link_libraries(spellchecker aspell)
	endif (WIN32)
endif (IS_ENABLED)  
