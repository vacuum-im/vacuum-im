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
			public IInfoWidget
{
	Q_OBJECT;
	Q_INTERFACES(IInfoWidget);
public:
	InfoWidget(IMessageWidgets *AMessageWidgets, const Jid& AStreamJid, const Jid &AContactJid, QWidget *AParent);
	~InfoWidget();
	virtual QWidget *instance() { return this; }
	virtual const Jid &streamJid() const;
	virtual void setStreamJid(const Jid &AStreamJid);
	virtual const Jid &contactJid() const;
	virtual void setContactJid(const Jid &AContactJid);
	virtual void autoUpdateFields();
	virtual void autoUpdateField(InfoField AField);
	virtual QVariant field(InfoField AField) const;
	virtual void setField(InfoField AField, const QVariant &AValue);
	virtual int autoUpdatedFields() const;
	virtual bool isFiledAutoUpdated(IInfoWidget::InfoField AField) const;
	virtual void setFieldAutoUpdated(IInfoWidget::InfoField AField, bool AAuto);
	virtual int visibleFields() const;
	virtual bool isFieldVisible(IInfoWidget::InfoField AField) const;
	virtual void setFieldVisible(IInfoWidget::InfoField AField, bool AVisible);
signals:
	void streamJidChanged(const Jid &ABefore);
	void contactJidChanged(const Jid &ABefore);
	void fieldChanged(IInfoWidget::InfoField AField, const QVariant &AValue);
protected:
	void initialize();
	void updateFieldLabel(IInfoWidget::InfoField AField);
protected slots:
	void onAccountChanged(const OptionsNode &ANode);
	void onRosterItemReceived(const IRosterItem &ARosterItem);
	void onPresenceReceived(const IPresenceItem &APresenceItem);
	void onAvatarChanged(const Jid &AContactJid);
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
	Jid FStreamJid;
	Jid FContactJid;
	QMap<int, QVariant> FFieldValues;
};

#endif // INFOWIDGET_H
