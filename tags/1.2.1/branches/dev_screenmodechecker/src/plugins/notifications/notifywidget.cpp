#include "notifywidget.h"

#include <QTimer>
#include <QScrollBar>
#include <QTextFrame>
#include <QTextDocument>

#define ANIMATE_STEPS             17
#define ANIMATE_TIME              700
#define ANIMATE_STEP_TIME         (ANIMATE_TIME/ANIMATE_STEPS)
#define ANIMATE_OPACITY_START     0.0
#define ANIMATE_OPACITY_END       0.9
#define ANIMATE_OPACITY_STEP      (ANIMATE_OPACITY_END - ANIMATE_OPACITY_START)/ANIMATE_STEPS

#define MAX_TEXT_LINES            5

QList<NotifyWidget *> NotifyWidget::FWidgets;
QDesktopWidget *NotifyWidget::FDesktop = new QDesktopWidget;
IMainWindow *NotifyWidget::FMainWindow = NULL;
QRect NotifyWidget::FDisplay = QRect();

NotifyWidget::NotifyWidget(const INotification &ANotification) : QWidget(NULL, Qt::ToolTip|Qt::WindowStaysOnTopHint|Qt::X11BypassWindowManagerHint)
{
	ui.setupUi(this);
	setFocusPolicy(Qt::NoFocus);
	setAttribute(Qt::WA_DeleteOnClose,true);

	QPalette pallete = ui.frmWindowFrame->palette();
	pallete.setColor(QPalette::Window, pallete.color(QPalette::Base));
	ui.frmWindowFrame->setPalette(pallete);
	ui.frmWindowFrame->setAutoFillBackground(true);
	ui.frmWindowFrame->setAttribute(Qt::WA_TransparentForMouseEvents,true);

	FYPos = -1;
	FAnimateStep = -1;
	FTimeOut = ANotification.data.value(NDR_POPUP_TIMEOUT,Options::node(OPV_NOTIFICATIONS_POPUPTIMEOUT).value().toInt()*1000).toInt();

	FCaption = ANotification.data.value(NDR_POPUP_CAPTION,tr("Notification")).toString();
	ui.lblCaption->setVisible(!FCaption.isEmpty());

	FTitle = ANotification.data.value(NDR_POPUP_TITLE).toString();
	ui.lblTitle->setVisible(!FTitle.isEmpty());

	QIcon icon = qvariant_cast<QIcon>(ANotification.data.value(NDR_ICON));
	if (!icon.isNull())
		ui.lblIcon->setPixmap(icon.pixmap(QSize(32,32)));
	else
		ui.lblIcon->setVisible(false);

	QString text = ANotification.data.value(NDR_POPUP_HTML).toString();
	if (!text.isEmpty())
	{
		QTextDocument doc;
		doc.setHtml(text);
		if (doc.rootFrame()->lastPosition() > 140)
		{
			QTextCursor cursor(&doc);
			cursor.movePosition(QTextCursor::NextCharacter,QTextCursor::MoveAnchor,120);
			cursor.movePosition(QTextCursor::End,QTextCursor::KeepAnchor);
			cursor.removeSelectedText();
			cursor.insertText("...");
			text = TextManager::getDocumentBody(doc);
		}
		ui.ntbText->setHtml(text);
		ui.ntbText->setContentsMargins(0,0,0,0);
		ui.ntbText->setWordWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
		ui.ntbText->setMaxHeight(ui.ntbText->fontMetrics().height()*MAX_TEXT_LINES + (ui.ntbText->frameWidth() + qRound(ui.ntbText->document()->documentMargin()))*2);

		QImage image = qvariant_cast<QImage>(ANotification.data.value(NDR_POPUP_IMAGE));
		if (!image.isNull())
			ui.lblImage->setPixmap(QPixmap::fromImage(image.scaled(ui.lblImage->maximumSize(),Qt::KeepAspectRatio)));
		else
			ui.lblImage->setVisible(false);
	}
	else
	{
		ui.lblImage->setVisible(false);
		ui.ntbText->setVisible(false);
	}

	updateElidedText();
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

		if (FWidgets.isEmpty())
			FDisplay = FDesktop->availableGeometry(FMainWindow->instance());
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

void NotifyWidget::setAnimated(bool AAnimated)
{
	ui.ntbText->setAnimated(AAnimated);
}

void NotifyWidget::setNetworkAccessManager(QNetworkAccessManager *ANetworkAccessManager)
{
	ui.ntbText->setNetworkAccessManager(ANetworkAccessManager);
}

void NotifyWidget::setMainWindow(IMainWindow *AMainWindow)
{
	FMainWindow = AMainWindow;
}

void NotifyWidget::resizeEvent(QResizeEvent *AEvent)
{
	QWidget::resizeEvent(AEvent);
	ui.ntbText->verticalScrollBar()->setSliderPosition(0);
	updateElidedText();
	layoutWidgets();
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
	int ypos = FDisplay.bottom();
	for (int i=0; ypos>0 && i<FWidgets.count(); i++)
	{
		NotifyWidget *widget = FWidgets.at(i);
		if (!widget->isVisible())
		{
			widget->show();
			widget->move(FDisplay.right() - widget->frameGeometry().width(), FDisplay.bottom());
			QTimer::singleShot(0,widget,SLOT(adjustHeight()));
			QTimer::singleShot(10,widget,SLOT(adjustHeight()));
		}
		ypos -=  widget->frameGeometry().height();
		widget->animateTo(ypos);
	}
}

void NotifyWidget::adjustHeight()
{
	resize(width(),sizeHint().height());
}

void NotifyWidget::updateElidedText()
{
	ui.lblCaption->setText(ui.lblCaption->fontMetrics().elidedText(FCaption,Qt::ElideRight,ui.lblCaption->width() - ui.lblCaption->frameWidth()*2));
	ui.lblTitle->setText(ui.lblTitle->fontMetrics().elidedText(FTitle,Qt::ElideRight,ui.lblTitle->width() - ui.lblTitle->frameWidth()*2));
}
