add_subdirectory(thirdparty)
add_subdirectory(utils)
add_subdirectory(loader)
add_subdirectory(plugins)
add_subdirectory(definitions)
add_subdirectory(interfaces)

if (INSTALL_SDK)
	install(FILES "sdkconfig.cmake" "sdkconfig.inc"
		DESTINATION "${INSTALL_INCLUDES}"
		COMPONENT sdk)
endif (INSTALL_SDK)
