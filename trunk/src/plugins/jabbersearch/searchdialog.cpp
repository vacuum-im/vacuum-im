#include "searchdialog.h"

#include <QHeaderView>
#include <QMessageBox>

#define CJID                    0
#define CFIRST                  1
#define CLAST                   2
#define CNICK                   3
#define CEMAIL                  4

#define IN_SEARCH               "psi/search"
#define IN_ADDCONTACT           "psi/addContact"
#define IN_VCARD                "psi/vCard"
#define IN_INFO                 "psi/statusmsg"

SearchDialog::SearchDialog(IJabberSearch *ASearch, IPluginManager *APluginManager, const Jid &AStreamJid, 
                           const Jid &AServiceJid, QWidget *AParent) : QDialog(AParent)
{
  ui.setupUi(this);
  setAttribute(Qt::WA_DeleteOnClose,true);
  setWindowIcon(Skin::getSkinIconset(SYSTEM_ICONSETFILE)->iconByName(IN_SEARCH));

  FPluginManager = APluginManager;
  FSearch = ASearch;
  FStreamJid = AStreamJid;
  FServiceJid = AServiceJid;

  FDataForms = NULL;
  FDiscovery = NULL;
  FCurrentForm = NULL;
  FRosterChanger = NULL;
  FVCardPlugin = NULL;

  QToolBar *toolBar = new QToolBar(this);
  FToolBarChanger = new ToolBarChanger(toolBar);
  FToolBarChanger->setSeparatorsVisible(false);
  layout()->setMenuBar(toolBar);

  ui.pgeForm->setLayout(new QVBoxLayout);
  ui.pgeForm->layout()->setMargin(0);

  connect(FSearch->instance(),SIGNAL(searchFields(const QString &, const ISearchFields &)),
    SLOT(onSearchFields(const QString &, const ISearchFields &)));
  connect(FSearch->instance(),SIGNAL(searchResult(const QString &, const ISearchResult &)),
    SLOT(onSearchResult(const QString &, const ISearchResult &)));
  connect(FSearch->instance(),SIGNAL(searchError(const QString &, const QString &)),
    SLOT(onSearchError(const QString &, const QString &)));
  connect(ui.dbbButtons,SIGNAL(clicked(QAbstractButton *)),SLOT(onDialogButtonClicked(QAbstractButton *)));

  initialize();
  createToolBarActions();

  requestFields();
}

SearchDialog::~SearchDialog()
{

}

ISearchItem SearchDialog::currentItem() const
{
  ISearchItem item;
  if (FCurrentForm && FCurrentForm->tableWidget())
  {
    int row = FCurrentForm->tableWidget()->instance()->currentRow();
    if (row >= 0)
    {
      item.itemJid = FCurrentForm->tableWidget()->dataField(row,"jid").value.toString();
      item.firstName = FCurrentForm->tableWidget()->dataField(row,"first").value.toString();
      item.lastName = FCurrentForm->tableWidget()->dataField(row,"last").value.toString();
      item.nick = FCurrentForm->tableWidget()->dataField(row,"nick").value.toString();
      item.email = FCurrentForm->tableWidget()->dataField(row,"email").value.toString();
    }
  }
  else if (ui.tbwResult->currentRow() >= 0)
  {
    int row = ui.tbwResult->currentRow();
    item.itemJid = ui.tbwResult->item(row,CJID)->text();
    item.firstName = ui.tbwResult->item(row,CFIRST)->text();
    item.lastName = ui.tbwResult->item(row,CLAST)->text();
    item.nick = ui.tbwResult->item(row,CNICK)->text();
    item.email = ui.tbwResult->item(row,CEMAIL)->text();
  }
  return item;
}

void SearchDialog::resetDialog()
{
  setWindowTitle(tr("Search in %1").arg(FServiceJid.full()));
  FToolBarChanger->toolBar()->hide();
  if (FCurrentForm)
  {
    ui.pgeForm->layout()->removeWidget(FCurrentForm->instance());
    FCurrentForm->instance()->deleteLater();
    FCurrentForm = NULL;
  }
  ui.tbwResult->clear();
  ui.lblInstructions->setText("");
  ui.lblFirst->setVisible(false);
  ui.lneFirst->setVisible(false);
  ui.lblLast->setVisible(false);
  ui.lneLast->setVisible(false);
  ui.lblNick->setVisible(false);
  ui.lneNick->setVisible(false);
  ui.lblEmail->setVisible(false);
  ui.lneEmail->setVisible(false);
  ui.stwWidgets->setCurrentWidget(ui.pgeFields);
}

void SearchDialog::requestFields()
{
  FRequestId = FSearch->sendRequest(FStreamJid,FServiceJid);

  resetDialog();
  if (!FRequestId.isEmpty())
  {
    ui.lblInstructions->setText(tr("Waiting for host response ..."));
    ui.dbbButtons->setStandardButtons(QDialogButtonBox::Cancel);
  }
  else
  {
    ui.lblInstructions->setText(tr("Error: Can`t send request to host."));
    ui.dbbButtons->setStandardButtons(QDialogButtonBox::Retry|QDialogButtonBox::Cancel);
  }
}

void SearchDialog::requestResult()
{
  if (!FCurrentForm || FCurrentForm->checkForm(true))
  {
    ISearchSubmit submit;
    submit.serviceJid = FServiceJid;
    if (!FCurrentForm)
    {
      submit.item.firstName = ui.lneFirst->text();
      submit.item.lastName = ui.lneLast->text();
      submit.item.nick = ui.lneNick->text();
      submit.item.email = ui.lneEmail->text();
    }
    else
      submit.form = FDataForms->dataSubmit(FCurrentForm->userDataForm());

    FRequestId = FSearch->sendSubmit(FStreamJid,submit);
    
    resetDialog();
    if (!FRequestId.isEmpty())
    {
      ui.lblInstructions->setText(tr("Waiting for host response ..."));
      ui.dbbButtons->setStandardButtons(QDialogButtonBox::Cancel);
    }
    else
    {
      ui.lblInstructions->setText(tr("Error: Can`t send request to host."));
      ui.dbbButtons->setStandardButtons(QDialogButtonBox::Retry|QDialogButtonBox::Close);
    }
  }
}

bool SearchDialog::setDataForm(const IDataForm &AForm)
{
  if (FDataForms && !AForm.type.isEmpty())
  {
    FCurrentForm = FDataForms->formWidget(AForm,ui.pgeForm);
    ui.pgeForm->layout()->addWidget(FCurrentForm->instance());
    if (!AForm.title.isEmpty())
      setWindowTitle(AForm.title);
    if (FCurrentForm->tableWidget())
      FCurrentForm->tableWidget()->instance()->setSortingEnabled(true);
    ui.stwWidgets->setCurrentWidget(ui.pgeForm);
    return true;
  }
  return false;
}

void SearchDialog::initialize()
{
  IPlugin *plugin = FPluginManager->getPlugins("IDataForms").value(0,NULL);
  if (plugin)
    FDataForms = qobject_cast<IDataForms *>(plugin->instance());

  plugin = FPluginManager->getPlugins("IServiceDiscovery").value(0,NULL);
  if (plugin)
    FDiscovery = qobject_cast<IServiceDiscovery *>(plugin->instance());

  plugin = FPluginManager->getPlugins("IRosterChanger").value(0,NULL);
  if (plugin)
    FRosterChanger = qobject_cast<IRosterChanger *>(plugin->instance());

  plugin = FPluginManager->getPlugins("IVCardPlugin").value(0,NULL);
  if (plugin)
    FVCardPlugin = qobject_cast<IVCardPlugin *>(plugin->instance());
}

void SearchDialog::createToolBarActions()
{
  FDiscoInfo = new Action(FToolBarChanger);
  FDiscoInfo->setText(tr("Disco info"));
  FDiscoInfo->setIcon(SYSTEM_ICONSETFILE,IN_INFO);
  FToolBarChanger->addAction(FDiscoInfo,AG_DEFAULT,false);
  connect(FDiscoInfo,SIGNAL(triggered(bool)),SLOT(onToolBarActionTriggered(bool)));

  FAddContact = new Action(FToolBarChanger);
  FAddContact->setText(tr("Add Contact"));
  FAddContact->setIcon(SYSTEM_ICONSETFILE,IN_ADDCONTACT);
  FToolBarChanger->addAction(FAddContact,AG_DEFAULT,false);
  connect(FAddContact,SIGNAL(triggered(bool)),SLOT(onToolBarActionTriggered(bool)));

  FShowVCard = new Action(FToolBarChanger);
  FShowVCard->setText(tr("vCard"));
  FShowVCard->setIcon(SYSTEM_ICONSETFILE,IN_VCARD);
  FToolBarChanger->addAction(FShowVCard,AG_DEFAULT,false);
  connect(FShowVCard,SIGNAL(triggered(bool)),SLOT(onToolBarActionTriggered(bool)));
}

void SearchDialog::onSearchFields(const QString &AId, const ISearchFields &AFields)
{
  if (FRequestId == AId)
  {
    resetDialog();
    if (!setDataForm(AFields.form))
    {
      ui.lblInstructions->setText(AFields.instructions);
      ui.lneFirst->setText(AFields.item.firstName);
      ui.lblFirst->setVisible((AFields.fieldMask & ISearchFields::First) > 0);
      ui.lneFirst->setVisible((AFields.fieldMask & ISearchFields::First) > 0);

      ui.lneLast->setText(AFields.item.lastName);
      ui.lblLast->setVisible((AFields.fieldMask & ISearchFields::Last) > 0);
      ui.lneLast->setVisible((AFields.fieldMask & ISearchFields::Last) > 0);

      ui.lneNick->setText(AFields.item.nick);
      ui.lblNick->setVisible((AFields.fieldMask & ISearchFields::Nick) > 0);
      ui.lneNick->setVisible((AFields.fieldMask & ISearchFields::Nick) > 0);

      ui.lneEmail->setText(AFields.item.email);
      ui.lblEmail->setVisible((AFields.fieldMask & ISearchFields::Email) > 0);
      ui.lneEmail->setVisible((AFields.fieldMask & ISearchFields::Email) > 0);
      
      ui.stwWidgets->setCurrentWidget(ui.pgeFields);
    }
    ui.dbbButtons->setStandardButtons(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
  }
}

void SearchDialog::onSearchResult(const QString &AId, const ISearchResult &AResult)
{
  if (FRequestId == AId)
  {
    resetDialog();
    if (!setDataForm(AResult.form))
    {
      int row = 0;
      ui.tbwResult->setRowCount(AResult.items.count());
      foreach(ISearchItem item, AResult.items)
      {
        QTableWidgetItem *itemJid = new QTableWidgetItem(item.itemJid.full());
        itemJid->setFlags(Qt::ItemIsEnabled|Qt::ItemIsSelectable);
        QTableWidgetItem *itemFirst = new QTableWidgetItem(item.firstName);
        itemFirst->setFlags(Qt::ItemIsEnabled|Qt::ItemIsSelectable);
        QTableWidgetItem *itemLast = new QTableWidgetItem(item.lastName);
        itemLast->setFlags(Qt::ItemIsEnabled|Qt::ItemIsSelectable);
        QTableWidgetItem *itemNick = new QTableWidgetItem(item.nick);
        itemNick->setFlags(Qt::ItemIsEnabled|Qt::ItemIsSelectable);
        QTableWidgetItem *itemEmail = new QTableWidgetItem(item.email);
        itemEmail->setFlags(Qt::ItemIsEnabled|Qt::ItemIsSelectable);
        ui.tbwResult->setItem(row,CJID,itemJid);
        ui.tbwResult->setItem(row,CFIRST,itemFirst);
        ui.tbwResult->setItem(row,CLAST,itemLast);
        ui.tbwResult->setItem(row,CNICK,itemNick);
        ui.tbwResult->setItem(row,CEMAIL,itemEmail);
        row++;
      }
      ui.tbwResult->horizontalHeader()->resizeSections(QHeaderView::ResizeToContents);
      ui.tbwResult->verticalHeader()->resizeSections(QHeaderView::ResizeToContents);
      ui.stwWidgets->setCurrentWidget(ui.pgeResult);
    }
    ui.dbbButtons->setStandardButtons(QDialogButtonBox::Retry|QDialogButtonBox::Close);
    FToolBarChanger->toolBar()->show();
  }
}

void SearchDialog::onSearchError(const QString &AId, const QString &AError)
{
  if (FRequestId == AId)
  {
    resetDialog();
    ui.lblInstructions->setText(tr("Requested operation failed: %1").arg(Qt::escape(AError)));
    ui.dbbButtons->setStandardButtons(QDialogButtonBox::Retry|QDialogButtonBox::Close);
  }
}

void SearchDialog::onToolBarActionTriggered(bool)
{
  ISearchItem item = currentItem();
  if (item.itemJid.isValid())
  {
    Action *action = qobject_cast<Action *>(sender());
    if (action == FDiscoInfo)
    {
      FDiscovery->showDiscoInfo(FStreamJid,item.itemJid,"",this);
    }
    else if (action == FAddContact)
    {
      IAddContactDialog *dialog = FRosterChanger!=NULL ? FRosterChanger->showAddContactDialog(FStreamJid) : NULL;
      if (dialog)
      {
        dialog->setContactJid(item.itemJid);
        dialog->setNickName(item.nick);
      }
    }
    else if (action == FShowVCard)
    {
      FVCardPlugin->showVCardDialog(FStreamJid,item.itemJid);
    }
  }
}

void SearchDialog::onDialogButtonClicked(QAbstractButton *AButton)
{
  if (ui.dbbButtons->standardButton(AButton) == QDialogButtonBox::Ok)
    requestResult();
  else if (ui.dbbButtons->standardButton(AButton) == QDialogButtonBox::Retry)
    requestFields();
  else if (ui.dbbButtons->standardButton(AButton) == QDialogButtonBox::Cancel)
    close();
  else if (ui.dbbButtons->standardButton(AButton) == QDialogButtonBox::Close)
    close();
}

