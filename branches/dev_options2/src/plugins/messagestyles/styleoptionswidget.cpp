#include "styleoptionswidget.h"

#include <QTimer>
#include <QVBoxLayout>
#include <definitions/optionvalues.h>
#include <utils/message.h>
#include <utils/options.h>
#include <utils/logger.h>

StyleOptionsWidget::StyleOptionsWidget(IMessageStyles *AMessageStyles, QWidget *AParent) : QWidget(AParent)
{
	ui.setupUi(this);

	FUpdateStarted = false;

	FActiveView = NULL;
	FActiveStyle = NULL;
	FActiveSettings = NULL;
	FMessageStyles = AMessageStyles;

	ui.cmbMessageType->addItem(tr("Chat"),Message::Chat);
	ui.cmbMessageType->addItem(tr("Conference"),Message::GroupChat);
	ui.cmbMessageType->addItem(tr("Single"),Message::Normal);
	ui.cmbMessageType->addItem(tr("Headline"),Message::Headline);
	ui.cmbMessageType->addItem(tr("Error"),Message::Error);

	foreach(const QString &spluginId, FMessageStyles->pluginList())
		ui.cmbStyleEngine->addItem(FMessageStyles->pluginById(spluginId)->pluginName(),spluginId);

	ui.wdtStyleOptions->setLayout(new QVBoxLayout);
	ui.wdtStyleOptions->layout()->setMargin(0);

	ui.frmExample->setLayout(new QVBoxLayout);
	ui.frmExample->layout()->setMargin(0);

	connect(ui.cmbMessageType,SIGNAL(currentIndexChanged(int)),SLOT(onMessageTypeChanged(int)));
	connect(ui.cmbStyleEngine,SIGNAL(currentIndexChanged(int)),SLOT(onStyleEngineChanged(int)));

	reset();
}

StyleOptionsWidget::~StyleOptionsWidget()
{

}

void StyleOptionsWidget::apply()
{
	foreach(int messageType, FMessagePlugin.keys())
	{
		IOptionsDialogWidget *widget = FMessageWidget.value(messageType);
		IMessageStylePlugin *plugin = FMessageStyles->pluginById(FMessagePlugin.value(messageType));
		if (plugin && widget)
		{
			OptionsNode node = Options::node(OPV_MESSAGESTYLE_MTYPE_ITEM,QString::number(messageType)).node("context");
			node.setValue(plugin->pluginId(),"style-type");
			plugin->saveStyleSettings(widget,node.node("style",plugin->pluginId()));
		}
	}
	emit childApply();
}

void StyleOptionsWidget::reset()
{
	FActiveSettings = NULL;
	foreach(IOptionsDialogWidget *widget, FMessageWidget.values())
	{
		widget->instance()->setParent(NULL);
		delete widget->instance();
	}
	FMessageWidget.clear();
	FMessagePlugin.clear();
	onMessageTypeChanged(ui.cmbMessageType->currentIndex());
	emit childReset();
}

void StyleOptionsWidget::startStyleViewUpdate()
{
	if (!FUpdateStarted)
	{
		FUpdateStarted = true;
		QTimer::singleShot(0,this,SLOT(onUpdateStyleView()));
	}
}

void StyleOptionsWidget::createViewContent()
{
	if (FActiveStyle && FActiveView)
	{
		int curMessageType = ui.cmbMessageType->itemData(ui.cmbMessageType->currentIndex()).toInt();

		IMessageContentOptions i_options;
		IMessageContentOptions o_options;

		i_options.noScroll = true;
		i_options.kind = IMessageContentOptions::KindMessage;
		i_options.direction = IMessageContentOptions::DirectionIn;
		i_options.senderId = "remote";
		i_options.senderName = tr("Receiver");
		i_options.senderColor = "blue";
		i_options.senderIcon = FMessageStyles->contactIcon(i_options.senderId,IPresence::Chat,SUBSCRIPTION_BOTH,false);

		o_options.noScroll = true;
		o_options.kind = IMessageContentOptions::KindMessage;
		o_options.direction = IMessageContentOptions::DirectionOut;
		o_options.senderId = "myself";
		o_options.senderName = tr("Sender");
		o_options.senderColor = "red";
		o_options.senderIcon = FMessageStyles->contactIcon(i_options.senderId,IPresence::Online,SUBSCRIPTION_BOTH,false);

		if (curMessageType==Message::Normal || curMessageType==Message::Headline || curMessageType==Message::Error)
		{
			i_options.time = QDateTime::currentDateTime().addDays(-1);
			i_options.timeFormat = FMessageStyles->timeFormat(i_options.time);

			if (curMessageType == Message::Error)
			{
				i_options.senderName.clear();
				i_options.kind = IMessageContentOptions::KindMessage;
				QString html = "<b>"+tr("The message with a error code %1 is received").arg(999)+"</b>";
				html += "<p style='color:red;'>"+tr("Error description")+"</p>";
				html += "<hr>";
				FActiveStyle->appendContent(FActiveView,html,i_options);
			}

			i_options.kind = IMessageContentOptions::KindTopic;
			FActiveStyle->appendContent(FActiveView,tr("Subject: Message subject"),i_options);

			i_options.kind = IMessageContentOptions::KindMessage;
			FActiveStyle->appendContent(FActiveView,tr("Message body line 1")+"<br>"+tr("Message body line 2"),i_options);
		}
		else if (curMessageType==Message::Chat || curMessageType==Message::GroupChat)
		{
			if (curMessageType==Message::GroupChat)
			{
				i_options.senderColor.clear();
				o_options.senderColor.clear();
			}

			i_options.time = QDateTime::currentDateTime().addDays(-1);
			i_options.type = IMessageContentOptions::TypeHistory;
			if (Options::node(OPV_MESSAGES_SHOWDATESEPARATORS).value().toBool())
			{
				i_options.timeFormat = " ";
				i_options.kind = IMessageContentOptions::KindStatus;
				i_options.status = IMessageContentOptions::StatusDateSeparator;
				FActiveStyle->appendContent(FActiveView,FMessageStyles->dateSeparator(i_options.time.date()),i_options);
			}

			i_options.kind = IMessageContentOptions::KindMessage;
			i_options.status = IMessageContentOptions::StatusEmpty;
			if (Options::node(OPV_MESSAGES_SHOWDATESEPARATORS).value().toBool())
				i_options.timeFormat = FMessageStyles->timeFormat(i_options.time,i_options.time);
			else
				i_options.timeFormat = FMessageStyles->timeFormat(i_options.time);
			FActiveStyle->appendContent(FActiveView,tr("Incoming history message"),i_options);

			i_options.time = QDateTime::currentDateTime().addDays(-1).addSecs(80);
			FActiveStyle->appendContent(FActiveView,tr("Incoming history consecutive message"),i_options);

			i_options.kind = IMessageContentOptions::KindStatus;
			FActiveStyle->appendContent(FActiveView,tr("Incoming status message"),i_options);

			o_options.type = IMessageContentOptions::TypeHistory;
			o_options.time = QDateTime::currentDateTime().addDays(-1).addSecs(100);
			if (Options::node(OPV_MESSAGES_SHOWDATESEPARATORS).value().toBool())
				o_options.timeFormat = FMessageStyles->timeFormat(o_options.time,o_options.time);
			else
				o_options.timeFormat = FMessageStyles->timeFormat(o_options.time);
			FActiveStyle->appendContent(FActiveView,tr("Outgoing history message"),o_options);

			o_options.kind = IMessageContentOptions::KindStatus;
			FActiveStyle->appendContent(FActiveView,tr("Outgoing status message"),o_options);

			i_options.type = 0;
			i_options.time = QDateTime::currentDateTime();
			if (Options::node(OPV_MESSAGES_SHOWDATESEPARATORS).value().toBool())
			{
				i_options.timeFormat = " ";
				i_options.kind = IMessageContentOptions::KindStatus;
				i_options.status = IMessageContentOptions::StatusDateSeparator;
				FActiveStyle->appendContent(FActiveView,FMessageStyles->dateSeparator(i_options.time.date()),i_options);
			}

			i_options.status = IMessageContentOptions::StatusEmpty;
			if (Options::node(OPV_MESSAGES_SHOWDATESEPARATORS).value().toBool())
				i_options.timeFormat = FMessageStyles->timeFormat(i_options.time,i_options.time);
			else
				i_options.timeFormat = FMessageStyles->timeFormat(i_options.time);
			if (curMessageType==Message::GroupChat)
			{
				i_options.kind = IMessageContentOptions::KindTopic;
				FActiveStyle->appendContent(FActiveView,tr("Groupchat topic"),i_options);
			}

			i_options.kind = IMessageContentOptions::KindMessage;
			FActiveStyle->appendContent(FActiveView,tr("Incoming message"),i_options);

			i_options.type = IMessageContentOptions::TypeEvent;
			i_options.kind = IMessageContentOptions::KindStatus;
			FActiveStyle->appendContent(FActiveView,tr("Incoming event"),i_options);

			i_options.type = IMessageContentOptions::TypeNotification;
			FActiveStyle->appendContent(FActiveView,tr("Incoming notification"),i_options);

			if (curMessageType==Message::GroupChat)
			{
				i_options.kind = IMessageContentOptions::KindMessage;
				i_options.type = IMessageContentOptions::TypeMention;
				FActiveStyle->appendContent(FActiveView,tr("Incoming mention message"),i_options);
			}

			o_options.type = 0;
			o_options.time = QDateTime::currentDateTime();
			if (Options::node(OPV_MESSAGES_SHOWDATESEPARATORS).value().toBool())
				o_options.timeFormat = FMessageStyles->timeFormat(o_options.time,o_options.time);
			else
				o_options.timeFormat = FMessageStyles->timeFormat(o_options.time);
			o_options.kind = IMessageContentOptions::KindMessage;
			FActiveStyle->appendContent(FActiveView,tr("Outgoing message"),o_options);

			o_options.time = QDateTime::currentDateTime().addSecs(5);
			FActiveStyle->appendContent(FActiveView,tr("Outgoing consecutive message"),o_options);
		}
	}
}

QWidget *StyleOptionsWidget::updateActiveSettings()
{
	QWidget *oldWidget = NULL;

	if (FActiveSettings)
	{
		oldWidget = FActiveSettings->instance();
		oldWidget->setVisible(false);
		ui.wdtStyleOptions->layout()->removeWidget(oldWidget);
	}

	int curMessageType = ui.cmbMessageType->itemData(ui.cmbMessageType->currentIndex()).toInt();
	FActiveSettings = FMessageWidget.value(curMessageType,NULL);
	if (!FActiveSettings)
	{
		QString pluginId = FMessagePlugin.value(curMessageType);
		OptionsNode node = Options::node(OPV_MESSAGESTYLE_MTYPE_ITEM,QString::number(curMessageType)).node("context.style",pluginId);
		FActiveSettings = FMessageStyles->pluginById(pluginId)->styleSettingsWidget(node,curMessageType,ui.wdtStyleOptions);
		if (FActiveSettings)
		{
			connect(FActiveSettings->instance(),SIGNAL(modified()),SIGNAL(modified()));
			connect(FActiveSettings->instance(),SIGNAL(modified()),SLOT(startStyleViewUpdate()));
		}
	}

	if (FActiveSettings)
	{
		ui.wdtStyleOptions->layout()->addWidget(FActiveSettings->instance());
		FActiveSettings->instance()->setVisible(true);
	}
	FMessageWidget.insert(curMessageType, FActiveSettings);

	return oldWidget;
}

void StyleOptionsWidget::onUpdateStyleView()
{
	IMessageStyleOptions soptions;
	int curMessageType = ui.cmbMessageType->itemData(ui.cmbMessageType->currentIndex()).toInt();
	IMessageStylePlugin *plugin = FMessageStyles->pluginById(FMessagePlugin.value(curMessageType));
	plugin->saveStyleSettings(FActiveSettings,soptions);
	IMessageStyle *style = plugin->styleForOptions(soptions);
	if (style != FActiveStyle)
	{
		if (FActiveView)
		{
			ui.frmExample->layout()->removeWidget(FActiveView);
			FActiveView->deleteLater();
			FActiveView = NULL;
		}

		FActiveStyle = style;
		if (FActiveStyle)
		{
			FActiveView = FActiveStyle->createWidget(soptions,ui.frmExample);
			ui.frmExample->layout()->addWidget(FActiveView);
		}
	}
	else if (FActiveStyle)
	{
		FActiveStyle->changeOptions(FActiveView,soptions);
	}
	createViewContent();
	FUpdateStarted = false;
}

void StyleOptionsWidget::onMessageTypeChanged(int AIndex)
{
	int newMessageType = ui.cmbMessageType->itemData(AIndex).toInt();

	if (!FMessagePlugin.contains(newMessageType))
		FMessagePlugin.insert(newMessageType,FMessageStyles->styleOptions(newMessageType).pluginId);
	updateActiveSettings();
	startStyleViewUpdate();

	ui.cmbStyleEngine->setCurrentIndex(ui.cmbStyleEngine->findData(FMessagePlugin.value(newMessageType)));
}

void StyleOptionsWidget::onStyleEngineChanged(int AIndex)
{
	QString newPluginId = ui.cmbStyleEngine->itemData(AIndex).toString();
	int curMessageType = ui.cmbMessageType->itemData(ui.cmbMessageType->currentIndex()).toInt();
	if (FMessagePlugin.value(curMessageType) != newPluginId)
	{
		FMessagePlugin.insert(curMessageType,newPluginId);
		FMessageWidget.remove(curMessageType);
		delete updateActiveSettings();
		startStyleViewUpdate();
		emit modified();
	}
}
