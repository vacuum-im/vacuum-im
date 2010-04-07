#ifndef OPTIONSCONTAINER_H
#define OPTIONSCONTAINER_H

#include <interfaces/ioptionsmanager.h>

class OptionsContainer : 
  public QWidget,
  public IOptionsContainer
{
  Q_OBJECT;
  Q_INTERFACES(IOptionsContainer);
public:
  OptionsContainer(QWidget *AParent);
  ~OptionsContainer();
  virtual QWidget* instance() { return this; }
  virtual void registerChild(IOptionsWidget *AWidget);
public slots:
  virtual void apply();
  virtual void reset();
signals:
  void modified();
  void childApply();
  void childReset();
};

#endif // OPTIONSCONTAINER_H
