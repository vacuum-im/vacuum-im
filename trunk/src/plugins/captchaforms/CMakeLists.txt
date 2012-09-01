project(captchaforms)

set(PLUGIN_NAME "captchaforms")
set(PLUGIN_DISPLAY_NAME "CAPTCHA forms")
set(PLUGIN_DEPENDENCIES dataforms xmppstreams stanzaprocessor) # used only in CPack

include("captchaforms.cmake")
include("${CMAKE_SOURCE_DIR}/src/plugins/plugins.cmake")
