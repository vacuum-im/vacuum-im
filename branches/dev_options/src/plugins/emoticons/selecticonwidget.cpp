#include "selecticonwidget.h"

#include <QCursor>
#include <QToolTip>

SelectIconWidget::SelectIconWidget(IconStorage *AStorage, QWidget *AParent) : QWidget(AParent)
{
  FPressed = NULL;
  FStorage = AStorage;

  FLayout = new QGridLayout(this);
  FLayout->setMargin(2);
  FLayout->setHorizontalSpacing(3);
  FLayout->setVerticalSpacing(3);

  createLabels();
}

SelectIconWidget::~SelectIconWidget()
{

}

void SelectIconWidget::createLabels()
{
  QList<QString> keys = FStorage->fileFirstKeys();

  int columns = keys.count()/2 + 1;
  while (columns>1 && columns*columns>keys.count())
    columns--;

  int row =0;
  int column = 0;
  foreach(QString key, keys)
  {
    QLabel *label = new QLabel(this);
    label->setMargin(2);
    label->setAlignment(Qt::AlignCenter);
    label->setFrameShape(QFrame::Box);
    label->setFrameShadow(QFrame::Sunken);
    label->setToolTip(key);
    label->installEventFilter(this);
    FStorage->insertAutoIcon(label,key,0,0,"pixmap");
    FKeyByLabel.insert(label,key);
    FLayout->addWidget(label,row,column);
    column = (column+1) % columns;
    row += column==0 ? 1 : 0;
  }
}

bool SelectIconWidget::eventFilter(QObject *AWatched, QEvent *AEvent)
{
  QLabel *label = qobject_cast<QLabel *>(AWatched);
  if (AEvent->type() == QEvent::Enter)
  {
    label->setFrameShadow(QFrame::Plain);
    QToolTip::showText(QCursor::pos(),label->toolTip());
  }
  else if (AEvent->type() == QEvent::Leave)
  {
    label->setFrameShadow(QFrame::Sunken);
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
