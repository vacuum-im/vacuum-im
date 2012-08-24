#include "closebutton.h"

#include <QStyle>
#include <QPainter>
#include <QStyleOption>

CloseButton::CloseButton(QWidget *AParent) : QAbstractButton(AParent)
{
	setFocusPolicy(Qt::NoFocus);
	setCursor(Qt::ArrowCursor);
	resize(sizeHint());
}

CloseButton::~CloseButton()
{

}

QSize CloseButton::sizeHint() const
{
	ensurePolished();
	int width = style()->pixelMetric(QStyle::PM_TabCloseIndicatorWidth, 0, this);
	int height = style()->pixelMetric(QStyle::PM_TabCloseIndicatorHeight, 0, this);
	return QSize(width, height);
}

void CloseButton::enterEvent(QEvent *AEvent)
{
	if (isEnabled())
		update();
	QAbstractButton::enterEvent(AEvent);
}

void CloseButton::leaveEvent(QEvent *AEvent)
{
	if (isEnabled())
		update();
	QAbstractButton::leaveEvent(AEvent);
}

void CloseButton::paintEvent(QPaintEvent *AEvent)
{
	Q_UNUSED(AEvent);

	QStyleOption opt;
	opt.init(this);
	opt.state |= QStyle::State_AutoRaise;
	if (isEnabled() && underMouse() && !isChecked() && !isDown())
		opt.state |= QStyle::State_Raised;
	if (isChecked())
		opt.state |= QStyle::State_On;
	if (isDown())
		opt.state |= QStyle::State_Sunken;

	QPainter p(this);
	style()->drawPrimitive(QStyle::PE_IndicatorTabClose, &opt, &p, this);
}
