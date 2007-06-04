#include <QApplication>
#include <QTimer>
#include "PluginManager.h"

int main(int argc, char *argv[])
{
  QApplication app(argc, argv);
  app.setQuitOnLastWindowClosed(false);
  PluginManager pm(&app);
  pm.loadPlugins();
  pm.initPlugins();
  pm.startPlugins(); 
  return app.exec();
}
