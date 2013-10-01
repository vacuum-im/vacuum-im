#include "editlistsdialog.h"

#include <QTimer>
#include <QLineEdit>
#include <QMessageBox>
#include <QInputDialog>
#include <QTextDocument>

#define DR_NAME       Qt::UserRole
#define DR_INDEX      Qt::UserRole+1

#define RULE_NONE     "<None>"

EditListsDialog::EditListsDialog(IPrivacyLists *APrivacyLists, IRoster *ARoster, const Jid &AStreamJid, QWidget *AParent) : QDialog(AParent)
{
	ui.setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose,true);
	setWindowTitle(tr("Edit Privacy Lists - %1").arg(AStreamJid.uBare()));
	IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->insertAutoIcon(this,MNI_PRIVACYLISTS,0,0,"windowIcon");

	FPrivacyLists = APrivacyLists;
	FRoster = ARoster;
	FStreamJid = AStreamJid;

	ui.cmbActive->addItem(tr("<None>"),QString());
	ui.cmbDefault->addItem(tr("<None>"),QString());

	ui.cmbType->addItem(tr("jid"),PRIVACY_TYPE_JID);
	ui.cmbType->addItem(tr("group"),PRIVACY_TYPE_GROUP);
	ui.cmbType->addItem(tr("subscription"),PRIVACY_TYPE_SUBSCRIPTION);
	ui.cmbType->addItem(tr("<always>"),PRIVACY_TYPE_ALWAYS);
	onRuleConditionTypeChanged(ui.cmbType->currentIndex());

	ui.cmbAction->addItem(tr("deny"),PRIVACY_ACTION_DENY);
	ui.cmbAction->addItem(tr("allow"),PRIVACY_ACTION_ALLOW);

	connect(FPrivacyLists->instance(),SIGNAL(listLoaded(const Jid &, const QString &)),
	        SLOT(onListLoaded(const Jid &, const QString &)));
	connect(FPrivacyLists->instance(),SIGNAL(listRemoved(const Jid &, const QString &)),
	        SLOT(onListRemoved(const Jid &, const QString &)));
	connect(FPrivacyLists->instance(),SIGNAL(activeListChanged(const Jid &, const QString &)),
	        SLOT(onActiveListChanged(const Jid &, const QString &)));
	connect(FPrivacyLists->instance(),SIGNAL(defaultListChanged(const Jid &, const QString &)),
	        SLOT(onDefaultListChanged(const Jid &, const QString &)));
	connect(FPrivacyLists->instance(),SIGNAL(requestCompleted(const QString &)),
	        SLOT(onRequestCompleted(const QString &)));
	connect(FPrivacyLists->instance(),SIGNAL(requestFailed(const QString &, const XmppError &)),
	        SLOT(onRequestFailed(const QString &, const XmppError &)));

	connect(ui.tlbAddList,SIGNAL(clicked()),SLOT(onAddListClicked()));
	connect(ui.tlbDeleteList,SIGNAL(clicked()),SLOT(onDeleteListClicked()));
	connect(ui.tlbAddRule,SIGNAL(clicked()),SLOT(onAddRuleClicked()));
	connect(ui.tlbDeleteRule,SIGNAL(clicked()),SLOT(onDeleteRuleClicked()));
	connect(ui.tlbRuleUp,SIGNAL(clicked()),SLOT(onRuleUpClicked()));
	connect(ui.tlbRuleDown,SIGNAL(clicked()),SLOT(onRuleDownClicked()));


	connect(ui.cmbType,SIGNAL(currentIndexChanged(int)),SLOT(onRuleConditionTypeChanged(int)));
	connect(ui.cmbType,SIGNAL(currentIndexChanged(int)),SLOT(onRuleConditionChanged()));
	connect(ui.cmbValue,SIGNAL(editTextChanged(QString)),SLOT(onRuleConditionChanged()));
	connect(ui.cmbValue,SIGNAL(currentIndexChanged(int)),SLOT(onRuleConditionChanged()));
	connect(ui.cmbAction,SIGNAL(currentIndexChanged(int)),SLOT(onRuleConditionChanged()));
	connect(ui.chbMessage,SIGNAL(stateChanged(int)),SLOT(onRuleConditionChanged()));
	connect(ui.chbQueries,SIGNAL(stateChanged(int)),SLOT(onRuleConditionChanged()));
	connect(ui.chbPresenceIn,SIGNAL(stateChanged(int)),SLOT(onRuleConditionChanged()));
	connect(ui.chbPresenceOut,SIGNAL(stateChanged(int)),SLOT(onRuleConditionChanged()));

	connect(ui.ltwLists,SIGNAL(currentItemChanged(QListWidgetItem *, QListWidgetItem *)),
	        SLOT(onCurrentListItemChanged(QListWidgetItem *, QListWidgetItem *)));
	connect(ui.ltwRules,SIGNAL(currentItemChanged(QListWidgetItem *, QListWidgetItem *)),
	        SLOT(onCurrentRuleItemChanged(QListWidgetItem *, QListWidgetItem *)));

	connect(ui.dbbButtons,SIGNAL(clicked(QAbstractButton *)),SLOT(onDialogButtonClicked(QAbstractButton *)));

	reset();
	updateEnabledState();
}

EditListsDialog::~EditListsDialog()
{
	emit destroyed(FStreamJid);
}

void EditListsDialog::apply()
{
	QList<QString> notValidLists;
	QHash<QString, IPrivacyList>::iterator it = FLists.begin();
	while (it != FLists.end())
	{
		QList<int> ruleOrders;
		IPrivacyList &newList = it.value();
		for (int i=0; i<newList.rules.count(); i++)
		{
			const IPrivacyRule &rule = newList.rules.at(i);
			bool validRule = rule.order>=0 && !ruleOrders.contains(rule.order);
			validRule &= rule.type == PRIVACY_TYPE_ALWAYS || !rule.value.isEmpty();
			if (!validRule)
			{
				newList.rules.removeAt(i);
				i--;
			}
			else
				ruleOrders.append(rule.order);
		}

		if (!newList.rules.isEmpty())
		{
			int ruleIndex = 0;
			IPrivacyList oldList = FPrivacyLists->privacyList(FStreamJid,newList.name);
			bool changedList = newList.rules.count() != oldList.rules.count();
			while (!changedList && ruleIndex<newList.rules.count())
			{
				const IPrivacyRule &newRule = newList.rules.at(ruleIndex);
				const IPrivacyRule &oldRule = oldList.rules.at(ruleIndex);
				changedList |= newRule.order != oldRule.order;
				changedList |= newRule.type != oldRule.type;
				changedList |= newRule.value != oldRule.value;
				changedList |= newRule.action != oldRule.action;
				changedList |= newRule.stanzas != oldRule.stanzas;
				ruleIndex++;
			}
			if (changedList)
			{
				QString requestId = FPrivacyLists->savePrivacyList(FStreamJid,newList);
				if (!requestId.isEmpty())
					FSaveRequests.insert(requestId,newList.name);
			}
		}
		else
			notValidLists.append(newList.name);
		++it;
	}

	QString newActiveList = ui.cmbActive->itemData(ui.cmbActive->currentIndex()).toString();
	if (!notValidLists.contains(newActiveList) && newActiveList!=FPrivacyLists->activeList(FStreamJid))
	{
		QString requestId = FPrivacyLists->setActiveList(FStreamJid, newActiveList);
		if (!requestId.isEmpty())
			FActiveRequests.insert(requestId,newActiveList);
	}

	QString newDefaultList = ui.cmbDefault->itemData(ui.cmbDefault->currentIndex()).toString();
	if (!notValidLists.contains(newDefaultList) && newDefaultList!=FPrivacyLists->defaultList(FStreamJid))
	{
		QString requestId = FPrivacyLists->setDefaultList(FStreamJid, newDefaultList);
		if (!requestId.isEmpty())
			FDefaultRequests.insert(requestId,newDefaultList);
	}

	foreach(const QString &listName, notValidLists)
	{
		IPrivacyList oldList = FPrivacyLists->privacyList(FStreamJid,listName);
		if (oldList.name == listName)
			onListLoaded(FStreamJid,listName);
		else
			onListRemoved(FStreamJid,listName);
	}

	QList<IPrivacyList> oldLists = FPrivacyLists->privacyLists(FStreamJid);
	foreach(const IPrivacyList &oldList, oldLists)
	{
		if (!FLists.contains(oldList.name))
		{
			QString requestId = FPrivacyLists->removePrivacyList(FStreamJid,oldList.name);
			if (!requestId.isEmpty())
				FRemoveRequests.insert(requestId,oldList.name);
		}
	}
	updateListRules();
	QTimer::singleShot(0,this,SLOT(onUpdateEnabledState()));
}

void EditListsDialog::reset()
{
	foreach(const IPrivacyList &list, FLists)
	{
		onListRemoved(FStreamJid,list.name);
	}

	QList<IPrivacyList> lists = FPrivacyLists->privacyLists(FStreamJid);
	foreach(const IPrivacyList &list, lists)
	{
		onListLoaded(FStreamJid,list.name);
	}
	onActiveListChanged(FStreamJid,FPrivacyLists->activeList(FStreamJid));
	onDefaultListChanged(FStreamJid,FPrivacyLists->defaultList(FStreamJid));

	if (lists.count()>0)
	{
		ui.ltwLists->setCurrentRow(0);
		ui.ltwRules->setCurrentRow(0);
	}
	else
		ui.grbRuleCondition->setEnabled(false);
}

QString EditListsDialog::ruleName(const IPrivacyRule &ARule)
{
	QString stanzas;
	if (ARule.stanzas != IPrivacyRule::AnyStanza)
	{
		if (ARule.stanzas & IPrivacyRule::Messages)
			stanzas += " "+tr("messages")+",";
		if (ARule.stanzas & IPrivacyRule::Queries)
			stanzas += " "+tr("queries")+",";
		if (ARule.stanzas & IPrivacyRule::PresencesIn)
			stanzas += " "+tr("pres-in")+",";
		if (ARule.stanzas & IPrivacyRule::PresencesOut)
			stanzas += " "+tr("pres-out")+",";
		stanzas.chop(1);
	}
	else
		stanzas += " "+tr("<any stanza>");

	if (ARule.type != PRIVACY_TYPE_ALWAYS)
	{
		return tr("%1: if %2 = '%3' then %4 [%5 ]")
		       .arg(ARule.order)
		       .arg(tr(ARule.type.toAscii()))
		       .arg(ARule.value)
		       .arg(!ARule.action.isEmpty() ? tr(ARule.action.toAscii()) : tr("<action>"))
		       .arg(stanzas);
	}
	else
	{
		return tr("%1: always %2 [%3 ]")
		       .arg(ARule.order)
		       .arg(!ARule.action.isEmpty() ? tr(ARule.action.toAscii()) : tr("<action>"))
		       .arg(stanzas);
	}
	return QString::null;
}

void EditListsDialog::updateListRules()
{
	if (!FListName.isEmpty())
	{
		IPrivacyList list = FLists.value(FListName);
		for (int row = 0; row < list.rules.count(); row++)
		{
			QListWidgetItem *ruleItem = row<ui.ltwRules->count() ? ui.ltwRules->item(row) : new QListWidgetItem(ui.ltwRules);
			ruleItem->setText(ruleName(list.rules.at(row)));
			ruleItem->setToolTip(ruleItem->text());
			ruleItem->setData(DR_INDEX,row);
		}
		while (list.rules.count() < ui.ltwRules->count())
			delete ui.ltwRules->takeItem(list.rules.count());
		updateRuleCondition();
	}
	else
		ui.ltwRules->clear();
}

void EditListsDialog::updateRuleCondition()
{
	IPrivacyRule listRule = FLists.value(FListName).rules.value(FRuleIndex);
	if (!listRule.action.isEmpty())
	{
		ui.cmbType->setCurrentIndex(ui.cmbType->findData(listRule.type));
		int valueIndex = ui.cmbValue->findData(listRule.value);
		if (valueIndex >= 0)
			ui.cmbValue->setCurrentIndex(valueIndex);
		else if (ui.cmbValue->isEditable())
			ui.cmbValue->setEditText(listRule.value);
		ui.cmbAction->setCurrentIndex(ui.cmbAction->findData(listRule.action));
		ui.chbMessage->setChecked(listRule.stanzas & IPrivacyRule::Messages);
		ui.chbQueries->setChecked(listRule.stanzas & IPrivacyRule::Queries);
		ui.chbPresenceIn->setChecked(listRule.stanzas & IPrivacyRule::PresencesIn);
		ui.chbPresenceOut->setChecked(listRule.stanzas & IPrivacyRule::PresencesOut);
		ui.grbRuleCondition->setEnabled(true);
	}
	else
	{
		ui.cmbType->setCurrentIndex(ui.cmbType->findData(PRIVACY_TYPE_JID));
		ui.cmbAction->setCurrentIndex(ui.cmbAction->findData(PRIVACY_ACTION_DENY));
		ui.chbMessage->setChecked(false);
		ui.chbQueries->setChecked(false);
		ui.chbPresenceIn->setChecked(false);
		ui.chbPresenceOut->setChecked(false);
		ui.grbRuleCondition->setEnabled(false);
	}
}

void EditListsDialog::updateEnabledState()
{
	bool enabled = FSaveRequests.isEmpty() && FRemoveRequests.isEmpty() && FActiveRequests.isEmpty() && FDefaultRequests.isEmpty();
	if (enabled && !FWarnings.isEmpty())
	{
		QMessageBox::warning(this,tr("Privacy List Error"),FWarnings.join("<br>"));
		FWarnings.clear();
	}
	ui.grbDefaults->setEnabled(enabled);
	ui.grbLists->setEnabled(enabled);
	ui.grbRulesList->setEnabled(enabled);
	ui.grbRuleCondition->setEnabled(enabled);
	ui.grbRuleCondition->setEnabled(enabled && FRuleIndex>=0);
	if (enabled)
		ui.dbbButtons->setStandardButtons(QDialogButtonBox::Ok|QDialogButtonBox::Cancel|QDialogButtonBox::Apply|QDialogButtonBox::Reset);
	else
		ui.dbbButtons->setStandardButtons(QDialogButtonBox::Cancel);
}

void EditListsDialog::onListLoaded(const Jid &AStreamJid, const QString &AName)
{
	if (AStreamJid == FStreamJid)
	{
		QListWidgetItem *listWidget = ui.ltwLists->findItems(AName,Qt::MatchExactly).value(0);
		if (!listWidget)
		{
			ui.cmbActive->addItem(AName,AName);
			ui.cmbDefault->addItem(AName,AName);
			listWidget = new QListWidgetItem(AName);
			listWidget->setData(DR_NAME,AName);
			ui.ltwLists->addItem(listWidget);
		}
		IPrivacyList list = FPrivacyLists->privacyList(FStreamJid,AName);
		FLists.insert(AName,list);
		updateListRules();
	}
}

void EditListsDialog::onListRemoved(const Jid &AStreamJid, const QString &AName)
{
	if (AStreamJid == FStreamJid)
	{
		QListWidgetItem *listWidget = ui.ltwLists->findItems(AName,Qt::MatchExactly).value(0);
		if (listWidget)
		{
			ui.cmbActive->removeItem(ui.cmbActive->findData(AName));
			ui.cmbDefault->removeItem(ui.cmbDefault->findData(AName));
			ui.ltwLists->takeItem(ui.ltwLists->row(listWidget));
			delete listWidget;
		}
		FLists.remove(AName);
	}
}

void EditListsDialog::onActiveListChanged(const Jid &AStreamJid, const QString &AName)
{
	if (AStreamJid == FStreamJid)
	{
		ui.cmbActive->setCurrentIndex(ui.cmbActive->findData(AName));
	}
}

void EditListsDialog::onDefaultListChanged(const Jid &AStreamJid, const QString &AName)
{
	if (AStreamJid == FStreamJid)
	{
		ui.cmbDefault->setCurrentIndex(ui.cmbDefault->findData(AName));
	}
}

void EditListsDialog::onRequestCompleted(const QString &AId)
{
	FActiveRequests.remove(AId);
	FDefaultRequests.remove(AId);
	FSaveRequests.remove(AId);
	FRemoveRequests.remove(AId);
	updateEnabledState();
}

void EditListsDialog::onRequestFailed(const QString &AId, const XmppError &AError)
{
	QString warning;
	if (FActiveRequests.contains(AId))
	{
		warning = tr("Privacy list '%1' could not be active: %2").arg(Qt::escape(FActiveRequests.take(AId))).arg(Qt::escape(AError.errorMessage()));
		onActiveListChanged(FStreamJid,FPrivacyLists->activeList(FStreamJid));
	}
	else if (FDefaultRequests.contains(AId))
	{
		warning = tr("Privacy list '%1' could not be default: %2").arg(Qt::escape(FDefaultRequests.take(AId))).arg(Qt::escape(AError.errorMessage()));
		onDefaultListChanged(FStreamJid,FPrivacyLists->defaultList(FStreamJid));
	}
	else if (FSaveRequests.contains(AId))
	{
		warning = tr("Privacy list '%1' could not be saved: %2").arg(Qt::escape(FSaveRequests.take(AId))).arg(Qt::escape(AError.errorMessage()));
	}
	else if (FRemoveRequests.contains(AId))
	{
		warning = tr("Privacy list '%1' could not be removed: %2").arg(Qt::escape(FRemoveRequests.take(AId))).arg(Qt::escape(AError.errorMessage()));
	}
	if (!warning.isEmpty())
		FWarnings.append(warning);
	updateEnabledState();
}

void EditListsDialog::onAddListClicked()
{
	QString name = QInputDialog::getText(this,tr("New Privacy List"),tr("Enter list name:"));
	if (!name.isEmpty() && ui.ltwLists->findItems(name,Qt::MatchExactly).isEmpty())
	{
		IPrivacyList list;
		list.name = name;
		FLists.insert(name,list);
		QListWidgetItem *listWidget = new QListWidgetItem(name);
		listWidget->setData(DR_NAME,name);
		ui.ltwLists->addItem(listWidget);
		ui.cmbActive->addItem(name,name);
		ui.cmbDefault->addItem(name,name);
		ui.ltwLists->setCurrentItem(listWidget);
	}
}

void EditListsDialog::onDeleteListClicked()
{
	if (FLists.contains(FListName))
	{
		if (QMessageBox::question(this,tr("Remove Privacy List"),
		                          tr("Are you really want to delete privacy list '%1' with rules?").arg(FListName), QMessageBox::Yes|QMessageBox::No)==QMessageBox::Yes)
		{
			FLists.remove(FListName);
			QListWidgetItem *listWidget = ui.ltwLists->findItems(FListName,Qt::MatchExactly).value(0);
			if (listWidget)
			{
				ui.cmbActive->removeItem(ui.cmbActive->findData(FListName));
				ui.cmbDefault->removeItem(ui.cmbDefault->findData(FListName));
				ui.ltwLists->takeItem(ui.ltwLists->row(listWidget));
				delete listWidget;
			}
		}
	}
}

void EditListsDialog::onAddRuleClicked()
{
	if (FLists.contains(FListName))
	{
		IPrivacyRule rule;
		rule.order = !FLists.value(FListName).rules.isEmpty() ? FLists.value(FListName).rules.last().order + 1 : 1;
		rule.type = PRIVACY_TYPE_SUBSCRIPTION;
		rule.value = SUBSCRIPTION_NONE;
		rule.action = PRIVACY_ACTION_DENY;
		rule.stanzas = IPrivacyRule::AnyStanza;
		FLists[FListName].rules.append(rule);
		updateListRules();
		ui.ltwRules->setCurrentRow(ui.ltwRules->count()-1);
	}
}

void EditListsDialog::onDeleteRuleClicked()
{
	if (FLists.contains(FListName) && FRuleIndex >= 0)
	{
		FLists[FListName].rules.removeAt(FRuleIndex);
		updateListRules();
	}
}

void EditListsDialog::onRuleUpClicked()
{
	if (FLists.contains(FListName) && FRuleIndex > 0)
	{
		qSwap(FLists[FListName].rules[FRuleIndex].order,FLists[FListName].rules[FRuleIndex-1].order);
		FLists[FListName].rules.move(FRuleIndex,FRuleIndex-1);
		updateListRules();
		ui.ltwRules->setCurrentRow(FRuleIndex-1);
	}
}

void EditListsDialog::onRuleDownClicked()
{
	if (FLists.contains(FListName) && FRuleIndex < FLists.value(FListName).rules.count()-1)
	{
		qSwap(FLists[FListName].rules[FRuleIndex].order,FLists[FListName].rules[FRuleIndex+1].order);
		FLists[FListName].rules.move(FRuleIndex,FRuleIndex+1);
		updateListRules();
		ui.ltwRules->setCurrentRow(FRuleIndex+1);
	}
}

void EditListsDialog::onRuleConditionChanged()
{
	if (FLists.contains(FListName) && FRuleIndex>=0 && FRuleIndex<FLists.value(FListName).rules.count())
	{
		IPrivacyRule &listRule = FLists[FListName].rules[FRuleIndex];
		listRule.type = ui.cmbType->itemData(ui.cmbType->currentIndex()).toString();
		int valueIndex = ui.cmbValue->currentIndex();
		if (valueIndex>=0 && ui.cmbValue->itemText(valueIndex)==ui.cmbValue->currentText())
			listRule.value = ui.cmbValue->itemData(valueIndex).toString();
		else
			listRule.value = ui.cmbValue->currentText();
		listRule.action = ui.cmbAction->itemData(ui.cmbAction->currentIndex()).toString();
		listRule.stanzas = IPrivacyRule::EmptyType;
		if (ui.chbMessage->isChecked())
			listRule.stanzas |= IPrivacyRule::Messages;
		if (ui.chbQueries->isChecked())
			listRule.stanzas |= IPrivacyRule::Queries;
		if (ui.chbPresenceIn->isChecked())
			listRule.stanzas |= IPrivacyRule::PresencesIn;
		if (ui.chbPresenceOut->isChecked())
			listRule.stanzas |= IPrivacyRule::PresencesOut;
		if (listRule.stanzas == IPrivacyRule::EmptyType)
			listRule.stanzas = IPrivacyRule::AnyStanza;
		if (ui.ltwRules->currentRow()>=0)
		{
			QListWidgetItem *ruleItem = ui.ltwRules->item(ui.ltwRules->currentRow());
			ruleItem->setText(ruleName(listRule));
			ruleItem->setToolTip(ruleItem->text());
		}
	}
}

void EditListsDialog::onRuleConditionTypeChanged(int AIndex)
{
	QString type = ui.cmbType->itemData(AIndex).toString();
	ui.cmbValue->blockSignals(true);
	while (ui.cmbValue->count() > 0)
		ui.cmbValue->removeItem(0);
	ui.cmbValue->setEnabled(type != PRIVACY_TYPE_ALWAYS);
	if (type == PRIVACY_TYPE_SUBSCRIPTION)
	{
		ui.cmbValue->setInsertPolicy(QComboBox::InsertAtBottom);
		ui.cmbValue->setEditable(false);
		ui.cmbValue->addItem(tr("none","Subscription type"),SUBSCRIPTION_NONE);
		ui.cmbValue->addItem(tr("to","Subscription type"),SUBSCRIPTION_TO);
		ui.cmbValue->addItem(tr("from","Subscription type"),SUBSCRIPTION_FROM);
		ui.cmbValue->addItem(tr("both","Subscription type"),SUBSCRIPTION_BOTH);
		ui.cmbValue->blockSignals(false);
		ui.cmbValue->setCurrentIndex(0);
	}
	else
	{
		ui.cmbValue->setInsertPolicy(QComboBox::InsertAlphabetically);
		if (type == PRIVACY_TYPE_JID)
		{
			QList<IRosterItem> ritems = FRoster!=NULL ? FRoster->rosterItems() : QList<IRosterItem>();
			foreach(const IRosterItem &ritem, ritems)
			{
				QString itemName = !ritem.name.isEmpty() ? ritem.name + " <"+ritem.itemJid.uFull()+">" : ritem.itemJid.uFull();
				ui.cmbValue->addItem(itemName,ritem.itemJid.full());
			}
		}
		else if (type == PRIVACY_TYPE_GROUP)
		{
			QSet<QString> groups = FRoster!=NULL ? FRoster->allGroups() : QSet<QString>();
			foreach(const QString &group, groups)
				ui.cmbValue->addItem(group,group);
		}
		ui.cmbValue->setEditable(true);
		ui.cmbValue->blockSignals(false);
		ui.cmbValue->setEditText(QString::null);
	}
}

void EditListsDialog::onCurrentListItemChanged(QListWidgetItem *ACurrent, QListWidgetItem *APrevious)
{
	Q_UNUSED(APrevious);
	FListName = ACurrent!=NULL ? ACurrent->data(DR_NAME).toString() : QString::null;
	updateListRules();
}

void EditListsDialog::onCurrentRuleItemChanged(QListWidgetItem *ACurrent, QListWidgetItem *APrevious)
{
	Q_UNUSED(APrevious);
	FRuleIndex = ACurrent!=NULL ? ACurrent->data(DR_INDEX).toInt() : -1;
	updateRuleCondition();
}

void EditListsDialog::onDialogButtonClicked(QAbstractButton *AButton)
{
	switch (ui.dbbButtons->buttonRole(AButton))
	{
	case QDialogButtonBox::AcceptRole:
		apply();
		accept();
		break;
	case QDialogButtonBox::ApplyRole:
		apply();
		break;
	case QDialogButtonBox::ResetRole:
		reset();
		break;
	case QDialogButtonBox::RejectRole:
		reject();
		break;
	default:
		break;
	}
}

void EditListsDialog::onUpdateEnabledState()
{
	//Откладываем выполнение этой функции т.к. нельзя менять набор кнопок из функции обработки клика
	updateEnabledState();
}
