#include "simpleoptionswidget.h"

#include <QColor>
#include <QFileInfo>
#include <QFileDialog>
#include <QFontDialog>

SimpleOptionsWidget::SimpleOptionsWidget(SimpleMessageStyleEngine *AEngine, const OptionsNode &ANode, QWidget *AParent) : QWidget(AParent)
{
	ui.setupUi(this);
	ui.lblParameters->setText(QString("<h2>%1</h2>").arg(tr("Parameters")));
	ui.lblBackground->setText(QString("<h2>%1</h2>").arg(tr("Background")));

	FStyleNode = ANode;
	FStyleEngine = AEngine;

	foreach(const QString &variant, FStyleEngine->styleVariants(FStyleNode.nspace()))
		ui.cmbVariant->addItem(variant,variant);
	if (ui.cmbVariant->count() <= 0)
		ui.cmbVariant->addItem(tr("Default"));

	ui.cmbColor->addItem(tr("Default"));
	QStringList sortedColors = QColor::colorNames();
	sortedColors.sort();
	foreach(const QString &color,sortedColors)
	{
		ui.cmbColor->addItem(color,color);
		ui.cmbColor->setItemData(ui.cmbColor->count()-1,QColor(color),Qt::DecorationRole);
	}

	connect(ui.cmbVariant,SIGNAL(currentIndexChanged(int)),SLOT(onVariantChanged(int)));
	connect(ui.tlbFontChange,SIGNAL(clicked()),SLOT(onFontChangeClicked()));
	connect(ui.tlbFontReset,SIGNAL(clicked()),SLOT(onFontResetClicked()));
	connect(ui.cmbColor,SIGNAL(currentIndexChanged(int)),SLOT(onColorChanged(int)));
	connect(ui.tlbImageChange,SIGNAL(clicked()),SLOT(onImageChangeClicked()));
	connect(ui.tlbImageReset,SIGNAL(clicked()),SLOT(onImageResetClicked()));

	reset();
}

SimpleOptionsWidget::~SimpleOptionsWidget()
{

}

IMessageStyleOptions SimpleOptionsWidget::styleOptions() const
{
	return FStyleOptions;
}

void SimpleOptionsWidget::apply()
{
	FStyleNode.setValue(FStyleOptions.extended.value(MSO_VARIANT),"variant");
	FStyleNode.setValue(FStyleOptions.extended.value(MSO_FONT_FAMILY),"font-family");
	FStyleNode.setValue(FStyleOptions.extended.value(MSO_FONT_SIZE),"font-size");
	FStyleNode.setValue(FStyleOptions.extended.value(MSO_BG_COLOR),"bg-color");
	FStyleNode.setValue(FStyleOptions.extended.value(MSO_BG_IMAGE_FILE),"bg-image-file");
	emit childApply();
}

void SimpleOptionsWidget::reset()
{
	FStyleOptions = FStyleEngine->styleOptions(FStyleNode.parent(),FStyleNode.nspace());
	QMap<QString, QVariant> styleInfo = FStyleEngine->styleInfo(FStyleOptions.styleId);

	int variantIndex;
	if ( (variantIndex = ui.cmbVariant->findData(FStyleOptions.extended.value(MSO_VARIANT))) >= 0)
		ui.cmbVariant->setCurrentIndex(variantIndex);
	else if ( (variantIndex = ui.cmbVariant->findData(styleInfo.value(MSIV_DEFAULT_VARIANT))) >= 0)
		ui.cmbVariant->setCurrentIndex(variantIndex);
	else
		ui.cmbVariant->setCurrentIndex(0);

	bool isCustomBackgroundEnabled = !styleInfo.value(MSIV_DISABLE_CUSTOM_BACKGROUND,false).toBool();
	ui.cmbColor->setEnabled(isCustomBackgroundEnabled);
	ui.tlbImageChange->setEnabled(isCustomBackgroundEnabled);
	ui.tlbImageReset->setEnabled(isCustomBackgroundEnabled);

	ui.cmbColor->setItemData(0,styleInfo.value(MSIV_DEFAULT_BACKGROUND_COLOR));
	ui.cmbColor->setCurrentIndex(ui.cmbColor->findData(FStyleOptions.extended.value(MSO_BG_COLOR)));

	updateOptionsWidgets();

	emit childReset();
}

void SimpleOptionsWidget::updateOptionsWidgets()
{
	QString fontFamily = FStyleOptions.extended.value(MSO_FONT_FAMILY).toString();
	int fontSize = FStyleOptions.extended.value(MSO_FONT_SIZE).toInt();
	fontFamily = fontFamily.isEmpty() ? QFont().family() : fontFamily;
	fontSize = fontSize<=0 ? QFont().pointSize() : fontSize;
	ui.lneFont->setText(QString("%1 %2").arg(fontFamily).arg(fontSize));

	QFileInfo fileInfo(FStyleOptions.extended.value(MSO_BG_IMAGE_FILE).toString());
	ui.lneImage->setText(fileInfo.isFile() ? fileInfo.fileName() : QString::null);
}

void SimpleOptionsWidget::onVariantChanged(int AIndex)
{
	FStyleOptions.extended.insert(MSO_VARIANT,ui.cmbVariant->itemData(AIndex));
	emit modified();
}

void SimpleOptionsWidget::onFontChangeClicked()
{
	bool ok;
	QFont font(FStyleOptions.extended.value(MSO_FONT_FAMILY,QFont().family()).toString(),FStyleOptions.extended.value(MSO_FONT_SIZE,QFont().pointSize()).toInt());
	font = QFontDialog::getFont(&ok,font,this,tr("Select font family and size"));
	if (ok)
	{
		FStyleOptions.extended.insert(MSO_FONT_FAMILY,font.family());
		FStyleOptions.extended.insert(MSO_FONT_SIZE,font.pointSize());
		updateOptionsWidgets();
		emit modified();
	}
}

void SimpleOptionsWidget::onFontResetClicked()
{
	QMap<QString,QVariant> styleInfo = FStyleEngine->styleInfo(FStyleOptions.styleId);
	FStyleOptions.extended.insert(MSO_FONT_FAMILY,styleInfo.value(MSIV_DEFAULT_FONT_FAMILY));
	FStyleOptions.extended.insert(MSO_FONT_SIZE,styleInfo.value(MSIV_DEFAULT_FONT_SIZE));
	updateOptionsWidgets();
	emit modified();
}

void SimpleOptionsWidget::onColorChanged(int AIndex)
{
	FStyleOptions.extended.insert(MSO_BG_COLOR,ui.cmbColor->itemData(AIndex));
	emit modified();
}

void SimpleOptionsWidget::onImageChangeClicked()
{
	QString fileName = QFileDialog::getOpenFileName(this, tr("Select background image"),QString::null,tr("Image Files (*.png *.jpg *.bmp *.gif)"));
	if (!fileName.isEmpty())
	{
		FStyleOptions.extended.insert(MSO_BG_IMAGE_FILE,fileName);
		updateOptionsWidgets();
		emit modified();
	}
}

void SimpleOptionsWidget::onImageResetClicked()
{
	FStyleOptions.extended.insert(MSO_BG_IMAGE_FILE,QVariant());
	updateOptionsWidgets();
	emit modified();
}
