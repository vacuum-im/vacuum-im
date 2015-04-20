#include "infowidget.h"

#include <QIcon>
#include <QMovie>
#include <QTimer>
#include <QToolTip>
#include <QHelpEvent>
#include <QHBoxLayout>
#include <QContextMenuEvent>
#include <definitions/toolbargroups.h>
#include <utils/textmanager.h>
#include <utils/pluginhelper.h>

#define ADR_STREAM_JID           Action::DR_StreamJid
#define ADR_CONTACT_JID          Action::DR_Parametr1

InfoWidget::InfoWidget(IMessageWidgets *AMessageWidgets, IMessageWindow *AWindow, QWidget *AParent) : QWidget(AParent)
{
	ui.setupUi(this);

	FWindow = AWindow;
	FMessageWidgets = AMessageWidgets;

	FAvatars = PluginHelper::pluginInstance<IAvatars>();

	FAddressMenuVisible = false;
	ui.lblAvatar->setVisible(false);
	ui.lblIcon->setVisible(false);
	ui.wdtInfoToolBar->setVisible(false);

	QToolBar *toolBar = new QToolBar;
	toolBar->setMovable(false);
	toolBar->setFloatable(false);
	toolBar->setIconSize(QSize(16,16));
	toolBar->layout()->setMargin(0);
	toolBar->setStyleSheet("QToolBar { border: none; }");
	toolBar->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Preferred);

	FInfoToolBar = new ToolBarChanger(toolBar);
	FInfoToolBar->setMinimizeWidth(true);
	FInfoToolBar->setSeparatorsVisible(false);
	connect(FInfoToolBar,SIGNAL(itemRemoved(QAction *)),SLOT(onUpdateInfoToolBarVisibility()));
	connect(FInfoToolBar,SIGNAL(itemInserted(QAction *, QAction *, Action *, QWidget *, int)),SLOT(onUpdateInfoToolBarVisibility()));

	ui.wdtInfoToolBar->setLayout(new QHBoxLayout);
	ui.wdtInfoToolBar->layout()->setMargin(0);
	ui.wdtInfoToolBar->layout()->addWidget(toolBar);

	FAddressMenu = new Menu(this);
	connect(FAddressMenu,SIGNAL(aboutToShow()),SLOT(onAddressMenuAboutToShow()));

	onUpdateInfoToolBarVisibility();
}

InfoWidget::~InfoWidget()
{

}

bool InfoWidget::isVisibleOnWindow() const
{
	return isVisibleTo(FWindow->instance());
}

IMessageWindow *InfoWidget::messageWindow() const
{
	return FWindow;
}

Menu *InfoWidget::addressMenu() const
{
	return FAddressMenu;
}

bool InfoWidget::isAddressMenuVisible() const
{
	return FAddressMenuVisible;
}

void InfoWidget::setAddressMenuVisible(bool AVisible)
{
	if (FAddressMenuVisible != AVisible)
	{
		FAddressMenuVisible = AVisible;
		if (AVisible)
		{
			if (messageWindow()->editWidget())
			{
				QAction *sendHandle = messageWindow()->editWidget()->editToolBarChanger()->groupItems(TBG_MWEWTB_SENDMESSAGE).value(0);
				QToolButton *button = qobject_cast<QToolButton *>(messageWindow()->editWidget()->editToolBarChanger()->handleWidget(sendHandle));
				if (button)
				{
					button->setMenu(FAddressMenu);
					button->setPopupMode(QToolButton::MenuButtonPopup);
					button->setIconSize(messageWindow()->editWidget()->editToolBarChanger()->toolBar()->iconSize());
				}
			}

			QToolButton *button = FInfoToolBar->insertAction(FAddressMenu->menuAction(),TBG_MWIWTB_ADDRESSMENU);
			button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
			button->setPopupMode(QToolButton::InstantPopup);
		}
		else
		{
			if (messageWindow()->editWidget())
			{
				QAction *sendHandle = messageWindow()->editWidget()->editToolBarChanger()->groupItems(TBG_MWEWTB_SENDMESSAGE).value(0);
				QToolButton *button = qobject_cast<QToolButton *>(messageWindow()->editWidget()->editToolBarChanger()->handleWidget(sendHandle));
				if (button)
					button->setMenu(NULL);
			}

			FInfoToolBar->removeItem(FInfoToolBar->actionHandle(FAddressMenu->menuAction()));
		}
		emit addressMenuVisibleChanged(AVisible);
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

ToolBarChanger *InfoWidget::infoToolBarChanger() const
{
	return FInfoToolBar;
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
					break;
				}
			case QVariant::Pixmap:
				iconVisible = true;
				ui.lblIcon->setPixmap(icon.value<QPixmap>());
				break;
			case QVariant::Image:
				iconVisible = true;
				ui.lblIcon->setPixmap(QPixmap::fromImage(icon.value<QImage>()));
				break;
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
				QString tooltip = QString("<span>%1</span>").arg(QStringList(toolTipsMap.values()).join("<p/><nbsp>"));
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
	FAddressMenu->clear();
	emit addressMenuRequested(FAddressMenu);
}

void InfoWidget::onUpdateInfoToolBarVisibility()
{
	ui.wdtInfoToolBar->setVisible(!FInfoToolBar->isEmpty());
}
