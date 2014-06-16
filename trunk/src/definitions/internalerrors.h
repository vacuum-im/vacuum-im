#ifndef DEF_INTERNALERRORS_H
#define DEF_INTERNALERRORS_H

// BitsOfBinary
#define IERR_BOB_DATA_LOAD_ERROR                             "bob-data-load-error"
#define IERR_BOB_DATA_SAVE_ERROR                             "bob-data-save-error"
#define IERR_BOB_INVALID_RESPONCE                            "bob-invalid-responce"

// Compression
#define IERR_COMPRESS_UNKNOWN_ERROR                          "compress-unknown-error"
#define IERR_COMPRESS_OUT_OF_MEMORY                          "compress-out-of-memory"
#define IERR_COMPRESS_VERSION_MISMATCH                       "compress-version-mismatch"
#define IERR_COMPRESS_INVALID_DEFLATE_DATA                   "compress-invalid-deflate-data"
#define IERR_COMPRESS_INVALID_COMPRESSION_LEVEL              "compress-invalid-compression-level"

// ConnectionManager
#define IERR_CONNECTIONMANAGER_CONNECT_ERROR                 "connectionmanager-connect-error"

// DefaultConnection
#define IERR_DEFAULTCONNECTION_CERT_NOT_TRUSTED              "defaultconnection-cert-not-trusted"

// DataForms
#define IERR_DATAFORMS_MEDIA_INVALID_TYPE                    "dataforms-media-invalid-type"
#define IERR_DATAFORMS_MEDIA_INVALID_FORMAT                  "dataforms-media-invalid-format"
#define IERR_DATAFORMS_URL_INVALID_SCHEME                    "dataforms-url-invalid-scheme"
#define IERR_DATAFORMS_URL_NETWORK_ERROR                     "dataforms-url-network-error"

// DataStreamsManager
#define IERR_DATASTREAMS_PROFILE_INVALID_SETTINGS            "datastreams-profile-invalid-settings"
#define IERR_DATASTREAMS_STREAM_STREAMID_EXISTS              "datastreams-stream-streamid-exists"
#define IERR_DATASTREAMS_STREAM_INVALID_INIT_RESPONCE        "datastreams-stream-invalid-init-responce"

// FileMessageArchive
#define IERR_FILEARCHIVE_DATABASE_NOT_CREATED                "filearchive-database-not-created"
#define IERR_FILEARCHIVE_DATABASE_NOT_OPENED                 "filearchive-database-not-opened"
#define IERR_FILEARCHIVE_DATABASE_NOT_COMPATIBLE             "filearchive-database-not-compatible"
#define IERR_FILEARCHIVE_DATABASE_EXEC_FAILED                "filearchive-database-exec-failed"

// FileStreamsManager
#define IERR_FILESTREAMS_STREAM_FILE_IO_ERROR                "filestreams-stream-file-io-error"
#define IERR_FILESTREAMS_STREAM_FILE_SIZE_CHANGED            "filestreams-stream-file-size-changed"
#define IERR_FILESTREAMS_STREAM_CONNECTION_TIMEOUT           "filestreams-stream-connection-timeout"
#define IERR_FILESTREAMS_STREAM_TERMINATED_BY_REMOTE_USER    "filestreams-stream-terminated-by-remote-user"

// FileTransfer
#define IERR_FILETRANSFER_TRANSFER_NOT_STARTED               "filetransfer-transfer-not-started"
#define IERR_FILETRANSFER_TRANSFER_TERMINATED                "filetransfer-transfer-terminated"

// InBandStreams
#define IERR_INBAND_STREAM_DESTROYED                         "inband-stream-destroyed"
#define IERR_INBAND_STREAM_INVALID_DATA                      "inband-stream-invalid-data"
#define IERR_INBAND_STREAM_NOT_OPENED                        "inband-stream-not-opened"
#define IERR_INBAND_STREAM_INVALID_BLOCK_SIZE                "inband-stream-invalid-block-size"
#define IERR_INBAND_STREAM_DATA_NOT_SENT                     "inband-stream-data-not-sent"

// MessageArchive
#define IERR_HISTORY_HEADERS_LOAD_ERROR                      "history-headers-load-error"
#define IERR_HISTORY_CONVERSATION_SAVE_ERROR                 "history-conversation-save-error"
#define IERR_HISTORY_CONVERSATION_LOAD_ERROR                 "history-conversation-load-error"
#define IERR_HISTORY_CONVERSATION_REMOVE_ERROR               "history-conversation-remove-error"
#define IERR_HISTORY_MODIFICATIONS_LOAD_ERROR                "history-modifications-load-error"

// Registration
#define IERR_REGISTER_INVALID_FORM                           "register-invalid-form"
#define IERR_REGISTER_INVALID_DIALOG                         "register-invalid-dialog"
#define IERR_REGISTER_REJECTED_BY_USER                       "register-rejected-by-user"

// Roster
#define IERR_ROSTER_REQUEST_FAILED                           "roster-request-failed"

// SASLAuth
#define IERR_SASL_AUTH_INVALID_RESPONSE                      "sasl-auth-invalid-response"
#define IERR_SASL_BIND_INVALID_STREAM_JID                    "sasl-bind-invalid-stream-jid"

// SocksStreams
#define IERR_SOCKS5_STREAM_DESTROYED                         "socks5-stream-destroyed"
#define IERR_SOCKS5_STREAM_INVALID_MODE                      "socks5-stream-invalid-mode"
#define IERR_SOCKS5_STREAM_HOSTS_REJECTED                    "socks5-stream-hosts-rejected"
#define IERR_SOCKS5_STREAM_HOSTS_UNREACHABLE                 "socks5-stream-hosts-unreachable"
#define IERR_SOCKS5_STREAM_HOSTS_NOT_CREATED                 "socks5-stream-hosts-not-created"
#define IERR_SOCKS5_STREAM_NOT_ACTIVATED                     "socks5-stream-not-activated"
#define IERR_SOCKS5_STREAM_DATA_NOT_SENT                     "socks5-stream-data-not-sent"
#define IERR_SOCKS5_STREAM_NO_DIRECT_CONNECTION              "socks5-stream-no-direct-connections"
#define IERR_SOCKS5_STREAM_INVALID_HOST                      "socks5-stream-invalid-host"
#define IERR_SOCKS5_STREAM_INVALID_HOST_ADDRESS              "socks5-stream-invalid-host-address"
#define IERR_SOCKS5_STREAM_HOST_NOT_CONNECTED                "socks5-stream-host-not-connected"
#define IERR_SOCKS5_STREAM_HOST_DISCONNECTED                 "socks5-stream-host-disconnected"

// StartTLS
#define IERR_STARTTLS_NOT_STARTED                            "starttls-not-started"
#define IERR_STARTTLS_INVALID_RESPONCE                       "starttls-invalid-responce"
#define IERR_STARTTLS_NEGOTIATION_FAILED                     "starttls-negotiation-failed"

// XmppStreams
#define IERR_XMPPSTREAM_DESTROYED                            "xmppstream-destroyed"
#define IERR_XMPPSTREAM_NOT_SECURE                           "xmppstream-not-secure"
#define IERR_XMPPSTREAM_CLOSED_UNEXPECTEDLY                  "xmppstream-closed-unexpectedly"
#define IERR_XMPPSTREAM_FAILED_START_CONNECTION              "xmppstream-failed-to-start-connection"

#endif // DEF_INTERNALERRORS_H
