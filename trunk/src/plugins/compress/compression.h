#ifndef COMPRESSION_H
#define COMPRESSION_H

#include <definations/namespaces.h>
#include <interfaces/ixmppstreams.h>
#include "../../thirdparty/zlib/zlib.h"

class Compression : 
  public QObject,
  public IStreamFeature
{
  Q_OBJECT;
  Q_INTERFACES(IStreamFeature);
public:
  Compression(IXmppStream *AXmppStream);
  ~Compression();
  virtual QObject *instance() { return this; }
  virtual QString featureNS() const { return NS_FEATURE_COMPRESS; }
  virtual IXmppStream *xmppStream() const { return FXmppStream; }
  virtual bool start(const QDomElement &AElem); 
  virtual bool needHook(Direction /*ADirection*/) const { return FNeedHook; }
  virtual bool hookData(QByteArray &AData, Direction ADirection);
  virtual bool hookElement(QDomElement &AElem, Direction ADirection);
signals:
  void ready(bool ARestart); 
  void error(const QString &AError);
protected:
  bool startZlib();
  void stopZlib();
protected slots:
  void onStreamClosed();
private: 
  IXmppStream *FXmppStream;
private:
  bool FNeedHook;
  bool FRequest;
  bool FCompress;
  bool FZlibInited;
  z_stream FDefStruc;
  z_stream FInfStruc;
  QByteArray FOutBuffer;
};

#endif // COMPRESSION_H
