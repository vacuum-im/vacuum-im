#include "clientinfodialog.h"

#define IN_CLIENTINFO                   "psi/help"

ClientInfoDialog::ClientInfoDialog(IClientInfo *AClientInfo, const Jid &AStreamJid, const Jid &AContactJid,
                                   const QString &AContactName, int AInfoTypes, QWidget *AParent) : QDialog(AParent)
{
  ui.setupUi(this);
  setAttribute(Qt::WA_DeleteOnClose,true);
  setWindowTitle(tr("Client info - %1").arg(AContactName));
  setWindowIcon(Skin::getSkinIconset(SYSTEM_ICONSETFILE)->iconByName(IN_CLIENTINFO));

  FClientInfo = AClientInfo;
  FStreamJid = AStreamJid;
  FContactJid = AContactJid;
  FContactName = AContactName;
  FInfoTypes = AInfoTypes;

  connect(FClientInfo->instance(),SIGNAL(softwareInfoChanged(const Jid &)),SLOT(onClientInfoChanged(const Jid &)));
  connect(FClientInfo->instance(),SIGNAL(lastActivityChanged(const Jid &)),SLOT(onClientInfoChanged(const Jid &)));
  connect(FClientInfo->instance(),SIGNAL(entityTimeChanged(const Jid &)),SLOT(onClientInfoChanged(const Jid &)));

  setInfoTypes(AInfoTypes);
}

ClientInfoDialog::~ClientInfoDialog()
{
  emit clientInfoDialogClosed(FContactJid);
}

void ClientInfoDialog::setInfoTypes(int AInfoTypes)
{
  FInfoTypes = AInfoTypes;
  if ((FInfoTypes & IClientInfo::SoftwareVersion)>0)
    FClientInfo->requestSoftwareInfo(FStreamJid,FContactJid);
  if ((FInfoTypes & IClientInfo::LastActivity)>0)
    FClientInfo->requestLastActivity(FStreamJid,FContactJid);
  if ((FInfoTypes & IClientInfo::EntityTime)>0)
    FClientInfo->requestEntityTime(FStreamJid,FContactJid);
  updateText();
}

void ClientInfoDialog::updateText()
{
  QString itemMask = "%1 %2<br>";
  QString html = QString("<b>%1</b> (%2)<br><br>").arg(Qt::escape(FContactName)).arg(FContactJid.hFull());

  //Software Info
  if ((FInfoTypes & IClientInfo::SoftwareVersion)>0)
  {
    html += tr("<b>Software Version</b><br>");
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
  if ((FInfoTypes & IClientInfo::LastActivity)>0)
  {
    html += tr("<b>Last Activity</b><br>");
    if (FClientInfo->hasLastActivity(FContactJid))
    {
      html += itemMask.arg(tr("Date:")).arg(FClientInfo->lastActivityTime(FContactJid).toString());
      html += itemMask.arg(tr("Status:")).arg(Qt::escape(FClientInfo->lastActivityText(FContactJid)));
    }
    else if (!FClientInfo->lastActivityText(FContactJid).isEmpty())
      html += itemMask.arg(tr("Error:")).arg(FClientInfo->lastActivityText(FContactJid));
    else
      html += tr("Not loaded<br>");
    html += "<br>";
  }

  //Entity Time
  if ((FInfoTypes & IClientInfo::EntityTime)>0)
  {
    html += tr("<b>Entity time</b><br>");
    if (FClientInfo->hasEntityTime(FContactJid))
    {
      html += itemMask.arg(tr("Time:")).arg(FClientInfo->entityTime(FContactJid).time().toString());
      html += itemMask.arg(tr("Delta:")).arg(secsToString(FClientInfo->entityTimeDelta(FContactJid)));
      html += itemMask.arg(tr("Ping, msec:")).arg(FClientInfo->entityTimePing(FContactJid));
    }
    else if (FClientInfo->entityTimePing(FContactJid) < -1)
      html += tr("Loading ...<br>");
    else
      html += tr("Not available<br>");
    html += "<br>";
  }

  ui.tedText->setHtml(html);
  this->adjustSize();
}

QString ClientInfoDialog::secsToString(int ASecs) const
{
  const static int secsInMinute = 60;
  const static int secsInHour = 60*secsInMinute;
  const static int secsInDay = 24*secsInHour;
  const static int secsInYear = 365*secsInDay;

  QString time;
  int secs = ASecs;

  int years = secs / secsInYear;
  secs -= years * secsInYear;

  int days = secs / secsInDay;
  secs -= days * secsInDay;

  int hours = secs / secsInHour;
  secs -= hours * secsInHour;

  int minutes = secs / secsInMinute;
  secs -= minutes * secsInMinute;

  if (years > 0)
    time += tr("%1y ").arg(years);
  if (days > 0)
    time += tr("%1d ").arg(days);
  if (hours > 0)
    time += tr("%1h ").arg(hours);
  if (minutes > 0)
    time += tr("%1m ").arg(minutes);
  time += tr("%1s").arg(secs);

  return time;
}

void ClientInfoDialog::onClientInfoChanged(const Jid &AConatctJid)
{
  if (FContactJid == AConatctJid)
    updateText();
}

