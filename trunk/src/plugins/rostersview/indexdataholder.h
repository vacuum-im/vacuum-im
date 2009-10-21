#ifndef INDEXDATAHOLDER_H
#define INDEXDATAHOLDER_H

#include <definations/rosterindextyperole.h>
#include <definations/rosterdataholderorders.h>
#include <interfaces/irostersmodel.h>
#include <interfaces/irostersview.h>

class IndexDataHolder : 
  public QObject,
  public IRosterIndexDataHolder
{
  Q_OBJECT;
  Q_INTERFACES(IRosterIndexDataHolder);

public:
  IndexDataHolder(QObject *AParent);
  ~IndexDataHolder();

  //IRosterIndexDataHolder
  virtual QObject *instance() { return this; }
  virtual int order() const { return RDHO_DEFAULT; }
  virtual QList<int> roles() const;
  virtual QList<int> types() const;
  virtual QVariant data(const IRosterIndex *AIndex, int ARole) const;
  virtual bool setData(IRosterIndex * /*AIndex*/, int /*ARole*/, const QVariant &/*AValue*/) { return false; }
public:
  bool checkOption(IRostersView::Option AOption) const;
  void setOption(IRostersView::Option AOption, bool AValue);
signals:
  virtual void dataChanged(IRosterIndex *AIndex = NULL, int ARole = RDR_ANY_ROLE);
protected:
  QString toolTipText(const IRosterIndex *AIndex) const;
private:
  int FOptions;
};

#endif // INDEXDATAHOLDER_H
