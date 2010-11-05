#ifndef DEF_SHORTCUTS_H
#define DEF_SHORTCUTS_H

//PluginManager
#define SCTG_APPLICATION                                           "application"
#define   SCT_APP_ABOUTQT                                          "application.about-qt"
#define   SCT_APP_ABOUTPROGRAM                                     "application.about-program"
#define   SCT_APP_SETUPPLUGINS                                     "application.setup-plugins"
//OptionsManager
#define   SCT_APP_SHOWOPTIONS                                      "application.show-options"
#define   SCT_APP_CHANGEPROFILE                                    "application.change-profile"
//Notifications
#define   SCT_APP_TOGGLESOUND                                      "application.toggle-sound"
#define   SCT_APP_ACTIVATENOTIFICATIONS                            "application.activate-notifications"
#define   SCT_APP_REMOVENOTIFICATIONS                              "application.remove-notifications"
//MainWindow
#define   SCT_APP_SHOWROSTER                                       "application.show-roster"
//FileStreamsManager
#define   SCT_APP_SHOWFILETRANSFERS                                "application.show-filetransfers"
//MultiUserChat
#define   SCT_APP_MUCJOIN                                          "application.muc-join"
#define   SCT_APP_MUC_SHOWHIDDEN                                   "application.muc-show-hidden"
#define   SCT_APP_MUC_LEAVEHIDDEN                                  "application.muc-leave-hidden"

//MainWindow
#define SCTG_MAINWINDOW                                            "main-window"
#define   SCT_MAINWINDOW_HIDEROSTER                                "main-window.hide-roster"
//RostersView
#define   SCT_MAINWINDOW_TOGGLEOFFLINE                             "main-window.toggle-offline"

//MessageWidgets
#define SCTG_MESSAGEWINDOWS                                        "message-windows"
#define   SCT_MESSAGEWINDOWS_QUOTE                                 "message-windows.quote"
//FileTransfer
#define   SCT_MESSAGEWINDOWS_SENDFILE                              "message-windows.sendfile"
//MessageArchiver
#define   SCT_MESSAGEWINDOWS_HISTORYENABLE                         "message-windows.history-enable"
#define   SCT_MESSAGEWINDOWS_HISTORYDISABLE                        "message-windows.history-disable"
#define   SCT_MESSAGEWINDOWS_HISTORYREQUIREOTR                     "message-windows.history-require-otr"
#define   SCT_MESSAGEWINDOWS_HISTORYTERMINATEOTR                   "message-windows.history-terminate-otr"
#define   SCT_MESSAGEWINDOWS_SHOWHISTORY                           "message-windows.show-history"
//vCard
#define   SCT_MESSAGEWINDOWS_SHOWVCARD                             "message-windows.show-vcard"
//MessageWidgets
#define   SCTG_MESSAGEWINDOWS_CHAT                                 "message-windows.chat-window"
//MessageWidgets
#define   SCTG_MESSAGEWINDOWS_NORMAL                               "message-windows.normal-window"
//MultiUserChat
#define   SCTG_MESSAGEWINDOWS_MUC                                  "message-windows.muc-window"
#define     SCT_MESSAGEWINDOWS_MUC_CLEARWINDOW                     "message-windows.muc-window.clear-window"
#define     SCT_MESSAGEWINDOWS_MUC_CHANGENICK                      "message-windows.muc-window.change-nick"
#define     SCT_MESSAGEWINDOWS_MUC_CHANGETOPIC                     "message-windows.muc-window.change-topic"
#define     SCT_MESSAGEWINDOWS_MUC_ROOMSETTINGS                    "message-windows.muc-window.room-settings"
#define     SCT_MESSAGEWINDOWS_MUC_ENTER                           "message-windows.muc-window.enter"
#define     SCT_MESSAGEWINDOWS_MUC_EXIT                            "message-windows.muc-window.exit"
//Bookmarks
#define     SCT_MESSAGEWINDOWS_MUC_BOOKMARK                        "message-windows.muc-window.bookmark"
//MessageArchive
#define   SCTG_MESSAGEWINDOWS_HISTORY                              "message-windows.history-window"

//ServiceDiscovery
#define SCTG_DISCOWINDOW                                           "discovery-window"
#define   SCT_DISCOWINDOW_BACK                                     "discovery-window.back"
#define   SCT_DISCOWINDOW_FORWARD                                  "discovery-window.forward"
#define   SCT_DISCOWINDOW_DISCOVER                                 "discovery-window.discover"
#define   SCT_DISCOWINDOW_RELOAD                                   "discovery-window.reload"
#define   SCT_DISCOWINDOW_SHOWDISCOINFO                            "discovery-window.show-disco-info"
#define   SCT_DISCOWINDOW_ADDCONTACT                               "discovery-window.add-contact"
#define   SCT_DISCOWINDOW_SHOWVCARD                                "discovery-window.show-vcard"

//MessageArchive
#define SCTG_HISTORYWINDOW                                         "history-window"
#define   SCT_HISTORYWINDOW_GROUPNONE                              "history-window.group-none"
#define   SCT_HISTORYWINDOW_GROUPBYDATE                            "history-window.group-by-date"
#define   SCT_HISTORYWINDOW_GROUPBYCONTACT                         "history-window.group-by-contact"
#define   SCT_HISTORYWINDOW_GROUPBYDATECONTACT                     "history-window.group-by-date-contact"
#define   SCT_HISTORYWINDOW_GROUPBYCONTACTDATE                     "history-window.group-by-contact-date"
#define   SCT_HISTORYWINDOW_EXPANDALL                              "history-window.expand-all"
#define   SCT_HISTORYWINDOW_COLLAPSEALL                            "history-window.collapse-all"
#define   SCT_HISTORYWINDOW_SOURCEAUTO                             "history-window.source-auto"
#define   SCT_HISTORYWINDOW_SOURCELOCAL                            "history-window.source-local"
#define   SCT_HISTORYWINDOW_SOURCESERVER                           "history-window.source-server"
#define   SCT_HISTORYWINDOW_FILTERBYCONTACT                        "history-window.filter-by-contact"
#define   SCT_HISTORYWINDOW_RENAMECOLLECTION                       "history-window.rename-collection"
#define   SCT_HISTORYWINDOW_REMOVECOLLECTION                       "history-window.remove-collection"
#define   SCT_HISTORYWINDOW_RELOADCOLLECTIONS                      "history-window.reload-collections"

//RosterView
#define SCTG_ROSTERVIEW                                            "roster-view"

//MessageWidgets
#define SCTG_TABWINDOW                                             "tab-window"
#define   SCT_TABWINDOW_NEXTTAB                                    "tab-window.next-tab"
#define   SCT_TABWINDOW_PREVTAB                                    "tab-window.prev-tab"
#define   SCT_TABWINDOW_CLOSETAB                                   "tab-window.close-tab"
#define   SCT_TABWINDOW_DETACHTAB                                  "tab-window.detach-tab"
#define   SCT_TABWINDOW_SHOWCLOSEBUTTTONS							             "tab-window.show-close-buttons"
#define   SCT_TABWINDOW_TABSBOTTOM                                 "tab-window.show-tabs-at-bottom"
#define   SCT_TABWINDOW_SETASDEFAULT                               "tab-window.set-as-default"
#define   SCT_TABWINDOW_RENAMEWINDOW                               "tab-window.rename-window"
#define   SCT_TABWINDOW_DELETEWINDOW                               "tab-window.delete-window"
#define   SCT_TABWINDOW_QUICKTAB                                   "tab-window.quick-tabs.tab%1"

#endif // DEF_SHORTCUTS_H
