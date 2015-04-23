#include "styleeditoptionsdialog.h"

#include <QTimer>
#include <QVBoxLayout>
#include <definitions/optionvalues.h>
#include <utils/widgetmanager.h>
#include <utils/message.h>
#include <utils/options.h>
#include <utils/logger.h>

StyleEditOptionsDialog::StyleEditOptionsDialog(IMessageStyleManager *AMessageStyles, const OptionsNode &AStyleNode, QWidget *AParent) : QDialog(AParent)
{
	ui.setupUi(this);
	setWindowModality(Qt::WindowModal);
	setAttribute(Qt::WA_DeleteOnClose,true);
	ui.lblPreview->setText(QString("<h2>%1</h2>").arg(tr("Preview")));

	FUpdateStarted = false;
	FMessageStyleManager = AMessageStyles;

	QList<QString> params = AStyleNode.parentNSpaces();
	FMessageType = params.value(1).toInt();
	FContext = params.value(2);
	FEngineId = params.value(3);
	FStyleId = params.value(4);

	FStyleEngine = FMessageStyleManager->findStyleEngine(FEngineId);
	FStyleSettings = FStyleEngine!=NULL ? FStyleEngine->styleSettingsWidget(AStyleNode,this) : NULL;
	
	IMessageStyleOptions soptions = FStyleSettings!=NULL ? FStyleEngine->styleSettinsOptions(FStyleSettings) : IMessageStyleOptions();
	FStyle = FStyleSettings!=NULL ? FStyleEngine->styleForOptions(soptions) : NULL;
	FStyleView = FStyle!=NULL ? FStyle->createWidget(soptions,this) : NULL;

	if (FStyleEngine && FStyleSettings && FStyle && FStyleView)
	{
		setWindowTitle(tr("Message Style - %1 - %2").arg(FStyleEngine->engineName(),soptions.styleId));

		ui.wdtStyleSettings->setLayout(new QVBoxLayout);
		ui.wdtStyleSettings->layout()->setMargin(0);
		ui.wdtStyleSettings->layout()->addWidget(FStyleSettings->instance());
		connect(FStyleSettings->instance(),SIGNAL(modified()),SLOT(startStyleViewUpdate()));

		ui.frmStyleView->setLayout(new QVBoxLayout);
		ui.frmStyleView->layout()->setMargin(0);
		ui.frmStyleView->layout()->addWidget(FStyleView);
	}

	connect(ui.dbbButtons,SIGNAL(accepted()),SLOT(accept()));
	connect(ui.dbbButtons,SIGNAL(rejected()),SLOT(reject()));

	if (!restoreGeometry(Options::fileValue("message-styles.style-edit-options-dialog.geometry").toByteArray()))
		setGeometry(WidgetManager::alignGeometry(QSize(550,500),this));

	startStyleViewUpdate();
}

StyleEditOptionsDialog::~StyleEditOptionsDialog()
{
	Options::setFileValue(saveGeometry(),"message-styles.style-edit-options-dialog.geometry");
}

void StyleEditOptionsDialog::accept()
{
	if (FStyleSettings)
		FStyleSettings->apply();
	QDialog::accept();
}

void StyleEditOptionsDialog::startStyleViewUpdate()
{
	if (!FUpdateStarted)
	{
		FUpdateStarted = true;
		QTimer::singleShot(0,this,SLOT(onUpdateStyleView()));
	}
}

void StyleEditOptionsDialog::createViewContent()
{
	IMessageStyleContentOptions i_options;
	IMessageStyleContentOptions o_options;

	i_options.noScroll = true;
	i_options.kind = IMessageStyleContentOptions::KindMessage;
	i_options.direction = IMessageStyleContentOptions::DirectionIn;
	i_options.senderId = "remote";
	i_options.senderName = tr("Receiver");
	i_options.senderColor = "blue";
	i_options.senderIcon = FMessageStyleManager->contactIcon(i_options.senderId,IPresence::Chat,SUBSCRIPTION_BOTH,false);

	o_options.noScroll = true;
	o_options.kind = IMessageStyleContentOptions::KindMessage;
	o_options.direction = IMessageStyleContentOptions::DirectionOut;
	o_options.senderId = "myself";
	o_options.senderName = tr("Sender");
	o_options.senderColor = "red";
	o_options.senderIcon = FMessageStyleManager->contactIcon(i_options.senderId,IPresence::Online,SUBSCRIPTION_BOTH,false);

	if (FMessageType==Message::Normal || FMessageType==Message::Headline || FMessageType==Message::Error)
	{
		i_options.time = QDateTime::currentDateTime().addDays(-1);
		i_options.timeFormat = FMessageStyleManager->timeFormat(i_options.time);

		if (FMessageType == Message::Error)
		{
			i_options.senderName.clear();
			i_options.kind = IMessageStyleContentOptions::KindMessage;
			QString html = "<b>"+tr("The message with a error code %1 is received").arg(999)+"</b>";
			html += "<p style='color:red;'>"+tr("Error description")+"</p>";
			html += "<hr>";
			FStyle->appendContent(FStyleView,html,i_options);
		}

		i_options.kind = IMessageStyleContentOptions::KindTopic;
		FStyle->appendContent(FStyleView,tr("Subject: Message subject"),i_options);

		i_options.kind = IMessageStyleContentOptions::KindMessage;
		FStyle->appendContent(FStyleView,tr("Message body line 1")+"<br>"+tr("Message body line 2"),i_options);
	}
	else if (FMessageType==Message::Chat || FMessageType==Message::GroupChat)
	{
		if (FMessageType==Message::GroupChat)
		{
			i_options.senderColor.clear();
			o_options.senderColor.clear();
		}

		i_options.time = QDateTime::currentDateTime().addDays(-1);
		i_options.type = IMessageStyleContentOptions::TypeHistory;
		if (Options::node(OPV_MESSAGES_SHOWDATESEPARATORS).value().toBool())
		{
			i_options.timeFormat = " ";
			i_options.kind = IMessageStyleContentOptions::KindStatus;
			i_options.status = IMessageStyleContentOptions::StatusDateSeparator;
			FStyle->appendContent(FStyleView,FMessageStyleManager->dateSeparator(i_options.time.date()),i_options);
		}

		i_options.kind = IMessageStyleContentOptions::KindMessage;
		i_options.status = IMessageStyleContentOptions::StatusEmpty;
		if (Options::node(OPV_MESSAGES_SHOWDATESEPARATORS).value().toBool())
			i_options.timeFormat = FMessageStyleManager->timeFormat(i_options.time,i_options.time);
		else
			i_options.timeFormat = FMessageStyleManager->timeFormat(i_options.time);
		FStyle->appendContent(FStyleView,tr("Incoming history message"),i_options);

		i_options.time = QDateTime::currentDateTime().addDays(-1).addSecs(80);
		FStyle->appendContent(FStyleView,tr("Incoming history consecutive message"),i_options);

		i_options.kind = IMessageStyleContentOptions::KindStatus;
		FStyle->appendContent(FStyleView,tr("Incoming status message"),i_options);

		o_options.type = IMessageStyleContentOptions::TypeHistory;
		o_options.time = QDateTime::currentDateTime().addDays(-1).addSecs(100);
		if (Options::node(OPV_MESSAGES_SHOWDATESEPARATORS).value().toBool())
			o_options.timeFormat = FMessageStyleManager->timeFormat(o_options.time,o_options.time);
		else
			o_options.timeFormat = FMessageStyleManager->timeFormat(o_options.time);
		FStyle->appendContent(FStyleView,tr("Outgoing history message"),o_options);

		o_options.kind = IMessageStyleContentOptions::KindStatus;
		FStyle->appendContent(FStyleView,tr("Outgoing status message"),o_options);

		i_options.type = 0;
		i_options.time = QDateTime::currentDateTime();
		if (Options::node(OPV_MESSAGES_SHOWDATESEPARATORS).value().toBool())
		{
			i_options.timeFormat = " ";
			i_options.kind = IMessageStyleContentOptions::KindStatus;
			i_options.status = IMessageStyleContentOptions::StatusDateSeparator;
			FStyle->appendContent(FStyleView,FMessageStyleManager->dateSeparator(i_options.time.date()),i_options);
		}

		i_options.status = IMessageStyleContentOptions::StatusEmpty;
		if (Options::node(OPV_MESSAGES_SHOWDATESEPARATORS).value().toBool())
			i_options.timeFormat = FMessageStyleManager->timeFormat(i_options.time,i_options.time);
		else
			i_options.timeFormat = FMessageStyleManager->timeFormat(i_options.time);
		if (FMessageType == Message::GroupChat)
		{
			i_options.kind = IMessageStyleContentOptions::KindTopic;
			FStyle->appendContent(FStyleView,tr("Conference topic"),i_options);
		}

		i_options.kind = IMessageStyleContentOptions::KindMessage;
		FStyle->appendContent(FStyleView,tr("Incoming message"),i_options);

		i_options.type = IMessageStyleContentOptions::TypeEvent;
		i_options.kind = IMessageStyleContentOptions::KindStatus;
		FStyle->appendContent(FStyleView,tr("Incoming event"),i_options);

		i_options.type = IMessageStyleContentOptions::TypeNotification;
		FStyle->appendContent(FStyleView,tr("Incoming notification"),i_options);

		if (FMessageType == Message::GroupChat)
		{
			i_options.kind = IMessageStyleContentOptions::KindMessage;
			i_options.type = IMessageStyleContentOptions::TypeMention;
			FStyle->appendContent(FStyleView,tr("Incoming mention message"),i_options);
		}

		o_options.type = 0;
		o_options.time = QDateTime::currentDateTime();
		if (Options::node(OPV_MESSAGES_SHOWDATESEPARATORS).value().toBool())
			o_options.timeFormat = FMessageStyleManager->timeFormat(o_options.time,o_options.time);
		else
			o_options.timeFormat = FMessageStyleManager->timeFormat(o_options.time);
		o_options.kind = IMessageStyleContentOptions::KindMessage;
		FStyle->appendContent(FStyleView,tr("Outgoing message"),o_options);

		o_options.time = QDateTime::currentDateTime().addSecs(5);
		FStyle->appendContent(FStyleView,tr("Outgoing consecutive message"),o_options);
	}
}

void StyleEditOptionsDialog::onUpdateStyleView()
{
	if (FStyleEngine && FStyleSettings && FStyle && FStyleView)
	{
		FStyle->changeOptions(FStyleView,FStyleEngine->styleSettinsOptions(FStyleSettings),true);
		createViewContent();
	}
	FUpdateStarted = false;
}
