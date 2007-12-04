#include "clientinfodialog.h"

ClientInfoDialog::ClientInfoDialog(const QString &AContactName, const Jid &AContactJid, const Jid &AStreamJid, 
                                   IClientInfo *AClientInfo, QWidget *AParent) : QDialog(AParent)
{
  ui.setupUi(this);
  setAttribute(Qt::WA_DeleteOnClose);
  setWindowTitle(tr("Client info - %1").arg(AContactName));
  FContactName = AContactName;
  FContactJid = AContactJid;
  FStreamJid = AStreamJid;
  FClientInfo = AClientInfo;
  connect(FClientInfo->instance(),SIGNAL(softwareInfoChanged(const Jid &)),SLOT(onClientInfoChanged(const Jid &)));
  connect(FClientInfo->instance(),SIGNAL(lastActivityChanged(const Jid &)),SLOT(onClientInfoChanged(const Jid &)));

  if (!FClientInfo->hasSoftwareInfo(FContactJid))
    FClientInfo->requestSoftwareInfo(FContactJid,FStreamJid);
  if (!FClientInfo->hasLastActivity(FContactJid))
    FClientInfo->requestLastActivity(FContactJid,FStreamJid);

  updateText();
}

ClientInfoDialog::~ClientInfoDialog()
{
  emit clientInfoDialogClosed(FContactJid);
}

void ClientInfoDialog::updateText()
{
  QString itemMask = "%1 %2<br>";
  QString html = QString("<b>%1</b> (%2)<br><br>").arg(Qt::escape(FContactName)).arg(FContactJid.hFull());

  //Software Info
  if (!FContactJid.resource().isEmpty())
  {
    html += tr("<b>Software Info</b><br>");
    if (FClientInfo->hasSoftwareInfo(FContactJid))
    {
      html += itemMask.arg(tr("Name:")).arg(Qt::escape(FClientInfo->softwareName(FContactJid)));
      html += itemMask.arg(tr("Version:")).arg(Qt::escape(FClientInfo->softwareVersion(FContactJid)));
      html += itemMask.arg(tr("OS:")).arg(Qt::escape(FClientInfo->softwareOs(FContactJid)));
    }
    else if (FClientInfo->softwareStatus(FContactJid) == IClientInfo::SoftwareError)
      html += itemMask.arg(tr("Error:")).arg(FClientInfo->softwareName(FContactJid));
    else if (FClientInfo->softwareStatus(FContactJid) == IClientInfo::SoftwareNotLoaded)
      html += tr("Not loaded<br>");
    else
      html += tr("Loading ...<br>");
    html += "<br>";
  }

  //Last Activity
  html += tr("<b>Last Activity</b><br>");
  if (FClientInfo->hasLastActivity(FContactJid))
  {
    html += itemMask.arg(tr("Date:")).arg(Qt::escape(FClientInfo->lastActivityTime(FContactJid).toString()));
    html += itemMask.arg(tr("Status:")).arg(Qt::escape(FClientInfo->lastActivityText(FContactJid)));
  }
  else if (FClientInfo->lastActivityRequest(FContactJid).isValid())
    html += itemMask.arg(tr("Error:")).arg(FClientInfo->lastActivityText(FContactJid));
  else
    html += tr("Not loaded<br>");
  html += "<br>";

  ui.tedText->setHtml(html);
  this->adjustSize();
}

void ClientInfoDialog::onClientInfoChanged(const Jid &AConatctJid)
{
  if (FContactJid == AConatctJid)
    updateText();
}

