file(GLOB HEADERS "*.h")

if (INSTALL_SDK)
	install(FILES ${HEADERS}
		DESTINATION "${INSTALL_INCLUDES}/definitions"
		COMPONENT sdk)
endif (INSTALL_SDK)
