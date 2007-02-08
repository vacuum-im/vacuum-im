#ifndef IXMPPSTREAMS_H
#define IXMPPSTREAMS_H

#include <QStringList>
#include <QByteArray>
#include <QTcpSocket>
#include "../../utils/jid.h"
#include "../../utils/stanza.h"

#define NS_JABBER_CLIENT		"jabber:client"
#define NS_JABBER_STREAMS		"http://etherx.jabber.org/streams"
#define NS_XMPP_STREAMS			"urn:ietf:params:xml:ns:xmpp-streams"


class IStreamConnection
{
public:
  virtual QObject *instance()=0;
  virtual const QTcpSocket &socket()=0;
  virtual void connectToHost(const QString &, qint16)=0;
  virtual qint64 write(const QByteArray &)=0;
  virtual QByteArray read(qint64)=0;
  virtual bool isValid() const=0;
  virtual bool isOpen() const=0;
  virtual void close()=0;
  virtual QStringList proxyTypes() const=0;
  virtual void setProxyType(int)=0;
  virtual int proxyType() const=0;
  virtual void setProxyHost(const QString &) =0;
  virtual QString proxyHost() const =0;
  virtual void setProxyPort(qint16) =0;
  virtual qint16 proxyPort() const =0;
  virtual void setProxyUsername(const QString &) =0;
  virtual QString proxyUsername() const =0;
  virtual void setProxyPassword(const QString &) =0;
  virtual QString proxyPassword() const =0;
  virtual void setPollServer(const QString &) =0;
  virtual QString pollServer() const =0;
  virtual qint64 bytesWriten() const =0;
  virtual qint64 bytesReaded() const =0;
signals:
  virtual void connected()=0;
  virtual void readyRead(qint64 bytes)=0;
  virtual void disconnected()=0;
  virtual void error(const QString &errString)=0;
};


class IXmppStream;

class IStreamFeature
{
public:
  enum Direction {
    DirectionIn,
    DirectionOut,
  };
public:
  virtual QObject *instance()=0;
  virtual QString name() const=0;
  virtual QString nsURI() const=0;
  virtual IXmppStream *stream() const =0;
  virtual bool start(const QDomElement &)=0; 
  virtual bool needHook(Direction) const=0;
  virtual bool hookData(QByteArray *,Direction)=0;
  virtual bool hookElement(QDomElement *,Direction)=0;
signals:
  virtual void finished(bool restartStream)=0; 
  virtual void error(const QString &errStr)=0;
};


class IStreamFeaturePlugin
{
public:
  virtual IStreamFeature *newInstance(IXmppStream *)=0;
};


class IXmppStream
{
public:
  virtual QObject *instance()=0;
  virtual bool isOpen() const=0;
  virtual qint64 sendStanza(const Stanza &stanza)=0;
  virtual void setJid(const Jid &)=0;
  virtual const Jid &jid() const=0;
  virtual QString streamId() const=0;
  virtual QString lastError() const=0;
  virtual void setHost(const QString &)=0;
  virtual QString host() const=0;
  virtual void setPort(const qint16)=0;
  virtual qint16 port() const=0;
  virtual void setPassword(const QString &)=0;
  virtual QString password() const=0;
  virtual void setDefaultLang(const QString &) =0;
  virtual QString defaultLang() const =0;
  virtual void setXmppVersion(const QString &) =0;
  virtual QString xmppVersion() const =0;
  virtual IStreamConnection *connection()=0;
  virtual bool setConnection(IStreamConnection *) =0;
  virtual void addFeature(IStreamFeature *)=0;
  virtual void removeFeature(IStreamFeature *)=0;
  virtual QList<IStreamFeature *> &features()=0;
public slots:
  virtual void open()=0;
  virtual void close()=0;
signals:
  virtual void opened(IXmppStream *)=0;
  virtual void element(IXmppStream *, const QDomElement &elem)=0;
  virtual void aboutToClose(IXmppStream *)=0;
  virtual void closed(IXmppStream *)=0;
  virtual void error(IXmppStream *, const QString &errStr)=0;
};


class IXmppStreams 
{
public:
  virtual QObject *instance()=0;
  virtual IXmppStream *newStream(const Jid &)=0;
  virtual bool addStream(IXmppStream *)=0;
  virtual IXmppStream *getStream(const Jid &) const=0;
  virtual const QList<IXmppStream *> &getStreams() const=0;
  virtual void removeStream(IXmppStream *)=0;
signals:
  virtual void added(IXmppStream *)=0;
  virtual void opened(IXmppStream *)=0;
  virtual void element(IXmppStream *, const QDomElement &elem)=0;
  virtual void aboutToClose(IXmppStream *)=0;
  virtual void closed(IXmppStream *)=0;
  virtual void error(IXmppStream *, const QString &errString)=0;
  virtual void removed(IXmppStream *)=0;
};


Q_DECLARE_INTERFACE(IStreamConnection,"Vacuum.Plugin.IStreamConnection/1.0")
Q_DECLARE_INTERFACE(IStreamFeature,"Vacuum.Plugin.IStreamFeature/1.0");
Q_DECLARE_INTERFACE(IStreamFeaturePlugin,"Vacuum.Plugin.IStreamFeaturePlugin/1.0");
Q_DECLARE_INTERFACE(IXmppStream, "Vacuum.Plugin.IXmppStream/1.0")
Q_DECLARE_INTERFACE(IXmppStreams,"Vacuum.Plugin.IXmppStreams/1.0")

#endif
