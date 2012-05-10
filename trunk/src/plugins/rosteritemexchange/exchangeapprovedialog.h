#ifndef EXCHANGEAPPROVEDIALOG_H
#define EXCHANGEAPPROVEDIALOG_H

#include <QDialog>
#include <definitions/resources.h>
#include <definitions/menuicons.h>
#include <interfaces/iroster.h>
#include <interfaces/irosteritemexchange.h>
#include <utils/iconstorage.h>
#include "ui_exchangeapprovedialog.h"

class ExchangeApproveDialog : 
	public QDialog
{
	Q_OBJECT;
public:
	ExchangeApproveDialog(IRoster *ARoster, const IRosterExchangeRequest &ARequest, QWidget *AParent = NULL);
	~ExchangeApproveDialog();
	bool subscribeNewContacts() const;
	IRosterExchangeRequest receivedRequest() const;
	IRosterExchangeRequest approvedRequest() const;
signals:
	void dialogDestroyed();
protected:
	QString groupSetToString(const QSet<QString> AGroups) const;
	QString contactName(const Jid &AContactJid, bool AWithJid = true) const;
	void appendRequestItems(const QList<IRosterExchangeItem> &AItems);
private:
	Ui::ExchangeApproveDialogClass ui;
private:
	IRoster *FRoster;
	IRosterExchangeRequest FRequest;
	QMap<QTableWidgetItem *, IRosterExchangeItem> FItems;
};

#endif // EXCHANGEAPPROVEDIALOG_H
