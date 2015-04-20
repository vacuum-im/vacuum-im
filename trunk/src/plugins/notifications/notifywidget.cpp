#include "notifywidget.h"

#include <QTimer>
#include <QScrollBar>
#include <QTextFrame>
#include <QSizePolicy>
#include <QToolButton>
#include <QTextDocument>
#include <definitions/optionvalues.h>
#include <definitions/notificationdataroles.h>
#include <utils/textmanager.h>
#include <utils/options.h>
#include <utils/message.h>
#include <utils/options.h>
#include <utils/logger.h>

#define ANIMATE_STEPS             17
#define ANIMATE_TIME              700
#define ANIMATE_STEP_TIME         (ANIMATE_TIME/ANIMATE_STEPS)
#define ANIMATE_OPACITY_START     0.0
#define ANIMATE_OPACITY_END       0.95
#define ANIMATE_OPACITY_STEP      (ANIMATE_OPACITY_END - ANIMATE_OPACITY_START)/ANIMATE_STEPS

#define LEFT_MARGIN               5
#define BOTTOM_MARGIN             5

#define MAX_TEXT_LINES            5

#define UNDER_MOUSE_ADD_TIMEOUT   15

QRect NotifyWidget::FDisplay = QRect();
QList<NotifyWidget *> NotifyWidget::FWidgets;
QDesktopWidget *NotifyWidget::FDesktop = new QDesktopWidget;

IMainWindow *NotifyWidget::FMainWindow = NULL;
QNetworkAccessManager *NotifyWidget::FNetworkManager = NULL;

NotifyWidget::NotifyWidget(const INotification &ANotification)
#if defined(Q_OS_MAC)
	: QFrame(NULL, Qt::FramelessWindowHint|Qt::WindowSystemMenuHint|Qt::WindowStaysOnTopHint)
#else
	: QFrame(NULL, Qt::ToolTip|Qt::WindowStaysOnTopHint|Qt::X11BypassWindowManagerHint)
#endif
{
	REPORT_VIEW;
	ui.setupUi(this);
	setFocusPolicy(Qt::NoFocus);
	setAttribute(Qt::WA_DeleteOnClose,true);
	setAttribute(Qt::WA_ShowWithoutActivating,true);
	setAttribute(Qt::WA_TransparentForMouseEvents,true);

	QPalette windowPalette = palette();
	windowPalette.setColor(QPalette::Window, windowPalette.color(QPalette::Base));
	setPalette(windowPalette);

	QPalette noticePalette = ui.lblNotice->palette();
	noticePalette.setColor(QPalette::WindowText,noticePalette.color(QPalette::Disabled,QPalette::WindowText));
	ui.lblNotice->setPalette(noticePalette);

	FYPos = -1;
	FAnimateStep = -1;
	FTimeOut = ANotification.data.value(NDR_POPUP_TIMEOUT,Options::node(OPV_NOTIFICATIONS_POPUPTIMEOUT).value().toInt()).toInt();

	FCaption = ANotification.data.value(NDR_POPUP_CAPTION).toString();
	ui.lblCaption->setVisible(!FCaption.isEmpty());
	ui.hlnCaptionLine->setVisible(!FCaption.isEmpty());

	FTitle = ANotification.data.value(NDR_POPUP_TITLE).toString();
	ui.lblTitle->setVisible(!FTitle.isEmpty());

	FNotice = ANotification.data.value(NDR_POPUP_NOTICE).toString();
	ui.lblNotice->setVisible(!FNotice.isEmpty());

	QIcon icon = qvariant_cast<QIcon>(ANotification.data.value(NDR_ICON));
	if (!icon.isNull())
		ui.lblIcon->setPixmap(icon.pixmap(QSize(32,32)));
	else
		ui.lblIcon->setVisible(false);

	QImage image = qvariant_cast<QImage>(ANotification.data.value(NDR_POPUP_IMAGE));
	if (!image.isNull())
		ui.lblAvatar->setPixmap(QPixmap::fromImage(image.scaled(ui.lblAvatar->minimumSize(),Qt::KeepAspectRatio,Qt::SmoothTransformation)));
	else
		ui.lblAvatar->setVisible(false);

	QString htmlText = ANotification.data.value(NDR_POPUP_HTML).toString();
	QString plainText = ANotification.data.value(NDR_POPUP_TEXT).toString();
	if (!plainText.isEmpty() || !htmlText.isEmpty())
	{
		QTextDocument doc;
		if (!htmlText.isEmpty())
			doc.setHtml(htmlText);
		else
			doc.setPlainText(plainText);

		if (doc.rootFrame()->lastPosition() > 140)
		{
			QTextCursor cursor(&doc);
			cursor.movePosition(QTextCursor::NextCharacter,QTextCursor::MoveAnchor,120);
			cursor.movePosition(QTextCursor::End,QTextCursor::KeepAnchor);
			cursor.removeSelectedText();
			cursor.insertText("...");
		}

		ui.ntbText->setAnimated(true);
		ui.ntbText->setContentsMargins(0,0,0,0);
		ui.ntbText->document()->setDocumentMargin(0);
		ui.ntbText->setNetworkAccessManager(FNetworkManager);
		ui.ntbText->setHtml(TextManager::getDocumentBody(doc));
		ui.ntbText->setWordWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
		ui.ntbText->setMaxHeight(ui.ntbText->fontMetrics().height()*MAX_TEXT_LINES + (ui.ntbText->frameWidth() + qRound(ui.ntbText->document()->documentMargin()))*2);
	}
	else
	{
		ui.ntbText->setHtml(QString("<i>%1</i>").arg(tr("Message is empty or hidden")));
	}

	if (!ANotification.actions.isEmpty())
	{
		QHBoxLayout *buttonsLayout = new QHBoxLayout;
		ui.wdtButtons->setLayout(buttonsLayout);
		buttonsLayout->setMargin(0);
		for (int i=0; i<ANotification.actions.count(); i++)
		{
			QToolButton *button = new QToolButton(ui.wdtButtons);
			button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
			button->setDefaultAction(ANotification.actions.at(i));
			button->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Preferred);
			ui.wdtButtons->layout()->addWidget(button);
		}
		buttonsLayout->addStretch();
	}
	else
	{
		ui.wdtButtons->setVisible(false);
	}

	FCloseTimer.setInterval(1000);
	FCloseTimer.setSingleShot(false);
	connect(&FCloseTimer,SIGNAL(timeout()),SLOT(onCloseTimerTimeout()));

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
			FCloseTimer.start();

		setWindowOpacity(ANIMATE_OPACITY_START);

		if (FMainWindow!=NULL && FWidgets.isEmpty())
			FDisplay = FDesktop->availableGeometry(FMainWindow->instance());
		FWidgets.prepend(this);
		layoutWidgets();
	}
}

void NotifyWidget::setMainWindow(IMainWindow *AMainWindow)
{
	FMainWindow = AMainWindow;
}

void NotifyWidget::setNetworkManager(QNetworkAccessManager *ANetworkManager)
{
	FNetworkManager = ANetworkManager;
}

void NotifyWidget::animateTo(int AYPos)
{
	if (FYPos != AYPos)
	{
		FYPos = AYPos;
		FAnimateStep = ANIMATE_STEPS;
	}
}

void NotifyWidget::enterEvent(QEvent *AEvent)
{
	FTimeOut += UNDER_MOUSE_ADD_TIMEOUT;
	QFrame::enterEvent(AEvent);
}

void NotifyWidget::leaveEvent(QEvent *AEvent)
{
	FTimeOut -= UNDER_MOUSE_ADD_TIMEOUT;
	QFrame::leaveEvent(AEvent);
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
	if (geometry().contains(AEvent->globalPos()))
	{
		if (AEvent->button() == Qt::LeftButton)
			emit notifyActivated();
		else if (AEvent->button() == Qt::RightButton)
			emit notifyRemoved();
	}
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

void NotifyWidget::adjustHeight()
{
	resize(width(),minimumSizeHint().height());
}

void NotifyWidget::updateElidedText()
{
	ui.lblCaption->setText(ui.lblCaption->fontMetrics().elidedText(FCaption,Qt::ElideRight,ui.lblCaption->width() - ui.lblCaption->frameWidth()*2));
	ui.lblTitle->setText(ui.lblTitle->fontMetrics().elidedText(FTitle,Qt::ElideRight,ui.lblTitle->width() - ui.lblTitle->frameWidth()*2));
	ui.lblNotice->setText(ui.lblNotice->fontMetrics().elidedText(FNotice,Qt::ElideRight,ui.lblTitle->width() - ui.lblTitle->frameWidth()*2));
}

void NotifyWidget::onCloseTimerTimeout()
{
	if (FTimeOut > 0)
		FTimeOut--;
	else
		deleteLater();
}

void NotifyWidget::layoutWidgets()
{
	int ypos = FDisplay.bottom() - BOTTOM_MARGIN;
	for (int i=0; ypos>0 && i<FWidgets.count(); i++)
	{
		NotifyWidget *widget = FWidgets.at(i);
		if (!widget->isVisible())
		{
			widget->show();
			widget->move(FDisplay.right() - widget->frameGeometry().width() - LEFT_MARGIN, FDisplay.bottom() - BOTTOM_MARGIN);
			QTimer::singleShot(0,widget,SLOT(adjustHeight()));
			QTimer::singleShot(10,widget,SLOT(adjustHeight()));
		}
		ypos -=  widget->frameGeometry().height();
		widget->animateTo(ypos);
	}
}
