cmake_minimum_required(VERSION 2.8)

project(vacuum-im)

find_package(Qt4 REQUIRED)

add_custom_target(updatets)

include("config.cmake")
install(DIRECTORY "resources/menuicons"
	DESTINATION "${INSTALL_RESOURCES}"
	PATTERN ".svn" EXCLUDE)
install(FILES "AUTHORS" "CHANGELOG" "README" "COPYING"
	DESTINATION "${INSTALL_DOCUMENTS}")

add_subdirectory(src)
