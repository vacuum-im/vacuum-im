#ifndef CLIENTINFODIALOG_H
#define CLIENTINFODIALOG_H

#include <QDialog>
#include <definitions/resources.h>
#include <definitions/menuicons.h>
#include <interfaces/iclientinfo.h>
#include <utils/iconstorage.h>
#include "ui_clientinfodialog.h"

class ClientInfoDialog :
			public QDialog
{
	Q_OBJECT;
public:
	ClientInfoDialog(IClientInfo *AClientInfo, const Jid &AStreamJid, const Jid &AContactJid, const QString &AContactName, int AInfoTypes, QWidget *AParent = NULL);
	~ClientInfoDialog();
	Jid streamJid() const { return FStreamJid; }
	Jid contactJid() const { return FContactJid; }
	int infoTypes() const { return FInfoTypes; }
	void setInfoTypes(int AInfoTypes);
signals:
	void clientInfoDialogClosed(const Jid &FContactJid);
protected:
	void updateText();
	QString secsToString(int ASecs) const;
protected slots:
	void onClientInfoChanged(const Jid &AConatctJid);
private:
	Ui::ClientInfoDialogClass ui;
private:
	IClientInfo *FClientInfo;
private:
	int FInfoTypes;
	Jid FStreamJid;
	Jid FContactJid;
	QString FContactName;
};

#endif // CLIENTINFODIALOG_H
