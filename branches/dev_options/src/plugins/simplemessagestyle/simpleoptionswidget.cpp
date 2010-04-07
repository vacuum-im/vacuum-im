#include "simpleoptionswidget.h"

#include <QTimer>
#include <QColor>
#include <QPixmap>
#include <QFileDialog>
#include <QFontDialog>

SimpleOptionsWidget::SimpleOptionsWidget(SimpleMessageStylePlugin *APlugin, int AMessageType, const QString &AContext, QWidget *AParent) : QWidget(AParent)
{
  ui.setupUi(this);

  FModifyEnabled = false;
  FTimerStarted = false;
  FStylePlugin = APlugin;

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

  connect(ui.cmbStyle,SIGNAL(currentIndexChanged(int)),SLOT(onStyleChanged(int)));
  connect(ui.cmbVariant,SIGNAL(currentIndexChanged(int)),SLOT(onVariantChanged(int)));
  connect(ui.cmbBackgoundColor,SIGNAL(currentIndexChanged(int)),SLOT(onBackgroundColorChanged(int)));
  connect(ui.tlbSetFont,SIGNAL(clicked()),SLOT(onSetFontClicked()));
  connect(ui.tlbDefaultFont,SIGNAL(clicked()),SLOT(onDefaultFontClicked()));
  connect(ui.tlbSetImage,SIGNAL(clicked()),SLOT(onSetImageClicked()));
  connect(ui.tlbDefaultImage,SIGNAL(clicked()),SLOT(onDefaultImageClicked()));
  connect(this,SIGNAL(settingsChanged()),SLOT(onSettingsChanged()));

  loadSettings(AMessageType,AContext);
}

SimpleOptionsWidget::~SimpleOptionsWidget()
{

}

int SimpleOptionsWidget::messageType() const
{
  return FActiveType;
}

QString SimpleOptionsWidget::context() const
{
  return FActiveContext;
}

bool SimpleOptionsWidget::isModified( int AMessageType, const QString &AContext ) const
{
  return FModified.value(AMessageType).value(AContext,false);
}

void SimpleOptionsWidget::setModified(bool AModified, int AMessageType, const QString &AContext)
{
  FModified[AMessageType][AContext] = AModified;
}

IMessageStyleOptions SimpleOptionsWidget::styleOptions(int AMessageType, const QString &AContext) const
{
  if (FOptions.value(AMessageType).contains(AContext))
    return FOptions.value(AMessageType).value(AContext);
  return FStylePlugin->styleOptions(AMessageType,AContext);
}

void SimpleOptionsWidget::loadSettings(int AMessageType, const QString &AContext)
{
  FActiveType = AMessageType;
  FActiveContext = AContext;

  IMessageStyleOptions &soptions = FOptions[FActiveType][FActiveContext];
  if (soptions.pluginId.isEmpty())
    soptions = FStylePlugin->styleOptions(FActiveType,FActiveContext);

  FModifyEnabled = isModified(AMessageType,AContext);
  disconnect(ui.cmbVariant,SIGNAL(currentIndexChanged(int)),this,SLOT(onVariantChanged(int)));

  ui.cmbStyle->setCurrentIndex(ui.cmbStyle->findData(soptions.extended.value(MSO_STYLE_ID)));
  ui.cmbVariant->setCurrentIndex(ui.cmbVariant->findData(soptions.extended.value(MSO_VARIANT).toString()));
  ui.cmbBackgoundColor->setCurrentIndex(ui.cmbBackgoundColor->findData(soptions.extended.value(MSO_BG_COLOR)));

  connect(ui.cmbVariant,SIGNAL(currentIndexChanged(int)),SLOT(onVariantChanged(int)));
  FModifyEnabled = true;

  updateOptionsWidgets();
  startSignalTimer();
}

void SimpleOptionsWidget::startSignalTimer()
{
  if (!FTimerStarted)
  {
    QTimer::singleShot(0,this,SIGNAL(settingsChanged()));
    FTimerStarted = true;
  }
}

void SimpleOptionsWidget::updateOptionsWidgets()
{
  const IMessageStyleOptions &soptions = FOptions[FActiveType][FActiveContext];

  QString family = soptions.extended.value(MSO_FONT_FAMILY).toString();
  int size = soptions.extended.value(MSO_FONT_SIZE).toInt();
  if (family.isEmpty())
    family = QFont().family();
  if (size==0)
    size = QFont().pointSize();
  ui.lblFont->setText(family + " " +QString::number(size));
}

void SimpleOptionsWidget::onStyleChanged(int AIndex)
{
  QString styleId = ui.cmbStyle->itemData(AIndex).toString();

  ui.cmbVariant->clear();
  foreach(QString variant, FStylePlugin->styleVariants(styleId))
    ui.cmbVariant->addItem(variant,variant);
  ui.cmbVariant->setEnabled(ui.cmbVariant->count() > 0);

  IMessageStyleOptions &soptions = FOptions[FActiveType][FActiveContext];
  soptions.extended.insert(MSO_STYLE_ID,styleId);
  FModified[FActiveType][FActiveContext] = FModifyEnabled;

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
    ui.cmbBackgoundColor->setEnabled(false);
    onDefaultImageClicked();
  }
  else
  {
    ui.tlbSetImage->setEnabled(true);
    ui.tlbDefaultImage->setEnabled(true);
    ui.cmbBackgoundColor->setEnabled(true);
    startSignalTimer();
  }
}

void SimpleOptionsWidget::onVariantChanged(int AIndex)
{
  IMessageStyleOptions &soptions = FOptions[FActiveType][FActiveContext];
  soptions.extended.insert(MSO_VARIANT,ui.cmbVariant->itemData(AIndex));
  FModified[FActiveType][FActiveContext] = FModifyEnabled;
  startSignalTimer();
}

void SimpleOptionsWidget::onSetFontClicked()
{
  bool ok;
  IMessageStyleOptions &soptions = FOptions[FActiveType][FActiveContext];

  QFont font;
  if (!soptions.extended.value(MSO_FONT_FAMILY).toString().isEmpty())
    font.setFamily(soptions.extended.value(MSO_FONT_FAMILY).toString());
  if (soptions.extended.value(MSO_FONT_SIZE).toInt()>0)
    font.setPointSize(soptions.extended.value(MSO_FONT_SIZE).toInt());

  font = QFontDialog::getFont(&ok,font,this,tr("Select font family and size"));
  if (ok)
  {
    soptions.extended.insert(MSO_FONT_FAMILY,font.family());
    soptions.extended.insert(MSO_FONT_SIZE,font.pointSize());
    FModified[FActiveType][FActiveContext] = FModifyEnabled;
    startSignalTimer();
  }
}

void SimpleOptionsWidget::onDefaultFontClicked()
{
  IMessageStyleOptions &soptions = FOptions[FActiveType][FActiveContext];
  QMap<QString,QVariant> info = FStylePlugin->styleInfo(ui.cmbStyle->itemData(ui.cmbStyle->currentIndex()).toString());
  soptions.extended.insert(MSO_FONT_FAMILY,info.value(MSIV_DEFAULT_FONT_FAMILY));
  soptions.extended.insert(MSO_FONT_SIZE,info.value(MSIV_DEFAULT_FONT_SIZE));
  FModified[FActiveType][FActiveContext] = FModifyEnabled;
  startSignalTimer();
}

void SimpleOptionsWidget::onBackgroundColorChanged(int AIndex)
{
  IMessageStyleOptions &soptions = FOptions[FActiveType][FActiveContext];
  soptions.extended.insert(MSO_BG_COLOR,ui.cmbBackgoundColor->itemData(AIndex));
  FModified[FActiveType][FActiveContext] = FModifyEnabled;
  startSignalTimer();
}

void SimpleOptionsWidget::onSetImageClicked()
{
  QString fileName = QFileDialog::getOpenFileName(this, tr("Select background image"),"",tr("Image Files (*.png *.jpg *.bmp *.gif)"));
  if (!fileName.isEmpty())
  {
    IMessageStyleOptions &soptions = FOptions[FActiveType][FActiveContext];
    soptions.extended.insert(MSO_BG_IMAGE_FILE,fileName);
    FModified[FActiveType][FActiveContext] = FModifyEnabled;
    startSignalTimer();
  }
}

void SimpleOptionsWidget::onDefaultImageClicked()
{
  IMessageStyleOptions &soptions = FOptions[FActiveType][FActiveContext];
  soptions.extended.remove(MSO_BG_IMAGE_FILE);

  QMap<QString,QVariant> info = FStylePlugin->styleInfo(ui.cmbStyle->itemData(ui.cmbStyle->currentIndex()).toString());
  soptions.extended.insert(MSO_BG_COLOR,info.value(MSIV_DEFAULT_BACKGROUND_COLOR));
  ui.cmbBackgoundColor->setCurrentIndex(ui.cmbBackgoundColor->findData(soptions.extended.value(MSO_BG_COLOR)));

  FModified[FActiveType][FActiveContext] = FModifyEnabled;

  startSignalTimer();
}

void SimpleOptionsWidget::onSettingsChanged()
{
  updateOptionsWidgets();
  FTimerStarted = false;
}
