#ifndef INDEXDATAHOLDER_H
#define INDEXDATAHOLDER_H

#include "../../interfaces/irostersmodel.h"
#include "../../utils/skin.h"

class IndexDataHolder : 
  public QObject,
  public IRosterIndexDataHolder
{
  Q_OBJECT;
  Q_INTERFACES(IRosterIndexDataHolder);

public:
  IndexDataHolder(QObject *AParent);
  ~IndexDataHolder();

  virtual QObject *instance() { return this; }
  virtual bool setData(IRosterIndex *AIndex, int ARole, const QVariant &AValue);
  virtual QVariant data(const IRosterIndex *AIndex, int ARole) const;
  virtual QList<int> roles() const;
signals:
  virtual void dataChanged(IRosterIndex *, int ARole);
protected:
  QIcon statusIcon(const IRosterIndex *AIndex) const;
private:
  SkinIconset FStatusIconset;
};

#endif // INDEXDATAHOLDER_H
