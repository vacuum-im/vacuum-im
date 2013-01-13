#ifndef CONSOLEWIDGET_H
#define CONSOLEWIDGET_H

#include <QWidget>
#include "../../definations/resources.h"
#include "../../definations/menuicons.h"
#include "../../interfaces/ipluginmanager.h"
#include "../../interfaces/ixmppstreams.h"
#include "../../interfaces/istanzaprocessor.h"
#include "../../interfaces/isettings.h"
#include "../../utils/iconstorage.h"
#include "ui_consolewidget.h"

class ConsoleWidget : 
  public QWidget
{
  Q_OBJECT;
public:
  ConsoleWidget(IPluginManager *APluginManager, QWidget *AParent = NULL);
  ~ConsoleWidget();
protected:
  void colorXml(QString &AXml) const;
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
  void onStreamConsoleElement(IXmppStream *AXmppStream, const QDomElement &AElem, bool ASended);
  void onStreamJidChanged(IXmppStream *AXmppStream, const Jid &ABefour);
  void onStreamDestroyed(IXmppStream *AXmppStream);
  void onConditionAppended(int AHandlerId, const QString &ACondition);
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
