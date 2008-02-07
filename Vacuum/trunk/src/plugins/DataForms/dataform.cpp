#include "dataform.h"

#include <QDebug>
#include <QFocusEvent>
#include <QHeaderView>

DataForm::DataForm(const QDomElement &AElem, QWidget *AParent) : QWidget(AParent)
{
  FFocusedField = NULL;
  FTableWidget = NULL;
  FPageControl = NULL;
  FPageControlLabel = NULL;

  FStackedWidget = new QStackedWidget;
  FScrollArea = new QScrollArea(this);
  FScrollArea->setWidgetResizable(true);
  FScrollArea->setFrameShape(QFrame::NoFrame);
  FScrollArea->setWidget(FStackedWidget);
  FInstructLabel = new QLabel(this);
  FInstructLabel->setWordWrap(true);
  FInstructLabel->setAlignment(Qt::AlignCenter);
  setLayout(new QVBoxLayout(this));
  layout()->addWidget(FInstructLabel);
  layout()->addWidget(FScrollArea);
  layout()->setMargin(0);

  FFormDoc.appendChild(FFormDoc.createElement("form")).appendChild(AElem.cloneNode(true));
  FType = AElem.attribute("type",FORM_FORM);
  FTitle = AElem.firstChildElement("title").text();

  QDomElement elem = AElem.firstChildElement("instructions");
  while (!elem.isNull())
  {
    FInstructions.append(elem.text());
    elem = elem.nextSiblingElement("instructions");
  }
  setInstructions(FInstructions.join("<br>"));

  elem = AElem.firstChildElement("field");
  while(!elem.isNull())
  {
    if (!elem.attribute("var").isEmpty() && field(elem.attribute("var"))==NULL)
    {
      IDataField *dataField = new DataField(elem,FType==FORM_FORM ? IDataField::Edit : (FType==FORM_SUBMIT ? IDataField::Value : IDataField::View), this);
      connect(dataField->instance(),SIGNAL(fieldGotFocus(Qt::FocusReason)),SLOT(onFieldGotFocus(Qt::FocusReason)));
      connect(dataField->instance(),SIGNAL(fieldLostFocus(Qt::FocusReason)),SLOT(onFieldLostFocus(Qt::FocusReason)));
      FDataFields.append(dataField);
    }
    elem = elem.nextSiblingElement("field");
  }

  elem = AElem.firstChildElement("reported");
  if (!elem.isNull())
  {
    QStringList tableVars;
    elem = elem.firstChildElement("field");
    while (!elem.isNull())
    {
      if (!elem.attribute("var").isEmpty() && !tableVars.contains(elem.attribute("var")))
      {
        IDataField *dataField = new DataField(elem,IDataField::TableHeader,this);
        FColumns.append(dataField);
        tableVars.append(dataField->var());
      }
      elem = elem.nextSiblingElement("field");
    }

    int row = 0;
    elem = AElem.firstChildElement("item");
    while(!elem.isNull())
    {
      QHash<QString,IDataField *> &rowFields = FRows[row++];
      QDomElement fieldElem = elem.firstChildElement("field");
      while(!fieldElem.isNull())
      {
        if (tableVars.contains(fieldElem.attribute("var")))
        {
          IDataField *dataField = new DataField(fieldElem,IDataField::TableCell,this);
          rowFields.insert(dataField->var(),dataField);
        }
        fieldElem = fieldElem.nextSiblingElement("field");
      }
      elem = elem.nextSiblingElement("item");
    }

    QStringList headerLabels;
    FTableWidget = new QTableWidget(FRows.count(),FColumns.count(),this);
    for (int col = 0; col < FColumns.count(); col++)
    {
      IDataField *colField = FColumns.at(col);
      headerLabels.append(colField->label().isEmpty() ? colField->var() : colField->label());
      for (int row = 0; row < FRows.count(); row++)
      {
        IDataField *cellField = FRows.value(row).value(colField->var());
        if (cellField && cellField->tableItem())
          FTableWidget->setItem(row,col,cellField->tableItem());
      }
    }
    FTableWidget->installEventFilter(this);
    FTableWidget->setHorizontalHeaderLabels(headerLabels);
    FTableWidget->horizontalHeader()->resizeSections(QHeaderView::ResizeToContents);
    FTableWidget->verticalHeader()->resizeSections(QHeaderView::ResizeToContents);
    FTableWidget->setMinimumHeight(FTableWidget->verticalHeader()->length() + FTableWidget->horizontalHeader()->height());
    FTableWidget->setMinimumWidth(FTableWidget->horizontalHeader()->length() + FTableWidget->verticalHeader()->height());
    FTableWidget->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    FTableWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    connect(FTableWidget,SIGNAL(cellActivated(int,int)),SLOT(onCellActivated(int,int)));
    connect(FTableWidget,SIGNAL(currentCellChanged(int,int,int,int)),SLOT(onCurrentCellChanged(int,int,int,int)));
  }

  QDomNodeList pagesList = AElem.elementsByTagNameNS(NS_JABBER_XDATALAYOUT,"page");
  if (pagesList.isEmpty())
  {
    QWidget *pageWidget = new QWidget(this);
    pageWidget->setLayout(new QVBoxLayout(pageWidget));
    pageWidget->layout()->setMargin(0);
    insertFields(formElement(),pageWidget->layout(),0);
    FStackedWidget->addWidget(pageWidget);
  }
  else 
  {
    FPageControl = new QWidget(this);
    FPageControl->setLayout(new QHBoxLayout);
    QToolButton *prev = new QToolButton(FPageControl);
    prev->setText("<");
    connect(prev,SIGNAL(clicked()),SLOT(onShowPrevPageClicked()));
    FPageControlLabel = new QLabel(FPageControl);
    FPageControlLabel->setAlignment(Qt::AlignCenter);
    QToolButton *next = new QToolButton(FPageControl);
    next->setText(">");
    connect(next,SIGNAL(clicked()),SLOT(onShowNextPageClicked()));
    FPageControl->layout()->addWidget(prev);
    FPageControl->layout()->addWidget(FPageControlLabel);
    FPageControl->layout()->addWidget(next);
    FPageControl->layout()->setMargin(0);
    for(int pageIndex = 0; pageIndex < pagesList.count(); pageIndex++)
    {
      QDomElement pageElem = pagesList.at(pageIndex).toElement();
      QWidget *pageWidget = new QWidget(this);
      pageWidget->setLayout(new QVBoxLayout(pageWidget));
      pageWidget->layout()->setMargin(0);
      QString label = pageElem.attribute("label");
      if (!label.isEmpty())
      {
        QLabel *labelWidget = new QLabel(Qt::escape(label),pageWidget);
        labelWidget->setWordWrap(true);
        labelWidget->setAlignment(Qt::AlignCenter);
        pageWidget->layout()->addWidget(labelWidget);
      }
      FPageLabels.append(label);
      insertFields(pageElem,pageWidget->layout(),pageIndex);
      FStackedWidget->addWidget(pageWidget);
    }
  }

  setCurrentPage(0);
}

DataForm::~DataForm()
{
  delete FPageControl;
}

bool DataForm::isValid(int APage) const
{
  QList<IDataField *> dataFields = APage >= 0 ? FPageFields.values(APage) : FDataFields;
  foreach(IDataField *dataField, dataFields)
    if (!dataField->isValid())
      return false;
  return true;
}

QString DataForm::invalidMessage(int APage) const
{
  QString message = tr("Current form values are not acceptable:");
  message += tr("<br>Invalid values in fields:");
  QList<IDataField *> dataFields = APage >= 0 ? FPageFields.values(APage) : FDataFields;
  foreach(IDataField *field, dataFields)
    if (!field->isValid())
      message += tr("<br>- %1").arg(field->label());
  return message;
}

void DataForm::setInstructions(const QString &AInstructions)
{
  FInstructLabel->setText(AInstructions);
  FInstructLabel->setVisible(!AInstructions.isEmpty());
}

void DataForm::createSubmit(QDomElement &AFormElem) const
{
  AFormElem.setAttribute("type",FORM_SUBMIT);
  foreach(IDataField *dataField, FDataFields)
    if (!dataField->isEmpty())
    {
      QDomElement elem = AFormElem.appendChild(AFormElem.ownerDocument().createElement("field")).toElement();
      elem.setAttribute("type",dataField->type());
      elem.setAttribute("var",dataField->var());
      QVariant variant = dataField->value();
      if (variant.type() == QVariant::StringList)
      {
        QStringList values = variant.toStringList();
        foreach(QString fieadValue, values)
          elem.appendChild(elem.ownerDocument().createElement("value")).appendChild(elem.ownerDocument().createTextNode(fieadValue));
      }
      else
        elem.appendChild(elem.ownerDocument().createElement("value")).appendChild(elem.ownerDocument().createTextNode(variant.toString()));
    }
}

void DataForm::setCurrentPage(int APage)
{
  if (APage >= 0 && APage < pageCount())
  {
    if (FPageControlLabel)
      FPageControlLabel->setText(tr("Page %1 of %2").arg(APage+1).arg(pageCount()));
    if (currentPage() != APage)
    {
      FStackedWidget->setCurrentIndex(APage);
      FScrollArea->ensureVisible(0,0);
      emit currentPageChanged(APage);
    }
  }
}

IDataField *DataForm::tableField(int ATableCol, int ATableRow) const
{
  IDataField *colField = FColumns.value(ATableCol);
  if (colField)
    return FRows.value(ATableRow).value(colField->var());
  return NULL;
}

IDataField *DataForm::tableField(const QString &AVar, int ATableRow) const
{
  return FRows.value(ATableRow).value(AVar);
}

IDataField *DataForm::field(const QString &AVar) const
{
  foreach(IDataField *dataField, FDataFields)
    if (dataField->var() == AVar)
      return dataField;
  return NULL;
}

QVariant DataForm::fieldValue(const QString &AVar) const
{
  IDataField *dataField = field(AVar);
  if (dataField)
    return dataField->value();
  return QVariant();
}

QList<IDataField *> DataForm::notValidFields() const
{
  QList<IDataField *> dataFields;
  foreach(IDataField *dataFiled, FDataFields)
    if (!dataFiled->isValid())
      dataFields.append(dataFiled);
  return dataFields;
}

void DataForm::insertFields(const QDomElement &AElem, QLayout *ALayout, int APage)
{
  QDomElement elem = AElem.firstChildElement();
  while(!elem.isNull())
  {
    if (elem.tagName() == "fieldref" || elem.tagName() == "field")
    {
      if (!elem.attribute("var").isEmpty())
      {
        IDataField *dataField = field(elem.attribute("var"));
        if (dataField && dataField->widget() && !FInsertedFields.contains(dataField->widget()))
        {
          ALayout->addWidget(dataField->widget());
          FInsertedFields.append(dataField->widget());
          FPageFields.insert(APage,dataField);
        }
      }
      else if (elem.attribute("type") == FIELD_FIXED)
      {
        QString value = elem.firstChildElement("value").text();
        if (!value.isEmpty())
        {
          QLabel *label = new QLabel(Qt::escape(value),ALayout->parentWidget());
          label->setWordWrap(true);
          ALayout->addWidget(label);
        }
      }
    }
    else if (elem.tagName() == "reportedref" || elem.tagName() == "reported")
    {
      if (FTableWidget && !FInsertedFields.contains(FTableWidget))
      {
        ALayout->addWidget(FTableWidget);
        FInsertedFields.append(FTableWidget);
      }
    }
    else if (elem.tagName() == "text")
    {
      if (!elem.text().isEmpty())
      {
        QLabel *label = new QLabel(Qt::escape(elem.text()),ALayout->parentWidget());
        label->setWordWrap(true);
        ALayout->addWidget(label);
      }
    }
    else if (elem.tagName() == "section")
    {
      QGroupBox *groupBox = new QGroupBox(elem.attribute("label"));
      groupBox->setLayout(new QVBoxLayout(groupBox));
      ALayout->addWidget(groupBox);
      insertFields(elem,groupBox->layout(),APage);
    }
    elem = elem.nextSiblingElement();
  }
}

void DataForm::setFocusedField(IDataField *AField, Qt::FocusReason AReason)
{
  if (FFocusedField != AField)
  {
    if (FFocusedField)
      emit fieldLostFocus(FFocusedField,AReason);
    if (AField)
      emit fieldGotFocus(AField,AReason);
    FFocusedField = AField;
    emit focusedFieldChanged(FFocusedField);
  }
}

bool DataForm::eventFilter(QObject *AObject, QEvent *AEvent)
{
  if (AObject == FTableWidget && (AEvent->type() == QEvent::FocusIn || AEvent->type() == QEvent::FocusOut))
  {
    QFocusEvent *focusEvent = static_cast<QFocusEvent *>(AEvent);
    if (focusEvent)
    {
      if (AEvent->type() == QEvent::FocusIn)
      {
        IDataField *dataField = tableField(FTableWidget->currentColumn(),FTableWidget->currentRow());
        setFocusedField(dataField,focusEvent->reason());
      }
      else
        setFocusedField(NULL,focusEvent->reason());
    }
  }
  return QWidget::eventFilter(AObject,AEvent);
}

void DataForm::onFieldGotFocus(Qt::FocusReason AReason)
{
  IDataField *dataFiled = qobject_cast<IDataField *>(sender());
  if (dataFiled)
    setFocusedField(dataFiled,AReason);
}

void DataForm::onFieldLostFocus(Qt::FocusReason AReason)
{
  IDataField *dataFiled = qobject_cast<IDataField *>(sender());
  if (dataFiled && dataFiled == FFocusedField)
    setFocusedField(NULL,AReason);
}

void DataForm::onCellActivated(int ARow, int ACol)
{
  IDataField *dataField = tableField(ACol,ARow);
  if (dataField)
    emit fieldActivated(dataField);
}

void DataForm::onCurrentCellChanged(int ARow, int ACol, int /*APrevRow*/, int /*APrevCol*/)
{
  IDataField *dataField = tableField(ACol,ARow);
  setFocusedField(dataField,Qt::NoFocusReason);
}

void DataForm::onShowPrevPageClicked()
{
  setCurrentPage(currentPage()-1);
}

void DataForm::onShowNextPageClicked()
{
  setCurrentPage(currentPage()+1);
}

