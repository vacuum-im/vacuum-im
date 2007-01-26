#include "rostersview.h"
#include <QHeaderView>

RostersView::RostersView(IRostersModel *AModel, QWidget *AParent)
  : QTreeView(AParent)
{
  FRostersModel = AModel;
  setModel(AModel);
  header()->hide();
}

RostersView::~RostersView()
{

}
