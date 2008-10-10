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
  virtual QString defaultIconFile() const =0;
  virtual void setDefaultIconFile(const QString &AIconFile) =0;
  virtual void insertRule(const QString &APattern, const QString &AIconFile, RuleType ARuleType) =0;
  virtual QStringList rules(RuleType ARuleType) const =0;
  virtual QString ruleIconFile(const QString &APattern, RuleType ARuleType) const =0;
  virtual void removeRule(const QString &APattern, RuleType ARuleType) =0;
  virtual QIcon iconByJid(const Jid &AStreamJid, const Jid &AJid) const =0;
  virtual QIcon iconByJidStatus(const Jid &AJid, int AShow, const QString &ASubscription, bool AAsk) const =0;
  virtual QString iconFileByJid(const Jid &AJid) const =0;
  virtual QString iconNameByStatus(int AShow, const QString &ASubscription, bool AAsk) const =0;
  virtual QIcon iconByStatus(int AShow, const QString &ASubscription, bool AAsk) const =0;
signals:
  virtual void defaultIconFileChanged(const QString &AIconFile) =0;
  virtual void ruleInserted(const QString &APattern, const QString &AIconFile, RuleType ARuleType) =0;
  virtual void ruleRemoved(const QString &APattern, RuleType ARuleType) =0;
  virtual void defaultIconsChanged() =0;
  virtual void statusIconsChanged() =0;
  virtual void optionsAccepted() =0;
  virtual void optionsRejected() =0;
};

Q_DECLARE_INTERFACE(IStatusIcons,"Vacuum.Plugin.IStatusIcons/1.0")

#endif
