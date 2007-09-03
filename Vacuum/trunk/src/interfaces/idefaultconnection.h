#ifndef IDEFAULTCONNECTION_H
#define IDEFAULTCONNECTION_H

#define DEFAULTCONNECTION_UUID "{68F9B5F2-5898-43f8-9DD1-19F37E9779AC}"

class IDefaultConnection
{
public:
  enum Options {
    CO_Host,
    CO_Port,
    CO_ProxyType,
    CO_ProxyHost,
    CO_ProxyPort,
    CO_ProxyUserName,
    CO_ProxyPassword,
    CO_UserOptions = 100
  };

  virtual QObject *instance() =0;
};

Q_DECLARE_INTERFACE(IDefaultConnection,"Vacuum.Plugin.IDefaultConnection/1.0")

#endif