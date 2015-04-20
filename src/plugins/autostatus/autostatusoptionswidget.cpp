#include "autostatusoptionswidget.h"

#include <QMessageBox>
#include <definitions/optionvalues.h>
#include "autorulesoptionsdialog.h"

AutoStatusOptionsWidget::AutoStatusOptionsWidget(IAutoStatus *AAutoStatus, IStatusChanger *AStatusChanger, QWidget *AParent) : QWidget(AParent)
{
	ui.setupUi(this);

	FAutoStatus = AAutoStatus;
	FStatusChanger = AStatusChanger;

	ui.cmbAwayStatus->addItem(FStatusChanger->iconByShow(IPresence::Away),FStatusChanger->nameByShow(IPresence::Away),IPresence::Away);
	ui.cmbAwayStatus->addItem(FStatusChanger->iconByShow(IPresence::DoNotDisturb),FStatusChanger->nameByShow(IPresence::DoNotDisturb),IPresence::DoNotDisturb);
	ui.cmbAwayStatus->addItem(FStatusChanger->iconByShow(IPresence::ExtendedAway),FStatusChanger->nameByShow(IPresence::ExtendedAway),IPresence::ExtendedAway);
	ui.cmbAwayStatus->addItem(FStatusChanger->iconByShow(IPresence::Invisible),FStatusChanger->nameByShow(IPresence::Invisible),IPresence::Invisible);

	ui.lblShowRules->setText(QString("<a href='show-rules'>%1</a>").arg(tr("Show all rules for the automatic change of status...")));
	connect(ui.lblShowRules,SIGNAL(linkActivated(const QString &)),SLOT(onShowRulesLinkActivayed()));

	connect(ui.tlbAwayHelp,SIGNAL(clicked(bool)),SLOT(onHelpButtonClicked()));

	connect(ui.chbAwayEnable,SIGNAL(stateChanged(int)),SIGNAL(modified()));
	connect(ui.cmbAwayStatus,SIGNAL(currentIndexChanged(int)),SIGNAL(modified()));
	connect(ui.spbAwayTime,SIGNAL(valueChanged(int)),SIGNAL(modified()));
	connect(ui.lneAwayText,SIGNAL(textChanged(const QString &)),SIGNAL(modified()));
	connect(ui.chbOfflineEnable,SIGNAL(stateChanged(int)),SIGNAL(modified()));

	reset();
}

void AutoStatusOptionsWidget::apply()
{
	QUuid awayId = Options::node(OPV_AUTOSTARTUS_AWAYRULE).value().toString();
	IAutoStatusRule awayRule = FAutoStatus->ruleValue(awayId);
	awayRule.time = ui.spbAwayTime->value() * 60;
	awayRule.show = ui.cmbAwayStatus->itemData(ui.cmbAwayStatus->currentIndex()).toInt();
	awayRule.text = ui.lneAwayText->text();
	FAutoStatus->updateRule(awayId,awayRule);
	FAutoStatus->setRuleEnabled(awayId,ui.chbAwayEnable->isChecked());

	QUuid offlineId = Options::node(OPV_AUTOSTARTUS_OFFLINERULE).value().toString();
	IAutoStatusRule offlineRule = FAutoStatus->ruleValue(offlineId);
	offlineRule.time = ui.spbOfflineTime->value() * 60;
	FAutoStatus->updateRule(offlineId,offlineRule);
	FAutoStatus->setRuleEnabled(offlineId,ui.chbOfflineEnable->isChecked());

	emit childApply();
}

void AutoStatusOptionsWidget::reset()
{
	static const QList<int> awayShows = QList<int>() 
		<< IPresence::Away << IPresence::DoNotDisturb << IPresence::ExtendedAway << IPresence::Invisible;
	static const QList<int> offlineShows = QList<int>() 
		<< IPresence::Offline;

	QList<QUuid> availRules = FAutoStatus->rules();

	QUuid awayId = Options::node(OPV_AUTOSTARTUS_AWAYRULE).value().toString();
	if (availRules.contains(awayId))
	{
		IAutoStatusRule rule = FAutoStatus->ruleValue(awayId);
		awayId = awayShows.contains(rule.show) ? awayId : QUuid();
	}
	else foreach(const QUuid &id, availRules)
	{
		IAutoStatusRule rule = FAutoStatus->ruleValue(id);
		awayId = awayShows.contains(rule.show) ? id : QUuid();
	}
	if (!availRules.contains(awayId))
	{
		IAutoStatusRule awayRule;
		awayRule.time = 10*60;
		awayRule.show = IPresence::Away;
		awayRule.priority = 20;
		awayRule.text = tr("Auto status due to inactivity for more than #(m) minutes");
		
		awayId = FAutoStatus->insertRule(awayRule);
		Options::node(OPV_AUTOSTARTUS_AWAYRULE).setValue(awayId.toString());
	}
	
	QUuid offlineId = Options::node(OPV_AUTOSTARTUS_OFFLINERULE).value().toString();
	if (availRules.contains(offlineId))
	{
		IAutoStatusRule rule = FAutoStatus->ruleValue(offlineId);
		offlineId = offlineShows.contains(rule.show) ? offlineId : QUuid();
	}
	else foreach(const QUuid &id, availRules)
	{
		IAutoStatusRule rule = FAutoStatus->ruleValue(id);
		offlineId = offlineShows.contains(rule.show) ? id : QUuid();
	}
	if (!availRules.contains(offlineId))
	{
		IAutoStatusRule offlineRule;
		offlineRule.time = 2*60*60;
		offlineRule.show = IPresence::Offline;
		offlineRule.priority = 0;
		offlineRule.text = tr("Disconnected due to inactivity for more than #(m) minutes");

		offlineId = FAutoStatus->insertRule(offlineRule);
		Options::node(OPV_AUTOSTARTUS_OFFLINERULE).setValue(offlineId.toString());
	}

	IAutoStatusRule awayRule = FAutoStatus->ruleValue(awayId);
	awayRule.time = awayRule.time>0 ? awayRule.time : 10*60;
	ui.chbAwayEnable->setChecked(FAutoStatus->isRuleEnabled(awayId));
	ui.cmbAwayStatus->setCurrentIndex(ui.cmbAwayStatus->findData(awayRule.show));
	ui.spbAwayTime->setValue(awayRule.time / 60);
	ui.lneAwayText->setText(awayRule.text);

	IAutoStatusRule offlineRule = FAutoStatus->ruleValue(offlineId);
	offlineRule.time = offlineRule.time>0 ? offlineRule.time : 2*60*60;
	ui.chbOfflineEnable->setChecked(FAutoStatus->isRuleEnabled(offlineId));
	ui.spbOfflineTime->setValue(offlineRule.time / 60);

	emit childReset();
}

void AutoStatusOptionsWidget::onHelpButtonClicked()
{
  QMessageBox::information(this,tr("Auto Status"),
    tr("You can insert date and time into auto status text:") + "\n" + \
    tr("   %(<format>) - current date and time") + "\n" + \
    tr("   $(<format>) - date and time you are idle form") + "\n" + \
    tr("   #(<format>) - time you are idle for") + "\n\n" + \
    tr("Date Format:") + "\n" + \
    tr("   d - the day as number without a leading zero (1 to 31)") + "\n" + \
    tr("   dd - the day as number with a leading zero (01 to 31)") + "\n" + \
    tr("   ddd - the abbreviated localized day name (e.g. 'Mon' to 'Sun')") + "\n" + \
    tr("   dddd - the long localized day name (e.g. 'Monday' to 'Sunday')") + "\n" + \
    tr("   M - the month as number without a leading zero (1-12)") + "\n" + \
    tr("   MM - the month as number with a leading zero (01-12)") + "\n" + \
    tr("   MMM - the abbreviated localized month name (e.g. 'Jan' to 'Dec')") + "\n" + \
    tr("   MMMM - the long localized month name (e.g. 'January' to 'December')") + "\n" + \
    tr("   yy - the year as two digit number (00-99)") + "\n" + \
    tr("   yyyy - the year as four digit number") + "\n\n" + \
    tr("Time Format:") + "\n" + \
    tr("   h - the hour without a leading zero (0 to 23 or 1 to 12 if AM/PM display)") + "\n" + \
    tr("   hh - the hour with a leading zero (00 to 23 or 01 to 12 if AM/PM display)") + "\n" + \
    tr("   H - the hour without a leading zero (0 to 23, even with AM/PM display)") + "\n" + \
    tr("   HH - the hour with a leading zero (00 to 23, even with AM/PM display)") + "\n" + \
    tr("   m - the minute without a leading zero (0 to 59)") + "\n" + \
    tr("   mm - the minute with a leading zero (00 to 59)") + "\n" + \
    tr("   s - the second without a leading zero (0 to 59)") + "\n" + \
    tr("   ss - the second with a leading zero (00 to 59)") + "\n" + \
    tr("   z - the milliseconds without leading zeroes (0 to 999)") + "\n" + \
    tr("   zzz - the milliseconds with leading zeroes (000 to 999)") + "\n" + \
    tr("   AP or A - interpret as an AM/PM time. AP must be either 'AM' or 'PM'") + "\n" + \
    tr("   ap or a - interpret as an AM/PM time. ap must be either 'am' or 'pm'") + "\n\n" + \
    tr("Example:") + "\n" + \
    tr("   Status is set to 'away' at %(hh:mm:ss), because of idle from $(hh:mm:ss) for #(mm) minutes and #(ss) seconds")
    );
}

void AutoStatusOptionsWidget::onShowRulesLinkActivayed()
{
	apply();

	AutoRulesOptionsDialog *dialog = new AutoRulesOptionsDialog(FAutoStatus,FStatusChanger,this);
	connect(dialog,SIGNAL(accepted()),SLOT(reset()));
	dialog->show();
}
