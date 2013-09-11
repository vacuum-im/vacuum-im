#include <QLibrary>
#include <QApplication>
#include <QDir>
#include "pluginmanager.h"
#ifdef WITH_BREAKPAD
  #include "crashhandler.h"
#endif

int main(int argc, char *argv[])
{
	QApplication app(argc, argv);
#ifdef WITH_BREAKPAD
	CrashHandler::instance()->init(QDir::homePath()+"/minidumps");
#endif
	app.setQuitOnLastWindowClosed(false);
	app.addLibraryPath(app.applicationDirPath());
	QLibrary utils(app.applicationDirPath()+"/utils",&app);
	utils.load();
	PluginManager pm(&app);
	pm.restart();

	return app.exec();
}
