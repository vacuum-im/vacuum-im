#include "styleselectoptionswidget.h"

#include <QPushButton>
#include <QHBoxLayout>
#include <definitions/optionvalues.h>
#include <utils/widgetmanager.h>
#include "styleeditoptionsdialog.h"

#define SEPARATOR  "=||="

StyleSelectOptionsWidget::StyleSelectOptionsWidget(IMessageStyleManager *AMessageStyleManager, int AMessageType, QWidget *AParent) : QWidget(AParent)
{
	FMessageType = AMessageType;
	FMessageStyleManager = AMessageStyleManager;

	lblType = new QLabel(this);
	switch (AMessageType)
	{
	case Message::Chat:
		lblType->setText(tr("Chat:"));
		break;
	case Message::GroupChat:
		lblType->setText(tr("Conference:"));
		break;
	case Message::Normal:
		lblType->setText(tr("Message:"));
		break;
	case Message::Headline:
		lblType->setText(tr("Headline:"));
		break;
	case Message::Error:
		lblType->setText(tr("Error:"));
		break;
	default:
		lblType->setText(tr("Unknown:"));
		break;
	}

	cmbStyle = new QComboBox(this);
	foreach(const QString &engineId, FMessageStyleManager->styleEngines())
	{
		IMessageStyleEngine *engine = FMessageStyleManager->findStyleEngine(engineId);
		if (engine!=NULL && engine->supportedMessageTypes().contains(FMessageType))
		{
			foreach(const QString &style, engine->styles())
				cmbStyle->addItem(QString("%1 - %2").arg(engine->engineName(), style), engine->engineId() + SEPARATOR + style);
		}
	}
	connect(cmbStyle,SIGNAL(currentIndexChanged(int)),SIGNAL(modified()));

	QPushButton *pbtEdit = new QPushButton(this);
	pbtEdit->setText(tr("Settings..."));
	connect(pbtEdit,SIGNAL(clicked()),SLOT(onEditStyleButtonClicked()));

	QHBoxLayout *hblLayout = new QHBoxLayout(this);
	hblLayout->setMargin(0);
	hblLayout->addWidget(lblType,2);
	hblLayout->addWidget(cmbStyle,10);
	hblLayout->addWidget(pbtEdit,1);

	reset();
}

void StyleSelectOptionsWidget::apply()
{
	QStringList params = cmbStyle->itemData(cmbStyle->currentIndex()).toString().split(SEPARATOR);
	QString engineId = params.value(0);
	QString styleId = params.value(1);

	OptionsNode node = Options::node(OPV_MESSAGESTYLE_MTYPE_ITEM,QString::number(FMessageType)).node("context",QString::null);
	node.node("engine-id").setValue(engineId);
	node.node("engine",engineId).node("style-id").setValue(styleId);

	emit childApply();
}

void StyleSelectOptionsWidget::reset()
{
	IMessageStyleOptions soptions = FMessageStyleManager->styleOptions(FMessageType);
	cmbStyle->setCurrentIndex(cmbStyle->findData(soptions.engineId + SEPARATOR + soptions.styleId));
	emit childReset();
}

void StyleSelectOptionsWidget::onEditStyleButtonClicked()
{
	QStringList params = cmbStyle->itemData(cmbStyle->currentIndex()).toString().split(SEPARATOR);
	QString engineId = params.value(0);
	QString styleId = params.value(1);

	OptionsNode styleNode = Options::node(OPV_MESSAGESTYLE_MTYPE_ITEM,QString::number(FMessageType)).node("context").node("engine",engineId).node("style",styleId);
	QDialog *dialog = new StyleEditOptionsDialog(FMessageStyleManager,styleNode,this);
	WidgetManager::showActivateRaiseWindow(dialog);
}
