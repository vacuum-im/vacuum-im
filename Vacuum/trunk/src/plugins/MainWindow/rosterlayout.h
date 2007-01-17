#ifndef ROSTERLAYOUT_H
#define ROSTERLAYOUT_H

#include <QLayoutItem>
#include <QLayout>

class RosterLayout : 
  public QLayout
{
  Q_OBJECT;

public:
  RosterLayout(QWidget *AParent);
  ~RosterLayout();

  virtual void addItem(QLayoutItem *AItem);
  virtual int count() const { return FItemList.count(); }
  virtual QSize sizeHint() const;
  virtual QSize minimumSize() const;
  virtual QLayoutItem *itemAt(int AIndex) const;
  virtual QLayoutItem *takeAt(int AIndex);
  virtual void setGeometry(const QRect &ARect);

private:
  QList<QLayoutItem *> FItemList;    
};

#endif // ROSTERLAYOUT_H
