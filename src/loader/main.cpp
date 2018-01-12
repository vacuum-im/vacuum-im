#include <QLibrary>
#include <QApplication>
#include "pluginmanager.h"

int main(int argc, char *argv[])
{
	QApplication::setAttribute(Qt::AA_DontShowIconsInMenus,false);
#if (QT_VERSION >= QT_VERSION_CHECK(5,6,0))
	QApplication::setAttribute(Qt::AA_EnableHighDpiScaling,true);
#endif

	QApplication app(argc, argv);
	app.setQuitOnLastWindowClosed(false);
	app.addLibraryPath(app.applicationDirPath());

	QLibrary utils(app.applicationDirPath()+"/utils",&app);
	utils.load();

	PluginManager pm(&app);
	pm.restart();

	return app.exec();
}

