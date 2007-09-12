#ifndef ROSTERINDEXDATAHOLDER_H
#define ROSTERINDEXDATAHOLDER_H

#include "../../interfaces/istatusicons.h"
#include "../../interfaces/irostersmodel.h"

class RosterIndexDataHolder : 
  public QObject,
  public IRosterIndexDataHolder
{
  Q_OBJECT;
  Q_INTERFACES(IRosterIndexDataHolder);

public:
  RosterIndexDataHolder(IStatusIcons *AStatusIcons);
  ~RosterIndexDataHolder();

  virtual QObject *instance() { return this; }

  virtual QVariant data(const IRosterIndex *AIndex, int ARole) const;
  virtual bool setData(IRosterIndex *AIndex, int ARole, const QVariant &AValue);
  virtual QList<int> roles() const;
signals:
  virtual void dataChanged(IRosterIndex *AIndex, int ARole);
public:
  QList<int> types() const;
private:
  IStatusIcons *FStatusIcons;    
};

#endif // ROSTERINDEXDATAHOLDER_H
