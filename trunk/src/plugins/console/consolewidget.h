#ifndef CONSOLEWIDGET_H
#define CONSOLEWIDGET_H

#include <QWidget>
#include <interfaces/ipluginmanager.h>
#include <interfaces/ixmppstreammanager.h>
#include <interfaces/istanzaprocessor.h>
#include "ui_consolewidget.h"

class ConsoleWidget :
	public QWidget,
	public IXmppStanzaHadler
{
	Q_OBJECT;
	Q_INTERFACES(IXmppStanzaHadler);
public:
	ConsoleWidget(QWidget *AParent = NULL);
	~ConsoleWidget();
	//IXmppStanzaHadler
	virtual bool xmppStanzaIn(IXmppStream *AStream, Stanza &AStanza, int AOrder);
	virtual bool xmppStanzaOut(IXmppStream *AStream, Stanza &AStanza, int AOrder);
protected:
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
	void onWordWrapButtonToggled(bool AChecked);
protected slots:
	void onTextHilightTimerTimeout();
	void onTextVisiblePositionBoundaryChanged();
	void onTextSearchStart();
	void onTextSearchNextClicked();
	void onTextSearchPreviousClicked();
	void onTextSearchTextChanged(const QString &AText);
protected slots:
	void onXmppStreamCreated(IXmppStream *AXmppStream);
	void onXmppStreamJidChanged(IXmppStream *AXmppStream, const Jid &ABefore);
	void onXmppStreamDestroyed(IXmppStream *AXmppStream);
	void onStanzaHandleInserted(int AHandleId, const IStanzaHandle &AHandle);
	void onOptionsOpened();
	void onOptionsClosed();
private:
	Ui::ConsoleWidgetClass ui;
private:
	IXmppStreamManager *FXmppStreamManager;
	IStanzaProcessor *FStanzaProcessor;
private:
	QUuid FContext;
	QTime FTimePoint;
private:
	bool FSearchMoveCursor;
	QTimer FTextHilightTimer;
	QMap<int,QTextEdit::ExtraSelection> FSearchResults;
};

#endif // CONSOLEWIDGET_H
