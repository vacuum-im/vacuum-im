include(../config.inc)
include(../install.inc)

TEMPLATE = subdirs

sdk_make.path   = $$INSTALL_INCLUDES
sdk_make.files  = sdkconfig.inc sdkconfig.cmake 
INSTALLS       += sdk_make
