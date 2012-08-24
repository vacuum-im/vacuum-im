#include "searchlineedit.h"

#include <QEvent>
#include <QHBoxLayout>

SearchLineEdit::SearchLineEdit(QWidget *AParent) : QLineEdit(AParent)
{
	FSelectText = true;
	FStartTimeout = 500;

	FSearchMenu = new Menu(this);
	FMenuButton = new QToolButton(this);
	FMenuButton->setVisible(false);
	FMenuButton->setAutoRaise(true);
	FMenuButton->setCursor(Qt::ArrowCursor);
	FMenuButton->setDefaultAction(FSearchMenu->menuAction());
	FMenuButton->setPopupMode(QToolButton::InstantPopup);

	FClearButton = new CloseButton(this);
	FClearButton->setVisible(false);
	connect(FClearButton,SIGNAL(clicked()),SLOT(onClearButtonClicked()));

	QHBoxLayout *hLayout = new QHBoxLayout(this);
	hLayout->addWidget(FMenuButton,0,Qt::AlignVCenter);
	hLayout->addStretch();
	hLayout->addWidget(FClearButton,0,Qt::AlignVCenter);
	hLayout->setContentsMargins(1,0,1,0);

	FStartTimer.setSingleShot(true);
	connect(&FStartTimer,SIGNAL(timeout()),SLOT(onStartTimerTimeout()));

	connect(this,SIGNAL(returnPressed()),SLOT(onLineEditReturnPressed()));
	connect(this,SIGNAL(textChanged(const QString &)),SLOT(onLineEditTextChanged(const QString &)));

	updateTextMargins();
}

SearchLineEdit::~SearchLineEdit()
{

}

Menu *SearchLineEdit::searchMenu() const
{
	return FSearchMenu;
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
	if (isSelectTextOnFocusEnabled() && AEvent->type()==QEvent::FocusIn)
		QTimer::singleShot(0,this,SLOT(selectAll()));
	return QLineEdit::event(AEvent);
}

void SearchLineEdit::showEvent(QShowEvent *AEvent)
{
	QLineEdit::showEvent(AEvent);
	updateTextMargins();
}

void SearchLineEdit::updateTextMargins()
{
	setTextMargins(isSearchMenuVisible() ? FMenuButton->size().width() : 0, 0, FClearButton->size().width(), 0);
}

void SearchLineEdit::onStartTimerTimeout()
{
	emit searchStart();
}

void SearchLineEdit::onClearButtonClicked()
{
	clear();
	restartTimeout(0);
}

void SearchLineEdit::onLineEditReturnPressed()
{
	if (isStartingSearch())
	{
		restartTimeout(0);
	}
	else if (!text().isEmpty())
	{
		emit searchNext();
	}
}

void SearchLineEdit::onLineEditTextChanged(const QString &AText)
{
	FClearButton->setVisible(!AText.isEmpty());
	restartTimeout(startSearchTimeout());
	updateTextMargins();
}
