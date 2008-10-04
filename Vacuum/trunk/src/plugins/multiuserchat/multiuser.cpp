#include "multiuser.h"

MultiUser::MultiUser(const Jid &ARoomJid, const QString &ANickName, QObject *AParent) : QObject(AParent)
{
  FRoomJid = ARoomJid;
  FContactJid = ARoomJid;
  FContactJid.setResource(ANickName);
  FNickName = ANickName;
  setData(MUDR_ROOMJID,FRoomJid.bare());
  setData(MUDR_NICK_NAME,FNickName);
  setData(MUDR_CONTACTJID,FContactJid.full());
  setData(MUDR_SHOW,IPresence::Offline);
  setData(MUDR_STATUS,tr("Disconnected"));
  setData(MUDR_ROLE,MUC_ROLE_NONE);
  setData(MUDR_AFFILIATION,MUC_AFFIL_NONE);
}

MultiUser::~MultiUser()
{

}

QVariant MultiUser::data(int ARole) const
{
  return FData.value(ARole);
}

void MultiUser::setData(int ARole, const QVariant &AValue)
{
  QVariant befour = data(ARole);
  if (befour != AValue)
  {
    if (AValue.isValid())
      FData.insert(ARole,AValue);
    else 
      FData.remove(ARole);
    emit dataChanged(ARole,befour,AValue);
  }
}

void MultiUser::setNickName(const QString &ANickName)
{
  FNickName = ANickName;
  FContactJid.setResource(ANickName);
  setData(MUDR_NICK_NAME,ANickName);
  setData(MUDR_CONTACTJID,FContactJid.full());
}