#ifndef CONSOLEWIDGET_H
#define CONSOLEWIDGET_H

#include <QWidget>
#include <definations/resources.h>
#include <definations/menuicons.h>
#include <definations/xmppstanzahandlerorders.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/ixmppstreams.h>
#include <interfaces/istanzaprocessor.h>
#include <interfaces/isettings.h>
#include <utils/iconstorage.h>
#include "ui_consolewidget.h"

class ConsoleWidget : 
  public QWidget,
  public IXmppStanzaHadler
{
  Q_OBJECT;
  Q_INTERFACES(IXmppStanzaHadler);
public:
  ConsoleWidget(IPluginManager *APluginManager, QWidget *AParent = NULL);
  ~ConsoleWidget();
  //IXmppStanzaHadler
  virtual bool xmppStanzaIn(IXmppStream *AStream, Stanza &AStanza, int AOrder);
  virtual bool xmppStanzaOut(IXmppStream *AStream, Stanza &AStanza, int AOrder);
protected:
  void colorXml(QString &AXml) const;
  void showElement(IXmppStream *AXmppStream, const QDomElement &AElem, bool ASended);
protected slots:
  void onAddConditionClicked();
  void onRemoveConditionClicked();
  void onSendXMLClicked();
  void onLoadContextClicked();
  void onSaveContextClicked();
  void onDeleteContextClicked();
  void onWordWrapStateChanged(int AState);
protected slots:
  void onStreamCreated(IXmppStream *AXmppStream);
  void onStreamJidChanged(IXmppStream *AXmppStream, const Jid &ABefour);
  void onStreamDestroyed(IXmppStream *AXmppStream);
  void onStanzaHandleInserted(int AHandleId, const IStanzaHandle &AHandle);
  void onSettingsOpened();
  void onSettingsClosed();
private:
  Ui::ConsoleWidgetClass ui;
private:
  IXmppStreams *FXmppStreams;
  IStanzaProcessor *FStanzaProcessor;
  ISettingsPlugin *FSettingsPlugin;
private:
  QTime FTimePoint;
};

#endif // CONSOLEWIDGET_H
