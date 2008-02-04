#include "editbookmarkdialog.h"

#include <QMessageBox>

EditBookmarkDialog::EditBookmarkDialog(IBookMark *ABookmark, QWidget *AParent) : QDialog(AParent)
{
  ui.setupUi(this);
  setAttribute(Qt::WA_DeleteOnClose,true);
  
  FBookmark = ABookmark;
  ui.lneName->setText(ABookmark->name);
  if (!ABookmark->conference.isEmpty())
  {
    ui.grbConference->setChecked(true);
    ui.grbURL->setChecked(false);
    ui.lneRoom->setText(ABookmark->conference);
    ui.lneNick->setText(ABookmark->nick);
    ui.lnePassword->setText(ABookmark->password);
    ui.chbAutoJoin->setChecked(ABookmark->autojoin);
  }
  else
  {
    ui.grbURL->setChecked(true);
    ui.grbConference->setChecked(false);
    ui.lneUrl->setText(ABookmark->url);
  }

  connect(ui.grbConference,SIGNAL(clicked(bool)),SLOT(onGroupBoxClicked(bool)));
  connect(ui.grbURL,SIGNAL(clicked(bool)),SLOT(onGroupBoxClicked(bool)));
  connect(ui.bbxButtons,SIGNAL(accepted()),SLOT(onDialogAccepted()));
}

EditBookmarkDialog::~EditBookmarkDialog()
{

}

void EditBookmarkDialog::onGroupBoxClicked(bool /*AChecked*/)
{
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
      if (!ui.lneRoom->text().isEmpty() && !ui.lneNick->text().isEmpty())
      {
        FBookmark->name = ui.lneName->text();
        FBookmark->conference = ui.lneRoom->text();
        FBookmark->nick = ui.lneNick->text();
        FBookmark->password = ui.lnePassword->text();
        FBookmark->autojoin = ui.chbAutoJoin->isChecked();
        FBookmark->url = "";
        accept();
      }
      else
        QMessageBox::warning(this,tr("Bookmark is not valid"),tr("In conference bookmark fields 'Room' and 'Nick' should not be empty"));
    }
    else
    {
      if (!ui.lneUrl->text().isEmpty())
      {
        FBookmark->name = ui.lneName->text();
        FBookmark->url = ui.lneUrl->text();
        FBookmark->conference = "";
        FBookmark->nick = "";
        FBookmark->password = "";
        FBookmark->autojoin = false;
        accept();
      }
      else
        QMessageBox::warning(this,tr("Bookmark is not valid"),tr("In URL bookmark field 'URL'should not be empty"));
    }
  }
  else
    QMessageBox::warning(this,tr("Bookmark is not valid"),tr("Field 'Name' should not be empty"));
}