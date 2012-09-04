if (INSTALL_SDK)
	install(FILES "config.cmake" "config.inc"
		DESTINATION "${INSTALL_INCLUDES}"
		COMPONENT sdk)
endif (INSTALL_SDK)
