#ifndef ERRORHANDLER_H
#define ERRORHANDLER_H

#include <QObject>
#include <QMultiHash>
#include <QDomDocument>
#include "utilsexport.h"

struct ErrorItem
{
  QString condition;
  QString type;
  int code;
  QString meaning;
};

class UTILS_EXPORT ErrorHandler : 
  public QObject
{
  Q_OBJECT

public:
  enum ErrorCode
  {
    UNKNOWN                       = 000,
    REDIRECT                      = 302,
    GONE                          = 302,
    BAD_REQUEST                   = 400,
    JID_MALFORMED                 = 400,
    UNEXPECTED_REQUEST            = 400,
    NOT_AUTHORIZED                = 401,
    PAYMENT_REQUIRED              = 402,
    FORBIDDEN                     = 403,
    ITEM_NOT_FOUND                = 404,
    REMOTE_SERVER_NOT_FOUND       = 404,
    RECIPIENT_UNAVAILABLE         = 404,
    NOT_ALLOWED                   = 405,
    NOT_ACCEPTABLE                = 406,
    REGISTRATION_REQUIRED         = 407,
    SUBSCRIPTION_REQUIRED         = 407,
    REQUEST_TIMEOUT               = 408,
    CONFLICT                      = 409,
    INTERNAL_SERVER_ERROR         = 500,
    RESOURCE_CONSTRAINT           = 500,
    UNDEFINED_CONDITION           = 500,
    FEATURE_NOT_IMPLEMENTED       = 501,
    REMOUTE_SERVER_ERROR          = 502,
    SERVICE_UNAVAILABLE           = 503,
    REMOTE_SERVER_TIMEOUT         = 504,
    DISCONNECTED                  = 510,
  };
  static const QString NULLSTRING;
  static const QString DEFAULTNS;
  static const QString CANCEL;
  static const QString WAIT;
  static const QString MODIFY;
  static const QString AUTH;

  static void addErrorItem(const QString &ANsURI, const QString &ACondition,
    const QString &AType, int ACode, const QString &AMeaning);
  static void addErrorItem(const QString &ANsURI, ErrorItem *AItem);
  static const ErrorItem *conditionToItem(const QString &ANsURI, 
    const QString &ACondition);
  static const int conditionToCode(const QString &ANsURI, 
    const QString &ACondition);
  static const QString &conditionToType(const QString &ANsURI, 
    const QString &ACondition);
  static const QString &conditionToMeaning(const QString &ANsURI, 
    const QString &ACondition);
  static const ErrorItem *codeToItem(const QString &ANsURI, int &ACode);
  static const QString &codeToCondition(const QString &ANsURI, int ACode);
  static const QString &codeToType(const QString &ANsURI, int ACode);
  static const QString &codeToMeaning(const QString &ANsURI, int ACode);
private:
  static QMultiHash<QString, ErrorItem *> FStore;
  static bool FInited;
  static void init();
public:
  ErrorHandler(QObject *parent = 0);
  ErrorHandler(const QString &ANsURI, const QDomElement &AElem, QObject *parent = 0);
  ErrorHandler(const QString &ANsURI, const QString &ACondition, int ACode, QObject *parent = 0);
  ErrorHandler(const QString &ANsURI, const QString &ACondition, QObject *parent = 0);
  ErrorHandler(const QString &ANsURI, int ACode, QObject *parent = 0);
  virtual ~ErrorHandler();

  ErrorHandler &parseElement(const QString &ANsURI,
    const QDomElement &AErrElem);
  const QString condition() const { return FCondition; }
  const int code() const { return FCode; }
  const QString type() const { return FType; }
  const QString meaning() const { return FMeaning; } 
  const QString text() const { return FText; }
  const QString context() const { return FContext; }
  ErrorHandler &setContext(const QString &AContext) { FContext = AContext; return *this; }
  const QString message() const;
private:
  QString FNsURI;
  QString FCondition;
  int			FCode;
  QString FType;
  QString FMeaning;
  QString FText;
  QString FContext;
};

#endif // ERRORHANDLER_H
