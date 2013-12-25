#include "selectpagewidget.h"

#include <QStyle>
#include <QLocale>
#include <QMouseEvent>
#include <definitions/resources.h>
#include <definitions/menuicons.h>
#include <utils/iconstorage.h>
#include <utils/menu.h>

#define ADR_MONTH      Action::DR_Parametr1

SelectPageWidget::SelectPageWidget(QWidget *AParent) : QWidget(AParent)
{
	ui.setupUi(this);
	FYear = -1;
	FMonth = -1;

	ui.spbYear->hide();
	connect(ui.spbYear,SIGNAL(editingFinished()),SLOT(onChangeYearBySpinbox()));
	connect(ui.tlbYear,SIGNAL(clicked()),SLOT(onStartEditYear()));

	Menu *monthMenu = new Menu(ui.tlbMonth);
	for (int month=1; month<=12; month++)
	{
		Action *action = new Action(monthMenu);
		action->setData(ADR_MONTH, month);
		action->setText(FLocale.standaloneMonthName(month));
		connect(action,SIGNAL(triggered()),SLOT(onChangeMonthByAction()));
		monthMenu->addAction(action);
	}
	ui.tlbMonth->setMenu(monthMenu);
	ui.tlbMonth->setPopupMode(QToolButton::InstantPopup);

	ui.tlbNext->setIcon(style()->standardIcon(isRightToLeft() ? QStyle::SP_ArrowLeft : QStyle::SP_ArrowRight, NULL, this));
	ui.tlbPrevious->setIcon(style()->standardIcon(isRightToLeft() ? QStyle::SP_ArrowRight : QStyle::SP_ArrowLeft, NULL, this));

	connect(ui.tlbNext,SIGNAL(clicked()),SLOT(showNextMonth()));
	connect(ui.tlbPrevious,SIGNAL(clicked()),SLOT(showPreviousMonth()));

	setCurrentPage(QDate::currentDate().year(),QDate::currentDate().month());
}

SelectPageWidget::~SelectPageWidget()
{

}

int SelectPageWidget::monthShown() const
{
	return FMonth;
}

int SelectPageWidget::yearShown() const
{
	return FYear;
}

void SelectPageWidget::showNextMonth()
{
	if (FMonth < 12)
		setCurrentPage(yearShown(),FMonth+1);
	else
		setCurrentPage(yearShown()+1,1);
}

void SelectPageWidget::showPreviousMonth()
{
	if (FMonth > 1)
		setCurrentPage(yearShown(),FMonth-1);
	else
		setCurrentPage(yearShown()-1,12);
}

void SelectPageWidget::setCurrentPage(int AYear, int AMonth)
{
	if (AYear!=FYear || AMonth!=FMonth)
	{
		if (AYear>=0 && AMonth>0 && AMonth<=12)
		{
			FYear = AYear;
			FMonth = AMonth;
			ui.spbYear->setValue(FYear);
			ui.tlbYear->setText(QString::number(FYear));
			ui.tlbMonth->setText(FLocale.standaloneMonthName(FMonth));
			emit currentPageChanged(FYear,FMonth);
		}
	}
}

bool SelectPageWidget::eventFilter(QObject *AWatched, QEvent *AEvent)
{
	if (AEvent->type()==QEvent::MouseButtonPress && ui.spbYear->hasFocus()) 
	{
		QWidget *thisWindow = window();
		QWidget *widget = static_cast<QWidget*>(AWatched);
		if (widget && widget->window()==thisWindow) 
		{
			QPoint mousePos = widget->mapTo(thisWindow, static_cast<QMouseEvent *>(AEvent)->pos());
			QRect geom = QRect(ui.spbYear->mapTo(thisWindow, QPoint(0, 0)), ui.spbYear->size());
			if (!geom.contains(mousePos)) 
			{
				AEvent->accept();
				onChangeYearBySpinbox();
				setFocus();
				return true;
			}
		}
	}
	return QWidget::eventFilter(AWatched, AEvent);
}

void SelectPageWidget::onStartEditYear()
{
	ui.tlbYear->hide();

	FOldFocusPolicy = focusPolicy();
	setFocusPolicy(Qt::NoFocus);

	qApp->installEventFilter(this);

	ui.spbYear->show();
	ui.spbYear->raise();
	ui.spbYear->selectAll();
	ui.spbYear->setFocus(Qt::MouseFocusReason);
}

void SelectPageWidget::onChangeYearBySpinbox()
{
	ui.spbYear->hide();
	ui.tlbYear->show();
	setFocusPolicy(FOldFocusPolicy);
	qApp->removeEventFilter(this);
	setCurrentPage(ui.spbYear->value(),monthShown());
}

void SelectPageWidget::onChangeMonthByAction()
{
	Action *action = qobject_cast<Action *>(sender());
	if (action)
		setCurrentPage(yearShown(),action->data(ADR_MONTH).toInt());
}
