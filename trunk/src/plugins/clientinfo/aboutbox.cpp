#include "aboutbox.h"

AboutBox::AboutBox(IClientInfo *AClientInfo, QWidget *AParent) : QDialog(AParent)
{
  ui.setupUi(this);
  setAttribute(Qt::WA_DeleteOnClose,true);

  ui.lblName->setText(CLIENT_NAME" Jabber IM");
  ui.lblVersion->setText(tr("Version: %1 Revision: %2").arg(AClientInfo->version()).arg(AClientInfo->revision()));
  ui.lblDate->setText(tr("Revision date: %1").arg(AClientInfo->revisionDate().toString()));
}

AboutBox::~AboutBox()
{

}
