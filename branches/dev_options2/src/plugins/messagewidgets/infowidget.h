#ifndef INFOWIDGET_H
#define INFOWIDGET_H

#include <interfaces/imessagewidgets.h>
#include <interfaces/iavatars.h>
#include <interfaces/iroster.h>
#include <interfaces/ipresence.h>
#include <interfaces/istatusicons.h>
#include <interfaces/iaccountmanager.h>
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
	virtual bool isVisibleOnWindow() const;
	virtual IMessageWindow *messageWindow() const;
	//IMessageInfoWidget
	virtual Menu *addressMenu() const;
	virtual bool isAddressMenuVisible() const;
	virtual void setAddressMenuVisible(bool AVisible);
	virtual QVariant fieldValue(int AField) const;
	virtual void setFieldValue(int AField, const QVariant &AValue);
	virtual ToolBarChanger *infoToolBarChanger() const;
signals:
	void fieldValueChanged(int AField);
	void addressMenuVisibleChanged(bool AVisible);
	void addressMenuRequested(Menu *AMenu);
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
	void onUpdateInfoToolBarVisibility();
private:
	Ui::InfoWidgetClass ui;
private:
	IAvatars *FAvatars;
	IMessageWidgets *FMessageWidgets;
private:
	Menu *FAddressMenu;
	bool FAddressMenuVisible;
	IMessageWindow *FWindow;
	ToolBarChanger *FInfoToolBar;
	QMap<int, QVariant> FFieldValues;
};

#endif // INFOWIDGET_H
