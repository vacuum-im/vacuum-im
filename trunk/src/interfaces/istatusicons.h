#ifndef ISTATUSICONS_H
#define ISTATUSICONS_H

#include <QString>
#include <QIcon>
#include "../../utils/jid.h"

#define STATUSICONS_UUID "{E477B0F3-5683-4a4f-883D-7E7D1ADF25FE}"

class IStatusIcons
{
public:
  enum RuleType {
    UserRule,
    DefaultRule
  };
public:
  virtual QObject *instance() =0;
  virtual QString defaultSubStorage() const =0;
  virtual void setDefaultSubStorage(const QString &ASubStorage) =0;
  virtual void insertRule(const QString &APattern, const QString &ASubStorage, RuleType ARuleType) =0;
  virtual QStringList rules(RuleType ARuleType) const =0;
  virtual QString ruleSubStorage(const QString &APattern, RuleType ARuleType) const =0;
  virtual void removeRule(const QString &APattern, RuleType ARuleType) =0;
  virtual QIcon iconByJid(const Jid &AStreamJid, const Jid &AContactJid) const =0;
  virtual QIcon iconByStatus(int AShow, const QString &ASubscription, bool AAsk) const =0;
  virtual QIcon iconByJidStatus(const Jid &AContactJid, int AShow, const QString &ASubscription, bool AAsk) const =0;
  virtual QString subStorageByJid(const Jid &AContactJid) const =0;
  virtual QString iconKeyByStatus(int AShow, const QString &ASubscription, bool AAsk) const =0;
signals:
  virtual void defaultStorageChanged(const QString &ASubStorage) =0;
  virtual void ruleInserted(const QString &APattern, const QString &ASubStorage, RuleType ARuleType) =0;
  virtual void ruleRemoved(const QString &APattern, RuleType ARuleType) =0;
  virtual void defaultIconsChanged() =0;
  virtual void statusIconsChanged() =0;
};

Q_DECLARE_INTERFACE(IStatusIcons,"Vacuum.Plugin.IStatusIcons/1.0")

#endif
