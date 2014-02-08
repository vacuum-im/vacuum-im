#include "editbookmarkdialog.h"

#include <QMessageBox>
#include <definitions/resources.h>
#include <definitions/menuicons.h>
#include <utils/iconstorage.h>
#include <utils/logger.h>

EditBookmarkDialog::EditBookmarkDialog(IBookmark *ABookmark, QWidget *AParent) : QDialog(AParent)
{
	REPORT_VIEW;
	ui.setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose,true);
	IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->insertAutoIcon(this,MNI_BOOKMARKS_EDIT,0,0,"windowIcon");

	FBookmark = ABookmark;
	ui.lneName->setText(ABookmark->name);
	if (ABookmark->type == IBookmark::Conference)
	{
		ui.grbURL->setChecked(false);
		ui.grbConference->setChecked(true);
		ui.lneRoom->setText(ABookmark->conference.roomJid.uBare());
		ui.lneNick->setText(ABookmark->conference.nick);
		ui.lnePassword->setText(ABookmark->conference.password);
		ui.chbAutoJoin->setChecked(ABookmark->conference.autojoin);
	}
	else if (ABookmark->type == IBookmark::Url)
	{
		ui.grbURL->setChecked(true);
		ui.grbConference->setChecked(false);
		ui.lneUrl->setText(ABookmark->url.url.toString());
	}

	connect(ui.grbConference,SIGNAL(clicked(bool)),SLOT(onGroupBoxClicked(bool)));
	connect(ui.grbURL,SIGNAL(clicked(bool)),SLOT(onGroupBoxClicked(bool)));
	connect(ui.bbxButtons,SIGNAL(accepted()),SLOT(onDialogAccepted()));
}

EditBookmarkDialog::~EditBookmarkDialog()
{

}

void EditBookmarkDialog::onGroupBoxClicked(bool AChecked)
{
	Q_UNUSED(AChecked);
	QGroupBox *groupBox = qobject_cast<QGroupBox *>(sender());
	if (groupBox == ui.grbConference)
		ui.grbURL->setChecked(!ui.grbConference->isChecked());
	else if (groupBox == ui.grbURL)
		ui.grbConference->setChecked(!ui.grbURL->isChecked());
}

void EditBookmarkDialog::onDialogAccepted()
{
	if (!ui.lneName->text().isEmpty())
	{
		if (ui.grbConference->isChecked())
		{
			if (!ui.lneRoom->text().isEmpty())
			{
				FBookmark->type = IBookmark::Conference;
				FBookmark->name = ui.lneName->text();
				FBookmark->conference.roomJid = Jid::fromUserInput(ui.lneRoom->text()).bare();
				FBookmark->conference.nick = ui.lneNick->text();
				FBookmark->conference.password = ui.lnePassword->text();
				FBookmark->conference.autojoin = ui.chbAutoJoin->isChecked();
				accept();
			}
			else
			{
				QMessageBox::warning(this,tr("Bookmark is not valid"),tr("In conference bookmark field 'Room' should not be empty"));
			}
		}
		else
		{
			if (!ui.lneUrl->text().isEmpty())
			{
				FBookmark->type = IBookmark::Url;
				FBookmark->name = ui.lneName->text();
				FBookmark->url.url = QUrl::fromUserInput(ui.lneUrl->text());
				accept();
			}
			else
			{
				QMessageBox::warning(this,tr("Bookmark is not valid"),tr("In URL bookmark field 'URL' should not be empty"));
			}
		}
	}
	else
	{
		QMessageBox::warning(this,tr("Bookmark is not valid"),tr("Field 'Name' should not be empty"));
	}
}
