#include "searchlineedit.h"

#include <QEvent>
#include <QStyle>
#include <QKeyEvent>
#include <QHBoxLayout>

SearchLineEdit::SearchLineEdit(QWidget *AParent) : QLineEdit(AParent)
{
	FSelectText = true;
	FTextChanged = false;
	FStartTimeout = 500;

	FSearchMenu = new Menu(this);
	FSearchMenu->menuAction()->setToolTip(tr("Search options"));

	FMenuButton = new QToolButton(this);
	FMenuButton->setVisible(false);
	FMenuButton->setAutoRaise(true);
	FMenuButton->setCursor(Qt::ArrowCursor);
	FMenuButton->setDefaultAction(FSearchMenu->menuAction());
	FMenuButton->setPopupMode(QToolButton::InstantPopup);

	FClearButton = new CloseButton(this);
	FClearButton->setVisible(false);
	FClearButton->setToolTip(tr("Clear text"));
	FClearButton->setIcon(QIcon::fromTheme(layoutDirection()==Qt::LeftToRight ? "edit-clear-locationbar-rtl" :"edit-clear-locationbar-ltr", QIcon::fromTheme("edit-clear")));
	connect(FClearButton,SIGNAL(clicked()),SLOT(onClearButtonClicked()));

	QHBoxLayout *hLayout = new QHBoxLayout(this);
	if (layoutDirection() == Qt::LeftToRight)
	{
		hLayout->addWidget(FMenuButton,0,Qt::AlignVCenter);
		hLayout->addStretch();
		hLayout->addWidget(FClearButton,0,Qt::AlignVCenter);
	}
	else
	{
		hLayout->addWidget(FClearButton,0,Qt::AlignVCenter);
		hLayout->addStretch();
		hLayout->addWidget(FMenuButton,0,Qt::AlignVCenter);
	}

	FStartTimer.setSingleShot(true);
	connect(&FStartTimer,SIGNAL(timeout()),SLOT(onStartTimerTimeout()));

	connect(this,SIGNAL(returnPressed()),SLOT(onLineEditReturnPressed()));
	connect(this,SIGNAL(textChanged(const QString &)),SLOT(onLineEditTextChanged(const QString &)));

	updateLayoutMargins();
	updateTextMargins();
}

SearchLineEdit::~SearchLineEdit()
{

}

Menu *SearchLineEdit::searchMenu() const
{
	return FSearchMenu;
}

CloseButton *SearchLineEdit::clearButton() const
{
	return FClearButton;
}

bool SearchLineEdit::isStartingSearch() const
{
	return FStartTimer.isActive();
}

void SearchLineEdit::restartTimeout(int AMSecs)
{
	if (AMSecs >= 0)
		FStartTimer.start(AMSecs);
	else
		FStartTimer.stop();
}

int SearchLineEdit::startSearchTimeout() const
{
	return FStartTimeout;
}

void SearchLineEdit::setStartSearchTimeout(int AMSecs)
{
	FStartTimeout = AMSecs;
}

bool SearchLineEdit::isSearchMenuVisible() const
{
	return FMenuButton->isVisible();
}

void SearchLineEdit::setSearchMenuVisible(bool AVisible)
{
	FMenuButton->setVisible(AVisible);
	updateTextMargins();
}

bool SearchLineEdit::isSelectTextOnFocusEnabled() const
{
	return FSelectText;
}

void SearchLineEdit::setSelectTextOnFocusEnabled(bool AEnabled)
{
	FSelectText = AEnabled;
}

bool SearchLineEdit::event(QEvent *AEvent)
{
	if (AEvent->type() == QEvent::ShortcutOverride)
	{
		QKeyEvent *keyEvent = static_cast<QKeyEvent *>(AEvent);
		if (!text().isEmpty() && keyEvent->key()==Qt::Key_Escape && keyEvent->modifiers()==0)
		{
			FClearButton->animateClick();
			AEvent->accept();
		}
	}
	else if (AEvent->type() == QEvent::FocusIn)
	{
		if (isSelectTextOnFocusEnabled())
			QTimer::singleShot(0,this,SLOT(selectAll()));
	}
	else if (AEvent->type() == QEvent::FocusOut)
	{
		if (FTextChanged)
			QTimer::singleShot(0,this,SLOT(onLineEditReturnPressed()));
	}
	return QLineEdit::event(AEvent);
}

void SearchLineEdit::showEvent(QShowEvent *AEvent)
{
	QLineEdit::showEvent(AEvent);
	updateTextMargins();
}

void SearchLineEdit::updateTextMargins()
{
	int left, top, right, bottom;
	getTextMargins(NULL,&top,NULL,&bottom);
	layout()->getContentsMargins(&left,NULL,&right,NULL);
	if (layoutDirection() == Qt::LeftToRight)
	{
		left += isSearchMenuVisible() ? FMenuButton->size().width()+1: 0;
		right += FClearButton->size().width()+1;
	}
	else
	{
		left += FClearButton->size().width()+1;
		right += isSearchMenuVisible() ? FMenuButton->size().width()+1 : 0;
	}
	setTextMargins(left,top,right,bottom);
}

void SearchLineEdit::updateLayoutMargins()
{
	ensurePolished();
	int correct=0;
	int frameWidth = style()->pixelMetric(QStyle::PM_DefaultFrameWidth);
	if (style()->objectName() == "oxygen")
		correct += 4;
	layout()->setContentsMargins(frameWidth,frameWidth,frameWidth+correct,frameWidth);
}

void SearchLineEdit::onStartTimerTimeout()
{
	FTextChanged = false;
	emit searchStart();
}

void SearchLineEdit::onClearButtonClicked()
{
	clear();
	restartTimeout(0);
}

void SearchLineEdit::onLineEditReturnPressed()
{
	if (FTextChanged)
		restartTimeout(0);
	else if (!text().isEmpty())
		emit searchNext();
}

void SearchLineEdit::onLineEditTextChanged(const QString &AText)
{
	FTextChanged = true;
	FClearButton->setVisible(!AText.isEmpty());
	restartTimeout(startSearchTimeout());
	updateTextMargins();
}
