#include "datafield.h"

#include <QFocusEvent>

DataField::DataField(const QDomElement &AElem, IDataField::FieldKind AKind, QWidget *AParent) : QObject(AParent)
{
  FCheckBox = NULL;
  FLabelWidget = NULL;
  FTextEdit = NULL;
  FGroupBox = NULL;
  FComboBox = NULL;
  FLineEdit = NULL;
  FWidget = NULL;
  FTableItem = NULL;

  FKind = AKind;
  FRequired = !AElem.firstChildElement("required").isNull();
  FVar = AElem.attribute("var");
  FType = AElem.attribute("type",FIELD_TEXTSINGLE);
  FLabel = AElem.attribute("label");
  FDesc = AElem.firstChildElement("desc").text();

  QDomElement elem = AElem.firstChildElement("validate");
  if (!elem.isNull())
  {
    FValidate.dataType = elem.attribute("datatype",VALIDATE_TYPE_STRING);
    if (FType == FIELD_LISTMULTI && !elem.firstChildElement(VALIDATE_METHOD_LIST_RANGE).isNull())
    {
      QDomElement methodElem = elem.firstChildElement(VALIDATE_METHOD_LIST_RANGE);
      FValidate.min = methodElem.attribute("min");
      FValidate.max = methodElem.attribute("max");
    }
    if (!elem.firstChildElement(VALIDATE_METHOD_RANGE).isNull())
    {
      QDomElement methodElem = elem.firstChildElement(VALIDATE_METHOD_RANGE);
      FValidate.method = VALIDATE_METHOD_RANGE;
      FValidate.min = methodElem.attribute("min");
      FValidate.max = methodElem.attribute("max");
    }
    else if (!elem.firstChildElement(VALIDATE_METHOD_REGEXP).isNull())
    {
      QDomElement methodElem = elem.firstChildElement(VALIDATE_METHOD_REGEXP);
      FValidate.method = VALIDATE_METHOD_REGEXP;
      FValidate.regexp.setPattern(methodElem.text());
    }
    else if (!elem.firstChildElement(VALIDATE_METHOD_OPEN).isNull())
      FValidate.method = VALIDATE_METHOD_OPEN;
    else
      FValidate.method = VALIDATE_METHOD_BASIC;
  }

  elem = AElem.firstChildElement("option");
  while (!elem.isNull())
  {
    QString optionValue = elem.firstChildElement("value").text();
    QString optionLabel = elem.attribute("label").isEmpty() ? optionValue : elem.attribute("label");
    if (!optionValue.isEmpty() && !FOptionLabels.contains(optionLabel))
    {
      FOptionLabels.append(optionLabel);
      FOptions.insert(optionLabel, optionValue);
    }
    elem = elem.nextSiblingElement("option");
  }

  QStringList values;
  elem = AElem.firstChildElement("value");
  while (!elem.isNull())
  {
    values.append(elem.text());
    elem = elem.nextSiblingElement("value");
  }

  if (FKind == IDataField::TableHeader)
  {
    FStaticValue = FLabel;
  }
  else if (FKind == IDataField::TableCell)
  {
    FTableItem = new QTableWidgetItem;
    FTableItem->setFlags(Qt::ItemIsEnabled);
    setValue(values.value(0));
  }
  else if (FKind == IDataField::Result)
  {
    if (FType == FIELD_JIDMULTI || FType == FIELD_TEXTMULTI || FType == FIELD_LISTMULTI)
      setValue(values);
    else
      setValue(values.value(0));
  }
  else if (FType == FIELD_BOOLEAN)
  {
    FCheckBox = new QCheckBox(AParent);
    FCheckBox->setText(FLabel.isEmpty() ? Qt::escape(FDesc) : Qt::escape(FLabel));
    FCheckBox->setToolTip(Qt::escape(FDesc));
    FCheckBox->installEventFilter(this);
    FCleanupHandler.add(FCheckBox);
    FWidget = FCheckBox;
    setValue(values.value(0));
  }
  else if (FType == FIELD_FIXED)
  {
    FLabelWidget = new QLabel(Qt::escape(FLabel),AParent);
    FLabelWidget->setToolTip(Qt::escape(FDesc));
    FLabelWidget->setWordWrap(true);
    FCleanupHandler.add(FLabelWidget);
    FWidget = FLabelWidget;
  }
  else if (FType == FIELD_HIDDEN)
  {
    FStaticValue = values.value(0);
  }
  else if (FType == FIELD_JIDMULTI || FType == FIELD_TEXTMULTI)
  {
    FWidget = new QWidget(AParent);
    FWidget->setLayout(new QVBoxLayout);
    FTextEdit = new QTextEdit(FWidget);
    if (!FDesc.isEmpty() || !FLabel.isEmpty())
    {
      FLabelWidget = new QLabel(FLabel.isEmpty() ? Qt::escape(FDesc) : Qt::escape(FLabel), FWidget);
      FLabelWidget->setWordWrap(true);
      FLabelWidget->setBuddy(FTextEdit);
      FWidget->layout()->addWidget(FLabelWidget);
    }
    FTextEdit->setAcceptRichText(false);
    FTextEdit->installEventFilter(this);
    FWidget->layout()->addWidget(FTextEdit);
    FWidget->layout()->setMargin(0);
    FCleanupHandler.add(FWidget);
    setValue(values);
  }
  else if (FType == FIELD_LISTMULTI)
  {
    FGroupBox = new QGroupBox(AParent);
    FGroupBox->setTitle(Qt::escape(FLabel));
    FGroupBox->setLayout(new QVBoxLayout);
    if (!FDesc.isEmpty())
    {
      FLabelWidget = new QLabel(Qt::escape(FDesc), FGroupBox);
      FLabelWidget->setWordWrap(true);
      FGroupBox->layout()->addWidget(FLabelWidget);
    }
    foreach(QString optionLabel, FOptionLabels)
    {
      QCheckBox *checkBox = new QCheckBox(FGroupBox);
      checkBox->setText(Qt::escape(optionLabel));
      checkBox->installEventFilter(this);
      FGroupBox->layout()->addWidget(checkBox);
      FCheckList.append(checkBox);
    }
    FWidget = FGroupBox;
    FCleanupHandler.add(FWidget);
    setValue(values);
  }
  else if (FType == FIELD_LISTSINGLE)
  {
    FWidget = new QWidget(AParent);
    FWidget->setLayout(new QVBoxLayout);
    FComboBox = new QComboBox(FWidget);
    if (!FDesc.isEmpty() || !FLabel.isEmpty())
    {
      FLabelWidget = new QLabel(FLabel.isEmpty() ? Qt::escape(FDesc) : Qt::escape(FLabel), FWidget);
      FLabelWidget->setWordWrap(true);
      FLabelWidget->setBuddy(FComboBox);
      FWidget->layout()->addWidget(FLabelWidget);
    }
    FComboBox->setToolTip(Qt::escape(FDesc));
    FComboBox->installEventFilter(this);
    foreach(QString optionLabel, FOptionLabels)
      FComboBox->addItem(optionLabel, FOptions.value(optionLabel));
    if (FValidate.method == VALIDATE_METHOD_OPEN)
    {
      FComboBox->setEditable(true);
      FComboBox->setValidator(createValidator(FComboBox));
      FLineEdit = FComboBox->lineEdit();
    }
    FWidget->layout()->addWidget(FComboBox);
    FWidget->layout()->setMargin(0);
    FCleanupHandler.add(FWidget);
    setValue(values.value(0));
  }
  else //FIELD_TEXTSINGLE FIELD_TEXTPRIVATE
  {
    FWidget = new QWidget(AParent);
    FWidget->setLayout(new QVBoxLayout);
    QWidget *editor = createValidatedEditor(FWidget);
    if (!FDesc.isEmpty() || !FLabel.isEmpty())
    {
      FLabelWidget = new QLabel(FLabel.isEmpty() ? Qt::escape(FDesc) : Qt::escape(FLabel), FWidget);
      FLabelWidget->setWordWrap(true);
      FLabelWidget->setBuddy(editor);
      FWidget->layout()->addWidget(FLabelWidget);
    }
    editor->installEventFilter(this);
    FWidget->layout()->addWidget(editor);
    FWidget->layout()->setMargin(0);
    FCleanupHandler.add(FWidget);
    setValue(values.value(0));
  }
}

DataField::~DataField()
{

}

bool DataField::isValid() const
{
  if (FKind != Normal)
  {
    return true;
  }
  else if (isEmpty())
  {
    return isRequired() ? false : true;
  }
  else if (FType == FIELD_JIDSINGLE)
  {
    Jid singleJid = FLineEdit->text();
    if (!singleJid.isValid())
      return false;
  }
  else if (FType == FIELD_JIDMULTI)
  {
    QStringList multiJid = value().toStringList();
    foreach(QString stringJid, multiJid)
    {
      Jid singleJid = stringJid;
      if (!singleJid.isValid())
        return false;
    }
  }
  else if (FType == FIELD_LISTMULTI && (!FValidate.min.isEmpty() || !FValidate.max.isEmpty()))
  {
    QStringList listValues = value().toStringList();
    if (!FValidate.min.isEmpty() && listValues.count() < FValidate.min.toInt())
      return false;
    if (!FValidate.max.isEmpty() && listValues.count() > FValidate.max.toInt())
      return false;
  }
  else if (FLineEdit)
  {
    if (FValidate.dataType == VALIDATE_TYPE_URI)
    {
      if (!QUrl(FLineEdit->text()).isValid())
        return false;
    }
    else if (FLineEdit->validator()!=NULL)
    {
      int pos =0;
      QString fieldValue = FLineEdit->text();
      if (FLineEdit->validator()->validate(fieldValue,pos) != QValidator::Acceptable)
        return false;
    }
  }
  return true;
}

bool DataField::isEmpty() const
{
  QVariant fieldValue = value();
  return fieldValue.type() == QVariant::StringList ? fieldValue.toStringList().isEmpty() : fieldValue.toString().isEmpty();
}

QVariant DataField::value() const
{

  if (FKind == IDataField::TableHeader)
  {
    return FStaticValue;
  }
  else if (FKind == IDataField::TableCell)
  {
    return  FTableItem->text();
  }
  else if (FKind == IDataField::Result)
  {
    return FStaticValue;
  }
  else if (FType == FIELD_BOOLEAN)
  {
    return FCheckBox->checkState() == Qt::Checked ? "1" : "0";
  }
  else if (FType == FIELD_FIXED)
  {
    return FLabelWidget->text();
  }
  else if (FType == FIELD_HIDDEN)
  {
    return FStaticValue;
  }
  else if (FType == FIELD_JIDSINGLE)
  {
    Jid singleJid = FLineEdit->text();
    return singleJid.eFull();
  }
  else if (FType == FIELD_JIDMULTI)
  {
    QStringList multiJid = FTextEdit->toPlainText().split("\n", QString::SkipEmptyParts);
    for (int i = 0; i < multiJid.count(); i++)
    {
      Jid singleJid = multiJid.at(i);
      multiJid[i] = singleJid.eFull();
    }
    return multiJid;
  }
  else if (FType == FIELD_TEXTMULTI)
  {
    QStringList values;
    if (!FTextEdit->document()->isEmpty())
      values = FTextEdit->toPlainText().split("\n", QString::KeepEmptyParts);
    return values;
  }
  else if (FType == FIELD_LISTMULTI)
  {
    QStringList values;
    foreach(QCheckBox *checkBox, FCheckList)
      if (checkBox->checkState() == Qt::Checked)
        values.append(FOptions.value(checkBox->text()));
    return values;
  }
  else if (FType == FIELD_LISTSINGLE)
  {
    return FOptions.value(FComboBox->currentText(),FComboBox->currentText());
  }
  else
  {
    if (FValidate.dataType == VALIDATE_TYPE_DATETIME)
      return FDateTimeEdit->dateTime().toString(Qt::ISODate);
    else if (FValidate.dataType == VALIDATE_TYPE_DATE)
      return FDateEdit->date().toString(Qt::ISODate);
    else if (FValidate.dataType == VALIDATE_TYPE_TIME)
      return FTimeEdit->time().toString(Qt::ISODate);
    else
      return FLineEdit->text();
  }
}

void DataField::setValue(const QVariant &AValue)
{
  if (FKind == IDataField::TableHeader)
  {
    FStaticValue = AValue;
  }
  else if (FKind == IDataField::TableCell)
  {
    FTableItem->setText(AValue.toString());
  }
  else if (FKind == IDataField::Result)
  {
    FStaticValue = AValue;
  }
  else if (FType == FIELD_BOOLEAN)
  {
    FCheckBox->setCheckState(AValue.toBool() ? Qt::Checked : Qt::Unchecked);
  }
  else if (FType == FIELD_FIXED)
  {
    FLabelWidget->setText(Qt::escape(AValue.toString()));
  }
  else if (FType == FIELD_HIDDEN)
  {
    FStaticValue = AValue;
  }
  else if (FType == FIELD_JIDSINGLE)
  {
    Jid singleJid = AValue.toString();
    FLineEdit->setText(singleJid.full());
  }
  else if (FType == FIELD_JIDMULTI)
  {
    FTextEdit->clear();
    QStringList lines = AValue.toStringList();
    foreach(QString line, lines)
    {
      Jid singleJid = line;
      FTextEdit->append(singleJid.full());
    }
  }
  else if (FType == FIELD_TEXTMULTI)
  {
    FTextEdit->clear();
    QStringList lines = AValue.toStringList();
    foreach(QString line, lines)
      FTextEdit->append(line);
  }
  else if (FType == FIELD_LISTMULTI)
  {
    QStringList labels = AValue.toStringList();
    foreach(QCheckBox *checkBox, FCheckList)
      if (labels.contains(optionValue(checkBox->text())))
        checkBox->setCheckState(Qt::Checked);
      else
        checkBox->setCheckState(Qt::Unchecked);
  }
  else if (FType == FIELD_LISTSINGLE)
  {
    int index = FComboBox->findData(AValue.toString());
    if (index >=0 || !FComboBox->isEditable())
      FComboBox->setCurrentIndex(index);
    else if (FComboBox->lineEdit() != NULL)
      FComboBox->lineEdit()->setText(AValue.toString());
  }
  else
  {
    if (FValidate.dataType == VALIDATE_TYPE_DATETIME)
      FDateTimeEdit->setDateTime(QDateTime::fromString(AValue.toString(),Qt::ISODate));
    else if (FValidate.dataType == VALIDATE_TYPE_DATE)
      FDateEdit->setDate(QDateTime::fromString(AValue.toString(),Qt::ISODate).date());
    else if (FValidate.dataType == VALIDATE_TYPE_TIME)
      FTimeEdit->setTime(QDateTime::fromString(AValue.toString(),Qt::ISODate).time());
    else
      FLineEdit->setText(AValue.toString());
  }
}

QValidator *DataField::createValidator(QWidget *AParent)
{
  QValidator *fieldValidator = NULL;
  if ( FValidate.dataType == VALIDATE_TYPE_DECIMAL 
    || FValidate.dataType == VALIDATE_TYPE_INTEGER 
    || FValidate.dataType == VALIDATE_TYPE_LONG )
  {
    QIntValidator *intValidator = new QIntValidator(AParent);
    if (!FValidate.min.isEmpty())
      intValidator->setBottom(FValidate.min.toInt());
    if (!FValidate.max.isEmpty())
      intValidator->setTop(FValidate.max.toInt());
    fieldValidator = intValidator;
  }
  else if (FValidate.dataType == VALIDATE_TYPE_BYTE)
  {
    QIntValidator *intValidator = new QIntValidator(AParent);
    intValidator->setBottom(FValidate.min.isEmpty() ? -128 : FValidate.min.toInt());
    intValidator->setTop(FValidate.max.isEmpty() ? 127 : FValidate.max.toInt());
    fieldValidator = intValidator;
  }
  else if (FValidate.dataType == VALIDATE_TYPE_SHORT)
  {
    QIntValidator *intValidator = new QIntValidator(AParent);
    intValidator->setBottom(FValidate.min.isEmpty() ? -32768 : FValidate.min.toInt());
    intValidator->setTop(FValidate.max.isEmpty() ? -32767 : FValidate.max.toInt());
    fieldValidator = intValidator;
  }
  else if (FValidate.dataType == VALIDATE_TYPE_INT)
  {
    QIntValidator *intValidator = new QIntValidator(AParent);
    intValidator->setBottom(FValidate.min.isEmpty() ? -2147483647 : FValidate.min.toInt());
    intValidator->setTop(FValidate.max.isEmpty() ? 2147483647 : FValidate.max.toInt());
    fieldValidator = intValidator;
  }
  else if (FValidate.dataType == VALIDATE_TYPE_DOUBLE)
  {
    QDoubleValidator *doubleValidator = new QDoubleValidator(AParent);
    if (!FValidate.min.isEmpty())
      doubleValidator->setBottom(FValidate.min.toFloat());
    if (!FValidate.max.isEmpty())
      doubleValidator->setTop(FValidate.max.toFloat());
    fieldValidator = doubleValidator;
  }
  else if (FValidate.method == VALIDATE_METHOD_REGEXP)
  {
    QRegExpValidator *regexpValidator = new QRegExpValidator(AParent);
    regexpValidator->setRegExp(FValidate.regexp);
    fieldValidator = regexpValidator;
  }
  return fieldValidator;
}

QWidget *DataField::createValidatedEditor(QWidget *AParent)
{
  if (FValidate.dataType == VALIDATE_TYPE_DATETIME)
  {
    FDateTimeEdit = new QDateTimeEdit(AParent);
    FDateTimeEdit->setToolTip(Qt::escape(FDesc));
    FDateTimeEdit->setCalendarPopup(true);
    QDateTime min = QDateTime::fromString(FValidate.min,Qt::ISODate);
    QDateTime max = QDateTime::fromString(FValidate.max,Qt::ISODate);
    if (min.isValid())
    {
      FDateTimeEdit->setMinimumDate(min.date());
      FDateTimeEdit->setMinimumTime(min.time());
    }
    if (max.isValid())
    {
      FDateTimeEdit->setMaximumDate(max.date());
      FDateTimeEdit->setMaximumTime(max.time());
    }
    FCleanupHandler.add(FDateTimeEdit);
    return FDateTimeEdit;
  }
  else if (FValidate.dataType == VALIDATE_TYPE_DATE)
  {
    FDateEdit = new QDateEdit(AParent);
    FDateEdit->setToolTip(Qt::escape(FDesc));
    FDateEdit->setCalendarPopup(true);
    QDateTime min = QDateTime::fromString(FValidate.min,Qt::ISODate);
    QDateTime max = QDateTime::fromString(FValidate.max,Qt::ISODate);
    if (min.isValid())
      FDateEdit->setMinimumDate(min.date());
    if (max.isValid())
      FDateEdit->setMaximumDate(max.date());
    FCleanupHandler.add(FDateEdit);
    return FDateEdit;
  }
  else if (FValidate.dataType == VALIDATE_TYPE_TIME)
  {
    FTimeEdit = new QTimeEdit(AParent);
    FTimeEdit->setToolTip(Qt::escape(FDesc));
    QDateTime min = QDateTime::fromString(FValidate.min,Qt::ISODate);
    QDateTime max = QDateTime::fromString(FValidate.max,Qt::ISODate);
    if (min.isValid())
      FTimeEdit->setMinimumTime(min.time());
    if (max.isValid())
      FTimeEdit->setMaximumTime(max.time());
    FCleanupHandler.add(FTimeEdit);
    return FTimeEdit;
  }
  else
  {
    FLineEdit = new QLineEdit(FWidget);
    FLineEdit->setToolTip(Qt::escape(FDesc));
    if (FType == FIELD_TEXTPRIVATE)
      FLineEdit->setEchoMode(QLineEdit::Password);
    FLineEdit->setValidator(createValidator(FLineEdit));
    return FLineEdit;
  }
}

bool DataField::eventFilter(QObject *AObject, QEvent *AEvent)
{
  if (AEvent->type() == QEvent::FocusIn || AEvent->type() == QEvent::FocusOut)
  {
    QFocusEvent *focusEvent = static_cast<QFocusEvent *>(AEvent);
    if (focusEvent)
    {
      if (AEvent->type() == QEvent::FocusIn)
        emit fieldGotFocus(focusEvent->reason());
      else
        emit fieldLostFocus(focusEvent->reason());
    }
  }
  return QObject::eventFilter(AObject,AEvent);
}
