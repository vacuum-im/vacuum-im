TRANS_LANGS  = ru pl de uk es
TRANS_TARGET = $$member(TARGET,0)
TRANSLATIONS = \
               $${TRANS_SOURCE_ROOT}/translations/en/$${TRANS_TARGET}.ts \
               $${TRANS_SOURCE_ROOT}/translations/ru/$${TRANS_TARGET}.ts \
               $${TRANS_SOURCE_ROOT}/translations/pl/$${TRANS_TARGET}.ts \
               $${TRANS_SOURCE_ROOT}/translations/de/$${TRANS_TARGET}.ts \
               $${TRANS_SOURCE_ROOT}/translations/uk/$${TRANS_TARGET}.ts \
               $${TRANS_SOURCE_ROOT}/translations/es/$${TRANS_TARGET}.ts 
include(../make/translations.inc)
