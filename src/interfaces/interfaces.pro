include(../config.inc)
include(../install.inc)

TEMPLATE = subdirs

sdk_interfaces.path   = $$INSTALL_INCLUDES/interfaces
sdk_interfaces.files  = *.h
INSTALLS             += sdk_interfaces
