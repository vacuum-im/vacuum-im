#ifndef ACTIONGROUPS_H
#define ACTIONGROUPS_H

#define AG_NULL                                                 -1
#define AG_DEFAULT                                              500

//MainWindow (MainMwenu)
#define AG_ROSTERCHANGER_MMENU                                  AG_DEFAULT
#define AG_SETTINGS_MMENU                                       700
#define AG_ACCOUNTMANAGER_MMENU                                 700
#define AG_SKINMANAGER_MENU                                     700
#define AG_MAINWINDOW_MMENU_QUIT                                1000

//MainWindow (TopToolBar)
#define AG_ROSTERSVIEW_MWTTB                                    100
#define AG_SERVICEDISCOVERY_MWTTB                               200
#define AG_MULTIUSERCHAT_MWTTB                                  300

//RostersView (ContextMenu)
#define AG_MESSENGER                                            300
#define AG_STATUSCHANGER_ROSTER                                 400
#define AG_ROSTERCHANGER_ROSTER_ADD_CONTACT                     AG_DEFAULT
#define AG_ACCOUNTMANAGER_ROSTER                                AG_DEFAULT
#define AG_STATUSICONS_ROSTER                                   AG_DEFAULT
#define AG_CLIENTINFO_ROSTER                                    AG_DEFAULT
#define AG_VCARD_ROSTER                                         AG_DEFAULT
#define AG_SERVICE_DISCOVERY_ROSTER                             AG_DEFAULT
#define AG_MULTIUSERCHAT_ROSTER                                 500
#define AG_ROSTERCHANGER_ROSTER_SUBSCRIPTION                    700
#define AG_ROSTERCHANGER_ROSTER                                 700

//TrayManager (TrayMenu)
#define AG_STATUSCHANGER_TRAY                                   200
#define AG_ROSTERCHANGER_TRAY                                   500
#define AG_SETTINGS_TRAY                                        700
#define AG_SKINMANAGER_TRAY                                     700
#define AG_SERVICEDISCOVERY_TRAY                                700
#define AG_MULTIUSERCHAT_TRAY                                   700
#define AG_TRAYMANAGER_TRAY_QUIT                                1000

//StatusChanger (StatusMenu, StreamMenu)
#define AG_STATUSCHANGER_STATUSMENU_STREAMS                     350
#define AG_STATUSCHANGER_STATUSMENU_CUSTOM_MENU                 400

//EditWidgetToolbar
#define AG_EMOTICONS_EDITWIDGET                                 AG_DEFAULT

//MultiUserContextMenu
#define AG_ROSTERCHANGER_MUCM                                   AG_DEFAULT
#define AG_CLIENTINFO_MUCM                                      AG_DEFAULT
#define AG_VCARD_MUCM                                           AG_DEFAULT
#define AG_MULTIUSERCHAT_ROOM_UTILS                             800

//DiscoItemsWindow (ToolBar)
#define AG_DIWT_SERVICEDISCOVERY_NAVIGATE                       100
#define AG_DIWT_SERVICEDISCOVERY_DEFACTIONS                     300
#define AG_DIWT_SERVICEDISCOVERY_ACTIONS                        AG_DEFAULT
#define AG_DIWT_SERVICEDISCOVERY_FEATURE_ACTIONS                700

#endif
