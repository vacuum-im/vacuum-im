#include "emoticonsoptions.h"

#define IN_ARROW_UP           "psi/arrowUp"
#define IN_ARROR_DOWN         "psi/arrowDown"

EmoticonsOptions::EmoticonsOptions(IEmoticons *AEmoticons, QWidget *AParent) : QWidget(AParent)
{
  ui.setupUi(this);
  SkinIconset *system = Skin::getSkinIconset(SYSTEM_ICONSETFILE);
  ui.tbtUp->setIcon(system->iconByName(IN_ARROW_UP));
  ui.tbtDown->setIcon(system->iconByName(IN_ARROR_DOWN));

  FEmoticons = AEmoticons;
  ui.lwtEmoticons->setItemDelegate(new IconsetDelegate(ui.lwtEmoticons));
  connect(ui.tbtUp,SIGNAL(clicked()),SLOT(onUpButtonClicked()));
  connect(ui.tbtDown,SIGNAL(clicked()),SLOT(onDownButtonClicked()));
  connect(FEmoticons->instance(),SIGNAL(iconsetsChangedBySkin()),SLOT(onIconsetsChangedBySkin()));
  init();
}

EmoticonsOptions::~EmoticonsOptions()
{

}

void EmoticonsOptions::apply() const
{
  QStringList newFiles;
  for (int i = 0; i<ui.lwtEmoticons->count(); i++)
    if (ui.lwtEmoticons->item(i)->checkState() == Qt::Checked)
      newFiles.append(ui.lwtEmoticons->item(i)->data(Qt::DisplayRole).toString());

  if (newFiles != FEmoticons->iconsets())
    FEmoticons->setIconsets(newFiles);
}

void EmoticonsOptions::init()
{
  ui.lwtEmoticons->clear();
  QStringList files = FEmoticons->iconsets();
  for (int i = 0; i < files.count(); i++)
  {
    QListWidgetItem *item = new QListWidgetItem(files.at(i),ui.lwtEmoticons);
    item->setData(IDR_MAX_ICON_ROWS,2);
    item->setFlags(Qt::ItemIsUserCheckable|Qt::ItemIsSelectable|Qt::ItemIsEnabled);
    item->setCheckState(Qt::Checked);
  }

  QStringList allfiles = Skin::skinFilesWithDef(SKIN_TYPE_ICONSET,"emoticons","*.jisp");
  for (int i = 0; i < allfiles.count(); i++)
  {
    allfiles[i].prepend("emoticons/");
    if (!files.contains(allfiles.at(i)))
    {
      QListWidgetItem *item = new QListWidgetItem(allfiles.at(i),ui.lwtEmoticons);
      item->setData(IDR_MAX_ICON_ROWS,2);
      item->setFlags(Qt::ItemIsUserCheckable|Qt::ItemIsSelectable|Qt::ItemIsEnabled);
      item->setCheckState(Qt::Unchecked);
    }
  }
}

void EmoticonsOptions::onUpButtonClicked()
{
  if (ui.lwtEmoticons->currentRow() > 0)
  {
    ui.lwtEmoticons->insertItem(ui.lwtEmoticons->currentRow()-1, ui.lwtEmoticons->takeItem(ui.lwtEmoticons->currentRow()));
    ui.lwtEmoticons->setCurrentRow(ui.lwtEmoticons->currentRow()-1);
  }
}

void EmoticonsOptions::onDownButtonClicked()
{
  if (ui.lwtEmoticons->currentRow() < ui.lwtEmoticons->count()-1)
  {
    ui.lwtEmoticons->insertItem(ui.lwtEmoticons->currentRow()+1, ui.lwtEmoticons->takeItem(ui.lwtEmoticons->currentRow()));
    ui.lwtEmoticons->setCurrentRow(ui.lwtEmoticons->currentRow()+1);
  }
}

void EmoticonsOptions::onIconsetsChangedBySkin()
{
  init();
}
