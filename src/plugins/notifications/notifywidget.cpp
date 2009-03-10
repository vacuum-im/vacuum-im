#include "notifywidget.h"

#define DEFAUTL_TIMEOUT     8000
#define ANIMATE_STEPS       10
#define ANIMATE_TIME        500
#define ANIMATE_STEP_TIME   (ANIMATE_TIME/ANIMATE_STEPS)

QDesktopWidget *NotifyWidget::FDesktop = new QDesktopWidget;
QList<NotifyWidget *> NotifyWidget::FWidgets;

NotifyWidget::NotifyWidget(const INotification &ANotification)
{
  ui.setupUi(this);
  setAttribute(Qt::WA_DeleteOnClose,false);
  setAttribute(Qt::WA_ShowWithoutActivating,true);
  setWindowFlags(Qt::SplashScreen);

  FXPos = -1;
  FYPos = -1;
  FAnimateStep=0;

  QIcon icon = qvariant_cast<QIcon>(ANotification.data.value(NDR_ICON));
  QImage image = qvariant_cast<QImage>(ANotification.data.value(NDR_WINDOW_IMAGE));
  QString caption = ANotification.data.value(NDR_WINDOW_CAPTION,tr("Notification")).toString();
  QString title = ANotification.data.value(NDR_WINDOW_TITLE).toString();
  QString text = ANotification.data.value(NDR_WINDOW_TEXT).toString();
  FTimeOut = ANotification.data.value(NDR_WINDOW_TIMEOUT,DEFAUTL_TIMEOUT).toInt();

  if (!caption.isEmpty())
    ui.lblCaption->setText(caption);
  else
    ui.lblCaption->setVisible(false);

  if (!icon.isNull())
    ui.lblIcon->setPixmap(icon.pixmap(QSize(32,32)));
  else
    ui.lblIcon->setVisible(false);
  
  if (!title.isEmpty())
    ui.lblTitle->setText(title);
  else
    ui.lblTitle->setVisible(false);

  if (!text.isEmpty())
  {
    if (!image.isNull())
      ui.lblImage->setPixmap(QPixmap::fromImage(image.scaled(ui.lblImage->maximumSize(),Qt::KeepAspectRatio)));
    else
      ui.lblImage->setVisible(false);
    ui.lblText->setText(text);
  }
  else
  {
    ui.lblImage->setVisible(false);
    ui.lblText->setVisible(false);
  }
}

NotifyWidget::~NotifyWidget()
{
  disappear();
}

void NotifyWidget::appear()
{
  if (!FWidgets.contains(this))
  {
    FWidgets.append(this);
    layoutWidgets();
  }
}

void NotifyWidget::animateTo(int AXPos, int AYPos)
{
  QRect display = FDesktop->availableGeometry();
  if (display.contains(AXPos,AYPos))
  {
    if (!isVisible())
    {
      if (FTimeOut>0)
        QTimer::singleShot(FTimeOut,this,SLOT(disappear()));
      move(display.right(),AYPos);
      show();
    }
    if (FXPos!= AXPos || FYPos!=AYPos)
    {
      FXPos = AXPos;
      FYPos = AYPos;
      FAnimateStep = ANIMATE_STEPS;
      QTimer::singleShot(ANIMATE_STEP_TIME,this,SLOT(animateStep()));
    }
  }
}

void NotifyWidget::animateStep()
{
  if (FAnimateStep>0)
  {
    QTimer::singleShot(ANIMATE_STEP_TIME,this,SLOT(animateStep()));
    int xpos = x()+qRound((FXPos-x())/(FAnimateStep+1));
    int ypos = y()+qRound((FYPos-y())/(FAnimateStep+1));
    move(xpos,ypos);
    FAnimateStep--;
  }
  else
    move(FXPos,FYPos);
}

void NotifyWidget::disappear()
{
  if (FWidgets.contains(this))
  {
    FWidgets.removeAt(FWidgets.indexOf(this));
    close();
    layoutWidgets();
    emit windowDestroyed();
  }
}

void NotifyWidget::mouseReleaseEvent(QMouseEvent *AEvent)
{
  QWidget::mouseReleaseEvent(AEvent);
  if (AEvent->button() == Qt::LeftButton)
    emit notifyActivated();
  else if (AEvent->button() == Qt::RightButton)
    emit notifyRemoved();
}

void NotifyWidget::layoutWidgets()
{
  QRect display = FDesktop->availableGeometry();
  int ypos = display.bottom();
  foreach(NotifyWidget *widget, FWidgets)
  {
    QSize size = widget->frameGeometry().size();
    ypos -= size.height();
    int xpos = display.right() - size.width();
    widget->animateTo(xpos,ypos);
  }
}

