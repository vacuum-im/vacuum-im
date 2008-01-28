#include "joinmultichatdialog.h"

#include <QMessageBox>

#define SVN_RECENT                "resent:groupchat[]"
#define SVN_RESENT_ROOMJID        SVN_RECENT":roomJid"
#define SVN_RESENT_NICK           SVN_RECENT":nick"
#define SVN_RESENT_PASSWORD       SVN_RECENT":password"

#define JDR_RoomJid               Qt::UserRole
#define JDR_NickName              Qt::UserRole+1
#define JDR_Password              Qt::UserRole+2

JoinMultiChatDialog::JoinMultiChatDialog(IMultiUserChatPlugin *AChatPlugin, const Jid &AStreamJid, const Jid &ARoomJid, 
                                         const QString &ANick, const QString &APassword, QWidget *AParent) : QDialog(AParent)
{
  ui.setupUi(this);
  setAttribute(Qt::WA_DeleteOnClose,true);

  FXmppStreams = NULL;
  FSettingsPlugin = NULL;

  FChatPlugin = AChatPlugin;
  connect(FChatPlugin->instance(),SIGNAL(roomNickReceived(const Jid &, const Jid &, const QString &)),
    SLOT(onRoomNickReceived(const Jid &, const Jid &, const QString &)));
  connect(ui.tlbRecentDelete,SIGNAL(clicked()),SLOT(onRecentDeleteClicked()));
  connect(ui.tlbResolveNick,SIGNAL(clicked()),SLOT(onResolveNickClicked()));
  connect(ui.btbButtons,SIGNAL(accepted()),SLOT(onDialogAccepted()));

  initialize();
  ui.cmbStreamJid->setCurrentIndex(AStreamJid.isValid() ? ui.cmbStreamJid->findText(AStreamJid.full(),Qt::MatchFixedString) : 0);

  Jid streamJid = ui.cmbStreamJid->currentText();
  ui.lneHost->setText(ARoomJid.domane().isEmpty() ? QString("conference.%1").arg(streamJid.domane()) : ARoomJid.domane());
  ui.lneRoom->setText(ARoomJid.node());
  ui.lnePassword->setText(APassword);
  if (AStreamJid.isValid() && ARoomJid.isValid() && ANick.isEmpty())
    FChatPlugin->requestRoomNick(AStreamJid,ARoomJid);
  else
    ui.lneNick->setText(ANick.isEmpty() ? streamJid.node() : ANick);
}

JoinMultiChatDialog::~JoinMultiChatDialog()
{

}

void JoinMultiChatDialog::initialize()
{
  IPlugin *plugin = FChatPlugin->pluginManager()->getPlugins("IXmppStreams").value(0,NULL);
  if (plugin)
  {
    FXmppStreams = qobject_cast<IXmppStreams *>(plugin->instance());
    if (FXmppStreams)
    {
      QList<IXmppStream *> xmppStreams = FXmppStreams->getStreams();
      foreach(IXmppStream *xmppStream, xmppStreams)
        if (FXmppStreams->isActive(xmppStream))
          ui.cmbStreamJid->addItem(xmppStream->jid().full());
      ui.cmbStreamJid->model()->sort(0,Qt::AscendingOrder);
      connect(ui.cmbStreamJid,SIGNAL(currentIndexChanged(int)),SLOT(onStreamIndexChanged(int)));
      connect(FXmppStreams->instance(),SIGNAL(added(IXmppStream *)),SLOT(onStreamAdded(IXmppStream *)));
      connect(FXmppStreams->instance(),SIGNAL(removed(IXmppStream *)),SLOT(onStreamRemoved(IXmppStream *)));
      connect(FXmppStreams->instance(),SIGNAL(jidChanged(IXmppStream *, const Jid &)),SLOT(onStreamJidChanged(IXmppStream *, const Jid &)));
    }
  }

  plugin = FChatPlugin->pluginManager()->getPlugins("ISettingsPlugin").value(0,NULL);
  if (plugin)
  {
    FSettingsPlugin = qobject_cast<ISettingsPlugin *>(plugin->instance());
    if (FSettingsPlugin)
    {
      ISettings *settings = FSettingsPlugin->settingsForPlugin(MULTIUSERCHAT_UUID);
      QList<QString> resent = settings->values(SVN_RECENT).keys();
      for(int i=0; i<resent.count(); i++)
      {
        QString roomJid = settings->valueNS(SVN_RESENT_ROOMJID,resent.at(i)).toString();
        QString nick = settings->valueNS(SVN_RESENT_NICK,resent.at(i)).toString();
        QString password = settings->decript(settings->valueNS(SVN_RESENT_PASSWORD,resent.at(i)).toByteArray(),resent.at(i).toUtf8());
        ui.cmbRecent->insertItem(i,resent.at(i));
        ui.cmbRecent->setItemData(i,roomJid,JDR_RoomJid);
        ui.cmbRecent->setItemData(i,nick,JDR_NickName);
        ui.cmbRecent->setItemData(i,password,JDR_Password);
      }
      ui.cmbRecent->setCurrentIndex(-1);
      ui.cmbRecent->model()->sort(0,Qt::AscendingOrder);
      connect(ui.cmbRecent,SIGNAL(currentIndexChanged(int)),SLOT(onResentIndexChanged(int)));
    }
  }
}

void JoinMultiChatDialog::onDialogAccepted()
{
  QString host = ui.lneHost->text();
  QString room = ui.lneRoom->text();
  QString nick = ui.lneNick->text();
  QString password = ui.lnePassword->text();
  Jid streamJid = ui.cmbStreamJid->currentText();
  Jid roomJid(room,host,"");

  if (streamJid.isValid() && roomJid.isValid() && !host.isEmpty() && !room.isEmpty() && !nick.isEmpty())
  {
    IMultiUserChatWindow *chatWindow = FChatPlugin->multiChatWindow(streamJid,roomJid);
    if (!chatWindow)
    {
      chatWindow = FChatPlugin->getMultiChatWindow(streamJid,roomJid,nick,password);
      chatWindow->multiUserChat()->setAutoPresence(true);
      if (FSettingsPlugin)
      {
        ISettings *settings = FSettingsPlugin->settingsForPlugin(MULTIUSERCHAT_UUID);
        QString recent = tr("%1 as %2").arg(roomJid.pBare()).arg(nick);
        settings->setValueNS(SVN_RESENT_ROOMJID,recent,roomJid.bare());
        settings->setValueNS(SVN_RESENT_NICK,recent,nick);
        settings->setValueNS(SVN_RESENT_PASSWORD,recent,settings->encript(password,recent.toUtf8()));
      }
    }
    chatWindow->showWindow();
    accept();
  }
  else
  {
    QMessageBox::warning(this,windowTitle(),tr("Room parameters is not acceptable.\nCheck values and try again"));
  }
}

void JoinMultiChatDialog::onStreamIndexChanged(int /*AIndex*/)
{
  ui.tlbResolveNick->setEnabled(true);
}

void JoinMultiChatDialog::onResentIndexChanged(int AIndex)
{
  if (AIndex >= 0)
  {
    Jid roomJid = ui.cmbRecent->itemData(AIndex,JDR_RoomJid).toString();
    ui.lneHost->setText(roomJid.domane());
    ui.lneRoom->setText(roomJid.node());
    ui.lneNick->setText(ui.cmbRecent->itemData(AIndex,JDR_NickName).toString());
    ui.lnePassword->setText(ui.cmbRecent->itemData(AIndex,JDR_Password).toString());
  }
  ui.tlbResolveNick->setEnabled(true);
}

void JoinMultiChatDialog::onRecentDeleteClicked()
{
  int index = ui.cmbRecent->currentIndex();
  if (FSettingsPlugin && index>=0)
  {
    Jid roomJid = ui.cmbRecent->itemData(index,JDR_RoomJid).toString();
    QString nick = ui.cmbRecent->itemData(index,JDR_NickName).toString();
    QString recent = tr("%1 as %2").arg(roomJid.pBare()).arg(nick);
    ISettings *settings = FSettingsPlugin->settingsForPlugin(MULTIUSERCHAT_UUID);
    settings->deleteValueNS(SVN_RECENT,recent);
    ui.cmbRecent->removeItem(ui.cmbRecent->currentIndex());
  }
}

void JoinMultiChatDialog::onResolveNickClicked()
{
  Jid roomJid(ui.lneRoom->text(),ui.lneHost->text(),"");
  Jid streamJid = ui.cmbStreamJid->currentText();
  if (streamJid.isValid() && roomJid.isValid())
  {
    ui.lneNick->clear();
    ui.tlbResolveNick->setEnabled(false);
    FChatPlugin->requestRoomNick(streamJid,roomJid);
  }
  
}

void JoinMultiChatDialog::onRoomNickReceived(const Jid &AStreamJid, const Jid &ARoomJid, const QString &ANick)
{
  Jid roomJid(ui.lneRoom->text(),ui.lneHost->text(),"");
  Jid streamJid = ui.cmbStreamJid->currentText();
  if (AStreamJid == streamJid && ARoomJid == roomJid)
  {
    ui.tlbResolveNick->setEnabled(true);
    if (ui.lneNick->text().isEmpty())
      ui.lneNick->setText(ANick.isEmpty() ? streamJid.node() : ANick);
  }
}

void JoinMultiChatDialog::onStreamAdded(IXmppStream *AXmppStream)
{
  ui.cmbStreamJid->addItem(AXmppStream->jid().full());
}

void JoinMultiChatDialog::onStreamRemoved( IXmppStream *AXmppStream )
{
  ui.cmbStreamJid->removeItem(ui.cmbStreamJid->findText(AXmppStream->jid().full()));
}

void JoinMultiChatDialog::onStreamJidChanged(IXmppStream *AXmppStream, const Jid &ABefour)
{
  ui.cmbStreamJid->removeItem(ui.cmbStreamJid->findText(ABefour.full()));
  ui.cmbStreamJid->addItem(AXmppStream->jid().full());
}

