#include "simpleoptionswidget.h"

#include <QColor>
#include <QFileDialog>
#include <QFontDialog>

SimpleOptionsWidget::SimpleOptionsWidget(SimpleMessageStylePlugin *APlugin, const OptionsNode &ANode, int AMessageType, QWidget *AParent) : QWidget(AParent)
{
	ui.setupUi(this);

	FStylePlugin = APlugin;
	FOptions = ANode;
	FMessageType = AMessageType;

	foreach(const QString &styleId, FStylePlugin->styles())
		ui.cmbStyle->addItem(styleId,styleId);
	ui.cmbStyle->setCurrentIndex(-1);

	ui.cmbBackgoundColor->addItem(tr("Default"));
	QStringList colors = QColor::colorNames();
	colors.sort();
	foreach(const QString &color, colors)
	{
		ui.cmbBackgoundColor->addItem(color,color);
		ui.cmbBackgoundColor->setItemData(ui.cmbBackgoundColor->count()-1,QColor(color),Qt::DecorationRole);
	}

	connect(ui.cmbStyle,SIGNAL(currentIndexChanged(int)),SLOT(onStyleChanged(int)));
	connect(ui.cmbVariant,SIGNAL(currentIndexChanged(int)),SLOT(onVariantChanged(int)));
	connect(ui.cmbBackgoundColor,SIGNAL(currentIndexChanged(int)),SLOT(onBackgroundColorChanged(int)));
	connect(ui.tlbSetFont,SIGNAL(clicked()),SLOT(onSetFontClicked()));
	connect(ui.tlbDefaultFont,SIGNAL(clicked()),SLOT(onDefaultFontClicked()));
	connect(ui.tlbSetImage,SIGNAL(clicked()),SLOT(onSetImageClicked()));
	connect(ui.tlbDefaultImage,SIGNAL(clicked()),SLOT(onDefaultImageClicked()));
	connect(ui.chkEnableAnimation,SIGNAL(stateChanged(int)),SLOT(onAnimationEnableToggled(int)));

	reset();
}

SimpleOptionsWidget::~SimpleOptionsWidget()
{

}

void SimpleOptionsWidget::apply(OptionsNode ANode)
{
	OptionsNode node = ANode.isNull() ? FOptions : ANode;
	node.setValue(FStyleOptions.extended.value(MSO_STYLE_ID),"style-id");
	node.setValue(FStyleOptions.extended.value(MSO_VARIANT),"variant");
	node.setValue(FStyleOptions.extended.value(MSO_FONT_FAMILY),"font-family");
	node.setValue(FStyleOptions.extended.value(MSO_FONT_SIZE),"font-size");
	node.setValue(FStyleOptions.extended.value(MSO_BG_COLOR),"bg-color");
	node.setValue(FStyleOptions.extended.value(MSO_BG_IMAGE_FILE),"bg-image-file");
	node.setValue(FStyleOptions.extended.value(MSO_ANIMATION_DISABLED),"animation-disabled");
	emit childApply();
}

void SimpleOptionsWidget::apply()
{
	apply(FOptions);
}

void SimpleOptionsWidget::reset()
{
	disconnect(ui.cmbVariant,SIGNAL(currentIndexChanged(int)),this,SLOT(onVariantChanged(int)));

	FStyleOptions = FStylePlugin->styleOptions(FOptions,FMessageType);
	ui.cmbStyle->setCurrentIndex(ui.cmbStyle->findData(FStyleOptions.extended.value(MSO_STYLE_ID)));
	ui.cmbVariant->setCurrentIndex(ui.cmbVariant->findData(FStyleOptions.extended.value(MSO_VARIANT)));
	ui.cmbBackgoundColor->setCurrentIndex(ui.cmbBackgoundColor->findData(FStyleOptions.extended.value(MSO_BG_COLOR)));
	ui.chkEnableAnimation->setChecked(!FStyleOptions.extended.value(MSO_ANIMATION_DISABLED).toBool());
	updateOptionsWidgets();

	connect(ui.cmbVariant,SIGNAL(currentIndexChanged(int)),SLOT(onVariantChanged(int)));

	emit childReset();
}

IMessageStyleOptions SimpleOptionsWidget::styleOptions() const
{
	return FStyleOptions;
}

void SimpleOptionsWidget::updateOptionsWidgets()
{
	QString family = FStyleOptions.extended.value(MSO_FONT_FAMILY).toString();
	int size = FStyleOptions.extended.value(MSO_FONT_SIZE).toInt();
	if (family.isEmpty())
		family = QFont().family();
	if (size==0)
		size = QFont().pointSize();
	ui.lblFont->setText(family + " " +QString::number(size));
}

void SimpleOptionsWidget::onStyleChanged(int AIndex)
{
	QString styleId = ui.cmbStyle->itemData(AIndex).toString();
	FStyleOptions.extended.insert(MSO_STYLE_ID,styleId);

	ui.cmbVariant->clear();
	foreach(const QString &variant, FStylePlugin->styleVariants(styleId))
		ui.cmbVariant->addItem(variant,variant);
	ui.cmbVariant->setEnabled(ui.cmbVariant->count() > 0);

	QMap<QString, QVariant> info = FStylePlugin->styleInfo(styleId);
	if (info.contains(MSIV_DEFAULT_VARIANT))
	{
		int index = ui.cmbVariant->findData(info.value(MSIV_DEFAULT_VARIANT));
		ui.cmbVariant->setCurrentIndex(index>=0 ? index : 0);
	}

	bool backgroundEnabled = !info.value(MSIV_DISABLE_CUSTOM_BACKGROUND,false).toBool();
	ui.tlbSetImage->setEnabled(backgroundEnabled);
	ui.tlbDefaultImage->setEnabled(backgroundEnabled);
	ui.cmbBackgoundColor->setEnabled(backgroundEnabled);

	ui.cmbBackgoundColor->setItemData(0,info.value(MSIV_DEFAULT_BACKGROUND_COLOR));

	onDefaultImageClicked();
}

void SimpleOptionsWidget::onVariantChanged(int AIndex)
{
	FStyleOptions.extended.insert(MSO_VARIANT,ui.cmbVariant->itemData(AIndex));
	emit modified();
}

void SimpleOptionsWidget::onSetFontClicked()
{
	bool ok;
	QFont font(FStyleOptions.extended.value(MSO_FONT_FAMILY).toString(),FStyleOptions.extended.value(MSO_FONT_SIZE).toInt());
	font = QFontDialog::getFont(&ok,font,this,tr("Select font family and size"));
	if (ok)
	{
		FStyleOptions.extended.insert(MSO_FONT_FAMILY,font.family());
		FStyleOptions.extended.insert(MSO_FONT_SIZE,font.pointSize());
		updateOptionsWidgets();
		emit modified();
	}
}

void SimpleOptionsWidget::onDefaultFontClicked()
{
	QMap<QString,QVariant> info = FStylePlugin->styleInfo(ui.cmbStyle->itemData(ui.cmbStyle->currentIndex()).toString());
	FStyleOptions.extended.insert(MSO_FONT_FAMILY,info.value(MSIV_DEFAULT_FONT_FAMILY));
	FStyleOptions.extended.insert(MSO_FONT_SIZE,info.value(MSIV_DEFAULT_FONT_SIZE));
	updateOptionsWidgets();
	emit modified();
}

void SimpleOptionsWidget::onBackgroundColorChanged(int AIndex)
{
	FStyleOptions.extended.insert(MSO_BG_COLOR,ui.cmbBackgoundColor->itemData(AIndex));
	emit modified();
}

void SimpleOptionsWidget::onSetImageClicked()
{
	QString fileName = QFileDialog::getOpenFileName(this, tr("Select background image"),QString::null,tr("Image Files (*.png *.jpg *.bmp *.gif)"));
	if (!fileName.isEmpty())
	{
		FStyleOptions.extended.insert(MSO_BG_IMAGE_FILE,fileName);
		updateOptionsWidgets();
		emit modified();
	}
}

void SimpleOptionsWidget::onDefaultImageClicked()
{
	FStyleOptions.extended.remove(MSO_BG_IMAGE_FILE);

	ui.cmbBackgoundColor->setCurrentIndex(0);

	updateOptionsWidgets();
	emit modified();
}

void SimpleOptionsWidget::onAnimationEnableToggled(int AState)
{
	FStyleOptions.extended.insert(MSO_ANIMATION_DISABLED, AState!=Qt::Checked);
	emit modified();
}
