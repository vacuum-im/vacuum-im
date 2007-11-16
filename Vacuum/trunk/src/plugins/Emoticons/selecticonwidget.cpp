#include "selecticonwidget.h"
#include <QSet>

#define SMILEY_TAG_NAME                         "text"

SelectIconWidget::SelectIconWidget(const QString &AIconsetFile, QWidget *AParent)
  : QWidget(AParent)
{
  FCurrent = NULL;
  FPressed = NULL;
  FIconsetFile = AIconsetFile;

  FLayout = new QGridLayout(this);
  FLayout->setMargin(2);
  FLayout->setHorizontalSpacing(3);
  FLayout->setVerticalSpacing(3);

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
  const int columns = 7;
  
  SkinIconset *iconset = Skin::getSkinIconset(FIconsetFile);
  if (iconset->isValid())
  {
    QSet<QString> usedTags;
    QList<QString> files = iconset->iconFiles();
    foreach(QString file, files)
    {
      QSet<QString> fileTags = iconset->tagValues(SMILEY_TAG_NAME,file).toSet();
      if ((fileTags & usedTags).isEmpty())
      {
        QLabel *label = new QLabel(this);
        label->setMargin(2);
        label->setAlignment(Qt::AlignCenter);
        label->setFrameShape(QFrame::Panel);
        label->setPixmap(iconset->iconByFile(file).pixmap(QSize(16,16)));
        label->installEventFilter(this);
        FFileByLabel.insert(label,file);
        FLayout->addWidget(label,row,column);
        column = (column+1) % columns;
        if(column == 0)
          row++;
        usedTags += fileTags;
      }
    }
  }
}

bool SelectIconWidget::eventFilter(QObject *AWatched, QEvent *AEvent)
{
  QLabel *label = qobject_cast<QLabel *>(AWatched);
  if (AEvent->type() == QEvent::Enter)
  {
    FCurrent = label;
    label->setFrameShadow(QFrame::Sunken);
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
    if (FPressed = label)
      emit iconSelected(FIconsetFile,FFileByLabel.value(label));
    FPressed = NULL;
  }
  return QWidget::eventFilter(AWatched,AEvent);
}

