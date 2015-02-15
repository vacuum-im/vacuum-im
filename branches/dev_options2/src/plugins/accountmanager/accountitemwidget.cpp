#include "accountitemwidget.h"

#include <QTextDocument>

AccountItemWidget::AccountItemWidget(const QUuid &AAccountId, QWidget *AParent) : QWidget(AParent)
{
	ui.setupUi(this);

	FAccountId = AAccountId;

	connect(ui.chbActive,SIGNAL(clicked(bool)),SIGNAL(modified()));

	ui.lblSettings->setText(QString("<a href='settings'>%1</a>").arg(tr("Settings...")));
	connect(ui.lblSettings,SIGNAL(linkActivated(const QString &)),SLOT(onSettingsLinkActivated()));

	connect(ui.tlbRemove,SIGNAL(clicked()),SLOT(onRemoveButtonClicked()));
}

AccountItemWidget::~AccountItemWidget()
{

}

QUuid AccountItemWidget::accountId() const
{
	return FAccountId;
}

bool AccountItemWidget::isActive() const
{
	return ui.chbActive->isChecked();
}

void AccountItemWidget::setActive(bool AActive)
{
	ui.chbActive->setChecked(AActive);
}

void AccountItemWidget::setIcon(const QIcon &AIcon)
{
	if (!AIcon.isNull())
		ui.lblIcon->setPixmap(AIcon.pixmap(QSize(16,16)));
	else
		ui.lblIcon->setVisible(false);
}

QString AccountItemWidget::name() const
{
	return FName;
}

void AccountItemWidget::setName(const QString &AName)
{
	FName = AName;
	ui.lblName->setText(QString("<b>%1<b>").arg(Qt::escape(AName)));
}

Jid AccountItemWidget::streamJid() const
{
	return FStreamJid;
}

void AccountItemWidget::setStreamJid(const Jid &AStreamJid)
{
	FStreamJid = AStreamJid;
	ui.lblJid->setText(QString("<%1>").arg(AStreamJid.uBare()));
}

void AccountItemWidget::onRemoveButtonClicked()
{
	emit removeClicked(FAccountId);
}

void AccountItemWidget::onSettingsLinkActivated()
{
	emit settingsClicked(FAccountId);
}
