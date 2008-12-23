#ifndef SELECTICONWIDGET_H
#define SELECTICONWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QEvent>
#include <QGridLayout>
#include "../../utils/skin.h"

class SelectIconWidget : 
  public QWidget
{
  Q_OBJECT;
public:
  SelectIconWidget(const QString &AIconsetFile, QWidget *AParent = NULL);
  ~SelectIconWidget();
  QString iconsetFile() const { return FIconsetFile; }
signals:
  virtual void iconSelected(const QString &AIconsetFile, const QString &AIconfile);
protected:
  void createLabels();
protected:
  virtual bool eventFilter(QObject *AWatched, QEvent *AEvent);
private:
  QLabel *FCurrent;
  QLabel *FPressed;
  QString FIconsetFile;
  QGridLayout *FLayout;
  QHash<QLabel *, QString> FFileByLabel;
};

#endif // SELECTICONWIDGET_H
