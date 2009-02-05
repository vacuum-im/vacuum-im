#ifndef SEARCHDIALOG_H
#define SEARCHDIALOG_H

#include <QDialog>
#include "../../definations/namespaces.h"
#include "../../definations/actiongroups.h"
#include "../../definations/resources.h"
#include "../../definations/menuicons.h"
#include "../../interfaces/ipluginmanager.h"
#include "../../interfaces/ijabbersearch.h"
#include "../../interfaces/idataforms.h"
#include "../../interfaces/iservicediscovery.h"
#include "../../interfaces/irosterchanger.h"
#include "../../interfaces/ivcard.h"
#include "../../utils/toolbarchanger.h"
#include "ui_searchdialog.h"

class SearchDialog : 
  public QDialog
{
  Q_OBJECT;
public:
  SearchDialog(IJabberSearch *ASearch, IPluginManager *APluginManager, const Jid &AStreamJid, const Jid &AServiceJid, QWidget *AParent = NULL);
  ~SearchDialog();
public:
  virtual Jid streamJid() const { return FStreamJid; }
  virtual Jid serviceJid() const { return FServiceJid; }
  virtual ISearchItem currentItem() const;
protected:
  void resetDialog();
  void requestFields();
  void requestResult();
  bool setDataForm(const IDataForm &AForm);
  void initialize();
  void createToolBarActions();
protected slots:
  void onSearchFields(const QString &AId, const ISearchFields &AFields);
  void onSearchResult(const QString &AId, const ISearchResult &AResult);
  void onSearchError(const QString &AId, const QString &AError);
  void onToolBarActionTriggered(bool);
  void onDialogButtonClicked(QAbstractButton *AButton);
private:
  Ui::SearchDialogClass ui;
private:
  IPluginManager *FPluginManager;
  IJabberSearch *FSearch;
  IDataForms *FDataForms;
  IServiceDiscovery *FDiscovery;
  IVCardPlugin *FVCardPlugin;
  IRosterChanger *FRosterChanger;
private:
  Action *FDiscoInfo;
  Action *FAddContact;
  Action *FShowVCard;
  ToolBarChanger *FToolBarChanger;
private:
  Jid FStreamJid;
  Jid FServiceJid;
  QString FFieldsRequestId;
  QString FRequestId;
  IDataFormWidget *FCurrentForm;
};

#endif // SEARCHDIALOG_H
