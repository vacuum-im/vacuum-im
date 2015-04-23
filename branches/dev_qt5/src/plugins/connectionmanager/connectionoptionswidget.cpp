#include "connectionoptionswidget.h"

ConnectionOptionsWidget::ConnectionOptionsWidget(IConnectionManager *AManager, const OptionsNode &ANode, QWidget *AParent) : QWidget(AParent)
{
	ui.setupUi(this);
	FManager = AManager;

	FOptions = ANode;
	FEngineSettings = NULL;

	FEngineLayout = new QVBoxLayout(ui.wdtConnectionSettings);
	FEngineLayout->setMargin(0);

	foreach(const QString &engineId, FManager->connectionEngines())
		ui.cmbConnections->addItem(FManager->findConnectionEngine(engineId)->engineName(),engineId);
	ui.wdtSelectConnection->setVisible(ui.cmbConnections->count() > 1);

	connect(ui.cmbConnections, SIGNAL(currentIndexChanged(int)),SLOT(onComboConnectionsChanged(int)));

	reset();
}

ConnectionOptionsWidget::~ConnectionOptionsWidget()
{

}

void ConnectionOptionsWidget::apply()
{
	IConnectionEngine *engine = FManager->findConnectionEngine(FEngineId);
	if (engine)
	{
		FOptions.node("connection-type").setValue(FEngineId);
		if (FEngineSettings)
			engine->saveConnectionSettings(FEngineSettings);
	}
	emit childApply();
}

void ConnectionOptionsWidget::reset()
{
	QString engineId = FOptions.value("connection-type").toString();
	if (!FManager->connectionEngines().isEmpty())
		setEngineById(FManager->findConnectionEngine(engineId) ? engineId : FManager->connectionEngines().first());
	if (FEngineSettings)
		FEngineSettings->reset();
	emit childReset();
}

void ConnectionOptionsWidget::setEngineById(const QString &AEngineId)
{
	if (FEngineId != AEngineId)
	{
		if (FEngineSettings)
		{
			delete FEngineSettings->instance();
			FEngineSettings = NULL;
			FEngineId = QUuid().toString();
		}

		IConnectionEngine *engine = FManager->findConnectionEngine(AEngineId);
		if (engine)
		{
			FEngineId = AEngineId;
			FEngineSettings = engine->connectionSettingsWidget(FOptions.node("connection",AEngineId), ui.wdtConnectionSettings);
			if (FEngineSettings)
			{
				FEngineLayout->addWidget(FEngineSettings->instance());
				connect(FEngineSettings->instance(),SIGNAL(modified()),SIGNAL(modified()));
			}
		}

		if (ui.cmbConnections->itemData(ui.cmbConnections->currentIndex()).toString() != AEngineId)
			ui.cmbConnections->setCurrentIndex(ui.cmbConnections->findData(AEngineId));

		emit modified();
	}
}

void ConnectionOptionsWidget::onComboConnectionsChanged(int AIndex)
{
	if (AIndex != -1)
		setEngineById(ui.cmbConnections->itemData(AIndex).toString());
	else
		setEngineById(QUuid().toString());
}
