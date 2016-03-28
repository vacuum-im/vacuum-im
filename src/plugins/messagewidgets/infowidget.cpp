#include "infowidget.h"

#include <QIcon>
#include <QMovie>
#include <QTimer>
#include <QCursor>
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
	
	FCaptionClickable = false;
	FAddressMenuVisible = false;
	FInfoLabelUnderMouse = false;

	FAvatars = PluginHelper::pluginInstance<IAvatars>();

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

	ui.lblInfo->installEventFilter(this);
	ui.lblInfo->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(ui.lblInfo,SIGNAL(linkActivated(const QString &)),SLOT(onInfoLabelLinkActivated(const QString &)));
	connect(ui.lblInfo,SIGNAL(customContextMenuRequested(const QPoint &)),SLOT(onInfoLabelCustomContextMenuRequested(const QPoint &)));

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
			QToolButton *button = FInfoToolBar->insertAction(FAddressMenu->menuAction(),TBG_MWIWTB_ADDRESSMENU);
			button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
			button->setPopupMode(QToolButton::InstantPopup);
		}
		else
		{
			FInfoToolBar->removeItem(FInfoToolBar->actionHandle(FAddressMenu->menuAction()));
		}
		emit addressMenuVisibleChanged(AVisible);
	}
}

bool InfoWidget::isCaptionClickable() const
{
	return FCaptionClickable;
}

void InfoWidget::setCaptionClickable(bool AEnabled)
{
	if (FCaptionClickable != AEnabled)
	{
		FCaptionClickable = AEnabled;
		updateFieldView(Caption);
		captionClickableChanged(AEnabled);
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
	case IMessageInfoWidget::Caption:
	case IMessageInfoWidget::StatusText:
		{
			static const QString htmlTemplate = "<html><style>a { color: %1; text-decoration: %2; }</style><body>%3</body></html>";
			static const QString captionClickTemplate = "<big><b><a href='info-caption'>%1</a></b></big>";
			static const QString captionPlainTemplate = "<big><b>%1</b></big>";

			QString caption = fieldValue(IMessageInfoWidget::Caption).toString();
			QString status = fieldValue(IMessageInfoWidget::StatusText).toString();
			QString captionTemplate = FCaptionClickable ? captionClickTemplate : captionPlainTemplate;

			QString info;
			if (!caption.isEmpty() && !status.isEmpty())
				info = QString("%1 - %2").arg(captionTemplate.arg(caption.toHtmlEscaped())).arg(status.toHtmlEscaped());
			else if (!caption.isEmpty())
				info = captionTemplate.arg(caption.toHtmlEscaped());
			else if (!status.isEmpty())
				info = status.toHtmlEscaped();

			QPalette::ColorGroup colorGroup = ui.lblInfo->isEnabled() ? (isActiveWindow() ? QPalette::Active : QPalette::Inactive) : QPalette::Disabled;
			QString colorName = ui.lblInfo->palette().color(colorGroup, QPalette::WindowText).name();

			QString textDecoration = FCaptionClickable && FInfoLabelUnderMouse ? "underline" : "none";

			ui.lblInfo->setText(htmlTemplate.arg(colorName,textDecoration,info));
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

void InfoWidget::showContextMenu(const QPoint &AGlobalPos)
{
	Menu *menu = new Menu(this);
	menu->setAttribute(Qt::WA_DeleteOnClose,true);
	emit contextMenuRequested(menu);

	if (!menu->isEmpty())
		menu->popup(AGlobalPos);
	else
		delete menu;
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

bool InfoWidget::eventFilter(QObject *AObject, QEvent *AEvent)
{
	if (FCaptionClickable && AObject==ui.lblInfo)
	{
		if (AEvent->type() == QEvent::Enter)
		{
			FInfoLabelUnderMouse = true;
			updateFieldView(Caption);
		}
		else if (AEvent->type() == QEvent::Leave)
		{
			FInfoLabelUnderMouse = false;
			updateFieldView(Caption);
		}
	}
	return QWidget::eventFilter(AObject,AEvent);
}

void InfoWidget::contextMenuEvent(QContextMenuEvent *AEvent)
{
	showContextMenu(AEvent->globalPos());
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

void InfoWidget::onInfoLabelLinkActivated(const QString &ALink)
{
	if (ALink == "info-caption")
		emit captionFieldClicked();
}

void InfoWidget::onInfoLabelCustomContextMenuRequested(const QPoint &APos)
{
	showContextMenu(ui.lblInfo->mapToGlobal(APos));
}
