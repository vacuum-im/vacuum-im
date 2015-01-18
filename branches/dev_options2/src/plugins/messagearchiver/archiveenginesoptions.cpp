#include "archiveenginesoptions.h"

#include <QFrame>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>

EngineWidget::EngineWidget(IMessageArchiver *AArchiver, IArchiveEngine *AEngine, QWidget *AParent) : QGroupBox(AParent)
{
	FEngine = AEngine;
	FArchiver = AArchiver;

	setTitle(AEngine->engineName());

	QHBoxLayout *hlayout = new QHBoxLayout;
	hlayout->setMargin(0);

	QLabel *descr = new QLabel(this);
	descr->setWordWrap(true);
	descr->setTextFormat(Qt::PlainText);
	descr->setText(FEngine->engineDescription());
	descr->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Preferred);
	hlayout->addWidget(descr);

	pbtEnable = new QPushButton(this);
	connect(pbtEnable,SIGNAL(clicked()),SLOT(onEnableButtonClicked()));
	hlayout->addWidget(pbtEnable);

	pbtDisable = new QPushButton(this);
	connect(pbtDisable,SIGNAL(clicked()),SLOT(onDisableButtonClicked()));
	hlayout->addWidget(pbtDisable);

	QVBoxLayout *vlayout = new QVBoxLayout;
	vlayout->setMargin(5);
	vlayout->addLayout(hlayout);

	IOptionsDialogWidget *engineOptions = FEngine->engineSettingsWidget(this);
	if (engineOptions)
	{
		vlayout->addWidget(engineOptions->instance());
		connect(engineOptions->instance(),SIGNAL(modified()),SIGNAL(modified()));
		connect(this,SIGNAL(childApply()),engineOptions->instance(),SLOT(apply()));
		connect(this,SIGNAL(childReset()),engineOptions->instance(),SLOT(reset()));
	}

	setLayout(vlayout);

	reset();
}

EngineWidget::~EngineWidget()
{

}

void EngineWidget::apply()
{
	FArchiver->setArchiveEngineEnabled(FEngine->engineId(),FEnabled);
	emit childApply();
}

void EngineWidget::reset()
{
	setEngineState(FArchiver->isArchiveEngineEnabled(FEngine->engineId()));
	emit childReset();
}

void EngineWidget::setEngineState(bool AEnabled)
{
	if (AEnabled)
	{
		pbtEnable->setEnabled(false);
		pbtEnable->setText(tr("Enabled"));

		pbtDisable->setEnabled(true);
		pbtDisable->setText(tr("Disable"));
	}
	else
	{
		pbtEnable->setEnabled(true);
		pbtEnable->setText(tr("Enable"));

		pbtDisable->setEnabled(false);
		pbtDisable->setText(tr("Disabled"));
	}
	FEnabled = AEnabled;
}

void EngineWidget::onEnableButtonClicked()
{
	setEngineState(true);
	emit modified();
}

void EngineWidget::onDisableButtonClicked()
{
	setEngineState(false);
	emit modified();
}

ArchiveEnginesOptions::ArchiveEnginesOptions(IMessageArchiver *AArchiver, QWidget *AParent) : QWidget(AParent)
{
	FArchiver = AArchiver;

	QVBoxLayout *layout = new QVBoxLayout;
	layout->setMargin(0);
	setLayout(layout);

	QMultiMap<QString, IArchiveEngine *> engineOrder;
	foreach(IArchiveEngine *engine, FArchiver->archiveEngines())
		engineOrder.insertMulti(engine->engineName(),engine);

	foreach(IArchiveEngine *engine, engineOrder)
	{
		EngineWidget *widget = new EngineWidget(FArchiver,engine,this);
		connect(widget,SIGNAL(modified()),SIGNAL(modified()));
		layout->addWidget(widget);
		FEngines.insert(engine,widget);
	}

	reset();
}

ArchiveEnginesOptions::~ArchiveEnginesOptions()
{

}

void ArchiveEnginesOptions::apply()
{
	foreach(EngineWidget *widget, FEngines)
		widget->apply();
	emit childApply();
}

void ArchiveEnginesOptions::reset()
{
	foreach(EngineWidget *widget, FEngines)
		widget->reset();
	emit childReset();
}
