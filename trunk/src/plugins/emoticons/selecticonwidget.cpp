#include "selecticonwidget.h"

#include <QSet>
#include <QCursor>
#include <QToolTip>

SelectIconWidget::SelectIconWidget(IconStorage *AStorage, QWidget *AParent) : QWidget(AParent)
{
  FCurrent = NULL;
  FPressed = NULL;
  FStorage = AStorage;

  FLayout = new QGridLayout(this);
  FLayout->setMargin(2);
  FLayout->setHorizontalSpacing(3);
  FLayout->setVerticalSpacing(3);

  AParent->setAttribute(Qt::WA_AlwaysShowToolTips,true);
  setMouseTracking(true);
  setAutoFillBackground(true);
  createLabels();
}

SelectIconWidget::~SelectIconWidget()
{

}

void SelectIconWidget::createLabels()
{
  int column = 0;
  int row =0;
 
  QList<QString> keys = FStorage->fileFirstKeys();

  int columns = keys.count()/2;
  while (columns>1 && columns*columns>keys.count())
    columns--;

  foreach(QString key, keys)
  {
    QLabel *label = new QLabel(this);
    label->setMargin(2);
    label->setAlignment(Qt::AlignCenter);
    label->setFrameShape(QFrame::Panel);
    label->setToolTip(key);
    FStorage->insertAutoIcon(label,key,0,0,"pixmap");
    label->installEventFilter(this);
    FKeyByLabel.insert(label,key);
    FLayout->addWidget(label,row,column);
    column = (column+1) % columns;
    if(column == 0)
      row++;
  }
}

bool SelectIconWidget::eventFilter(QObject *AWatched, QEvent *AEvent)
{
  QLabel *label = qobject_cast<QLabel *>(AWatched);
  if (AEvent->type() == QEvent::Enter)
  {
    FCurrent = label;
    label->setFrameShadow(QFrame::Sunken);
    QToolTip::showText(QCursor::pos(),label->toolTip());
  }
  else if (AEvent->type() == QEvent::Leave)
  {
    label->setFrameShadow(QFrame::Plain);
  }
  else if (AEvent->type() == QEvent::MouseButtonPress)
  {
    FPressed = label;
  }
  else if (AEvent->type() == QEvent::MouseButtonRelease)
  {
    if (FPressed == label)
      emit iconSelected(FStorage->subStorage(),FKeyByLabel.value(label));
    FPressed = NULL;
  }
  return QWidget::eventFilter(AWatched,AEvent);
}

