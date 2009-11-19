#ifndef BITSOFBINARY_H
#define BITSOFBINARY_H

#include <QDir>
#include <QMap>
#include <definations/namespaces.h>
#include <definations/stanzahandlerorders.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/ibitsofbinary.h>
#include <interfaces/istanzaprocessor.h>
#include <utils/errorhandler.h>
#include <utils/stanza.h>

class BitsOfBinary : 
  public QObject,
  public IPlugin,
  public IBitsOfBinary,
  public IStanzaHandler,
  public IStanzaRequestOwner
{
  Q_OBJECT;
  Q_INTERFACES(IPlugin IBitsOfBinary IStanzaHandler IStanzaRequestOwner);
public:
  BitsOfBinary();
  ~BitsOfBinary();
  //IPlugin
  virtual QObject *instance() { return this; }
  virtual QUuid pluginUuid() const { return BITSOFBINARY_UUID; }
  virtual void pluginInfo(IPluginInfo *APluginInfo);
  virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
  virtual bool initObjects();
  virtual bool initSettings();
  virtual bool startPlugin() { return true; }
  //IStanzaHandler
  virtual bool stanzaEdit(int AHandleId, const Jid &AStreamJid, Stanza &AStanza, bool &AAccept);
  virtual bool stanzaRead(int AHandleId, const Jid &AStreamJid, const Stanza &AStanza, bool &AAccept);
  //IStanzaRequestOwner
  virtual void stanzaRequestResult(const Jid &AStreamJid, const Stanza &AStanza);
  virtual void stanzaRequestTimeout(const Jid &AStreamJid, const QString &AStanzaId);
  //IBitsOfBinary
  virtual QString contentIdentifier(const QByteArray &AData) const;
  virtual bool hasBinary(const QString &AContentId) const;
  virtual bool loadBinary(const QString &AContentId, const Jid &AStreamJid, const Jid &AContactJid);
  virtual bool loadBinary(const QString &AContentId, QString &AType, QByteArray &AData, quint64 &AMaxAge);
  virtual bool saveBinary(const QString &AContentId, const QString &AType, const QByteArray &AData, quint64 AMaxAge);
  virtual bool saveBinary(const QString &AContentId, const QString &AType, const QByteArray &AData, quint64 AMaxAge, Stanza &AStanza);
  virtual bool removeBinary(const QString &AContentId);
signals:
  virtual void binaryCached(const QString &AContentId, const QString &AType, const QByteArray &AData, quint64 AMaxAge);
  virtual void binaryError(const QString &AContentId, const QString &AError);
  virtual void binaryRemoved(const QString &AContentId);
protected:
  QString contentFileName(const QString &AContentId) const;
private:
  IPluginManager *FPluginManager;
  IStanzaProcessor *FStanzaProcessor;
private:
  int FSHIData;
  int FSHIRequest;
private:
  QDir FDataDir;
  QMap<QString, QString> FLoadRequests;
};

#endif // BITSOFBINARY_H
