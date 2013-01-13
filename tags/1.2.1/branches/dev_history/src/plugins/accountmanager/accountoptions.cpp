#include "accountoptions.h"

#include <QTimer>
#include <QMessageBox>
#include <QTextDocument>

AccountOptions::AccountOptions(IAccountManager *AManager, const QUuid &AAccountId, QWidget *AParent) : QWidget(AParent)
{
	ui.setupUi(this);
	FManager = AManager;

	FAccountId = AAccountId;
	FAccount = FManager->accountById(AAccountId);

	if (FAccount == NULL)
	{
		ui.lneResource->setText(CLIENT_NAME);
		ui.lneName->setText(tr("New Account"));
		ui.lneName->selectAll();
		QTimer::singleShot(0,ui.lneName,SLOT(setFocus()));
	}

	connect(ui.lneName,SIGNAL(textChanged(const QString &)),SIGNAL(modified()));
	connect(ui.lneJabberId,SIGNAL(textChanged(const QString &)),SIGNAL(modified()));
	connect(ui.lneResource,SIGNAL(textChanged(const QString &)),SIGNAL(modified()));
	connect(ui.lnePassword,SIGNAL(textChanged(const QString &)),SIGNAL(modified()));

	reset();
}

AccountOptions::~AccountOptions()
{
	if (FAccount == NULL)
	{
		Options::node(OPV_ACCOUNT_ROOT).removeChilds("account",FAccountId.toString());
	}
}

void AccountOptions::apply()
{
	FAccount = FAccount==NULL ? FManager->appendAccount(FAccountId) : FAccount;
	if (FAccount)
	{
		QString name = ui.lneName->text().trimmed();
		if (name.isEmpty())
			name = ui.lneJabberId->text().trimmed();
		if (name.isEmpty())
			name = tr("New Account");
		Jid jabberId = ui.lneJabberId->text();
		jabberId.setResource(ui.lneResource->text());

		bool changedJid = (FAccount->streamJid() != jabberId);

		FAccount->setName(name);
		FAccount->setStreamJid(jabberId);
		FAccount->setPassword(ui.lnePassword->text());

		if (!FAccount->isValid())
			QMessageBox::warning(this,tr("Invalid Account"),tr("Account '%1' is not valid, change its Jabber ID").arg(name));
		else if (changedJid && FAccount->isActive() && FAccount->xmppStream()->isConnected())
			QMessageBox::information(NULL,tr("Delayed Apply"),tr("Some options of account '%1' will be applied after disconnect").arg(name));
	}
	emit childApply();
}

void AccountOptions::reset()
{
	if (FAccount)
	{
		ui.lneName->setText(FAccount->name());
		ui.lneJabberId->setText(FAccount->streamJid().bare());
		ui.lneResource->setText(FAccount->streamJid().resource());
		ui.lnePassword->setText(FAccount->password());
	}
	emit childReset();
}
