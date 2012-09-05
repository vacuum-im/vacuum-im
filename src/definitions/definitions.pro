include(../make/config.inc)

TEMPLATE = subdirs

sdk_definitions.path   = $$INSTALL_INCLUDES/definitions
sdk_definitions.files  = *.h
INSTALLS              += sdk_definitions
