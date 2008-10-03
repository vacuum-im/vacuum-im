#include <QApplication>
#include "pluginmanager.h"

int main(int argc, char *argv[])
{
  QApplication app(argc, argv);
  app.setQuitOnLastWindowClosed(false);
  app.addLibraryPath(app.applicationDirPath());
  PluginManager pm(&app);
  pm.restart();
  return app.exec();
}
