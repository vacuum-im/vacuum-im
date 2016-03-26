#ifndef DEF_STATISTICSPARAMS_H
#define DEF_STATISTICSPARAMS_H

// Metrics
#define SCMP_ACCOUNTS_COUNT                                  1
#define SCMP_CONTACTS_COUNT                                  2
#define SCMP_AGENTS_COUNT                                    3
#define SCMP_GROUPS_COUNT                                    4
#define SCMP_CHATWINDOWS_COUNT                               5
#define SCMP_MULTICHATWINDOWS_COUNT                          6
#define SCMP_BOOKMARKS_COUNT                                 7
#define SCMP_STATUSITEMS_COUNT                               8

// Dimensions
#define SCDP_SERVER_NAME                                     1
#define SCDP_SERVER_VERSION                                  2

// Hit-Event
#define SEVP_APPLICATION_RESTART                             "application|restart|Application Restart"

#define SEVP_SESSION_STARTED                                 "session|started|Session Started"
#define SEVP_SESSION_FINISHED                                "session|finished|Session Finished"

#define SEVP_STATISTICS_SERVERS                              "statistics|servers|Statistics Servers"
#define SEVP_STATISTICS_METRICS                              "statistics|metrics|Statistics Metrics"
#define SEVP_STATISTICS_ENABLED                              "statistics|enabled|Statistics Enabled"
#define SEVP_STATISTICS_DISABLED                             "statistics|disabled|Statistics Disabled"

#define SEVP_FILESTREAM_SUCCESS                              "file-stream|success|File Stream Success"
#define SEVP_FILESTREAM_FAILURE                              "file-stream|failure|File Stream Failure"

#define SEVP_ACCOUNT_APPENDED                                "account|appended|Account Appended"
#define SEVP_ACCOUNT_REGISTERED                              "account|registered|Account Registered"

#define SEVP_MUC_WIZARD_JOIN                                 "muc|wizard-join|MUC Wizard Join"
#define SEVP_MUC_WIZARD_CREATE                               "muc|wizard-create|MUC Wizard Create"
#define SEVP_MUC_WIZARD_MANUAL                               "muc|wizard-manual|MUC Wizard Manual"
#define SEVP_MUC_CHAT_CONVERT                                "muc|chat-convert|MUC Chat Convert"

// Hit-Timing
#define STMP_APPLICATION_START                               "application|start|Application Start"
#define STMP_APPLICATION_QUIT                                "application|quit|Application Quit"

#define STMP_HISTORY_REPLICATE                               "history|replicate|History Replicate"
#define STMP_HISTORY_HEADERS_LOAD                            "history|headers-load|History Headers Load"
#define STMP_HISTORY_MESSAGES_LOAD                           "history|messages-load|History Messages Load"
#define STMP_HISTORY_FILE_DATABASE_SYNC                      "history|file-database-sync|History File Database Sync"

#define STMP_SOCKSSTREAM_CONNECTED                           "socks-stream|connected|Socks Stream Connected"

#endif // DEF_STATISTICSPARAMS_H
