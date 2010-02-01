#ifndef COMPRESSION_H
#define COMPRESSION_H

#include <definations/namespaces.h>
#include <definations/xmppdatahandlerorders.h>
#include <definations/xmppelementhandlerorders.h>
#include <interfaces/ixmppstreams.h>
#include "../../thirdparty/zlib/zlib.h"

class Compression : 
  public QObject,
  public IXmppFeature,
  public IXmppDataHandler,
  public IXmppElementHadler
{
  Q_OBJECT;
  Q_INTERFACES(IXmppFeature IXmppDataHandler IXmppElementHadler);
public:
  Compression(IXmppStream *AXmppStream);
  ~Compression();
  //IXmppDataHandler
  virtual bool xmppDataIn(IXmppStream *AXmppStream, QByteArray &AData, int AOrder);
  virtual bool xmppDataOut(IXmppStream *AXmppStream, QByteArray &AData, int AOrder);
  //IXmppElementHandler
  virtual bool xmppElementIn(IXmppStream *AXmppStream, QDomElement &AElem, int AOrder);
  virtual bool xmppElementOut(IXmppStream *AXmppStream, QDomElement &AElem, int AOrder);
  //IXmppFeature
  virtual QObject *instance() { return this; }
  virtual QString featureNS() const { return NS_FEATURE_COMPRESS; }
  virtual IXmppStream *xmppStream() const { return FXmppStream; }
  virtual bool start(const QDomElement &AElem); 
signals:
  void finished(bool ARestart); 
  void error(const QString &AError);
  void featureDestroyed();
protected:
  bool startZlib();
  void stopZlib();
  void processData(QByteArray &AData, bool ADataOut);
private: 
  IXmppStream *FXmppStream;
private:
  bool FZlibInited;
  z_stream FDefStruc;
  z_stream FInfStruc;
  QByteArray FOutBuffer;
};

#endif // COMPRESSION_H
