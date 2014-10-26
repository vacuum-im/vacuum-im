include(config.inc)

TEMPLATE = subdirs

sdk_make.path   = $$INSTALL_INCLUDES/make
sdk_make.files  = config.inc config.cmake translations.inc translations.cmake
INSTALLS       += sdk_make
