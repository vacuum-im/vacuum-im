#ifndef VIEWWIDGET_H
#define VIEWWIDGET_H

#include <QDropEvent>
#include <QDragMoveEvent>
#include <QDragEnterEvent>
#include <QDragLeaveEvent>
#include <interfaces/imessagewidgets.h>
#include <interfaces/imessageprocessor.h>
#include <utils/textmanager.h>
#include "ui_viewwidget.h"

class ViewWidget :
	public QWidget,
	public IViewWidget
{
	Q_OBJECT;
	Q_INTERFACES(IViewWidget);
public:
	ViewWidget(IMessageWidgets *AMessageWidgets, const Jid &AStreamJid, const Jid &AContactJid, QWidget *AParent);
	~ViewWidget();
	virtual QWidget *instance() { return this; }
	virtual const Jid &streamJid() const { return FStreamJid; }
	virtual void setStreamJid(const Jid &AStreamJid);
	virtual const Jid &contactJid() const { return FContactJid; }
	virtual void setContactJid(const Jid &AContactJid);
	virtual QWidget *styleWidget() const;
	virtual IMessageStyle *messageStyle() const;
	virtual void setMessageStyle(IMessageStyle *AStyle, const IMessageStyleOptions &AOptions);
	virtual void appendHtml(const QString &AHtml, const IMessageContentOptions &AOptions);
	virtual void appendText(const QString &AText, const IMessageContentOptions &AOptions);
	virtual void appendMessage(const Message &AMessage, const IMessageContentOptions &AOptions);
	virtual void contextMenuForView(const QPoint &APosition, Menu *AMenu);
	virtual QTextDocumentFragment selection() const;
	virtual QTextCharFormat textFormatAt(const QPoint &APosition) const;
	virtual QTextDocumentFragment textFragmentAt(const QPoint &APosition) const;
signals:
	void streamJidChanged(const Jid &ABefore);
	void contactJidChanged(const Jid &ABefore);
	void messageStyleChanged(IMessageStyle *ABefore, const IMessageStyleOptions &AOptions);
	void contentAppended(const QString &AHtml, const IMessageContentOptions &AOptions);
	void viewContextMenu(const QPoint &APosition, Menu *AMenu);
	void urlClicked(const QUrl &AUrl) const;
protected:
	void initialize();
protected:
	virtual void dropEvent(QDropEvent *AEvent);
	virtual void dragEnterEvent(QDragEnterEvent *AEvent);
	virtual void dragMoveEvent(QDragMoveEvent *AEvent);
	virtual void dragLeaveEvent(QDragLeaveEvent *AEvent);
protected slots:
	void onContentAppended(QWidget *AWidget, const QString &AHtml, const IMessageContentOptions &AOptions);
	void onUrlClicked(QWidget *AWidget, const QUrl &AUrl);
	void onCustomContextMenuRequested(const QPoint &APosition);
private:
	Ui::ViewWidgetClass ui;
private:
	IMessageStyle *FMessageStyle;
	IMessageWidgets *FMessageWidgets;
	IMessageProcessor *FMessageProcessor;
private:
	Jid FStreamJid;
	Jid FContactJid;
	QWidget *FStyleWidget;
	QList<IViewDropHandler *> FActiveDropHandlers;
};

#endif // VIEWWIDGET_H
