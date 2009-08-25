#ifndef IINBANDSTREAMS_H
#define IINBANDSTREAMS_H

#include <QIODevice>
class IDataStreamSocket;
class IDataStreamMethod;

#define INBANDSTREAMS_UUID    "{faaedbeb-5cfa-47fc-b9d9-7df611ea4ef0}"

class IInBandStream
{
public:
  virtual QObject *instance() =0;
};

class IInBandStreams
{
public:
  virtual QObject *instance() =0;
};

Q_DECLARE_INTERFACE(IInBandStream,"Vacuum.Plugin.IInBandStream/1.0")
Q_DECLARE_INTERFACE(IInBandStreams,"Vacuum.Plugin.IInBandStreams/1.0")

#endif // IINBANDSTREAMS_H
