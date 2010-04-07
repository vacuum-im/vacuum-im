#ifndef EMOTICONSOPTIONS_H
#define EMOTICONSOPTIONS_H

#include <QWidget>
#include <definations/resources.h>
#include <definations/menuicons.h>
#include <interfaces/iemoticons.h>
#include <utils/iconsetdelegate.h>
#include "ui_emoticonsoptions.h"

class EmoticonsOptions : 
  public QWidget
{
  Q_OBJECT;
public:
  EmoticonsOptions(IEmoticons *AEmoticons, QWidget *AParent = NULL);
  ~EmoticonsOptions();
public slots:
  void apply();
signals:
  void optionsAccepted();
protected:
  void init();
protected slots:
  void onUpButtonClicked();
  void onDownButtonClicked();
private:
  Ui::EmoticonsOptionsClass ui;
private:
  IEmoticons *FEmoticons;
};

#endif // EMOTICONSOPTIONS_H
