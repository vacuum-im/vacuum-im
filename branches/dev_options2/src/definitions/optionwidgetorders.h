#ifndef DEF_OPTIONWIDGETORDERS_H
#define DEF_OPTIONWIDGETORDERS_H

//Node = ON_ACCOUNTS
#define OHO_ACCOUNTS_ACCOUNTS                     100
#  define OWO_ACCOUNTS_ACCOUNTS                   110
#define OHO_ACCOUNTS_COMMON                       500
#  define OWO_ACCOUNTS_DEFAULTRESOURCE            530
#  define OWO_ACCOUNTS_DEFAULTPROXY               560

//Node = ON_ACCOUNTS.[id].Params
#define OHO_ACCOUNTS_PARAMS_ACCOUNT               100
#  define OWO_ACCOUNTS_PARAMS_NAME                110
#  define OWO_ACCOUNTS_PARAMS_RESOURCE            120
#  define OWO_ACCOUNTS_PARAMS_PASSWORD            130
#define OHO_ACCOUNTS_PARAMS_CONNECTION            300
#  define OWO_ACCOUNTS_PARAMS_CONNECTION          310

//Node = ON_ACCOUNTS.[id].History
#define OHO_ACCOUNTS_HISTORY_REPLICATION          300
#  define OWO_ACCOUNTS_HISTORY_REPLICATION        310
#  define OWO_ACCOUNTS_HISTORY_DUPLICATION        320
#define OHO_ACCOUNTS_HISTORY_SERVERSETTINGS       500
#  define OWO_ACCOUNTS_HISTORY_SERVERSETTINGS     510

//Node = ON_ACCOUNTS.[id].Additional
#define OHO_ACCOUNTS_ADDITIONAL_SETTINGS          100
#  define OWO_ACCOUNTS_ADDITIONAL_REQUIRESECURE   110
#  define OWO_ACCOUNTS_ADDITIONAL_AUTOCONNECT     170
#  define OWO_ACCOUNTS_ADDITIONAL_AUTORECONNECT   171
#  define OWO_ACCOUNTS_ADDITIONAL_STREAMCOMPRESS  180
#define OHO_ACCOUNTS_ADDITIONAL_CONFERENCES       500
#  define OWO_ACCOUNTS_ADDITIONAL_DISABLEAUTOJOIN 550

//Node = ON_STATUSICONS
#define OWO_STATUSICONS                           500

//Node = ON_ROSTER
#define OWO_ROSTER                                500
#define OWO_ROSTER_AVATARS                        800
#define OWO_ROSTER_CHENGER                        900
#define OWO_ROSTER_EXCHANGE                       1200

//Node = ON_EMOTICANS
#define OWO_EMOTICONS                             500

//Node = ON_HISTORY
#define OWO_HISTORY_ENGINES                       500

//Node = ON_HISTORY::<AccountId>
#define OWO_HISTORY_STREAM                        500

//Node = ON_NOTIFICATIONS
#define OWO_NOTIFICATIONS_EXTENDED                300
#define OWO_NOTIFICATIONS_COMMON                  500

//Node = ON_MISC
#define OWO_MISC_CLIENTINFO                       300
#define OWO_MISC_AUTOSTART                        500
#define OWO_MISC_VCARD                            550
#define OWO_MISC_STATISTICS                       600
#define OWO_MISC_DEFAULTPROXY                     700

//Node = ON_MESSAGES
#define OWO_MESSAGES                              300
#define OWO_MESSAGES_LOADHISTORY                  600
#define OWO_MESSAGES_SHOWDATESEPARATORS           600
#define OWO_MESSAGES_CHATSTATES                   800
#define OWO_MESSAGES_UNNOTIFYALLNORMAL            900
#define OHO_MESSAGES_HISORYENGINES                1000
#  define OWO_MESSAGES_HISORYENGINE               1010
#define OHO_MESSAGES_HISTORYENGINESETTINGS        1100
#  define OWO_MESSAGES_HISTORYENGINESETTINGS      1110

//Node = ON_MESSAGE_STYLES
#define OWO_MESSAGE_STYLES                        500

//Node = ON_DATASTREAMS
#define OWO_DATASTREAMS                           500

//Node = ON_FILETRANSFER
#define OWO_FILESTREAMSMANAGER                    500
#define OWO_FILETRANSFER                          600

//Node = ON_AUTOSTATUS
#define OWO_AUTOSTATUS                            500

//Node = ON_CONFERENCES
#define OWO_CONFERENCES                           500
#define OWO_CONFERENCES_SHOWAUTOJOINED            700

//Node = ON_SHORTCUTS
#define OWO_SHORTCUTS                             500

#endif
