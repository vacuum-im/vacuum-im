#include "selecticonwidget.h"

#include <QCursor>
#include <QToolTip>

SelectIconWidget::SelectIconWidget(IconStorage *AStorage, QWidget *AParent) : QWidget(AParent)
{
	FPressed = NULL;
	FStorage = AStorage;

	FLayout = new QGridLayout(this);
	FLayout->setMargin(2);
	FLayout->setHorizontalSpacing(3);
	FLayout->setVerticalSpacing(3);

	createLabels();
}

SelectIconWidget::~SelectIconWidget()
{

}

void SelectIconWidget::createLabels()
{
	QList<QString> keys = FStorage->fileFirstKeys();

	int row = 0;
	int column = 0;
	foreach(const QString &key, keys)
	{
		QLabel *label = new QLabel(this);
		label->setMargin(2);
		label->setAlignment(Qt::AlignCenter);
		label->setFrameShape(QFrame::Box);
		label->setFrameShadow(QFrame::Sunken);
		label->setToolTip(QString("<span>%1</span>").arg(key));
		label->installEventFilter(this);
		FStorage->insertAutoIcon(label,key,0,0,"pixmap");
		FKeyByLabel.insert(label,key);
		FLayout->addWidget(label,row,column);
		if (column == 8)
			row++;
		column < 8 ? column++ : column = 0;
	}
}

bool SelectIconWidget::eventFilter(QObject *AWatched, QEvent *AEvent)
{
	QLabel *label = qobject_cast<QLabel *>(AWatched);
	if (AEvent->type() == QEvent::Enter)
	{
		label->setFrameShadow(QFrame::Plain);
		QToolTip::showText(QCursor::pos(),label->toolTip());
	}
	else if (AEvent->type() == QEvent::Leave)
	{
		label->setFrameShadow(QFrame::Sunken);
	}
	else if (AEvent->type() == QEvent::MouseButtonPress)
	{
		FPressed = label;
	}
	else if (AEvent->type() == QEvent::MouseButtonRelease)
	{
		if (FPressed == label)
			emit iconSelected(FStorage->subStorage(),FKeyByLabel.value(label));
		FPressed = NULL;
	}
	return QWidget::eventFilter(AWatched,AEvent);
}
