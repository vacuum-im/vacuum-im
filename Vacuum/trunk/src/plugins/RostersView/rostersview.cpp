#include "rostersview.h"
#include "rosterindexdelegate.h"

RostersView::RostersView(IRostersModel *AModel, QWidget *AParent)
  : QTreeView(AParent)
{
  FRostersModel = AModel;
  setModel(AModel);
  setItemDelegate(new RosterIndexDelegate(this));
  header()->hide();
  setIndentation(3);
  setRootIsDecorated(false);
  setSelectionMode(NoSelection);
}

RostersView::~RostersView()
{

}
