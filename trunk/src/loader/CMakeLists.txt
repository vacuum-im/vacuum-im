include("${CMAKE_SOURCE_DIR}/config.cmake")
include("loader.cmake")

project(loader CXX)

link_directories("${CMAKE_BINARY_DIR}/src/libs")
include_directories("${CMAKE_SOURCE_DIR}/src" "${CMAKE_BINARY_DIR}/src/loader")
set(QT_USE_QTXML YES)
include(${QT_USE_FILE})

add_translations(TRANSLATIONS "${TARGET_LOADER}" ${SOURCES} ${UIS})
add_executable(${TARGET_LOADER} ${SOURCES} ${MOC_SOURCES} ${UI_HEADERS} ${TRANSLATIONS})
set_target_properties(${TARGET_LOADER} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
target_link_libraries(${TARGET_LOADER} ${TARGET_UTILS} ${QT_LIBRARIES})
