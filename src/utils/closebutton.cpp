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
	if (icon().isNull())
	{
		ensurePolished();
		int width = style()->pixelMetric(QStyle::PM_TabCloseIndicatorWidth, 0, this);
		int height = style()->pixelMetric(QStyle::PM_TabCloseIndicatorHeight, 0, this);
		return QSize(width, height);
	}
	return icon().actualSize(iconSize());
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
	if (icon().isNull())
	{
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
	else
	{
		QIcon::Mode mode = QIcon::Normal;
		if (!isEnabled())
			mode = QIcon::Disabled;
		else if (underMouse())
			mode = QIcon::Active;

		QIcon::State state = QIcon::Off;
		if (isChecked())
			state = QIcon::On;

		QPixmap pixmap = icon().pixmap(iconSize(),mode,state);
		QRect pixmapRect = QRect(0, 0, pixmap.width(), pixmap.height());
		pixmapRect.moveCenter(rect().center());

		QPainter p(this);
		p.drawPixmap(pixmapRect, pixmap);
	}
}
