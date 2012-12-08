#ifndef DEF_SHORTCUTS_H
#define DEF_SHORTCUTS_H

//PluginManager
#define SCTG_GLOBAL                                                "global"
//MainWindow
#define   SCT_GLOBAL_SHOWROSTER                                    "global.show-roster"
//ShortcutManager
#define   SCT_GLOBAL_HIDEALLWIDGETS                                "global.hide-all-widgets"
//Notifications
#define   SCT_GLOBAL_TOGGLESOUND                                   "global.toggle-sound"
#define   SCT_GLOBAL_ACTIVATELASTNOTIFICATION                      "global.activate-last-notification"
#define   SCT_GLOBAL_REMOVEALLNOTIFICATIONS                        "global.remove-all-lnotifications"

//PluginManager
#define SCTG_APPLICATION                                           "application"
#define   SCT_APP_ABOUTQT                                          "application.about-qt"
#define   SCT_APP_ABOUTPROGRAM                                     "application.about-program"
#define   SCT_APP_SETUPPLUGINS                                     "application.setup-plugins"
//OptionsManager
#define   SCT_APP_SHOWOPTIONS                                      "application.show-options"
#define   SCT_APP_CHANGEPROFILE                                    "application.change-profile"
//FileStreamsManager
#define   SCT_APP_SHOWFILETRANSFERS                                "application.show-filetransfers"
//MultiUserChat
#define   SCT_APP_MUCJOIN                                          "application.muc-join"
#define   SCT_APP_MUCSHOWHIDDEN                                    "application.muc-show-hidden"
#define   SCT_APP_MUCLEAVEHIDDEN                                   "application.muc-leave-hidden"

//MainWindow
#define SCTG_MAINWINDOW                                            "main-window"
#define   SCT_MAINWINDOW_CLOSEWINDOW                               "main-window.close-window"
//RostersView
#define   SCT_MAINWINDOW_TOGGLEOFFLINE                             "main-window.toggle-offline"
//MessageWidgets
#define   SCT_MAINWINDOW_COMBINEWITHMESSAGES                       "main-window.combine-with-messages"

//MessageWidgets
#define SCTG_MESSAGEWINDOWS                                        "message-windows"
#define   SCT_MESSAGEWINDOWS_QUOTE                                 "message-windows.quote"
#define   SCT_MESSAGEWINDOWS_CLOSEWINDOW                           "message-windows.close-window"
#define   SCT_MESSAGEWINDOWS_EDITPREVMESSAGE                       "message-windows.edit-prev-message"
#define   SCT_MESSAGEWINDOWS_EDITNEXTMESSAGE                       "message-windows.edit-next-message"
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
#define     SCT_MESSAGEWINDOWS_CHAT_SENDMESSAGE                    "message-windows.chat-window.send-message"
//ChatMessageHandler
#define     SCT_MESSAGEWINDOWS_CHAT_CLEARWINDOW                    "message-windows.chat-window.clear-window"
//MessageWidgets
#define   SCTG_MESSAGEWINDOWS_NORMAL                               "message-windows.normal-window"
#define     SCT_MESSAGEWINDOWS_NORMAL_SENDMESSAGE                  "message-windows.normal-window.send-message"
//MultiUserChat
#define   SCTG_MESSAGEWINDOWS_MUC                                  "message-windows.muc-window"
#define     SCT_MESSAGEWINDOWS_MUC_SENDMESSAGE                     "message-windows.muc-window.send-message"
#define     SCT_MESSAGEWINDOWS_MUC_CLEARWINDOW                     "message-windows.muc-window.clear-window"
#define     SCT_MESSAGEWINDOWS_MUC_CHANGENICK                      "message-windows.muc-window.change-nick"
#define     SCT_MESSAGEWINDOWS_MUC_CHANGETOPIC                     "message-windows.muc-window.change-topic"
#define     SCT_MESSAGEWINDOWS_MUC_ROOMSETTINGS                    "message-windows.muc-window.room-settings"
#define     SCT_MESSAGEWINDOWS_MUC_ENTER                           "message-windows.muc-window.enter"
#define     SCT_MESSAGEWINDOWS_MUC_EXIT                            "message-windows.muc-window.exit"
//Bookmarks
#define     SCT_MESSAGEWINDOWS_MUC_BOOKMARK                        "message-windows.muc-window.bookmark"

//MessageWidgets
#define SCTG_TABWINDOW                                             "tab-window"
#define   SCT_TABWINDOW_NEXTTAB                                    "tab-window.next-tab"
#define   SCT_TABWINDOW_PREVTAB                                    "tab-window.prev-tab"
#define   SCT_TABWINDOW_CLOSETAB                                   "tab-window.close-tab"
#define   SCT_TABWINDOW_CLOSEOTHERTABS                             "tab-window.close-other-tabs"
#define   SCT_TABWINDOW_DETACHTAB                                  "tab-window.detach-tab"
#define   SCT_TABWINDOW_SHOWCLOSEBUTTTONS                          "tab-window.show-close-buttons"
#define   SCT_TABWINDOW_TABSBOTTOM                                 "tab-window.show-tabs-at-bottom"
#define   SCT_TABWINDOW_TABSINDICES                                "tab-window.show-tabs-indices"
#define   SCT_TABWINDOW_SETASDEFAULT                               "tab-window.set-as-default"
#define   SCT_TABWINDOW_RENAMEWINDOW                               "tab-window.rename-window"
#define   SCT_TABWINDOW_CLOSEWINDOW                                "tab-window.close-window"
#define   SCT_TABWINDOW_DELETEWINDOW                               "tab-window.delete-window"
#define   SCT_TABWINDOW_QUICKTAB                                   "tab-window.quick-tabs.tab%1"

//ServiceDiscovery
#define SCTG_DISCOWINDOW                                           "discovery-window"
#define   SCT_DISCOWINDOW_BACK                                     "discovery-window.back"
#define   SCT_DISCOWINDOW_FORWARD                                  "discovery-window.forward"
#define   SCT_DISCOWINDOW_DISCOVER                                 "discovery-window.discover"
#define   SCT_DISCOWINDOW_RELOAD                                   "discovery-window.reload"
#define   SCT_DISCOWINDOW_SHOWDISCOINFO                            "discovery-window.show-disco-info"
#define   SCT_DISCOWINDOW_ADDCONTACT                               "discovery-window.add-contact"
#define   SCT_DISCOWINDOW_SHOWVCARD                                "discovery-window.show-vcard"
#define   SCT_DISCOWINDOW_CLOSEWINDOW                              "discovery-window.close-window"

//RosterView
#define SCTG_ROSTERVIEW                                            "roster-view"
#define   SCT_ROSTERVIEW_COPYJID                                   "roster-view.copy-jid"
#define   SCT_ROSTERVIEW_COPYNAME                                  "roster-view.copy-name"
#define   SCT_ROSTERVIEW_COPYSTATUS                                "roster-view.copy-status"
//ChatMessageHandler
#define   SCT_ROSTERVIEW_SHOWCHATDIALOG                            "roster-view.show-chat-dialog"
//NormalMessageHandler
#define   SCT_ROSTERVIEW_SHOWNORMALDIALOG                          "roster-view.show-normal-dialog"
//RosterChanger
#define   SCT_ROSTERVIEW_ADDCONTACT                                "roster-view.add-contact"
#define   SCT_ROSTERVIEW_RENAME                                    "roster-view.rename"
#define   SCT_ROSTERVIEW_REMOVEFROMGROUP                           "roster-view.remove-from-group"
#define   SCT_ROSTERVIEW_REMOVEFROMROSTER                          "roster-view.remove-from-roster"
#define   SCT_ROSTERVIEW_SUBSCRIBE                                 "roster-view.subscribe"
#define   SCT_ROSTERVIEW_UNSUBSCRIBE                               "roster-view.unsubscribe"
//MessageArchiver
#define   SCT_ROSTERVIEW_SHOWHISTORY                               "roster-view.show-history"
//vCard
#define   SCT_ROSTERVIEW_SHOWVCARD                                 "roster-view.show-vcard"
//FileTransfer
#define   SCT_ROSTERVIEW_SENDFILE                                  "roster-view.send-file"
//Gateways
#define   SCT_ROSTERVIEW_GATELOGIN                                 "roster-view.gate-login"
#define   SCT_ROSTERVIEW_GATELOGOUT                                "roster-view.gate-logout"
//Annotations
#define   SCT_ROSTERVIEW_EDITANNOTATION                            "roster-view.edit-annotation"
//RecentContacts
#define   SCT_ROSTERVIEW_INSERTFAVORITE                            "roster-view.insert-favorite"
#define   SCT_ROSTERVIEW_REMOVEFAVORITE                            "roster-view.remove-favorite"

#endif // DEF_SHORTCUTS_H
