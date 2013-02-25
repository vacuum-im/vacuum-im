#include "infowidget.h"

#include <QIcon>
#include <QMovie>
#include <QToolTip>
#include <QHelpEvent>
#include <QHBoxLayout>
#include <QContextMenuEvent>

InfoWidget::InfoWidget(IMessageWidgets *AMessageWidgets, IMessageWindow *AWindow, QWidget *AParent) : QWidget(AParent)
{
	ui.setupUi(this);

	FWindow = AWindow;
	FMessageWidgets = AMessageWidgets;

	FSelectorEnabled = false;

	FAddressMenu = new Menu(this);

	QToolBar *toolBar = new QToolBar;
	toolBar->setMovable(false);
	toolBar->setFloatable(false);
	toolBar->setIconSize(QSize(16,16));
	toolBar->layout()->setMargin(0);
	toolBar->setStyleSheet("QToolBar { border: none; }");
	toolBar->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Preferred);
	FInfoToolBar = new ToolBarChanger(toolBar);

	ui.wdtInfoToolBar->setLayout(new QHBoxLayout);
	ui.wdtInfoToolBar->layout()->setMargin(0);
	ui.wdtInfoToolBar->layout()->addWidget(toolBar);

	initialize();
	connect(FWindow->address()->instance(),SIGNAL(addressChanged(const Jid &, const Jid &)),SLOT(onAddressChanged(const Jid &, const Jid &)));
}

InfoWidget::~InfoWidget()
{

}

IMessageWindow *InfoWidget::messageWindow() const
{
	return FWindow;
}

ToolBarChanger *InfoWidget::toolBarChanger() const
{
	return FInfoToolBar;
}

bool InfoWidget::isAddressSelectorEnabled() const
{
	return FSelectorEnabled;
}

void InfoWidget::setAddressSelectorEnabled(bool AEnabled)
{
	if (FSelectorEnabled != AEnabled)
	{
		FSelectorEnabled = AEnabled;
		emit addressSelectorEnableChanged(AEnabled);
	}
}

QVariant InfoWidget::fieldValue(int AField) const
{
	return FFieldValues.value(AField);
}

void InfoWidget::setFieldValue(int AField, const QVariant &AValue)
{
	if (FFieldValues.value(AField) != AValue)
	{
		if (AValue.isNull())
			FFieldValues.remove(AField);
		else
			FFieldValues.insert(AField,AValue);
		updateFieldView(AField);
		emit fieldValueChanged(AField);
	}
}

void InfoWidget::initialize()
{
	FAvatars = NULL;

	IPlugin *plugin = FMessageWidgets->pluginManager()->pluginInterface("IAvatars").value(0);
	if (plugin)
	{
		FAvatars = qobject_cast<IAvatars *>(plugin->instance());
	}
}

void InfoWidget::updateFieldView(int AField)
{
	switch (AField)
	{
	case IMessageInfoWidget::Avatar:
		{
			bool avatarVisible = false;
			if (ui.lblAvatar->movie())
			{
				ui.lblAvatar->movie()->deleteLater();
				ui.lblAvatar->setMovie(NULL);
			}

			QVariant avatar = fieldValue(IMessageInfoWidget::Avatar);
			switch (avatar.type())
			{
			case QVariant::String:
				if (FAvatars)
				{
					QString avatarFile = FAvatars->avatarFileName(avatar.toString());
					if (!avatarFile.isEmpty())
					{
						avatarVisible = true;
						QMovie *movie = new QMovie(avatarFile,QByteArray(),ui.lblAvatar);
						QSize size = QImageReader(avatarFile).size();
						size.scale(ui.lblAvatar->maximumSize(),Qt::KeepAspectRatio);
						movie->setScaledSize(size);
						ui.lblAvatar->setMovie(movie);
						movie->start();
					}
				}
				break;
			case QVariant::Pixmap:
				avatarVisible = true;
				ui.lblAvatar->setPixmap(avatar.value<QPixmap>());
				break;
			case QVariant::Image:
				avatarVisible = true;
				ui.lblAvatar->setPixmap(QPixmap::fromImage(avatar.value<QImage>()));
				break;
			default:
				ui.lblAvatar->clear();
			}

			ui.lblAvatar->setVisible(avatarVisible);
			break;
		}
	case IMessageInfoWidget::Name:
	case IMessageInfoWidget::StatusText:
		{
			QString info;
			QString name = fieldValue(IMessageInfoWidget::Name).toString();
			QString status = fieldValue(IMessageInfoWidget::StatusText).toString();
			if (!name.isEmpty() && !status.isEmpty())
				info = QString("<big><b>%1</b></big> - %2").arg(Qt::escape(name),Qt::escape(status));
			else if (!name.isEmpty())
				info = QString("<big><b>%1</b></big>").arg(Qt::escape(name));
			else if (!status.isEmpty())
				info = Qt::escape(status);
			ui.lblInfo->setText(info);
			break;
		}
	case IMessageInfoWidget::StatusIcon:
		{
			bool iconVisible = false;
			QVariant icon = fieldValue(IMessageInfoWidget::StatusIcon);

			switch (icon.type())
			{
			case QVariant::Icon:
				{
					iconVisible = true;
					QIcon iconIcon = icon.value<QIcon>();
					ui.lblIcon->setPixmap(iconIcon.pixmap(iconIcon.actualSize(ui.lblIcon->maximumSize())));
				}
				break;
			case QVariant::Pixmap:
				iconVisible = true;
				ui.lblIcon->setPixmap(icon.value<QPixmap>());
			case QVariant::Image:
				iconVisible = true;
				ui.lblIcon->setPixmap(QPixmap::fromImage(icon.value<QImage>()));
			default:
				ui.lblIcon->clear();
			}

			ui.lblIcon->setVisible(iconVisible);
			break;
		}
	}
}

bool InfoWidget::event(QEvent *AEvent)
{
	if (AEvent->type() == QEvent::ToolTip)
	{
		QHelpEvent *helpEvent = static_cast<QHelpEvent *>(AEvent);
		if (helpEvent)
		{
			QMap<int,QString> toolTipsMap;
			emit toolTipsRequested(toolTipsMap);
			if (!toolTipsMap.isEmpty())
			{
				QString tooltip = QString("<span>%1</span>").arg(QStringList(toolTipsMap.values()).join("<p/>"));
				QToolTip::showText(helpEvent->globalPos(),tooltip,this);
			}
		}
		return true;
	}
	return QWidget::event(AEvent);
}

void InfoWidget::contextMenuEvent(QContextMenuEvent *AEvent)
{
	Menu *menu = new Menu(this);
	menu->setAttribute(Qt::WA_DeleteOnClose,true);

	emit contextMenuRequested(menu);

	if (!menu->isEmpty())
		menu->popup(AEvent->globalPos());
	else
		delete menu;
}

void InfoWidget::onAddressMenuAboutToShow()
{

}

void InfoWidget::onAddressChanged( const Jid &AStreamBefore, const Jid &AContactBefore )
{

}
