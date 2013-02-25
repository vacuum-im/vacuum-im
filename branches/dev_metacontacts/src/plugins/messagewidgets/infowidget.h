#ifndef INFOWIDGET_H
#define INFOWIDGET_H

#include <interfaces/imessagewidgets.h>
#include <interfaces/iavatars.h>
#include "ui_infowidget.h"

class InfoWidget :
	public QWidget,
	public IMessageInfoWidget
{
	Q_OBJECT;
	Q_INTERFACES(IMessageWidget IMessageInfoWidget);
public:
	InfoWidget(IMessageWidgets *AMessageWidgets, IMessageWindow *AWindow, QWidget *AParent);
	~InfoWidget();
	//IMessageWidget
	virtual QWidget *instance() { return this; }
	virtual IMessageWindow *messageWindow() const;
	//IMessageInfoWidget
	virtual ToolBarChanger *toolBarChanger() const;
	virtual bool isAddressSelectorEnabled() const;
	virtual void setAddressSelectorEnabled(bool AEnabled);
	virtual QVariant fieldValue(int AField) const;
	virtual void setFieldValue(int AField, const QVariant &AValue);
signals:
	void fieldValueChanged(int AField);
	void addressSelectorEnableChanged(bool AEnabled);
	void contextMenuRequested(Menu *AMenu);
	void toolTipsRequested(QMap<int,QString> &AToolTips);
protected:
	void initialize();
	void updateFieldView(int AField);
protected:
	bool event(QEvent *AEvent);
	void contextMenuEvent(QContextMenuEvent *AEvent);
protected slots:
	void onAddressMenuAboutToShow();
	void onAddressChanged(const Jid &AStreamBefore, const Jid &AContactBefore);
private:
	Ui::InfoWidgetClass ui;
private:
	IAvatars *FAvatars;
	IMessageWidgets *FMessageWidgets;
private:
	Menu *FAddressMenu;
	bool FSelectorEnabled;
	IMessageWindow *FWindow;
	ToolBarChanger *FInfoToolBar;
	QMap<int, QVariant> FFieldValues;
};

#endif // INFOWIDGET_H
