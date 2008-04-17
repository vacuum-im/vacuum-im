#ifndef ACTIONGROUPS_H
#define ACTIONGROUPS_H

#define AG_NULL                                                 -1
#define AG_DEFAULT                                              500

//MainWindow (MainMwenu)
#define AG_ROSTERCHANGER_MMENU                                  500
#define AG_SETTINGS_MMENU                                       700
#define AG_ACCOUNTMANAGER_MMENU                                 700
#define AG_SKINMANAGER_MENU                                     700
#define AG_MAINWINDOW_MMENU_QUIT                                1000

//MainWindow (TopToolBar)
#define AG_ROSTERSVIEW_MWTTB                                    100
#define AG_DISCOVERY_MWTTB                                      200
#define AG_MULTIUSERCHAT_MWTTB                                  300
#define AG_BOOKMARKS_MWTTB                                      400

//RostersView (ContextMenu)
#define AG_MESSENGER                                            200
#define AG_STATUSCHANGER_ROSTER                                 300
#define AG_GATEWAYS_LOGIN_ROSTER                                350
#define AG_CLIENTINFO_ROSTER                                    400
#define AG_DISCOVERY_ROSTER_FEATURES                            400
#define AG_ROSTERCHANGER_ROSTER_ADD_CONTACT                     500
#define AG_ACCOUNTMANAGER_ROSTER                                500
#define AG_STATUSICONS_ROSTER                                   500
#define AG_VCARD_ROSTER                                         500
#define AG_DISCOVERY_ROSTER                                     500
#define AG_MULTIUSERCHAT_ROSTER                                 500
#define AG_GATEWAYS_RESOLVE_ROSTER                              500
#define AG_ROSTERCHANGER_ROSTER_SUBSCRIPTION                    700
#define AG_ROSTERCHANGER_ROSTER                                 700

//TrayManager (TrayMenu)
#define AG_STATUSCHANGER_TRAY                                   200
#define AG_ROSTERCHANGER_TRAY                                   500
#define AG_SETTINGS_TRAY                                        700
#define AG_SKINMANAGER_TRAY                                     700
#define AG_DISCOVERY_TRAY                                       700
#define AG_MULTIUSERCHAT_TRAY                                   700
#define AG_BOOKMARKS_TRAY                                       700
#define AG_TRAYMANAGER_TRAY_QUIT                                1000

//StatusChanger (StatusMenu, StreamMenu)
#define AG_STATUSCHANGER_STATUSMENU_STREAMS                     300
#define AG_STATUSCHANGER_STATUSMENU_CUSTOM_MENU                 400

//EditWidgetToolbar
#define AG_EMOTICONS_EDITWIDGET                                 500

//MultiUserContextMenu
#define AG_MUCM_ROSTERCHANGER                                   500
#define AG_MUCM_CLIENTINFO                                      500
#define AG_MUCM_VCARD                                           500
#define AG_MULTIUSERCHAT_ROOM_UTILS                             800

//MultiUserRoomMenu
#define AG_MURM_MULTIUSERCHAT                                   500
#define AG_MURM_BOOKMARKS                                       700
#define AG_MURM_MULTIUSERCHAT_EXIT                              1000

//MultiUserToolsMenu
#define AG_MUTM_MULTIUSERCHAT                                   500
#define AG_MUTM_MULTIUSERCHAT_DESTROY                           700

//DiscoItemsWindow (ToolBar)
#define AG_DIWT_DISCOVERY_NAVIGATE                              100
#define AG_DIWT_DISCOVERY_DEFACTIONS                            300
#define AG_DIWT_DISCOVERY_ACTIONS                               500
#define AG_DIWT_DISCOVERY_FEATURE_ACTIONS                       700

//BookmarksMenu
#define AG_BMM_BOOKMARKS_ITEMS                                  500
#define AG_BMM_BOOKMARKS_STREAMS                                600
#define AG_BBM_BOOKMARKS_TOOLS                                  700

//ArchiveWindow - Groups Tools
#define AG_AWGT_ARCHIVE_GROUPING                                200
#define AG_AWGT_ARCHIVE_DEFACTIONS                              300
#define AG_AWGT_ARCHIVE_ACTIONS                                 500
//ArchiveWindow - Messages Tools
#define AG_AWMT_ARCHIVE_ACTIONS                                 500

#endif
