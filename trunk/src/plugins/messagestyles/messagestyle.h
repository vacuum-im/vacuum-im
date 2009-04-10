#ifndef MESSAGESTYLE_H
#define MESSAGESTYLE_H

#include <QMap>
#include <QList>
#include <QWebView>
#include "../../definations/resources.h"
#include "../../interfaces/imessagestyles.h"
#include "../../utils/filestorage.h"

class MessageStyle : 
  public QObject,
  public IMessageStyle
{
  Q_OBJECT;
  Q_INTERFACES(IMessageStyle);
public:
  MessageStyle(const QString &AStylePath, QObject *AParent);
  ~MessageStyle();
  virtual QObject *instance() { return this; }
  virtual bool isValid() const;
  virtual int version() const;
  virtual QList<QString> variants() const;
  virtual QVariant infoValue(const QString &AKey, const QVariant &ADefValue = QVariant()) const;
  virtual void setStyle(QWebView *AView, const IMessageStyle::StyleOptions &AOptions) const;
  virtual void setVariant(QWebView *AView, const QString  &AVariant) const;
  virtual void appendContent(QWebView *AView, const QString &AMessage, const IMessageStyle::ContentOptions &AOptions) const;
signals:
  virtual void styleSeted(QWebView *AView, const IMessageStyle::StyleOptions &AOptions) const;
  virtual void variantSeted(QWebView *AView, const QString  &AVariant) const;
  virtual void contentAppended(QWebView *AView, const QString &AMessage, const IMessageStyle::ContentOptions &AOptions) const;
private:
  QString makeStyleTemplate(const IMessageStyle::StyleOptions &AOptions) const;
  QString makeContentTemplate(const IMessageStyle::ContentOptions &AOptions) const;
  void fillStyleKeywords(QString &AHtml, const IMessageStyle::StyleOptions &AOptions) const;
  void fillContentKeywords(QString &AHtml, const IMessageStyle::ContentOptions &AOptions) const;
  void escapeStringForScript(QString &AText) const;
  QString scriptForAppendContent(const IMessageStyle::ContentOptions &AOptions) const;
  QString loadFileData(const QString &AFileName, const QString &DefValue) const;
  void loadTemplates();
public:
  static QList<QString> styleVariants(const QString &AStylePath);
  static QMap<QString, QVariant> styleInfo(const QString &AStylePath);
private:
  QString FStatusHTML;
  QString FIn_ContentHTML;
  QString FIn_NextContentHTML;
  QString FIn_ContextHTML;
  QString FIn_NextContextHTML;
  QString FIn_ActionHTML;
  QString FOut_ContentHTML;
  QString FOut_NextContentHTML;
  QString FOut_ContextHTML;
  QString FOut_NextContextHTML;
  QString FOut_ActionHTML;
private:
  QString FResourcePath;
  QList<QString> FVariants;
  QMap<QString, QVariant> FInfo;
};

#endif // MESSAGESTYLE_H
