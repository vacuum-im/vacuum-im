#include <QLibrary>
#include <QApplication>
#include "pluginmanager.h"

static const int ext_argc = 1;
static const char *ext_argv[ext_argc] = {
	"--disable-web-security"            // Command line argument to enable local content in QWebEngine
};

int main(int argc, char *argv[])
{
#ifdef Q_OS_MACX
	if (QSysInfo::MacintoshVersion == QSysInfo::MV_YOSEMITE )
	{
		// https://bugreports.qt-project.org/browse/QTBUG-40833
		QFont::insertSubstitution(".Helvetica Neue DeskInterface", "Helvetica Neue");
	}
#endif

	// Extend command line arguments
	int all_argc = argc + ext_argc;
	char **all_argv = new char*[all_argc];
	memcpy(all_argv, argv, sizeof(char *) * argc);
	memcpy(all_argv + argc, ext_argv, sizeof(char *) * ext_argc);

	QApplication::setAttribute(Qt::AA_EnableHighDpiScaling,true);
	QApplication::setAttribute(Qt::AA_DontShowIconsInMenus,false);

	QApplication app(all_argc, all_argv);
	app.setQuitOnLastWindowClosed(false);
	app.addLibraryPath(app.applicationDirPath());

	QLibrary utils(app.applicationDirPath()+"/utils",&app);
	utils.load();

	PluginManager pm(&app);
	pm.restart();

	return app.exec();
}
