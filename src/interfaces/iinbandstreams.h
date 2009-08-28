#ifndef IINBANDSTREAMS_H
#define IINBANDSTREAMS_H

#include <QIODevice>

#define INBANDSTREAMS_UUID    "{faaedbeb-5cfa-47fc-b9d9-7df611ea4ef0}"

class IInBandStreams
{
public:
  virtual QObject *instance() =0;
};

Q_DECLARE_INTERFACE(IInBandStreams,"Vacuum.Plugin.IInBandStreams/1.0")

#endif // IINBANDSTREAMS_H
