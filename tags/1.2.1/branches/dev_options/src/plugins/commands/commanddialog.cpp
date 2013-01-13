#include "commanddialog.h"

#include <QMessageBox>
#include <QTextDocument>

CommandDialog::CommandDialog(ICommands *ACommands, IDataForms *ADataForms, const Jid &AStreamJid, const Jid ACommandJid, 
                             const QString &ANode, QWidget *AParent)  : QDialog(AParent)
{
  ui.setupUi(this);
  setAttribute(Qt::WA_DeleteOnClose,true);
  IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->insertAutoIcon(this,MNI_COMMANDS,0,0,"windowIcon");

  ui.wdtForm->setLayout(new QVBoxLayout);
  ui.wdtForm->layout()->setMargin(0);

  FCommands = ACommands;
  FDataForms = ADataForms;

  FStreamJid = AStreamJid;
  FCommandJid = ACommandJid;
  FNode = ANode;
  FCurrentForm = NULL;

  FPrevButton = new QPushButton(tr("<Back"));
  FNextButton = new QPushButton(tr("Next>"));
  FCompleteButton = new QPushButton(tr("Complete"));

  connect(ui.dbbButtons,SIGNAL(clicked(QAbstractButton *)),SLOT(onDialogButtonClicked(QAbstractButton *)));

  FCommands->insertClient(this);
}

CommandDialog::~CommandDialog()
{
  FCommands->removeClient(this);
}

bool CommandDialog::receiveCommandResult(const ICommandResult &AResult)
{
  if (AResult.stanzaId == FRequestId)
  {
    resetDialog();

    FRequestId.clear();
    FSessionId = AResult.sessionId;

    if (!AResult.form.type.isEmpty())
    {
      FCurrentForm = FDataForms->formWidget(AResult.form,ui.wdtForm);
      if (!AResult.form.title.isEmpty())
        setWindowTitle(AResult.form.title);
      if (FCurrentForm->tableWidget())
        FCurrentForm->tableWidget()->instance()->setSortingEnabled(true);
      ui.wdtForm->layout()->addWidget(FCurrentForm->instance());
      ui.wdtForm->setVisible(true);
    }

    if (!AResult.notes.isEmpty())
    {
      QStringList notes;
      foreach(ICommandNote note, AResult.notes)
        notes.append(Qt::escape(note.message));
      ui.lblInfo->setText(notes.join("<br>"));
    }
    else if (AResult.status == COMMAND_STATUS_COMPLETED)
      ui.lblInfo->setText(tr("Command execution completed."));
    else if (AResult.status == COMMAND_STATUS_CANCELED)
      ui.lblInfo->setText(tr("Command execution canceled."));
    else
      ui.lblInfo->setVisible(false);

    if (!AResult.actions.isEmpty())
    {
      if (AResult.actions.contains(COMMAND_ACTION_PREVIOUS))
        ui.dbbButtons->addButton(FPrevButton,QDialogButtonBox::ActionRole);
      if (AResult.actions.contains(COMMAND_ACTION_NEXT))
        ui.dbbButtons->addButton(FNextButton,QDialogButtonBox::ActionRole);
      if (AResult.actions.contains(COMMAND_ACTION_COMPLETE))
        ui.dbbButtons->addButton(FCompleteButton,QDialogButtonBox::ActionRole);
    }
    else if (AResult.status == COMMAND_STATUS_EXECUTING)
      ui.dbbButtons->addButton(FCompleteButton,QDialogButtonBox::AcceptRole);
    else if (AResult.status == COMMAND_STATUS_COMPLETED)
      ui.dbbButtons->setStandardButtons(QDialogButtonBox::Retry|QDialogButtonBox::Close);
    else if (AResult.status == COMMAND_STATUS_CANCELED)
      close();

    return true;
  }
  return false;
}

bool CommandDialog::receiveCommandError(const ICommandError &AError)
{
  if (AError.stanzaId == FRequestId)
  {
    resetDialog();
    FRequestId.clear();
    ui.lblInfo->setText(tr("Requested operation failed: %1").arg(Qt::escape(AError.message)));
    ui.dbbButtons->setStandardButtons(QDialogButtonBox::Retry|QDialogButtonBox::Close);
    return true;
  }
  return false;
}

void CommandDialog::executeCommand()
{
  FSessionId.clear();
  executeAction(COMMAND_ACTION_EXECUTE);
}

void CommandDialog::resetDialog()
{
  setWindowTitle(tr("Executing command '%1' at %2").arg(FNode).arg(FCommandJid.full()));
  ui.lblInfo->setText("");
  ui.lblInfo->setVisible(true);
  if (FCurrentForm)
  {
    ui.wdtForm->layout()->removeWidget(FCurrentForm->instance());
    FCurrentForm->instance()->deleteLater();
    FCurrentForm = NULL;
  }
  ui.wdtForm->setVisible(false);
}

QString CommandDialog::sendRequest(const QString &AAction)
{
  ICommandRequest request;
  request.streamJid = FStreamJid;
  request.commandJid = FCommandJid;
  request.node = FNode;
  request.sessionId = FSessionId;
  request.action = AAction;
  if (FCurrentForm)
    request.form = FDataForms->dataSubmit(FCurrentForm->userDataForm());
  return FCommands->sendCommandRequest(request);
}

void CommandDialog::executeAction(const QString &AAction)
{
  if (AAction == COMMAND_ACTION_CANCEL || !FCurrentForm || FCurrentForm->checkForm(true))
  {
    ui.dbbButtons->removeButton(FPrevButton);
    ui.dbbButtons->removeButton(FNextButton);
    ui.dbbButtons->removeButton(FCompleteButton);

    FRequestId = sendRequest(AAction);

    resetDialog();
    if (!FRequestId.isEmpty())
    {
      ui.lblInfo->setText(tr("Waiting for host response ..."));
      ui.dbbButtons->setStandardButtons(AAction!=COMMAND_ACTION_CANCEL ? QDialogButtonBox::Cancel : QDialogButtonBox::Close);
    }
    else
    {
      ui.lblInfo->setText(tr("Error: Can`t send request to host."));
      ui.dbbButtons->setStandardButtons(QDialogButtonBox::Retry|QDialogButtonBox::Close);
    }
  }
}

void CommandDialog::onDialogButtonClicked(QAbstractButton *AButton)
{
  if (AButton == FPrevButton)
    executeAction(COMMAND_ACTION_PREVIOUS);
  else if (AButton == FNextButton)
    executeAction(COMMAND_ACTION_NEXT);
  else if (AButton == FCompleteButton)
    executeAction(COMMAND_ACTION_COMPLETE);
  else if (ui.dbbButtons->standardButton(AButton) == QDialogButtonBox::Retry)
    executeCommand();
  else if (ui.dbbButtons->standardButton(AButton) == QDialogButtonBox::Cancel)
    executeAction(COMMAND_ACTION_CANCEL);
  else if (ui.dbbButtons->standardButton(AButton) == QDialogButtonBox::Close)
    close();
}

