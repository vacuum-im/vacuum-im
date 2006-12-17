#include <QApplication>
#include <QTimer>
#include "PluginManager.h"

int main(int argc, char *argv[])
{
  QApplication app(argc, argv);
  PluginManager pm(&app);
  pm.loadPlugins();
  pm.initPlugins();
  pm.startPlugins(); 
  QTimer::singleShot(15000,&pm,SLOT(quit())); 
  return app.exec();
}
