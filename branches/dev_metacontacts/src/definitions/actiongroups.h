#ifndef DEF_ACTIONGROUPS_H
#define DEF_ACTIONGROUPS_H

//MainWindow  - MainMenu
#define AG_MMENU_OPTIONS                                        500
#define AG_MMENU_ACCOUNTMANAGER                                 500
#define AG_MMENU_SKINMANAGER                                    500
#define AG_MMENU_CONSOLE                                        500
#define AG_MMENU_FILESTREAMSMANAGER                             500
#define AG_MMENU_PLUGINMANAGER_SETUP                            500
#define AG_MMENU_PLUGINMANAGER_ABOUT                            900
#define AG_MMENU_MAINWINDOW                                     1000

//RostersView - ContextMenu
#define AG_RVCM_ROSTERSVIEW_STREAMS                             100
#define AG_RVCM_CHATMESSAGEHANDLER                              200
#define AG_RVCM_NORMALMESSAGEHANDLER                            200
#define AG_RVCM_MULTIUSERCHAT_OPEN                              200
#define AG_RVCM_STATUSCHANGER                                   300
#define AG_RVCM_RCHANGER_ADD_CONTACT                            300
#define AG_RVCM_GATEWAYS_ADD_LEGACY_USER                        300
#define AG_RVCM_BOOKMARS_MENU                                   350
#define AG_RVCM_CLIENTINFO                                      400
#define AG_RVCM_DISCOVERY_FEATURES                              400
#define AG_RVCM_MULTIUSERCHAT_JOIN                              500
#define AG_RVCM_AVATARS                                         500
#define AG_RVCM_ANNOTATIONS                                     500
#define AG_RVCM_ARCHIVER                                        500
#define AG_RVCM_DISCOVERY                                       500
#define AG_RVCM_ROSTERSVIEW_CLIPBOARD                           500
#define AG_RVCM_PRIVACYLISTS                                    500
#define AG_RVCM_STATUSICONS                                     500
#define AG_RVCM_RECENT_FAVORITES                                600
#define AG_RVCM_BOOKMARS_TOOLS                                  600
#define AG_RVCM_MULTIUSERCHAT_COMMON                            650
#define AG_RVCM_ACCOUNTMANAGER                                  700
#define AG_RVCM_RCHANGER                                        700
#define AG_RVCM_MULTIUSERCHAT_TOOLS                             700
#define AG_RVCM_RECENT_OPTIONS                                  700
#define AG_RVCM_GATEWAYS_LOGIN                                  800
#define AG_RVCM_GATEWAYS_RESOLVE                                800
#define AG_RVCM_GATEWAYS_REMOVE                                 800
#define AG_RVCM_VCARD                                           900
#define AG_RVCM_MULTIUSERCHAT_EXIT                              1000

//RostersView - ClipboardMenu
#define AG_RVCBM_NAME                                           100
#define AG_RVCBM_JABBERID                                       200
#define AG_RVCBM_STATUS                                         300
#define AG_RVCBM_ANNOTATION                                     400
#define AG_RVCBM_MUC_SUBJECT                                    500

//TrayManager - TrayMenu
#define AG_TMTM_NOTIFICATIONS_LAST                              50
#define AG_TMTM_MAINWINDOW                                      200
#define AG_TMTM_NOTIFICATIONS_MENU                              300
#define AG_TMTM_STATUSCHANGER                                   400
#define AG_TMTM_OPTIONS                                         500
#define AG_TMTM_DISCOVERY                                       500
#define AG_TMTM_FILESTREAMSMANAGER                              500
#define AG_TMTM_TRAYMANAGER                                     1000

//StatusChanger - StatusMenu, StreamMenu
#define AG_SCSM_STATUSCHANGER_ACTIONS                           200
#define AG_SCSM_STATUSCHANGER_STREAMS                           300
#define AG_SCSM_STATUSCHANGER_CUSTOM_STATUS                     400
#define AG_SCSM_STATUSCHANGER_DEFAULT_STATUS                    500

//MessageWidgets - TabWindowMenu
#define AG_MWTW_MWIDGETS_TAB_ACTIONS                            500
#define AG_MWTW_MWIDGETS_WINDOW_OPTIONS                         800

//MessageWidgets - TabWindowTabMenu
#define AG_MWTWTM_MWIDGETS_TAB_ACTIONS                          500

//MessageWidgets - ReceiversWidgetContextMenu
#define AG_MWRWCM_MWIDGETS_SELECT_ONLINE                        500
#define AG_MWRWCM_MWIDGETS_SELECT_NOTBUSY                       500
#define AG_MWRWCM_MWIDGETS_SELECT_ALL                           500
#define AG_MWRWCM_MWIDGETS_SELECT_CLEAR                         500
#define AG_MWRWCM_MWIDGETS_SELECT_LAST                          700
#define AG_MWRWCM_MWIDGETS_SELECT_LOAD                          700
#define AG_MWRWCM_MWIDGETS_SELECT_SAVE                          700
#define AG_MWRWCM_MWIDGETS_EXPAND_ALL                           800
#define AG_MWRWCM_MWIDGETS_COLLAPSE_ALL                         800
#define AG_MWRWCM_MWIDGETS_HIDE_OFFLINE                         900

//MultiuserChat - MultiUserContextMenu
#define AG_MUCM_MULTIUSERCHAT_UTILS                             100
#define AG_MUCM_MULTIUSERCHAT_PRIVATE                           200
#define AG_MUCM_DISCOVERY_FEATURES                              400
#define AG_MUCM_ROSTERCHANGER                                   500
#define AG_MUCM_CLIENTINFO                                      500
#define AG_MUCM_STATUSICONS                                     500
#define AG_MUCM_ARCHIVER                                        500
#define AG_MUCM_DISCOVERY                                       500
#define AG_MUCM_VCARD                                           900

//Bookmarks - BookmarksMenu
#define AG_BBM_BOOKMARKS_TOOLS                                  500
#define AG_BBM_BOOKMARKS_ITEMS                                  700

//ViewWidget - ContextMenu
#define AG_VWCM_MESSAGEWIDGETS_URL                              300
#define AG_VWCM_MESSAGEWIDGETS_COPY                             500
#define AG_VWCM_MESSAGEWIDGETS_QUOTE                            500
#define AG_VWCM_MESSAGEWIDGETS_SEARCH                           700

//EditWidget - ContextMenu
#define AG_EWCM_SPELLCHECKER_SUGGESTS                           100
#define AG_EWCM_MESSAGEWIDGETS_DEFAULT                          500
#define AG_EWCM_SPELLCHECKER_OPTIONS                            800

#endif
