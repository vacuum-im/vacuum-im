#ifndef IINBANDSTREAMS_H
#define IINBANDSTREAMS_H

#include <QIODevice>
#include <interfaces/idatastreamsmanager.h>

#define INBANDSTREAMS_UUID    "{faaedbeb-5cfa-47fc-b9d9-7df611ea4ef0}"

class IInBandStream : 
  public IDataStreamSocket
{
public:
  enum DataStanzaType {
    StanzaIq,
    StanzaMessage
  };
public:
  virtual QIODevice *instance() =0;
  virtual int blockSize() const =0;
  virtual void setBlockSize(int ASize) =0;
  virtual int maximumBlockSize() const=0;
  virtual void setMaximumBlockSize(int ASize) =0;
  virtual int dataStanzaType() const =0;
  virtual void setDataStanzaType(int AType) =0;
protected:
  virtual void propertiesChanged() =0;
};

class IInBandStreams:
  public IDataStreamMethod
{
public:
  virtual QObject *instance() =0;
};

Q_DECLARE_INTERFACE(IInBandStream,"Vacuum.Plugin.IInBandStream/1.0")
Q_DECLARE_INTERFACE(IInBandStreams,"Vacuum.Plugin.IInBandStreams/1.0")

#endif // IINBANDSTREAMS_H
