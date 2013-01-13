#include "notifywidget.h"

#include <QTimer>

#define DEFAUTL_TIMEOUT           8000
#define ANIMATE_STEPS             17
#define ANIMATE_TIME              700
#define ANIMATE_STEP_TIME         (ANIMATE_TIME/ANIMATE_STEPS)
#define ANIMATE_OPACITY_START     0.0
#define ANIMATE_OPACITY_END       0.9
#define ANIMATE_OPACITY_STEP      (ANIMATE_OPACITY_END - ANIMATE_OPACITY_START)/ANIMATE_STEPS

QList<NotifyWidget *> NotifyWidget::FWidgets;
QDesktopWidget *NotifyWidget::FDesktop = new QDesktopWidget;

NotifyWidget::NotifyWidget(const INotification &ANotification) : QWidget(NULL, Qt::ToolTip|Qt::WindowStaysOnTopHint)
{
	ui.setupUi(this);
	setFocusPolicy(Qt::NoFocus);
	setAttribute(Qt::WA_DeleteOnClose,true);

	QPalette pallete = ui.frmWindowFrame->palette();
	pallete.setColor(QPalette::Window, pallete.color(QPalette::Base));
	ui.frmWindowFrame->setPalette(pallete);

	FYPos = -1;
	FAnimateStep = -1;

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
	FWidgets.removeAll(this);
	layoutWidgets();
	emit windowDestroyed();
}

void NotifyWidget::appear()
{
	if (!FWidgets.contains(this))
	{
		QTimer *timer = new QTimer(this);
		timer->setSingleShot(false);
		timer->setInterval(ANIMATE_STEP_TIME);
		timer->start();
		connect(timer,SIGNAL(timeout()),SLOT(onAnimateStep()));

		if (FTimeOut > 0)
			QTimer::singleShot(FTimeOut,this,SLOT(deleteLater()));

		setWindowOpacity(ANIMATE_OPACITY_START);

		FWidgets.prepend(this);
		layoutWidgets();
	}
}

void NotifyWidget::animateTo(int AYPos)
{
	if (FYPos != AYPos)
	{
		FYPos = AYPos;
		FAnimateStep = ANIMATE_STEPS;
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

void NotifyWidget::onAnimateStep()
{
	if (FAnimateStep > 0)
	{
		int ypos = y()+(FYPos-y())/(FAnimateStep);
		setWindowOpacity(qMin(windowOpacity()+ANIMATE_OPACITY_STEP, ANIMATE_OPACITY_END));
		move(x(),ypos);
		FAnimateStep--;
	}
	else if (FAnimateStep == 0)
	{
		move(x(),FYPos);
		setWindowOpacity(ANIMATE_OPACITY_END);
		FAnimateStep--;
	}
}

void NotifyWidget::layoutWidgets()
{
	QRect display = FDesktop->availableGeometry();
	int ypos = display.bottom();
	for (int i=0; ypos>0 && i<FWidgets.count(); i++)
	{
		NotifyWidget *widget = FWidgets.at(i);
		if (!widget->isVisible())
		{
			widget->show();
			WidgetManager::raiseWidget(widget);
			widget->move(display.right() - widget->frameGeometry().width(), display.bottom());
		}
		ypos -=  widget->frameGeometry().height();
		widget->animateTo(ypos);
	}
}
