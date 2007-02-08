#ifndef IROSTERSVIEW_H
#define IROSTERSVIEW_H

#include <QTreeView>
#include "../../interfaces/irostersmodel.h"


class IRostersView :
  virtual public QTreeView
{
public:
  virtual QObject *instance() = 0;
};


class IRostersViewPlugin
{
public:
  virtual QObject *instance() = 0;
};

Q_DECLARE_INTERFACE(IRostersView,"Vacuum.Plugin.IRostersView/1.0");
Q_DECLARE_INTERFACE(IRostersViewPlugin,"Vacuum.Plugin.IRostersViewPlugin/1.0");

#endif