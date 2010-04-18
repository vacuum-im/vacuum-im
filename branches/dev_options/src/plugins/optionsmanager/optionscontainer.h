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
  OptionsContainer(const IOptionsManager *AOptionsManager, QWidget *AParent);
  ~OptionsContainer();
  virtual QWidget* instance() { return this; }
  virtual void registerChild(IOptionsWidget *AWidget);
  virtual IOptionsWidget *appendChild(const OptionsNode &ANode, const QString &ACaption);
public slots:
  virtual void apply();
  virtual void reset();
signals:
  void modified();
  void childApply();
  void childReset();
private:
  const IOptionsManager *FOptionsManager;
};

#endif // OPTIONSCONTAINER_H
