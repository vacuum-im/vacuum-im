#ifndef SELECTICONWIDGET_H
#define SELECTICONWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QEvent>
#include <QGridLayout>
#include <utils/iconstorage.h>

class SelectIconWidget : 
  public QWidget
{
  Q_OBJECT;
public:
  SelectIconWidget(IconStorage *AStorage, QWidget *AParent = NULL);
  ~SelectIconWidget();
  QString iconset() const { return FStorage->subStorage(); }
signals:
  void iconSelected(const QString &ASubStorage, const QString &AIconKey);
protected:
  void createLabels();
protected:
  virtual bool eventFilter(QObject *AWatched, QEvent *AEvent);
private:
  QLabel *FCurrent;
  QLabel *FPressed;
  QGridLayout *FLayout;
  IconStorage *FStorage;
  QHash<QLabel *, QString> FKeyByLabel;
};

#endif // SELECTICONWIDGET_H
