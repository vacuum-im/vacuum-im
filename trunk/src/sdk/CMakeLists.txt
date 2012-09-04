if (INSTALL_SDK)
	install(FILES "sdkconfig.cmake" "sdkconfig.inc"
		DESTINATION "${INSTALL_INCLUDES}"
		COMPONENT sdk)
endif (INSTALL_SDK)
