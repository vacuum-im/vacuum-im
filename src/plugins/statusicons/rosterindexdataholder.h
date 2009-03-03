#ifndef ROSTERINDEXDATAHOLDER_H
#define ROSTERINDEXDATAHOLDER_H

#include "../../definations/rosterindextyperole.h"
#include "../../definations/rosterdataholderorders.h"
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

  virtual int order() const { return RDHO_STATUSICONS; }
  virtual QList<int> roles() const;
  virtual QList<int> types() const;
  virtual QVariant data(const IRosterIndex *AIndex, int ARole) const;
  virtual bool setData(IRosterIndex * /*AIndex*/, int /*ARole*/, const QVariant &/*AValue*/) { return false; }
signals:
  virtual void dataChanged(IRosterIndex *AIndex = NULL, int ARole = RDR_ANY_ROLE);
protected slots:
  void onStatusIconsChanged();
private:
  IStatusIcons *FStatusIcons;    
};

#endif // ROSTERINDEXDATAHOLDER_H
