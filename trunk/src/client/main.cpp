#include <QLibrary>
#include <QApplication>
#include "pluginmanager.h"

int main(int argc, char *argv[])
{
  QApplication app(argc, argv);
  app.setQuitOnLastWindowClosed(false);
  app.addLibraryPath(app.applicationDirPath());
  QLibrary utils(app.applicationDirPath()+"/utils",&app);
  utils.load();
  PluginManager pm(&app);
  pm.restart();
  return app.exec();
}
