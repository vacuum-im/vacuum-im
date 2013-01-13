#include "adiumoptionswidget.h"

#include <QColor>
#include <QFileDialog>
#include <QFontDialog>
#include <QWebSettings>

AdiumOptionsWidget::AdiumOptionsWidget(AdiumMessageStylePlugin *APlugin, const OptionsNode &ANode, int AMessageType, QWidget *AParent) : QWidget(AParent)
{
  ui.setupUi(this);

  FStylePlugin = APlugin;
  FOptions = ANode;
  FMessageType = AMessageType;

  foreach(QString styleId, FStylePlugin->styles())
    ui.cmbStyle->addItem(styleId,styleId);
  ui.cmbStyle->setCurrentIndex(-1);

  ui.cmbBackgoundColor->addItem(tr("Default"));
  QStringList colors = QColor::colorNames();
  colors.sort();
  foreach(QString color, colors)
  {
    ui.cmbBackgoundColor->addItem(color,color);
    ui.cmbBackgoundColor->setItemData(ui.cmbBackgoundColor->count()-1,QColor(color),Qt::DecorationRole);
  }

  ui.cmbImageLayout->addItem(tr("Normal"),AdiumMessageStyle::ImageNormal);
  ui.cmbImageLayout->addItem(tr("Center"),AdiumMessageStyle::ImageCenter);
  ui.cmbImageLayout->addItem(tr("Title"),AdiumMessageStyle::ImageTitle);
  ui.cmbImageLayout->addItem(tr("Title center"),AdiumMessageStyle::ImageTitleCenter);
  ui.cmbImageLayout->addItem(tr("Scale"),AdiumMessageStyle::ImageScale);

  connect(ui.cmbStyle,SIGNAL(currentIndexChanged(int)),SLOT(onStyleChanged(int)));
  connect(ui.cmbVariant,SIGNAL(currentIndexChanged(int)),SLOT(onVariantChanged(int)));
  connect(ui.cmbImageLayout,SIGNAL(currentIndexChanged(int)),SLOT(onImageLayoutChanged(int)));
  connect(ui.cmbBackgoundColor,SIGNAL(currentIndexChanged(int)),SLOT(onBackgroundColorChanged(int)));
  connect(ui.tlbSetFont,SIGNAL(clicked()),SLOT(onSetFontClicked()));
  connect(ui.tlbDefaultFont,SIGNAL(clicked()),SLOT(onDefaultFontClicked()));
  connect(ui.tlbSetImage,SIGNAL(clicked()),SLOT(onSetImageClicked()));
  connect(ui.tlbDefaultImage,SIGNAL(clicked()),SLOT(onDefaultImageClicked()));

  reset();
}

AdiumOptionsWidget::~AdiumOptionsWidget()
{

}

void AdiumOptionsWidget::apply(OptionsNode ANode)
{
  OptionsNode node = ANode.isNull() ? FOptions : ANode;
  node.setValue(FStyleOptions.extended.value(MSO_STYLE_ID),"style-id");
  node.setValue(FStyleOptions.extended.value(MSO_VARIANT),"variant");
  node.setValue(FStyleOptions.extended.value(MSO_FONT_FAMILY),"font-family");
  node.setValue(FStyleOptions.extended.value(MSO_FONT_SIZE),"font-size");
  node.setValue(FStyleOptions.extended.value(MSO_BG_COLOR),"bg-color");
  node.setValue(FStyleOptions.extended.value(MSO_BG_IMAGE_FILE),"bg-image-file");
  node.setValue(FStyleOptions.extended.value(MSO_BG_IMAGE_LAYOUT),"bg-image-layout");
  emit childApply();
}

void AdiumOptionsWidget::apply()
{
  apply(FOptions);
}

void AdiumOptionsWidget::reset()
{
  disconnect(ui.cmbVariant,SIGNAL(currentIndexChanged(int)),this,SLOT(onVariantChanged(int)));

  FStyleOptions = FStylePlugin->styleOptions(FOptions,FMessageType);
  ui.cmbStyle->setCurrentIndex(ui.cmbStyle->findData(FStyleOptions.extended.value(MSO_STYLE_ID)));
  ui.cmbVariant->setCurrentIndex(ui.cmbVariant->findData(FStyleOptions.extended.value(MSO_VARIANT)));
  ui.cmbBackgoundColor->setCurrentIndex(ui.cmbBackgoundColor->findData(FStyleOptions.extended.value(MSO_BG_COLOR)));
  ui.cmbImageLayout->setCurrentIndex(ui.cmbImageLayout->findData(FStyleOptions.extended.value(MSO_BG_IMAGE_LAYOUT)));
  updateOptionsWidgets();

  connect(ui.cmbVariant,SIGNAL(currentIndexChanged(int)),SLOT(onVariantChanged(int)));

  emit childReset();
}

IMessageStyleOptions AdiumOptionsWidget::styleOptions() const
{
  return FStyleOptions;
}

void AdiumOptionsWidget::updateOptionsWidgets()
{
  QString family = FStyleOptions.extended.value(MSO_FONT_FAMILY).toString();
  int size = FStyleOptions.extended.value(MSO_FONT_SIZE).toInt();
  if (family.isEmpty())
    family = QWebSettings::globalSettings()->fontFamily(QWebSettings::StandardFont);
  if (size==0)
    size = QWebSettings::globalSettings()->fontSize(QWebSettings::DefaultFontSize);
  ui.lblFont->setText(family + " " +QString::number(size));
  ui.cmbImageLayout->setEnabled(!FStyleOptions.extended.value(MSO_BG_IMAGE_FILE).toString().isEmpty());
}

void AdiumOptionsWidget::onStyleChanged(int AIndex)
{
  QString styleId = ui.cmbStyle->itemData(AIndex).toString();

  ui.cmbVariant->clear();
  foreach(QString variant, FStylePlugin->styleVariants(styleId))
    ui.cmbVariant->addItem(variant,variant);
  ui.cmbVariant->setEnabled(ui.cmbVariant->count() > 0);

  FStyleOptions.extended.insert(MSO_STYLE_ID,styleId);

  QMap<QString, QVariant> info = FStylePlugin->styleInfo(styleId);
  if (info.contains(MSIV_DEFAULT_VARIANT))
  {
    int index = ui.cmbVariant->findData(info.value(MSIV_DEFAULT_VARIANT));
    ui.cmbVariant->setCurrentIndex(index>=0 ? index : 0);
  }

  if (info.value(MSIV_DISABLE_CUSTOM_BACKGROUND,false).toBool())
  {
    ui.tlbSetImage->setEnabled(false);
    ui.tlbDefaultImage->setEnabled(false);
    ui.cmbImageLayout->setEnabled(false);
    ui.cmbBackgoundColor->setEnabled(false);
    onDefaultImageClicked();
  }
  else
  {
    ui.tlbSetImage->setEnabled(true);
    ui.tlbDefaultImage->setEnabled(true);
    ui.cmbImageLayout->setEnabled(true);
    ui.cmbBackgoundColor->setEnabled(true);
    ui.cmbImageLayout->setCurrentIndex(ui.cmbImageLayout->findData(FStyleOptions.extended.value(MSO_BG_IMAGE_LAYOUT).toInt()));
    emit modified();
  }
}

void AdiumOptionsWidget::onVariantChanged(int AIndex)
{
  FStyleOptions.extended.insert(MSO_VARIANT,ui.cmbVariant->itemData(AIndex));
  emit modified();
}

void AdiumOptionsWidget::onSetFontClicked()
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

void AdiumOptionsWidget::onDefaultFontClicked()
{
  QMap<QString,QVariant> info = FStylePlugin->styleInfo(ui.cmbStyle->itemData(ui.cmbStyle->currentIndex()).toString());
  FStyleOptions.extended.insert(MSO_FONT_FAMILY,info.value(MSIV_DEFAULT_FONT_FAMILY));
  FStyleOptions.extended.insert(MSO_FONT_SIZE,info.value(MSIV_DEFAULT_FONT_SIZE));
  updateOptionsWidgets();
  emit modified();
}

void AdiumOptionsWidget::onImageLayoutChanged(int AIndex)
{
  FStyleOptions.extended.insert(MSO_BG_IMAGE_LAYOUT,ui.cmbImageLayout->itemData(AIndex));
  emit modified();
}

void AdiumOptionsWidget::onBackgroundColorChanged(int AIndex)
{
  FStyleOptions.extended.insert(MSO_BG_COLOR,ui.cmbBackgoundColor->itemData(AIndex));
  emit modified();
}

void AdiumOptionsWidget::onSetImageClicked()
{
  QString fileName = QFileDialog::getOpenFileName(this, tr("Select background image"),"",tr("Image Files (*.png *.jpg *.bmp *.gif)"));
  if (!fileName.isEmpty())
  {
    FStyleOptions.extended.insert(MSO_BG_IMAGE_FILE,fileName);
    updateOptionsWidgets();
    emit modified();
  }
}

void AdiumOptionsWidget::onDefaultImageClicked()
{
  FStyleOptions.extended.remove(MSO_BG_IMAGE_FILE);
  FStyleOptions.extended.remove(MSO_BG_IMAGE_LAYOUT);
  ui.cmbImageLayout->setCurrentIndex(ui.cmbImageLayout->findData(AdiumMessageStyle::ImageNormal));

  QMap<QString,QVariant> info = FStylePlugin->styleInfo(ui.cmbStyle->itemData(ui.cmbStyle->currentIndex()).toString());
  FStyleOptions.extended.insert(MSO_BG_COLOR,info.value(MSIV_DEFAULT_BACKGROUND_COLOR));
  ui.cmbBackgoundColor->setCurrentIndex(ui.cmbBackgoundColor->findData(FStyleOptions.extended.value(MSO_BG_COLOR)));

  updateOptionsWidgets();
  emit modified();
}
