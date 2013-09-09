#include <QLibrary>
#include <QApplication>
#include <QDir>
#include "pluginmanager.h"
#include "crashhandler.h"

int main(int argc, char *argv[])
{
	QApplication app(argc, argv);
	CrashHandler::instance()->init(QDir::homePath()+"/minidumps");
	app.setQuitOnLastWindowClosed(false);
	app.addLibraryPath(app.applicationDirPath());
	QLibrary utils(app.applicationDirPath()+"/utils",&app);
	utils.load();
	PluginManager pm(&app);
	pm.restart();

	return app.exec();
}
