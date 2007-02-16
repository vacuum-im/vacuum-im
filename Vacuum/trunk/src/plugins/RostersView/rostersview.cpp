#include "rostersview.h"
#include <QHeaderView>
#include "rosterindexdelegate.h"

RostersView::RostersView(IRostersModel *AModel, QWidget *AParent)
  : QTreeView(AParent)
{
  FRostersModel = AModel;
  setModel(AModel);
  setItemDelegate(new RosterIndexDelegate(this));
  header()->hide();
  setRootIsDecorated(false);
}

RostersView::~RostersView()
{

}
