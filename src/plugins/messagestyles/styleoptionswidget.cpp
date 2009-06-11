#include "styleoptionswidget.h"

#include <QVBoxLayout>

StyleOptionsWidget::StyleOptionsWidget(IMessageStyles *AMessageStyles, QWidget *AParent) : QWidget(AParent)
{
  ui.setupUi(this);

  FActiveView = NULL;
  FActiveStyle = NULL;
  FActivePlugin = NULL;
  FActiveSettings = NULL;
  FMessageStyles = AMessageStyles;

  ui.cmbMessageType->addItem(tr("Chat"),Message::Chat);
  ui.cmbMessageType->addItem(tr("Single"),Message::Normal);
  ui.cmbMessageType->addItem(tr("Conference"),Message::GroupChat);
  ui.cmbMessageType->addItem(tr("Headline"),Message::Headline);
  ui.cmbMessageType->addItem(tr("Error"),Message::Error);

  foreach(QString spluginId, FMessageStyles->stylePlugins())
    ui.cmbStyleEngine->addItem(spluginId,spluginId);

  ui.wdtStyleOptions->setLayout(new QVBoxLayout);
  ui.wdtStyleOptions->layout()->setMargin(0);

  ui.frmExample->setLayout(new QVBoxLayout);
  ui.frmExample->layout()->setMargin(0);

  onMessageTypeChanged(ui.cmbMessageType->currentIndex());
  onStyleEngineChanged(ui.cmbStyleEngine->currentIndex());

  connect(ui.cmbMessageType,SIGNAL(currentIndexChanged(int)),SLOT(onMessageTypeChanged(int)));
  connect(ui.cmbStyleEngine,SIGNAL(currentIndexChanged(int)),SLOT(onStyleEngineChanged(int)));
}

StyleOptionsWidget::~StyleOptionsWidget()
{

}

void StyleOptionsWidget::apply()
{
  QMap<int, QString>::const_iterator it = FPluginForMessage.constBegin();
  while (it != FPluginForMessage.constEnd())
  {
    IMessageStyleSettings *settings = FSettings.value(FMessageStyles->stylePluginById(it.value()));
    if (settings)
      FMessageStyles->setStyleOptions(settings->styleOptions(it.key(),QString::null),it.key());
    it++;
  }
  emit optionsApplied();
}

void StyleOptionsWidget::updateActiveSettings()
{
  int curMessageType = ui.cmbMessageType->itemData(ui.cmbMessageType->currentIndex()).toInt();
  if (FActiveSettings && FActiveSettings->messageType()!=curMessageType)
    FActiveSettings->loadSettings(curMessageType,QString::null);
  else
    onStyleSettingsChanged();
}

void StyleOptionsWidget::createViewContent()
{
  if (FActiveStyle && FActiveView)
  {
    int curMessageType = ui.cmbMessageType->itemData(ui.cmbMessageType->currentIndex()).toInt();

    IMessageContentOptions i_options;
    IMessageContentOptions o_options;

    i_options.noScroll = true;
    i_options.kind = IMessageContentOptions::Message;
    i_options.direction = IMessageContentOptions::DirectionIn;
    i_options.senderId = "remote";
    i_options.senderName = tr("Receiver");
    i_options.senderColor = "blue";
    i_options.senderIcon = FMessageStyles->userIcon(i_options.senderId,IPresence::Chat,SUBSCRIPTION_BOTH,false);

    o_options.noScroll = true;
    o_options.kind = IMessageContentOptions::Message;
    o_options.direction = IMessageContentOptions::DirectionOut;
    o_options.senderId = "myself";
    o_options.senderName = tr("Sender");
    o_options.senderColor = "red";
    o_options.senderIcon = FMessageStyles->userIcon(i_options.senderId,IPresence::Online,SUBSCRIPTION_BOTH,false);

    if (curMessageType==Message::Normal || curMessageType==Message::Headline || curMessageType==Message::Error)
    {
      i_options.time = QDateTime::currentDateTime().addDays(-1);
      i_options.timeFormat = FMessageStyles->timeFormat(i_options.time);

      if (curMessageType == Message::Error)
      {
        i_options.senderName.clear();
        i_options.kind = IMessageContentOptions::Message;
        QString html = "<b>"+tr("The message with a error code %1 is received").arg(999)+"</b>";
        html += "<p style='color:red;'>"+tr("Error description")+"</p>";
        html += "<hr>";
        FActiveStyle->appendContent(FActiveView,html,i_options);
      }

      i_options.kind = IMessageContentOptions::Topic;
      FActiveStyle->appendContent(FActiveView,tr("Subject: Message subject"),i_options);

      i_options.kind = IMessageContentOptions::Message;
      FActiveStyle->appendContent(FActiveView,tr("Message body line 1")+"<br>"+tr("Message body line 2"),i_options);
    }
    else if (curMessageType==Message::Chat || curMessageType==Message::GroupChat)
    {
      if (curMessageType==Message::GroupChat)
      {
        i_options.senderColor.clear();
        o_options.senderColor.clear();
      }

      i_options.type = IMessageContentOptions::History;
      i_options.time = QDateTime::currentDateTime().addDays(-1);
      i_options.timeFormat = FMessageStyles->timeFormat(i_options.time);
      FActiveStyle->appendContent(FActiveView,tr("Incoming history message"),i_options);

      i_options.time = QDateTime::currentDateTime().addDays(-1).addSecs(80);
      FActiveStyle->appendContent(FActiveView,tr("Incoming history consecutive message"),i_options);

      i_options.kind = IMessageContentOptions::Status;
      FActiveStyle->appendContent(FActiveView,tr("Incoming status message"),i_options);

      o_options.type = IMessageContentOptions::History;
      o_options.time = QDateTime::currentDateTime().addDays(-1).addSecs(100);
      o_options.timeFormat = FMessageStyles->timeFormat(o_options.time);
      FActiveStyle->appendContent(FActiveView,tr("Outgoing history message"),o_options);

      o_options.kind = IMessageContentOptions::Status;
      FActiveStyle->appendContent(FActiveView,tr("Outgoing status message"),o_options);

      if (curMessageType==Message::GroupChat)
      {
        i_options.kind = IMessageContentOptions::Topic;
        i_options.type = 0;
        i_options.time = QDateTime::currentDateTime();
        i_options.timeFormat = FMessageStyles->timeFormat(i_options.time);
        FActiveStyle->appendContent(FActiveView,tr("Groupchat topic"),i_options);
      }

      i_options.time = QDateTime::currentDateTime();
      i_options.timeFormat = FMessageStyles->timeFormat(i_options.time);
      i_options.kind = IMessageContentOptions::Message;
      i_options.type = 0;
      FActiveStyle->appendContent(FActiveView,tr("Incoming message"),i_options);

      i_options.type = IMessageContentOptions::Event;
      i_options.kind = IMessageContentOptions::Status;
      FActiveStyle->appendContent(FActiveView,tr("Incoming event"),i_options);

      i_options.type = IMessageContentOptions::Notification;
      FActiveStyle->appendContent(FActiveView,tr("Incoming notification"),i_options);

      if (curMessageType==Message::GroupChat)
      {
        i_options.kind = IMessageContentOptions::Message;
        i_options.type = IMessageContentOptions::Mention;
        FActiveStyle->appendContent(FActiveView,tr("Incoming mention message"),i_options);
      }
  
      o_options.time = QDateTime::currentDateTime();
      o_options.timeFormat = FMessageStyles->timeFormat(o_options.time);
      o_options.kind = IMessageContentOptions::Message;
      o_options.type = 0;
      FActiveStyle->appendContent(FActiveView,tr("Outgoing message"),o_options);

      o_options.time = QDateTime::currentDateTime().addSecs(5);
      FActiveStyle->appendContent(FActiveView,tr("Outgoing consecutive message"),o_options);
    }
  }
}

void StyleOptionsWidget::onStyleSettingsChanged()
{
  IMessageStyleOptions soptions = FActiveSettings!=NULL ? FActiveSettings->styleOptions(FActiveSettings->messageType(),FActiveSettings->context()) : IMessageStyleOptions();
  IMessageStyle *style = FActivePlugin!=NULL ? FActivePlugin->styleForOptions(soptions) : NULL;
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
    FActiveStyle->changeStyleOptions(FActiveView,soptions);
  }
  createViewContent();
}

void StyleOptionsWidget::onMessageTypeChanged(int AIndex)
{
  int curMessageType = ui.cmbMessageType->itemData(AIndex).toInt();
  QString curPluginId = ui.cmbStyleEngine->itemData(ui.cmbStyleEngine->currentIndex()).toString();
  
  QString spluginId;
  if (!FPluginForMessage.contains(curMessageType))
  {
    spluginId = FMessageStyles->styleOptions(ui.cmbMessageType->itemData(AIndex).toInt()).pluginId;
    FPluginForMessage.insert(curMessageType,spluginId);
  }
  else
    spluginId = FPluginForMessage.value(curMessageType);

  if (spluginId != curPluginId)
    ui.cmbStyleEngine->setCurrentIndex(ui.cmbStyleEngine->findData(spluginId));
  else
    updateActiveSettings();
}

void StyleOptionsWidget::onStyleEngineChanged(int AIndex)
{
  QString curPluginId = ui.cmbStyleEngine->itemData(AIndex).toString();
  int curMessageType = ui.cmbMessageType->itemData(ui.cmbMessageType->currentIndex()).toInt();
  FPluginForMessage.insert(curMessageType,curPluginId);

  FActivePlugin = FMessageStyles->stylePluginById(curPluginId);

  if (FActiveSettings)
  {
    ui.wdtStyleOptions->layout()->removeWidget(FActiveSettings->instance());
    FActiveSettings->instance()->setVisible(false);
    disconnect(FActiveSettings->instance(),SIGNAL(settingsChanged()),this,SLOT(onStyleSettingsChanged()));
  }

  if (FActivePlugin && !FSettings.contains(FActivePlugin))
  {
    FActiveSettings = FActivePlugin->styleSettings(Message::Chat,QString::null,ui.wdtStyleOptions);
    FSettings.insert(FActivePlugin,FActiveSettings);
  }
  else
    FActiveSettings = FSettings.value(FActivePlugin,NULL);

  if (FActiveSettings)
  {
    ui.wdtStyleOptions->layout()->addWidget(FActiveSettings->instance());
    FActiveSettings->instance()->setVisible(true);
    connect(FActiveSettings->instance(),SIGNAL(settingsChanged()),SLOT(onStyleSettingsChanged()));
  }
  updateActiveSettings();
}
