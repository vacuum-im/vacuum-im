#ifndef ACTIONGROUPS_INC
#define ACTIONGROUPS_INC

#define AG_NULL                                                 -1
#define AG_DEFAULT                                              500

//MainWindow (MainMwenu)
#define AG_ROSTERCHANGER_MMENU                                  500
#define AG_SETTINGS_MMENU                                       700
#define AG_ACCOUNTMANAGER_MMENU                                 700
#define AG_MAINWINDOW_MMENU_QUIT                                1000

//RostersView (ContextMenu)
#define AG_STATUSCHANGER_ROSTER                                 200
#define AG_ROSTERCHANGER_ROSTER_ADD_CONTACT                     300
#define AG_ACCOUNTMANAGER_ROSTER                                700
#define AG_ROSTERCHANGER_ROSTER_SUBSCRIPTION                    799
#define AG_ROSTERCHANGER_ROSTER                                 800

//TrayManager (TrayMenu)
#define AG_STATUSCHANGER_TRAY                                   200
#define AG_ROSTERCHANGER_TRAY                                   500
#define AG_SETTINGS_TRAY                                        700
#define AG_TRAYMANAGER_TRAY_QUIT                                1000

//StatusChanger (StatusMenu, StreamMenu)
#define AG_STATUSCHANGER_STATUSMENU_STREAMS                     350
#define AG_STATUSCHANGER_STATUSMENU_CUSTOM_MENU                 400

#endif