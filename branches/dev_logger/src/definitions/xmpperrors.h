#ifndef DEF_XMPPERRORS_H
#define DEF_XMPPERRORS_H

// Commands - NS_COMMANDS
#define XERR_COMMANDS_MALFORMED_ACTION                        "malformed-action"
#define XERR_COMMANDS_BAD_ACTION                              "bad-action"
#define XERR_COMMANDS_BAD_LOCALE                              "bad-locale"
#define XERR_COMMANDS_BAD_PAYLOAD                             "bad-payload"
#define XERR_COMMANDS_BAD_SESSIONID                           "bad-sessionid"
#define XERR_COMMANDS_SESSION_EXPIRED                         "session-expired"

// Compression - NS_FEATURE_COMPRESS
#define XERR_COMPRESS_UNSUPPORTED_METHOD                      "unsupported-method"
#define XERR_COMPRESS_SETUP_FAILED                            "setup-failed"

// DataStreamsManager - NS_STREAM_INITIATION
#define XERR_SI_BAD_PROFILE                                   "bad-profile"
#define XERR_SI_NO_VALID_STREAMS                              "no-valid-streams"

// SASLAuth - NS_FEATURE_SASL
#define XERR_SASL_ABORTED                                     "aborted"
#define XERR_SASL_ACCOUNT_DISABLED                            "account-disabled"
#define XERR_SASL_CREDENTIALS_EXPIRED                         "credentials-expired"
#define XERR_SASL_ENCRYPTION_REQUIRED                         "encryption-required"
#define XERR_SASL_INCORRECT_ENCODING                          "incorrect-encoding"
#define XERR_SASL_INVALID_AUTHZID                             "invalid-authzid"
#define XERR_SASL_INVALID_MECHANISM                           "invalid-mechanism"
#define XERR_SASL_MAILFORMED_REQUEST                          "malformed-request"
#define XERR_SASL_MECHANISM_TOO_WEAK                          "mechanism-too-weak"
#define XERR_SASL_NOT_AUTHORIZED                              "not-authorized"
#define XERR_SASL_TEMPORARY_AUTH_FAILURE                      "temporary-auth-failure"

#endif // DEF_XMPPERRORS_H
