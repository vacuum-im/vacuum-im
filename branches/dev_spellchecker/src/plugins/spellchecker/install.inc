win32 {
  INSTALL_BINS             = $$INSTALL_PREFIX/$$INSTALL_APP_DIR
  INSTALL_LIBS             = $$INSTALL_PREFIX/$$INSTALL_APP_DIR/$$INSTALL_LIB_DIR
  INSTALL_PLUGINS          = $$INSTALL_PREFIX/$$INSTALL_APP_DIR/$$INSTALL_LIB_DIR/plugins
  INSTALL_RESOURCES        = $$INSTALL_PREFIX/$$INSTALL_APP_DIR/$$INSTALL_RES_DIR/resources
  INSTALL_DOCUMENTS        = $$INSTALL_PREFIX/$$INSTALL_APP_DIR/$${TARGET}
  INSTALL_TRANSLATIONS     = $$INSTALL_PREFIX/$$INSTALL_APP_DIR/$$INSTALL_RES_DIR/translations
} else:unix:!macx {
  INSTALL_BINS             = $$INSTALL_PREFIX/bin
  INSTALL_LIBS             = $$INSTALL_PREFIX/$$INSTALL_LIB_DIR
  INSTALL_PLUGINS          = $$INSTALL_PREFIX/$$INSTALL_LIB_DIR/$$INSTALL_APP_DIR/plugins
  INSTALL_RESOURCES        = $$INSTALL_PREFIX/$$INSTALL_RES_DIR/$$INSTALL_APP_DIR/resources
  INSTALL_DOCUMENTS        = $$INSTALL_PREFIX/$$INSTALL_RES_DIR/doc/$$INSTALL_APP_DIR/$${TARGET}
  INSTALL_TRANSLATIONS     = $$INSTALL_PREFIX/$$INSTALL_RES_DIR/$$INSTALL_APP_DIR/translations
} else:macx {
  INSTALL_BINS             = $$INSTALL_PREFIX
  INSTALL_LIBS             = $$INSTALL_PREFIX/$$INSTALL_APP_DIR/Contents/$$INSTALL_LIB_DIR
  INSTALL_PLUGINS          = $$INSTALL_PREFIX/$$INSTALL_APP_DIR/Contents/PlugIns
  INSTALL_RESOURCES        = $$INSTALL_PREFIX/$$INSTALL_APP_DIR/Contents/$$INSTALL_RES_DIR
  INSTALL_DOCUMENTS        = $$INSTALL_PREFIX/$$INSTALL_APP_DIR/Contents/$$INSTALL_RES_DIR/$${TARGET}
  INSTALL_TRANSLATIONS     = $$INSTALL_PREFIX/$$INSTALL_APP_DIR/Contents/$$INSTALL_RES_DIR/translations
}

#Install
target.path         = $$INSTALL_PLUGINS
resources.path      = $$INSTALL_RESOURCES
resources.files     = ./resources/*
INSTALLS           += target resources

#MaxOS Install
macx {
  name_tool.path        = $$INSTALL_PLUGINS
  name_tool.extra       = install_name_tool -change lib$${TARGET_UTILS}.1.dylib @executable_path/../Frameworks/lib$${TARGET_UTILS}.1.dylib $(INSTALL_ROOT)$$INSTALL_PLUGINS/lib$${TARGET}.dylib
  name_tool_QtGui.path  = $$INSTALL_PLUGINS
  name_tool_QtGui.extra = install_name_tool -change QtGui.framework/Versions/4/QtGui @executable_path/../Frameworks/QtGui.framework/Versions/4/QtGui $(INSTALL_ROOT)$$INSTALL_PLUGINS/lib$${TARGET}.dylib
  name_tool_QtCore.path  = $$INSTALL_PLUGINS
  name_tool_QtCore.extra = install_name_tool -change QtCore.framework/Versions/4/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/4/QtCore $(INSTALL_ROOT)$$INSTALL_PLUGINS/lib$${TARGET}.dylib
  name_tool_QtXml.path  = $$INSTALL_PLUGINS
  name_tool_QtXml.extra = install_name_tool -change QtXml.framework/Versions/4/QtXml @executable_path/../Frameworks/QtXml.framework/Versions/4/QtXml $(INSTALL_ROOT)$$INSTALL_PLUGINS/lib$${TARGET}.dylib

  INSTALLS         += name_tool name_tool_QtGui name_tool_QtCore name_tool_QtXml
}
