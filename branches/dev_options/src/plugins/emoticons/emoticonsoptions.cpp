#include "emoticonsoptions.h"

EmoticonsOptions::EmoticonsOptions(IEmoticons *AEmoticons, QWidget *AParent) : QWidget(AParent)
{
  ui.setupUi(this);
  IconStorage *storage = IconStorage::staticStorage(RSR_STORAGE_MENUICONS);
  storage->insertAutoIcon(ui.tbtUp,MNI_EMOTICONS_ARROW_UP);
  storage->insertAutoIcon(ui.tbtDown,MNI_EMOTICONS_ARROW_DOWN);

  FEmoticons = AEmoticons;
  ui.lwtEmoticons->setItemDelegate(new IconsetDelegate(ui.lwtEmoticons));
  connect(ui.tbtUp,SIGNAL(clicked()),SLOT(onUpButtonClicked()));
  connect(ui.tbtDown,SIGNAL(clicked()),SLOT(onDownButtonClicked()));
  init();
}

EmoticonsOptions::~EmoticonsOptions()
{

}

void EmoticonsOptions::apply()
{
  QStringList newFiles;
  for (int i = 0; i<ui.lwtEmoticons->count(); i++)
    if (ui.lwtEmoticons->item(i)->checkState() == Qt::Checked)
      newFiles.append(ui.lwtEmoticons->item(i)->data(IDR_STORAGE_SUBDIR).toString());

  if (newFiles != FEmoticons->iconsets())
    FEmoticons->setIconsets(newFiles);

  emit optionsAccepted();
}

void EmoticonsOptions::init()
{
  ui.lwtEmoticons->clear();
  QStringList storages = FEmoticons->iconsets();
  for (int i = 0; i < storages.count(); i++)
  {
    QListWidgetItem *item = new QListWidgetItem(RSR_STORAGE_EMOTICONS"/"+storages.at(i),ui.lwtEmoticons);
    item->setData(IDR_STORAGE_NAME,RSR_STORAGE_EMOTICONS);
    item->setData(IDR_STORAGE_SUBDIR,storages.at(i));
    item->setData(IDR_ICON_ROWS,2);
    item->setFlags(Qt::ItemIsUserCheckable|Qt::ItemIsSelectable|Qt::ItemIsEnabled);
    item->setCheckState(Qt::Checked);
  }

  QList<QString> allstorages = FileStorage::availSubStorages(RSR_STORAGE_EMOTICONS);
  for (int i = 0; i < allstorages.count(); i++)
  {
    if (!storages.contains(allstorages.at(i)))
    {
      QListWidgetItem *item = new QListWidgetItem(allstorages.at(i),ui.lwtEmoticons);
      item->setData(IDR_STORAGE_NAME,RSR_STORAGE_EMOTICONS);
      item->setData(IDR_STORAGE_SUBDIR,allstorages.at(i));
      item->setData(IDR_ICON_ROWS,2);
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
