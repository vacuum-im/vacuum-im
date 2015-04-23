#ifndef EDITLISTSDIALOG_H
#define EDITLISTSDIALOG_H

#include <QDialog>
#include <interfaces/irostermanager.h>
#include <interfaces/iprivacylists.h>
#include "ui_editlistsdialog.h"

class EditListsDialog :
	public QDialog
{
	Q_OBJECT;
public:
	EditListsDialog(IPrivacyLists *APrivacyLists, IRoster *ARoster, const Jid &AStreamJid, QWidget *AParent = NULL);
	~EditListsDialog();
	void apply();
	void reset();
signals:
	void destroyed(const Jid &AStreamJid);
protected:
	QString ruleName(const IPrivacyRule &ARule);
	void updateListRules();
	void updateRuleCondition();
	void updateEnabledState();
protected slots:
	void onListLoaded(const Jid &AStreamJid, const QString &AName);
	void onListRemoved(const Jid &AStreamJid, const QString &AName);
	void onActiveListChanged(const Jid &AStreamJid, const QString &AName);
	void onDefaultListChanged(const Jid &AStreamJid, const QString &AName);
	void onRequestCompleted(const QString &AId);
	void onRequestFailed(const QString &AId, const XmppError &AError);
	void onAddListClicked();
	void onDeleteListClicked();
	void onAddRuleClicked();
	void onDeleteRuleClicked();
	void onRuleUpClicked();
	void onRuleDownClicked();
	void onRuleConditionChanged();
	void onRuleConditionTypeChanged(int AIndex);
	void onCurrentListItemChanged(QListWidgetItem *ACurrent, QListWidgetItem *APrevious);
	void onCurrentRuleItemChanged(QListWidgetItem *ACurrent, QListWidgetItem *APrevious);
	void onDialogButtonClicked(QAbstractButton *AButton);
	void onUpdateEnabledState();
private:
	Ui::EditListsDialogClass ui;
private:
	IRoster *FRoster;
	IPrivacyLists *FPrivacyLists;
private:
	Jid FStreamJid;
	int FRuleIndex;
	QString FListName;
	QHash<QString,IPrivacyList> FLists;
	QStringList FWarnings;
	QHash<QString,QString> FActiveRequests;
	QHash<QString,QString> FDefaultRequests;
	QHash<QString,QString> FSaveRequests;
	QHash<QString,QString> FRemoveRequests;
};

#endif // EDITLISTSDIALOG_H
