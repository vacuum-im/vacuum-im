#ifndef CHATWINDOWMENU_H
#define CHATWINDOWMENU_H

#include <definitions/namespaces.h>
#include <definitions/resources.h>
#include <definitions/menuicons.h>
#include <definitions/shortcuts.h>
#include <interfaces/imessagearchiver.h>
#include <interfaces/imessagewidgets.h>
#include <interfaces/idataforms.h>
#include <interfaces/isessionnegotiation.h>
#include <interfaces/iservicediscovery.h>
#include <utils/menu.h>

class ChatWindowMenu :
			public Menu
{
	Q_OBJECT;
public:
	ChatWindowMenu(IMessageArchiver *AArchiver, IPluginManager *APluginManager, IToolBarWidget *AToolBarWidget, QWidget *AParent);
	~ChatWindowMenu();
protected:
	void initialize(IPluginManager *APluginManager);
	void createActions();
protected slots:
	void onActionTriggered(bool);
	void onArchivePrefsChanged(const Jid &AStreamJid);
	void onRequestCompleted(const QString &AId);
	void onRequestFailed(const QString &AId, const QString &AError);
	void onDiscoInfoReceived(const IDiscoInfo &ADiscoInfo);
	void onStanzaSessionActivated(const IStanzaSession &ASession);
	void onStanzaSessionTerminated(const IStanzaSession &ASession);
	void onEditWidgetContactJidChanged(const Jid &ABefore);
private:
	IEditWidget *FEditWidget;
	IToolBarWidget *FToolBarWidget;
	IDataForms *FDataForms;
	IMessageArchiver *FArchiver;
	IServiceDiscovery *FDiscovery;
	ISessionNegotiation *FSessionNegotiation;
private:
	Action *FSaveTrue;
	Action *FSaveFalse;
	Action *FSessionRequire;
	Action *FSessionTerminate;
private:
	QString FSaveRequest;
	QString FSessionRequest;
};

#endif // CHATWINDOWMENU_H
