#include "errorhandler.h"
#include <QList>

QMultiHash<QString, ErrorItem *> ErrorHandler::FStore;
bool ErrorHandler::FInited = false;
const QString ErrorHandler::DEFAULTNS = "urn:ietf:params:xml:ns:xmpp-stanzas";
const QString ErrorHandler::CANCEL = "cancel";
const QString ErrorHandler::WAIT = "wait";
const QString ErrorHandler::MODIFY = "modify";
const QString ErrorHandler::AUTH = "auth";
const QString ErrorHandler::NULLSTRING;


void ErrorHandler::addErrorItem(const QString &ANsURI, ErrorItem *AItem)
{
  if (!FInited) init();
  QList<ErrorItem *> items = FStore.values(ANsURI);
  for(int i=0; i<items.count(); i++)
    if (items[i]->condition == AItem->condition && items[i]->code == AItem->code)
      return;
  FStore.insert(ANsURI,AItem); 
}

void ErrorHandler::addErrorItem(const QString &ANsURI, 
                                const QString &ACondition, 
                                const QString &AType, 
                                int ACode, 
                                const QString &AMeaning)
{
  ErrorItem *item = new ErrorItem;
  item->code = ACode;
  item->condition = ACondition;
  item->type = AType;
  item->meaning = AMeaning;
  addErrorItem(ANsURI,item);
}
const ErrorItem *ErrorHandler::conditionToItem(const QString &ANsURI, 
                                               const QString &ACondition)
{
  if (!FInited) init();
  QList<ErrorItem *> items = FStore.values(ANsURI);
  for(int i=0; i<items.count(); i++)
    if (items[i]->condition == ACondition)
      return items[i]; 
  return 0;
}

const int ErrorHandler::conditionToCode(const QString &ANsURI, 
                                        const QString &ACondition)
{
  if (!FInited) init();
  QList<ErrorItem *> items = FStore.values(ANsURI);
  for(int i=0; i<items.count(); i++)
    if (items[i]->condition == ACondition)
      return items[i]->code; 
  return UNKNOWN;
}
const QString &ErrorHandler::conditionToType(const QString &ANsURI, 
                                             const QString &ACondition)
{
  if (!FInited) init();
  QList<ErrorItem *> items = FStore.values(ANsURI);
  for(int i=0; i<items.count(); i++)
    if (items[i]->condition == ACondition)
      return items[i]->type; 
  return NULLSTRING;
}

const QString &ErrorHandler::conditionToMeaning(const QString &ANsURI, 
                                                const QString &ACondition)
{
  if (!FInited) init();
  QList<ErrorItem *> items = FStore.values(ANsURI);
  for(int i=0; i<items.count(); i++)
    if (items[i]->condition == ACondition)
      return items[i]->meaning; 
  return NULLSTRING;
}

const ErrorItem *ErrorHandler::codeToItem(const QString &ANsURI, int &ACode)
{
  if (!FInited) init();
  QList<ErrorItem *> items = FStore.values(ANsURI);
  for(int i=0; i<items.count(); i++)
    if (items[i]->code == ACode)
      return items[i]; 
  return 0;  
}

const QString &ErrorHandler::codeToCondition(const QString &ANsURI, int ACode)
{
  if (!FInited) init();
  QList<ErrorItem *> items = FStore.values(ANsURI);
  for(int i=0; i<items.count(); i++)
    if (items[i]->code == ACode)
      return items[i]->condition; 
  return NULLSTRING;
}

const QString &ErrorHandler::codeToType(const QString &ANsURI, int ACode)
{
  if (!FInited) init();
  QList<ErrorItem *> items = FStore.values(ANsURI);
  for(int i=0; i<items.count(); i++)
    if (items[i]->code == ACode)
      return items[i]->type; 
  return NULLSTRING;
}

const QString &ErrorHandler::codeToMeaning(const QString &ANsURI, int ACode)
{
  if (!FInited) init();
  QList<ErrorItem *> items = FStore.values(ANsURI);
  for(int i=0; i<items.count(); i++)
    if (items[i]->code == ACode)
      return items[i]->meaning; 
  return NULLSTRING;
}

void ErrorHandler::init() 
{
  if (FInited) return;
  FInited = true;

  addErrorItem(DEFAULTNS, "redirect",                MODIFY, 302, tr("Redirect"));
  addErrorItem(DEFAULTNS, "gone",                    MODIFY, 302, tr("Redirect"));
  addErrorItem(DEFAULTNS, "bad-request",             MODIFY, 400, tr("Bad Request"));
  addErrorItem(DEFAULTNS, "unexpected-request",      WAIT,   400, tr("Unexpected Request"));
  addErrorItem(DEFAULTNS, "jid-malformed",           MODIFY, 400, tr("Jid Malformed"));
  addErrorItem(DEFAULTNS, "not-authorized",          AUTH,   401, tr("Not Authorized"));
  addErrorItem(DEFAULTNS, "payment-required",        AUTH,   402, tr("Payment Required"));
  addErrorItem(DEFAULTNS, "forbidden",               AUTH,   403, tr("Forbidden"));
  addErrorItem(DEFAULTNS, "item-not-found",          CANCEL, 404, tr("Not Found"));
  addErrorItem(DEFAULTNS, "recipient-unavailable",   WAIT,   404, tr("Recipient Unavailable"));
  addErrorItem(DEFAULTNS, "remote-server-not-found", CANCEL, 404, tr("Remote Server Not Found"));
  addErrorItem(DEFAULTNS, "not-allowed",             CANCEL, 405, tr("Not Allowed"));
  addErrorItem(DEFAULTNS, "not-acceptable",          MODIFY, 406, tr("Not Acceptable"));
  addErrorItem(DEFAULTNS, "registration-required",   AUTH,   407, tr("Registration Required"));
  addErrorItem(DEFAULTNS, "subscription-required",   AUTH,   407, tr("Subscription Required"));
  addErrorItem(DEFAULTNS, "request-timeout",         WAIT,   408, tr("Request Timeout"));
  addErrorItem(DEFAULTNS, "conflict",                CANCEL, 409, tr("Conflict"));
  addErrorItem(DEFAULTNS, "internal-server-error",   WAIT,   500, tr("Internal Server Error"));
  addErrorItem(DEFAULTNS, "resource-constraint",     WAIT,   500, tr("Resource Constraint"));
  addErrorItem(DEFAULTNS, "undefined-condition",     CANCEL, 500, tr("Undefined Condition"));
  addErrorItem(DEFAULTNS, "feature-not-implemented", CANCEL, 501, tr("Not Implemented"));
  addErrorItem(DEFAULTNS, "remoute-server-error",    CANCEL, 502, tr("Remoute Server Error"));
  addErrorItem(DEFAULTNS, "service-unavailable",     CANCEL, 503, tr("Service Unavailable"));
  addErrorItem(DEFAULTNS, "remote-server-timeout",   WAIT,   504, tr("Remote Server timeout"));
  addErrorItem(DEFAULTNS, "disconnected",            CANCEL, 510, tr("Disconnected"));
}

ErrorHandler::ErrorHandler(QObject *parent) 
  : QObject(parent)
{
  if (!FInited) init();
  FCode = 0;
}

ErrorHandler::ErrorHandler(const QString &ANsURI, 
                           const QDomElement &AElem, QObject *parent)
  : QObject(parent)
{
  if (!FInited) init();
  parseElement(ANsURI,AElem);
}

ErrorHandler::ErrorHandler(const QString &ANsURI, 
                           const QString &ACondition, int ACode, QObject *parent)
  : QObject(parent)
{
  if (!FInited) init();
  FNsURI = ANsURI;
  FCondition = ACondition;
  FCode = ACode;
  const ErrorItem *item = conditionToItem(ANsURI,ACondition);
  if (item)
  {
    FType = item->type; 
    FMeaning = item->meaning;
  }
}

ErrorHandler::ErrorHandler(const QString &ANsURI, 
                           const QString &ACondition, QObject *parent)
  : QObject(parent)
{
  if (!FInited) init();
  FCode = 0;
  FNsURI = ANsURI;
  const ErrorItem *item = conditionToItem(ANsURI,ACondition);
  if (item)
  {
    FCode = item->code; 
    FType = item->type; 
    FMeaning = item->meaning;
  }
}

ErrorHandler::ErrorHandler(const QString &ANsURI, 
                           int ACode, QObject *parent)
  : QObject(parent)
{
  if (!FInited) init();
  FNsURI = ANsURI;
  FCode = ACode;
  const ErrorItem *item = codeToItem(ANsURI,ACode);
  if (item)
  {
    FCondition = item->condition; 
    FType = item->type; 
    FMeaning = item->meaning;
  }
}

ErrorHandler::~ErrorHandler()
{

}

ErrorHandler &ErrorHandler::parseElement(const QString &ANsURI,
                                         const QDomElement &AErrElem)
{
  FCode = 0;
  FNsURI = ANsURI;
  FType.clear();
  FCondition.clear();
  FMeaning.clear();
  FText.clear(); 

  if (AErrElem.isNull()) 
    return *this;

  const QDomNode *node = &AErrElem.elementsByTagName("error").at(0);
  if (node->isNull()) 
    node = &AErrElem;

  const ErrorItem *item = 0;
  FCode = node->toElement().attribute("code","0").toInt();
  FType = node->toElement().attribute("type");
  node = &node->firstChild();
  while (!node->isNull() && item == 0) 
  {
    if (FText.isNull() && node->isText())
      FText = node->toText().data(); 

    if (node->toElement().tagName() != "text")
    {
      bool defaultNS = false;
      item = conditionToItem(ANsURI,node->toElement().tagName());
      if (item == 0)
      {
        item = conditionToItem(DEFAULTNS,node->toElement().tagName());
        defaultNS = true;
      }
      if (item != 0)
      {
        FCondition = item->condition;
        if (FCode == 0)
          FCode = item->code;
        if (FType.isEmpty())
          FType = item->type;
        FMeaning = item->meaning; 
        if (defaultNS)
          item = 0;
      }
      else if (FCondition.isEmpty())
        FCondition = node->toElement().tagName();
    }
    else
      FText = node->firstChild().toText().data();

    node = &node->nextSibling(); 
  }

  if (FCode == 0 && !FCondition.isEmpty())
    FCode = conditionToCode(ANsURI, FCondition);

  if (FCondition.isEmpty() && FCode != 0)
    FCondition = codeToCondition(ANsURI, FCode);

  if (FType.isEmpty() && FCode != 0)
    FType = conditionToType(ANsURI,FCondition);
  if (FType.isEmpty() && !FCondition.isEmpty())
    FType = conditionToType(ANsURI,FCondition);

  if (FMeaning.isEmpty() && !FCondition.isEmpty())
    FMeaning = conditionToMeaning(ANsURI, FCondition);
  if (FMeaning.isEmpty() && FCode != 0)
    FMeaning = codeToMeaning(ANsURI, FCode);
  if (FMeaning.isEmpty() && !FCondition.isEmpty())
    FMeaning = conditionToMeaning(DEFAULTNS, FCondition);

  if (FCondition.isEmpty() && FMeaning.isEmpty() && FText.isEmpty() && FCode == 0)
    FMeaning = tr("Unknown Error");

  return *this;
}

const QString ErrorHandler::message() const
{
  QString msg;

  if (!FContext.isEmpty())
    msg +=FContext+"\n\n"; 

  msg += FMeaning.isEmpty()  ? FCondition : FMeaning;

  if (!FText.isEmpty() && FMeaning.isEmpty() && FCondition.isEmpty())
    msg += FText;

  return msg;
}