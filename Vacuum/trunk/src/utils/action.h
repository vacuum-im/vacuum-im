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
    CT_UserDefined = 64
  }; 

public:
  Action(int AOrder, const QString &AActionId, QObject *AParent = NULL);
  ~Action();

  int order() const { return FOrder; }
  const QString &actionId() const { return FActionId; }
  bool isContextDepended() const { return FContextDepended; }
  void setContextDepended(bool AContextDepended) { FContextDepended = AContextDepended; }
  bool contextIsSupported(const ActionContext &AContext) const;
  const ActionContext &context() const { return FContext; }
  void setContext(const ActionContext &AContext);
signals:
  void newContext(Action *, const ActionContext &);
  void supportContext(const Action *, const ActionContext &, bool &) const;
protected:
  void onNewContext();
private:
  int FOrder;
  QString FActionId;
  bool FContextDepended;
  ActionContext FContext;
};

#endif // ACTION_H
