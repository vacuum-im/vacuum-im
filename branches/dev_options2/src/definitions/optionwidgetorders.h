#ifndef DEF_OPTIONWIDGETORDERS_H
#define DEF_OPTIONWIDGETORDERS_H

//Node = ON_COMMON
#define OHO_COMMON_SETTINGS                       100
#  define OWO_COMMON_AUTOSTART                    110
#  define OWO_COMMON_SENDSTATISTICS               120
#  define OWO_COMMON_SENDCLIENTINFO               130
#  define OWO_COMMON_VCARDIMAGE                   140
#define OHO_COMMON_LOCALIZATION                   300
#  define OWO_COMMON_LANGUAGE                     310

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

//Node = ON_ROSTERVIEW
#define OHO_ROSTER_VIEW                           100
#define   OWO_ROSTER_SHOWOFFLINE                  110
#define   OWO_ROSTER_MERGESTREAMS                 120
#define   OWO_ROSTER_SHOWRESOURCE                 130
#define   OWO_ROSTER_HIDESCROLLBAR                140
#define   OWO_ROSTER_VIEWMODE                     150
#define   OWO_ROSTER_SORTMODE                     160
#define OHO_ROSTER_MANAGEMENT                     300
#define   OWO_ROSTER_AUTOSUBSCRIBE                310
#define   OWO_ROSTER_AUTOUNSUBSCRIBE              320
#define   OWO_ROSTER_EXCHANGEAUTO                 330

//Node = ON_MESSAGES
#define OHO_MESSAGES_VIEW                         100
#define   OWO_MESSAGES_COMBINEWITHROSTER          110
#define   OWO_MESSAGES_TABWINDOWSENABLE           120
#define   OWO_MESSAGES_EDITORAUTORESIZE           130
#define   OWO_MESSAGES_EDITORMINIMUMLINES         140
#define OHO_MESSAGES_BEHAVIOR                     300
#define   OWO_MESSAGES_LOADHISTORY                310
#define   OWO_MESSAGES_SHOWSTATUS                 320
#define   OWO_MESSAGES_ARCHIVESTATUS              330
#define   OWO_MESSAGES_SHOWDATESEPARATORS         340
#define   OWO_MESSAGES_CHATSTATESENABLED          360
#define   OWO_MESSAGES_UNNOTIFYALLNORMAL          370
#define OHO_MESSAGES_CONFERENCES                  500
#define   OWO_MESSAGES_MUC_SHOWENTERS             510
#define   OWO_MESSAGES_MUC_SHOWSTATUS             520
#define   OWO_MESSAGES_MUC_ARCHIVESTATUS          530
#define   OWO_MESSAGES_MUC_QUITONWINDOWCLOSE      540
#define   OWO_MESSAGES_MUC_REJOINAFTERKICK        550
#define   OWO_MESSAGES_MUC_REFERENUMERATION       560
#define   OWO_MESSAGES_MUC_SHOWAUTOJOINED         570

// ON_HISTORY
#define OHO_HISORY_ENGINES                        300
#define   OWO_HISORY_ENGINE                       310
#define OHO_HISTORY_ENGINNAME                     500
#define   OWO_HISTORY_ENGINESETTINGS              505

//Node = ON_STATUSITEMS
#define OHO_AUTOSTATUS                            100
#define   OWO_AUTOSTATUS                          150
#define OHO_STATUS_ITEMS                          300
#define   OWO_STATUS_ITEMS                        350

//Node = ON_NOTIFICATIONS
#define OHO_NOTIFICATIONS                         100
#define   OWO_NOTIFICATIONS_DISABLEIFAWAY         110
#define   OWO_NOTIFICATIONS_DISABLEIFDND          120
#define   OWO_NOTIFICATIONS_NATIVEPOPUPS          130
#define   OWO_NOTIFICATIONS_FORCESOUND            140
#define   OWO_NOTIFICATIONS_HIDEMESSAGE           150
#define   OWO_NOTIFICATIONS_EXPANDGROUPS          160
#define   OWO_NOTIFICATIONS_SOUNDCOMMAND          170
#define   OWO_NOTIFICATIONS_POPUPTIMEOUT          180
#define OHO_NOTIFICATIONS_KINDS                   500
#define   OWO_NOTIFICATIONS_ALERTWINDOW           510
#define   OWO_NOTIFICATIONS_KINDS                 590

//Node = ON_SHORTCUTS
#define OHO_SHORTCUTS                             500
#define   OWO_SHORTCUTS                           510

//Node = ON_DATATRANSFER
#define OHO_DATATRANSFER_FILETRANSFER             100
#define   OWO_DATATRANSFER_FILESTREAMS            110
#define   OWO_DATATRANSFER_GROUPBYSENDER          120
#define   OWO_DATATRANSFER_AUTORECEIVE            130
#define   OWO_DATATRANSFER_HIDEONSTART            140
#define   OWO_DATATRANSFER_DEFAULTMETHOD          150
#define OHO_DATATRANSFER_METHODNAME               500
#define   OWO_DATATRANSFER_METHODSETTINGS         505

//Node = OPN_APPEARANCE
#define OHO_APPEARANCE_MESSAGESTYLE               200
#define   OWO_APPEARANCE_MESSAGETYPESTYLE         201
#define OHO_APPEARANCE_ROSTER                     400
#define   OWO_APPEARANCE_STATUSICONS              430
#define OHO_APPEARANCE_MESSAGES                   700
#define   OWO_APPEARANCE_EMOTICONS                730

#endif // DEF_OPTIONWIDGETORDERS_H
