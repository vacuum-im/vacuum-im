if (INSTALL_SDK)
	install(FILES "config.cmake" "config.inc" "translations.cmake" "translations.inc"
		DESTINATION "${INSTALL_INCLUDES}/make"
		COMPONENT sdk)
endif (INSTALL_SDK)
