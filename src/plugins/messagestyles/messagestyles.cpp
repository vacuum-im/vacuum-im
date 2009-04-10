#include "messagestyles.h"

#include <QtDebug>
#include <QWebView>


MessageStyles::MessageStyles()
{

}

MessageStyles::~MessageStyles()
{

}

void MessageStyles::pluginInfo(IPluginInfo *APluginInfo)
{
  APluginInfo->author = "Potapov S.A. aka Lion";
  APluginInfo->description = tr("Managing message styles");
  APluginInfo->homePage = "http://jrudevels.org";
  APluginInfo->name = tr("Message Styles"); 
  APluginInfo->uid = MESSAGESTYLES_UUID;
  APluginInfo->version = "0.1";
}

bool MessageStyles::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
  return true;
}

bool MessageStyles::startPlugin()
{
  IMessageStyle::StyleOptions sparams;
  sparams.chatName = "ChatName";
  sparams.selfName = "SelfName";
  sparams.contactName = "ContactName";
  sparams.startTime = QDateTime::currentDateTime();

  IMessageStyle::ContentOptions mparams;
  mparams.sendTime = QDateTime::currentDateTime();
  mparams.isSameSender = false;
  mparams.isAction = false;
  mparams.isArchive = false;
  mparams.isAlignLTR = true;
  mparams.isStatus = false;
  mparams.willAppendMoreContent = false;
  mparams.replaceLastContent = false;

  IMessageStyle::ContentOptions stparams = mparams;
  stparams.isStatus = true;

  MessageStyle *style = new MessageStyle("c:/Qt/Projects/Vacuum/resources/messagestyles/Stockholm",this);
  
  QWebView *view = new QWebView();
  style->setStyle(view,sparams);
  
  mparams.isDirectionIn = true;
  mparams.isSameSender = false;
  mparams.senderName = "Contact";
  mparams.messageClasses = QStringList() << "incoming" << "message";
  style->appendContent(view,"Test message1",mparams);

  mparams.isSameSender = true;
  mparams.messageClasses = QStringList() << "incoming" << "message" << "consecutive";
  style->appendContent(view,"Test message2",mparams);

  mparams.isDirectionIn = false;
  mparams.isSameSender = false;
  mparams.senderName = "Self";
  mparams.messageClasses = QStringList() << "outgoing" << "message";
  style->appendContent(view,"Test message3",mparams);

  stparams.statusHint = "away";
  //stparams.messageClasses = QStringList() << "status";
  style->appendContent(view,"User online",stparams);

  mparams.isDirectionIn = true;
  mparams.isSameSender = false;
  mparams.senderName = "Contact";
  mparams.messageClasses = QStringList() << "incoming" << "message";
  style->appendContent(view,"Test message4",mparams);

  style->setVariant(view,style->variants().value(5));

  view->show();

  return true;
}

Q_EXPORT_PLUGIN2(MessageStylesPlugin, MessageStyles)
