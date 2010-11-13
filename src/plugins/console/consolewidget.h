#ifndef CONSOLEWIDGET_H
#define CONSOLEWIDGET_H

#include <QWidget>
#include <definitions/optionvalues.h>
#include <definitions/resources.h>
#include <definitions/menuicons.h>
#include <definitions/xmppstanzahandlerorders.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/ixmppstreams.h>
#include <interfaces/istanzaprocessor.h>
#include <utils/options.h>
#include <utils/iconstorage.h>
#include <utils/widgetmanager.h>
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
	void initialize(IPluginManager *APluginManager);
	void loadContext(const QUuid &AContextId);
	void saveContext(const QUuid &AContextId);
	void colorXml(QString &AXml) const;
	void hidePasswords(QString &AXml) const;
	void showElement(IXmppStream *AXmppStream, const QDomElement &AElem, bool ASended);
protected slots:
	void onAddConditionClicked();
	void onRemoveConditionClicked();
	void onSendXMLClicked();
	void onAddContextClicked();
	void onRemoveContextClicked();
	void onContextChanged(int AIndex);
	void onWordWrapStateChanged(int AState);
protected slots:
	void onStreamCreated(IXmppStream *AXmppStream);
	void onStreamJidChanged(IXmppStream *AXmppStream, const Jid &ABefore);
	void onStreamDestroyed(IXmppStream *AXmppStream);
	void onStanzaHandleInserted(int AHandleId, const IStanzaHandle &AHandle);
	void onOptionsOpened();
	void onOptionsClosed();
private:
	Ui::ConsoleWidgetClass ui;
private:
	IXmppStreams *FXmppStreams;
	IStanzaProcessor *FStanzaProcessor;
private:
	QUuid FContext;
	QTime FTimePoint;
};

#endif // CONSOLEWIDGET_H
