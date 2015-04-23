# Translation file name
TRANS_TARGET = $$member(TARGET,0)

# Translations to build and install
TRANS_LANGS  = ru pl de uk es

# Translations to update from source
TRANSLATIONS += $${TRANS_SOURCE_ROOT}/translations/ru/$${TRANS_TARGET}.ts

include(../make/translations.inc)
