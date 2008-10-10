#ifndef UTILS_EXPORT
#include <qglobal.h>
# if defined Q_WS_WIN
#   ifdef UTILS_DLL
#     define UTILS_EXPORT __declspec(dllexport)  //Export to dll
#   else
#     define UTILS_EXPORT __declspec(dllimport)  //Import from dll
#   endif
# else
#   define UTILS_EXPORT   //Linux
# endif
#endif //UTILSEXPORT_H
