#ifndef CHATWINDOWMENU_H
#define CHATWINDOWMENU_H

#include <definations/namespaces.h>
#include <definations/resources.h>
#include <definations/menuicons.h>
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
  ChatWindowMenu(IMessageArchiver *AArchiver, IChatWindow *AWindow);
  ~ChatWindowMenu();
protected:
  void initialize();
  void createActions();
protected slots:
  void onActionTriggered(bool);
  void onArchivePrefsChanged(const Jid &AStreamJid, const IArchiveStreamPrefs &APrefs);
  void onRequestCompleted(const QString &AId);
  void onRequestFailed(const QString &AId, const QString &AError);
  void onDiscoInfoReceived(const IDiscoInfo &ADiscoInfo);
  void onStanzaSessionActivated(const IStanzaSession &ASession);
  void onStanzaSessionTerminated(const IStanzaSession &ASession);
  void onWindowContactJidChanged(const Jid &ABefour);
private:
  IChatWindow *FWindow;
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
