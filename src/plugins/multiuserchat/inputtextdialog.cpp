#include "inputtextdialog.h"

InputTextDialog::InputTextDialog(QWidget *AParent, const QString &ATitle, const QString &ALabel, QString &AText) : QDialog(AParent), FText(AText)
{
  ui.setupUi(this);
  setAttribute(Qt::WA_DeleteOnClose, true);

  setWindowTitle(ATitle);
  ui.lblCaptionl->setText(ALabel);
  ui.pteText->setPlainText(AText);

  connect(ui.dbbButtons, SIGNAL(clicked(QAbstractButton *)),SLOT(onDialogButtonClicked(QAbstractButton *)));
}

InputTextDialog::~InputTextDialog()
{

}

void InputTextDialog::onDialogButtonClicked(QAbstractButton *AButton)
{
  switch (ui.dbbButtons->standardButton(AButton))
  {
  case QDialogButtonBox::Ok:
    FText = ui.pteText->toPlainText();
    accept();
    break;
  default:
    reject();
  }
}
