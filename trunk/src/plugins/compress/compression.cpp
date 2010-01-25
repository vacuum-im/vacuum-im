#include "compression.h"

#define CHUNK 5120

Compression::Compression(IXmppStream *AXmppStream)
  : QObject(AXmppStream->instance())
{
  FNeedHook = false;
  FRequest = false;
  FCompress = false;
  FZlibInited = false;
  FXmppStream = AXmppStream;
  connect(FXmppStream->instance(),SIGNAL(closed()), SLOT(onStreamClosed()));
}

Compression::~Compression()
{

}

bool Compression::start(const QDomElement &AElem)
{
  if (AElem.tagName() == "compression")
  {
    FNeedHook = false;
    QDomElement elem = AElem.firstChildElement("method");
    while (!elem.isNull())
    {
      if (elem.text() == "zlib")
      {
        if (startZlib())
        {
          Stanza compress("compress");
          compress.setAttribute("xmlns",NS_PROTOCOL_COMPRESS);
          compress.addElement("method").appendChild(compress.createTextNode("zlib"));
          FXmppStream->sendStanza(compress);   
          FNeedHook = true;
          FRequest = true;
          return true;
        }
        return false;
      }
      elem = elem.nextSiblingElement("method");
    }
  }
  return false;
}

bool Compression::hookData(QByteArray &AData, Direction ADirection)
{
  if (FCompress && AData.size()>0)
  {
    z_streamp zstream;
    int (*zfunc) OF((z_streamp strm, int flush));
    if (ADirection == IStreamFeature::DirectionOut)
    {
      zstream = &FDefStruc;
      zfunc = deflate;
    }
    else
    {
      zstream = &FInfStruc;
      zfunc = inflate;
    }

    int ret;
    int dataPosOut = 0;
    zstream->avail_in = AData.size();
    zstream->next_in = (Bytef *)(AData.constData());
    do 
    {
      zstream->avail_out = FOutBuffer.capacity() - dataPosOut;
      zstream->next_out = (Bytef *)(FOutBuffer.data() + dataPosOut);
      ret = zfunc(zstream,Z_SYNC_FLUSH);
      switch (ret)
      {
      case Z_OK:
        dataPosOut = FOutBuffer.capacity() - zstream->avail_out;
        if (zstream->avail_out == 0)
          FOutBuffer.reserve(FOutBuffer.capacity() + CHUNK);
        break;
      case Z_STREAM_ERROR:
        emit error(tr("Invalid compression level"));
        break;
      case Z_DATA_ERROR:
        emit error(tr("invalid or incomplete deflate data"));
        break;
      case Z_MEM_ERROR:
        emit error(tr("Out of memory for Zlib"));
        break;
      case Z_VERSION_ERROR:
        emit error(tr("Zlib version mismatch!"));
        break;
      }
    } while(ret == Z_OK && zstream->avail_out == 0);
    AData.resize(dataPosOut);
    memcpy(AData.data(),FOutBuffer.constData(),dataPosOut);
  }
  return false;
}

bool Compression::hookElement(QDomElement &AElem, Direction ADirection)
{
  if (!FRequest || ADirection != DirectionIn || AElem.namespaceURI() != NS_PROTOCOL_COMPRESS)
    return false;

  FRequest = false;
  if (AElem.tagName() == "compressed")
  {
    FCompress = true;
    emit ready(true);
    return true;
  }
  else if (AElem.tagName() == "failure")
  {
    FNeedHook = false;
    FCompress = false;
    emit ready(false);
    return true;
  }
  return false;
}

bool Compression::startZlib()
{
  if (!FZlibInited)
  {
    FDefStruc.zalloc = Z_NULL;
    FDefStruc.zfree = Z_NULL;
    FDefStruc.opaque = Z_NULL;
    int retDef = deflateInit(&FDefStruc,Z_BEST_COMPRESSION);

    FInfStruc.zalloc = Z_NULL;
    FInfStruc.zfree = Z_NULL;
    FInfStruc.opaque = Z_NULL;
    FInfStruc.avail_in = 0;
    FInfStruc.next_in = Z_NULL;
    int retInf = inflateInit(&FInfStruc);

    if (retInf == Z_OK && retDef == Z_OK)
    {
      FZlibInited = true;
      FOutBuffer.reserve(CHUNK);
    }
    else
    {
      if (retDef == Z_OK)
        deflateEnd(&FDefStruc);
      if (retInf == Z_OK)
        inflateEnd(&FInfStruc);
    }
  }
  return FZlibInited;
}

void Compression::stopZlib()
{
  if (FZlibInited)
  {
    deflateEnd(&FDefStruc);
    inflateEnd(&FInfStruc);
    FOutBuffer.squeeze();
    FZlibInited = false;
  }
}

void Compression::onStreamClosed()
{
  FNeedHook = false;
  FCompress = false;
  stopZlib();
}

