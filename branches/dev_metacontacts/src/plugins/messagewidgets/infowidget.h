#ifndef INFOWIDGET_H
#define INFOWIDGET_H

#include <definitions/optionvalues.h>
#include <interfaces/imessagewidgets.h>
#include <interfaces/iaccountmanager.h>
#include <interfaces/iroster.h>
#include <interfaces/ipresence.h>
#include <interfaces/iavatars.h>
#include <interfaces/istatuschanger.h>
#include <utils/options.h>
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
	virtual void autoUpdateFields();
	virtual void autoUpdateField(InfoField AField);
	virtual QVariant field(InfoField AField) const;
	virtual void setField(InfoField AField, const QVariant &AValue);
	virtual int autoUpdatedFields() const;
	virtual bool isFiledAutoUpdated(IMessageInfoWidget::InfoField AField) const;
	virtual void setFieldAutoUpdated(IMessageInfoWidget::InfoField AField, bool AAuto);
	virtual int visibleFields() const;
	virtual bool isFieldVisible(IMessageInfoWidget::InfoField AField) const;
	virtual void setFieldVisible(IMessageInfoWidget::InfoField AField, bool AVisible);
signals:
	void fieldChanged(int AField, const QVariant &AValue);
protected:
	void initialize();
	void updateFieldLabel(IMessageInfoWidget::InfoField AField);
protected slots:
	void onAccountChanged(const OptionsNode &ANode);
	void onRosterItemReceived(const IRosterItem &AItem, const IRosterItem &ABefore);
	void onPresenceItemReceived(const IPresenceItem &AItem, const IPresenceItem &ABefore);
	void onAvatarChanged(const Jid &AContactJid);
	void onAddressChanged(const Jid &AStreamBefore, const Jid &AContactBefore);
private:
	Ui::InfoWidgetClass ui;
private:
	IAccount *FAccount;
	IRoster *FRoster;
	IPresence *FPresence;
	IAvatars *FAvatars;
	IStatusChanger *FStatusChanger;
	IMessageWidgets *FMessageWidgets;
private:
	int FAutoFields;
	int FVisibleFields;
	IMessageWindow *FWindow;
	QMap<int, QVariant> FFieldValues;
};

#endif // INFOWIDGET_H
