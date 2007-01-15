#ifndef ACTION_H
#define ACTION_H

#include <QAction>
#include <QHash>
#include <QVariant>
#include "utilsexport.h"

typedef QHash<int,QVariant> ActionContext;

class UTILS_EXPORT Action : 
  public QAction
{
  Q_OBJECT;

public:
  enum ContextTypes {
    CT_UserDefined = 32
  }; 

public:
  Action(int AOrder, QObject *AParent);
  Action(int AOrder, const QString &AText, QObject *AParent);
  Action(int AOrder, const QIcon &AIcon, const QString &AText, QObject *AParent);
  ~Action();

  int order() const { return FOrder; }
  bool isContextDepended() const { return FContextDepended; }
  void setContextDepended(bool AContextDepended) { FContextDepended = AContextDepended; }
  bool contextSupported(const ActionContext &AContext) const;
  const ActionContext &context() const { return FContext; }
  void setContext(const ActionContext &AContext);
signals:
  void newContext(Action *, const ActionContext &);
  void supportContext(const Action *, const ActionContext &, bool &) const;
protected:
  void onNewContext();
private:
  int FOrder;
  bool FContextDepended;
  ActionContext FContext;
};

#endif // ACTION_H
