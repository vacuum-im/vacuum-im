#ifndef DEF_CONFIG_H
#define DEF_CONFIG_H

#if defined Q_WS_WIN
#  define PLUGINS_DIR           "./plugins"
#  define RESOURCES_DIR         "./resources"
#  define TRANSLATIONS_DIR      "./translations"
#elif defined Q_WS_X11
#  define PLUGINS_DIR           "../lib/vacuum/plugins"
#  define RESOURCES_DIR         "../share/vacuum/resources"
#  define TRANSLATIONS_DIR      "../share/vacuum/translations"
#elif defined Q_WS_MAC
#  define PLUGINS_DIR           "../PlugIns"
#  define RESOURCES_DIR         "../Resources"
#  define TRANSLATIONS_DIR      "../Resources/translations"
#endif

#endif
