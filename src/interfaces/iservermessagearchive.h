#ifndef ISERVERMESSAGEARCHIVE_H
#define ISERVERMESSAGEARCHIVE_H

#include <interfaces/imessagearchiver.h>

#define SERVERMESSAGEARCHIVE_UUID "{5309B204-651E-4cc8-9993-6A50D66301AA}"

class IServerMesssageArchive :
	public IArchiveEngine
{
public:
	virtual QObject *instance() =0;
};

Q_DECLARE_INTERFACE(IServerMesssageArchive,"Vacuum.Plugin.IServerMesssageArchive/1.2")

#endif //ISERVERMESSAGEARCHIVE_H
